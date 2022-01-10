// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/relations/es_TreeModel.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "Spanish/relations/es_RelationUtilities.h"
#include "Spanish/relations/es_OldRelationFinder.h"
#include "Generic/relations/RelationTypeSet.h"
#include "Generic/relations/discmodel/DTRelationSet.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/theories/Parse.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/TrainingLoader.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/PropositionSet.h"
/* DK todo
#include "Spanish/common/es_StringTransliterator.h"
*/

#include <wchar.h>
#include "math.h"
#include <boost/scoped_ptr.hpp>

#ifdef _WIN32
	#define swprintf _snwprintf
#endif

Symbol TreeModel::LEFT = Symbol(L"LEFT");
Symbol TreeModel::RIGHT = Symbol(L"RIGHT");
Symbol TreeModel::NESTED = Symbol(L"NESTED");
Symbol TreeModel::DUMMY = Symbol(L"DUMMY");

TreeModel::TreeModel() : 
	_originalData(0), _attachmentModel(0), _nodeModel(0), _predicateModel(0), 
	_constructionModel(0), _priorModel(0), _observation(0), _inst(0) 
{}

TreeModel::TreeModel(const char *file_prefix, bool splitlevel) : 
	_originalData(0), _attachmentModel(0), _nodeModel(0), _predicateModel(0), 
		_constructionModel(0), _priorModel(0), _observation(0), _inst(0),
	SPLIT_LEVEL_DECISION(splitlevel) 
{
	readInModels(file_prefix);
}

TreeModel::~TreeModel() {
	delete _originalData;
	delete _attachmentModel;
	delete _nodeModel;
	delete _predicateModel;
	delete _constructionModel;
	delete _priorModel;
	delete _observation;
	delete _inst;
}

void TreeModel::train(char *training_file, char* output_file_prefix)
{
	collect(training_file);
	printTables(output_file_prefix);
}

void TreeModel::trainFromStateFileList(char *training_file, char* output_file_prefix)
{	
	_priorModel = _new type_prior_model_t();
	_nodeModel = _new type_node_model_t();
	_constructionModel = _new type_construction_model_t();
	_attachmentModel = _new type_attachment_model_t();
	_predicateModel = _new type_predicate_model_t();

	_observation = _new RelationObservation();
	_inst = _new PotentialRelationInstance();
	
	TrainingLoader *trainingLoader = _new TrainingLoader(training_file, L"doc-relations-events");
	collectFromLoader(trainingLoader);
	delete trainingLoader;

	_attachmentModel->deriveModel();
	_nodeModel->deriveModel();
	_priorModel->deriveModel();
	_predicateModel->deriveModel();
	_constructionModel->deriveModel();

	printTables(output_file_prefix);
}


void TreeModel::collectFromLoader(TrainingLoader *trainingLoader)
{
	for (int i = 0; i < trainingLoader->getMaxSentences(); i++) {
		SentenceTheory *theory = trainingLoader->getNextSentenceTheory();
		if (theory == 0)
			break;
		const Parse* parse =  theory->getPrimaryParse();
		MentionSet* mentionSet =  theory->getMentionSet();
		PropositionSet *propSet = theory->getPropositionSet();
		ValueMentionSet *valueSet = theory->getValueMentionSet();
		propSet->fillDefinitionsArray();
		DTRelationSet *relSet = _new DTRelationSet(mentionSet->getNMentions(), 
			theory->getRelMentionSet(), Symbol(L"NONE"));
		collectFromSentence(parse, mentionSet, valueSet, propSet, relSet);
	}
}

bool TreeModel::hasZeroProbability(PotentialRelationInstance *instance, int type) {

	instance->setRelationType(RelationTypeSet::getRelationSymbol(type));
	return (getProbability(instance) <= -10000);
}

int TreeModel::findBestRelationType(PotentialRelationInstance *instance) {

	if (SPLIT_LEVEL_DECISION) {
		float all_other_score = 0;

		// get & report NONE score
		instance->setRelationType(RelationTypeSet::getRelationSymbol(0));
		float none_score = getProbability(instance);
		if (OldRelationFinder::DEBUG) {
			OldRelationFinder::_debugStream << L"TREE NONE SCORE: ";
			OldRelationFinder::_debugStream << none_score << L"\n";		
			OldRelationFinder::_debugStream << _last_prob_buffer << L"\n";
		}

		// get & sum all other scores
		for (int i = 1; i < RelationTypeSet::N_RELATION_TYPES; i++) {
			instance->setRelationType(RelationTypeSet::getRelationSymbol(i));
			float probability = getProbability(instance);
			if (probability != -10000)
				all_other_score += exp(probability);

			if (!RelationTypeSet::isSymmetric(i)) {
				int revtype = RelationTypeSet::reverse(i);
				instance->setRelationType(RelationTypeSet::getRelationSymbol(revtype));
				float probability = getProbability(instance);
				if (probability != -10000)
					all_other_score += exp(probability);
			}
		}

		// if no options other than NONE, return NONE
		if (all_other_score == 0)
			return 0; 
		
		// if there are options other than NONE, report them
		if (all_other_score != 0 && OldRelationFinder::DEBUG) {
			OldRelationFinder::_debugStream << L"ALL OTHER SCORE: ";
			OldRelationFinder::_debugStream << log(all_other_score) << L"\n\n";
		}

		// return NONE only if...
		if (none_score > log(all_other_score)) 
			return 0;
	}

	int best_answer = 0;
	float best_score = -10000;
	int second_best_answer = 0;
	float second_best_score = -10000;	

	for (int i = 0; i < RelationTypeSet::N_RELATION_TYPES; i++) {
		if (SPLIT_LEVEL_DECISION && i==0)
			continue;

		instance->setRelationType(RelationTypeSet::getRelationSymbol(i));
		float probability = getProbability(instance);

		if (probability > best_score) {
			second_best_score = best_score;
			second_best_answer = best_answer;
			best_score = probability;
			best_answer = i;
		} else if (probability > second_best_score) {
			second_best_score = probability;
			second_best_answer = i;
		}

		if (!RelationTypeSet::isSymmetric(i)) {
			int revtype = RelationTypeSet::reverse(i);
			instance->setRelationType(RelationTypeSet::getRelationSymbol(revtype));
			float probability = getProbability(instance);

			if (probability > best_score) {
				second_best_score = best_score;
				second_best_answer = best_answer;
				best_score = probability;
				best_answer = revtype;
			} else if (probability > second_best_score) {
				second_best_score = probability;
				second_best_answer = revtype;
			}
		}
	}

	if (OldRelationFinder::DEBUG) {
		OldRelationFinder::_debugStream << L"TREE BEST: ";
		OldRelationFinder::_debugStream << RelationTypeSet::getRelationSymbol(best_answer).to_string();
		OldRelationFinder::_debugStream << L" " << best_score << L"\n";
		instance->setRelationType(RelationTypeSet::getRelationSymbol(best_answer));
		getProbability(instance);
		OldRelationFinder::_debugStream << _last_prob_buffer << L"\n";
		OldRelationFinder::_debugStream << L"TREE SECOND BEST: ";
		OldRelationFinder::_debugStream << RelationTypeSet::getRelationSymbol(second_best_answer).to_string();
		OldRelationFinder::_debugStream << L" " << second_best_score << L"\n";
		instance->setRelationType(RelationTypeSet::getRelationSymbol(second_best_answer));
		getProbability(instance);
		OldRelationFinder::_debugStream << _last_prob_buffer << L"\n\n";
	}
	return best_answer;
}

float TreeModel::getProbability(PotentialRelationInstance* instance) {

	float prior = getPriorProbability(instance, DUMMY);
	float predicate = getPredicateProbability(instance, DUMMY);
	float construction = getConstructionProbability(instance, DUMMY);
	float left_node = getNodeProbability(instance, LEFT);
	float right_node = getNodeProbability(instance, RIGHT);
	float right_attachment = getAttachmentProbability(instance, RIGHT);
	float left_attachment = 0;
	if (instance->isMultiPlacePredicate()) {
		left_attachment = getAttachmentProbability(instance, LEFT);
	}

	float probability = prior + predicate + construction +
		left_node + right_node + left_attachment + right_attachment;


	if (instance->isNested()) {
		float nested_node = getNodeProbability(instance, NESTED);
		float nested_attachment = getAttachmentProbability(instance, NESTED);
		probability += nested_node + nested_attachment;
		_snwprintf(_last_prob_buffer, 1000, 
			L"%f = %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f", 
			probability, prior, predicate, construction, left_node, 
			right_node, left_attachment, right_attachment, 
			nested_node, nested_attachment);
	} else {
		_snwprintf(_last_prob_buffer, 1000, L"%f = %.3f %.3f %.3f %.3f %.3f %.3f %.3f", 
			probability, prior, predicate, construction, left_node, 
			right_node, left_attachment, right_attachment);
	}


	return probability;

}

void TreeModel::collect(const char* datafile) {

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(datafile);
	_originalData = _new NgramScoreTable(13, in);
	in.close();

	_priorModel = _new type_prior_model_t();
	_nodeModel = _new type_node_model_t();
	_constructionModel = _new type_construction_model_t();
	_attachmentModel = _new type_attachment_model_t();
	_predicateModel = _new type_predicate_model_t();

	NgramScoreTable::Table::iterator iter;
	PotentialRelationInstance *inst = _new PotentialRelationInstance();
	for (iter = _originalData->get_start(); iter != _originalData->get_end(); ++iter) {
		float count = (*iter).second;
		inst->setFromTrainingNgram((*iter).first);

		int t = RelationTypeSet::getTypeFromSymbol(inst->getRelationType());
		if (!RelationTypeSet::isSymmetric(t) && inst->isReversed())
		{
			t = RelationTypeSet::reverse(t);
			inst->setRelationType(RelationTypeSet::getRelationSymbol(t));
		}
		inst->setReverse(false);

		_priorModel->addEvent(inst, DUMMY);
		_predicateModel->addEvent(inst, DUMMY);
		_constructionModel->addEvent(inst, DUMMY);
		_nodeModel->addEvent(inst, LEFT);
		_nodeModel->addEvent(inst, RIGHT);
		_attachmentModel->addEvent(inst, RIGHT);
		if (inst->isMultiPlacePredicate()) {
			_attachmentModel->addEvent(inst, LEFT);
		}

		if (inst->isNested()) {
			_nodeModel->addEvent(inst, NESTED);
			_attachmentModel->addEvent(inst, NESTED);
		}

	}

	_attachmentModel->deriveModel();
	_nodeModel->deriveModel();
	_priorModel->deriveModel();
	_predicateModel->deriveModel();
	_constructionModel->deriveModel();

}

void TreeModel::collectFromSentence(const Parse* parse, const MentionSet* mset,
									const ValueMentionSet *vset,
									 PropositionSet *propSet, DTRelationSet *relSet)
{
	_observation->resetForNewSentence(parse, mset, vset, propSet);
	int nmentions = mset->getNMentions();

	for (int i = 0; i < nmentions; i++) {
		if (!mset->getMention(i)->isOfRecognizedType() || 
			mset->getMention(i)->getMentionType() == Mention::NONE ||
			mset->getMention(i)->getMentionType() == Mention::APPO ||
			mset->getMention(i)->getMentionType() == Mention::LIST)
			continue;
		for (int j = i + 1; j < nmentions; j++) {
			if (!mset->getMention(j)->isOfRecognizedType() || 
				mset->getMention(j)->getMentionType() == Mention::NONE ||
				mset->getMention(j)->getMentionType() == Mention::APPO ||
				mset->getMention(j)->getMentionType() == Mention::LIST) 
				continue;
			if (!RelationUtilities::get()->validRelationArgs(mset->getMention(i), mset->getMention(j)))
				continue;
			_observation->populate(i, j);
			RelationPropLink *link = _observation->getPropLink();
			if (!link->isEmpty() && !link->isNegative()) {
				//_inst->setStandardInstance(link->getArg1Role(), link->getArg2Role(),
				//	link->getTopProposition(), mset);
				_inst->setStandardInstance(_observation);

				_inst->setRelationType(relSet->getRelation(i,j));
				_priorModel->addEvent(_inst, DUMMY);
				_predicateModel->addEvent(_inst, DUMMY);
				_constructionModel->addEvent(_inst, DUMMY);
				_nodeModel->addEvent(_inst, LEFT);
				_nodeModel->addEvent(_inst, RIGHT);
				_attachmentModel->addEvent(_inst, RIGHT);
				if (_inst->isMultiPlacePredicate()) {
					_attachmentModel->addEvent(_inst, LEFT);
				}
				if (_inst->isNested()) {
					_nodeModel->addEvent(_inst, NESTED);
					_attachmentModel->addEvent(_inst, NESTED);
				}
			}
		}
	}	
}



void TreeModel::printTables(const char* prefix) {
	char buffer[500];
	UTF8OutputStream testStream;

	sprintf(buffer, "%s.priors", prefix);
	testStream.open(buffer);
	_priorModel->print(testStream);
	testStream.close();
	sprintf(buffer, "%s.predicates", prefix);
	testStream.open(buffer);
	_predicateModel->print(testStream);
	testStream.close();
	sprintf(buffer, "%s.constructions", prefix);
	testStream.open(buffer);
	_constructionModel->print(testStream);
	testStream.close();
	sprintf(buffer, "%s.nodes", prefix);
	testStream.open(buffer);
	_nodeModel->print(testStream);
	testStream.close();
	sprintf(buffer, "%s.attachments", prefix);
	testStream.open(buffer);
	_attachmentModel->print(testStream);
	testStream.close();
}

void TreeModel::readInModels(const char* prefix) {
	char buffer[500];

	sprintf(buffer, "%s.priors", prefix);
	boost::scoped_ptr<UTF8InputStream> priors_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& priors(*priors_scoped_ptr);
	priors.open(buffer);
	_priorModel = _new type_prior_model_t(priors);
	priors.close();

	sprintf(buffer, "%s.nodes", prefix);
	boost::scoped_ptr<UTF8InputStream> nodes_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& nodes(*nodes_scoped_ptr);
	nodes.open(buffer);
	_nodeModel = _new type_node_model_t(nodes);
	nodes.close();

	sprintf(buffer, "%s.constructions", prefix);
	boost::scoped_ptr<UTF8InputStream> constructions_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& constructions(*constructions_scoped_ptr);
	constructions.open(buffer);
	_constructionModel = _new type_construction_model_t(constructions);
	constructions.close();

	sprintf(buffer, "%s.attachments", prefix);
	boost::scoped_ptr<UTF8InputStream> attachments_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& attachments(*attachments_scoped_ptr);
	attachments.open(buffer);
	_attachmentModel = _new type_attachment_model_t(attachments);
	attachments.close();

	sprintf(buffer, "%s.predicates", prefix);
	boost::scoped_ptr<UTF8InputStream> predicates_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& predicates(*predicates_scoped_ptr);
	predicates.open(buffer);
	_predicateModel = _new type_predicate_model_t(predicates);
	predicates.close();
}
