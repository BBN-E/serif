// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/relations/VectorModel.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/ParamReader.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "Generic/relations/xx_RelationUtilities.h"
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
#include "Generic/common/StringTransliterator.h"
#include "math.h"
#include <boost/scoped_ptr.hpp>

VectorModel::VectorModel(const char *file_prefix, bool splitlevel)
{
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& stream(*stream_scoped_ptr);
	stream.open(file_prefix);
	_model = type_feature_vector_model_t::build(stream);
	stream.close();

	char b2pStr[500];
	sprintf(b2pStr, "%s.b2p", file_prefix);
	boost::scoped_ptr<UTF8InputStream> b2pStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& b2pStream(*b2pStream_scoped_ptr);
	b2pStream.open(b2pStr);
	_b2pModel = type_b2p_feature_vector_model_t::build(b2pStream);
	b2pStream.close();

	SPLIT_LEVEL_DECISION = splitlevel;
}

VectorModel::VectorModel() : SPLIT_LEVEL_DECISION(false)
{
}

VectorModel::~VectorModel() {
	delete _model;
	delete _b2pModel;
}

void VectorModel::train(char *training_file, char* output_file_prefix)
{
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(training_file);
	_originalData = _new NgramScoreTable(13, in);
	in.close();

	_model = type_feature_vector_model_t::build();
	_b2pModel = type_b2p_feature_vector_model_t::build();

	PotentialRelationInstance *inst = _new PotentialRelationInstance();
	NgramScoreTable::Table::iterator iter;
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

		_model->addEvent(inst, Symbol());
		_b2pModel->addEvent(inst, Symbol());
	}

	_model->deriveModel();
	_b2pModel->deriveModel();

	UTF8OutputStream stream;
	stream.open(output_file_prefix);
	_model->print(stream);
	stream.close();
	char b2pStr[500];
	sprintf(b2pStr, "%s.b2p", output_file_prefix);
	UTF8OutputStream b2pStream;
	b2pStream.open(b2pStr);
	_b2pModel->print(b2pStream);
	b2pStream.close();
}

void VectorModel::trainFromStateFileList(char *training_file, char* output_file_prefix)
{
	_model = type_feature_vector_model_t::build();
	_b2pModel = type_b2p_feature_vector_model_t::build();
	_observation = _new RelationObservation();
	_inst = _new PotentialRelationInstance();

	int beam_width = ParamReader::getOptionalIntParamWithDefaultValue("beam_width",1);

	TrainingLoader *trainingLoader = _new TrainingLoader(training_file, L"doc-relations-events");
	trainFromLoader(trainingLoader);
	delete trainingLoader;

	_model->deriveModel();
	_b2pModel->deriveModel();

	UTF8OutputStream stream;
	stream.open(output_file_prefix);
	_model->print(stream);
	stream.close();
	char b2pStr[500];
	sprintf(b2pStr, "%s.b2p", output_file_prefix);
	UTF8OutputStream b2pStream;
	b2pStream.open(b2pStr);
	_b2pModel->print(b2pStream);
	b2pStream.close();
}

void VectorModel::trainFromLoader(TrainingLoader *trainingLoader)
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
		trainFromSentence(parse, mentionSet, valueSet, propSet, relSet);
	}
}

void VectorModel::trainFromSentence(const Parse* parse, const MentionSet* mset,
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

			// ordering is changed by _observation --> it might not be i, j
			int left = _observation->getMention1()->getIndex();
			int right = _observation->getMention2()->getIndex();

			RelationPropLink *link = _observation->getPropLink();
			_inst->setStandardInstance(_observation);
			_inst->setRelationType(relSet->getRelation(left, right));

			if (!link->isEmpty() && !link->isNegative()) {
				_model->addEvent(_inst, Symbol());
			}

            if (relSet->hasReversedRelation(left, right)) {
				int t = RelationTypeSet::getTypeFromSymbol(_inst->getRelationType());
				if (!RelationTypeSet::isSymmetric(t))
				{
					t = RelationTypeSet::reverse(t);
					_inst->setRelationType(RelationTypeSet::getRelationSymbol(t));
				}
			}
			_b2pModel->addEvent(_inst, Symbol());

		}
	}
}


bool VectorModel::hasZeroProbability(PotentialRelationInstance *instance, int type) {

	instance->setRelationType(RelationTypeSet::getRelationSymbol(type));
	return (lookup(instance) <= -10000);
}

int VectorModel::findBestRelationType(PotentialRelationInstance *instance) {

	if (SPLIT_LEVEL_DECISION) {
		float all_other_score = 0;

		// get & report none score
		instance->setRelationType(RelationTypeSet::getRelationSymbol(0));
		float none_score = lookup(instance);
		/*if (RelationUtilities::get()->debugStreamIsOn()) {
			RelationUtilities::get()->getDebugStream() << L"VECTOR NONE SCORE: ";
			RelationUtilities::get()->getDebugStream() << none_score << L"\n";
		}*/

		// get & sum all other scores
		for (int i = 1; i < RelationTypeSet::N_RELATION_TYPES; i++) {
			instance->setRelationType(RelationTypeSet::getRelationSymbol(i));
			float probability = lookup(instance);
			if (probability != -10000)
				all_other_score += exp(probability);

			if (!RelationTypeSet::isSymmetric(i)) {
				int revtype = RelationTypeSet::reverse(i);
				instance->setRelationType(RelationTypeSet::getRelationSymbol(revtype));
				float probability = lookup(instance);
				if (probability != -10000)
					all_other_score += exp(probability);
			}
		}

		// if no options other than NONE, return NONE
		if (all_other_score == 0)
			return 0;

		// if there are options other than NONE, report them
		/*if (RelationUtilities::get()->debugStreamIsOn()) {
			RelationUtilities::get()->getDebugStream() << L"ALL OTHER SCORE: ";
			RelationUtilities::get()->getDebugStream() << log(all_other_score) << L"\n\n";
		}*/

		// return NONE only if...
		if (none_score > log(all_other_score)) {
			return 0;
		}
	}

	int best_answer = 0;
	float best_score = -10000;
	int second_best_answer = 0;
	float second_best_score = -10000;

    for (int i = 0; i < RelationTypeSet::N_RELATION_TYPES; i++) {
		if (SPLIT_LEVEL_DECISION && i==0)
			continue;

		instance->setRelationType(RelationTypeSet::getRelationSymbol(i));
		float probability = lookup(instance);

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
			float probability = lookup(instance);

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

	/*if (RelationUtilities::get()->debugStreamIsOn()) {
		RelationUtilities::get()->getDebugStream() << L"VECTOR BEST: ";
		RelationUtilities::get()->getDebugStream() << RelationTypeSet::getRelationSymbol(best_answer).to_string();
		RelationUtilities::get()->getDebugStream() << L" " << best_score << L"\n";
		RelationUtilities::get()->getDebugStream() << L"VECTOR SECOND BEST: ";
		RelationUtilities::get()->getDebugStream() << RelationTypeSet::getRelationSymbol(second_best_answer).to_string();
		RelationUtilities::get()->getDebugStream() << L" " << second_best_score << L"\n\n";
	}*/

	return best_answer;

}


int VectorModel::findConfidentRelationType(PotentialRelationInstance *instance,
										   double lambda_threshold) 
{

	double lambda = _model->getLambdaForFullHistory(instance, Symbol());
	if (lambda < lambda_threshold)
		return 0;

	return findBestRelationType(instance);
}



float VectorModel::lookup(PotentialRelationInstance *instance) const
{
	return (float)_model->getProbability(instance, Symbol());
}

float VectorModel::lookupB2P(PotentialRelationInstance *instance) const
{
	return (float)_b2pModel->getProbability(instance, Symbol());
}

