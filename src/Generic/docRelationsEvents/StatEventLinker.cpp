// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "docRelationsEvents/StatEventLinker.h"
#include "docRelationsEvents/EventLinkFeatureTypes.h"
#include "docRelationsEvents/EventLinkFeatureType.h"
#include "docRelationsEvents/EventLinkObservation.h"
#include "common/UnexpectedInputException.h"
#include "theories/EventMention.h"
#include "theories/EventSet.h"
#include "theories/Event.h"
#include "theories/DocTheory.h"
#include "common/ParamReader.h"
#include "maxent/MaxEntModel.h"
#include "discTagger/DTFeatureTypeSet.h"
#include "discTagger/DTTagSet.h"
#include "wordClustering/WordClusterTable.h"
#include "state/TrainingLoader.h"
#include "common/StringTransliterator.h"
#include <boost/scoped_ptr.hpp>

#define MAX_VMENTIONS_PER_DOC 200

StatEventLinker::StatEventLinker(int mode_) : MODE(mode_), _link_threshold(0)
{
	_observation = _new EventLinkObservation();
	EventLinkFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();

	// TAG SET
	std::string tag_set_file = ParamReader::getRequiredParam("event_link_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);
	_tagScores = _new double[_tagSet->getNTags()];

	// FEATURES
	std::string features_file = ParamReader::getRequiredParam("event_link_features_file");	
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), EventLinkFeatureType::modeltype);

	if (MODE == DECODE || MODE == ROUNDROBIN) {

		// LINK THRESHOLD
		_link_threshold = ParamReader::getRequiredFloatParam("event_link_threshold");

		// MODEL FILE
		std::string model_file = ParamReader::getParam("event_link_model_file");
		if (!model_file.empty())
		{
			_weights = _new DTFeature::FeatureWeightMap(50000);
			DTFeature::readWeights(*_weights, model_file.c_str(), EventLinkFeatureType::modeltype);
			_model = _new MaxEntModel(_tagSet, _featureTypes, _weights);
			
		} else if (!ParamReader::hasParam("event_round_robin_setup")) {

			throw UnexpectedInputException("StatEventLinker::StatEventLinker()",
				"Neither 'event_link_model_file' nor 'event_round_robin_setup' specified");

		} else {
			_weights = 0;
			_model = 0;
		}

		if (MODE == ROUNDROBIN) {
			_link_threshold = ParamReader::getRequiredFloatParam("event_link_threshold");
			_allVMentions = _new EventMention * [MAX_VMENTIONS_PER_DOC];
		} else {
			_allVMentions = 0;
		}

	} else if (MODE == TRAIN) {

		_allVMentions = _new EventMention * [MAX_VMENTIONS_PER_DOC];

		// PERCENT HELD OUT
		int percent_held_out = ParamReader::getRequiredIntParam("event_link_trainer_percent_held_out");
		if (percent_held_out < 0 || percent_held_out > 50) 
			throw UnexpectedInputException("StatEventLinker::StatEventLinker()",
			"Parameter 'event_link_trainer_percent_held_out' must be between 0 and 50");

		// MAX NUMBER OF ITERATIONS (STOPPING CONDITION)
		int max_iterations = ParamReader::getRequiredIntParam("event_link_trainer_max_iterations");
		
		// GAUSSIAN PRIOR VARIANCE
		double variance = ParamReader::getRequiredFloatParam("event_link_trainer_gaussian_variance");
		
		// MIN CHANGE IN LIKELIHOOD (STOPPING CONDITION)
		double likelihood_delta = ParamReader::getOptionalFloatParamWithDefaultValue("event_link_trainer_min_likelihood_delta", .0001);

		// FREQUENCY OF STOPPING CONDITION CHECKS (NUM ITERATIONS)		
		int stop_check_freq = ParamReader::getOptionalIntParamWithDefaultValue("event_link_trainer_stop_check_frequency", 1);

		_weights = _new DTFeature::FeatureWeightMap(50000);

		const std::string train_vector_file = ParamReader::getParam("training_vectors_file");
		if (train_vector_file != "") {
			SessionLogger::info("SERIF") << "Dumping feature vectors to " << train_vector_file;
		}

		_model = _new MaxEntModel(_tagSet, _featureTypes, _weights, 
			MaxEntModel::SCGIS, percent_held_out, max_iterations, variance,
			likelihood_delta, stop_check_freq,
			train_vector_file.c_str(), "");
	}
}


StatEventLinker::~StatEventLinker() {
	delete _model;
	delete _weights;
	delete _tagSet;
	delete[] _tagScores;
	delete _featureTypes;
	delete _observation;
	delete[] _allVMentions;
}

void StatEventLinker::replaceModel(char *model_file) {
	delete _model;
	delete _weights;
	_weights = _new DTFeature::FeatureWeightMap(50000);
	DTFeature::readWeights(*_weights, model_file, EventLinkFeatureType::modeltype);
	_model = _new MaxEntModel(_tagSet, _featureTypes, _weights);
}

void StatEventLinker::train() 
{
	if (MODE != TRAIN)
		return;
	
	// PRUNING
	int pruning = ParamReader::getRequiredIntParam("event_link_trainer_pruning_cutoff");

	// MODEL FILE
	std::string model_file = ParamReader::getRequiredParam("event_link_model_file");

	// TRAINING FILE
	std::string training_file_list = ParamReader::getRequiredParam("event_training_file_list");

	loadTrainingDataFromList(training_file_list.c_str());
	addTrainingDataToModel();
	_model->deriveModel(pruning);

	UTF8OutputStream out;
	out.open(model_file.c_str());

	if (out.fail()) {
		throw UnexpectedInputException("StatEventLinker::train()",
			"Could not open model file for writing");
	}

	dumpTrainingParameters(out);
	DTFeature::writeWeights(*_weights, out);
	out.close();
	
	for (int doc = 0; doc < _num_documents; doc++) {
		delete _docTheories[doc];
	}
	delete[] _docTheories;
	_docTheories = 0;

	// we are now set to decode
	MODE = DECODE;

}

void StatEventLinker::roundRobin() {
	
	std::string setup_file = ParamReader::getRequiredParam("event_round_robin_setup");
	std::string output_file = ParamReader::getRequiredParam("event_round_robin_results");

	UTF8OutputStream resultStream(output_file.c_str());

	boost::scoped_ptr<UTF8InputStream> countStream_scoped_ptr(UTF8InputStream::build(setup_file.c_str()));
	UTF8InputStream& countStream(*countStream_scoped_ptr);
	UTF8Token modelToken;
	UTF8Token messageToken;
	//int max_docs = 50; // fake it for now... takes too long to count in debug mode :) 
	int max_docs = 0;
	while (!countStream.eof()) {
		countStream >> messageToken;
		if (wcscmp(messageToken.chars(), L"") == 0)
			break;
		countStream >> modelToken;
		if (wcscmp(modelToken.chars(), L"") == 0)
			break;
		int ndocs = TrainingLoader::countDocumentsInFile(messageToken.chars());
		max_docs = (ndocs > max_docs) ? ndocs : max_docs;	
	}
	countStream.close();

	_docTheories = _new DocTheory * [max_docs];

	boost::scoped_ptr<UTF8InputStream> setupFileStream_scoped_ptr(UTF8InputStream::build(setup_file.c_str()));
	UTF8InputStream& setupFileStream(*setupFileStream_scoped_ptr);
	resetRoundRobinStatistics();

	while (!setupFileStream.eof()) {
		setupFileStream >> messageToken;
		if (wcscmp(messageToken.chars(), L"") == 0)
			break;
		setupFileStream >> modelToken;
		if (wcscmp(modelToken.chars(), L"") == 0)
			break;

		_num_documents = 0;
		loadTrainingData(messageToken.chars(), _num_documents);

		char char_str[501];
		StringTransliterator::transliterateToEnglish(char_str, modelToken.chars(), 500);
		replaceModel(char_str);

		int link_index = _tagSet->getLinkTagIndex();
		for (int doc = 0; doc < _num_documents; doc++) {
			_observation->setEntitySet(_docTheories[doc]->getEntitySet());
			int n_vmentions = fillAllVMentionsArray(_docTheories[doc]);
			for (int i = 0; i < n_vmentions; i++) {
				for (int j = i + 1; j < n_vmentions; j++) {
					if (_allVMentions[i]->getEventType() != _allVMentions[j]->getEventType())
						continue;
					_observation->setEventMentions(_allVMentions[i], _allVMentions[j]);
					_model->decodeToDistribution(_observation, _tagScores, _tagSet->getNTags());
					if (_tagScores[link_index] > _link_threshold) {
						if (_allVMentions[i]->getEventID() == _allVMentions[j]->getEventID()) {
							_correct_link++;
							resultStream << L"<font color=\"red\">CORRECT</font> (";
							resultStream << _allVMentions[i]->getEventType() << "): <br>\n";
							printHTMLEventMention(_allVMentions[i], resultStream);
							printHTMLEventMention(_allVMentions[j], resultStream);
							_model->printHTMLDebugInfo(_observation, 0, resultStream);
							_model->printHTMLDebugInfo(_observation, link_index, resultStream);
							resultStream << "<br>\n";							
						} else {
							_spurious++;
							resultStream << L"<font color=\"blue\">SPURIOUS</font> (";
							resultStream << _allVMentions[i]->getEventType() << "): <br>\n";
							printHTMLEventMention(_allVMentions[i], resultStream);
							printHTMLEventMention(_allVMentions[j], resultStream);
							_model->printHTMLDebugInfo(_observation, 0, resultStream);
							_model->printHTMLDebugInfo(_observation, link_index, resultStream);
							resultStream << "<br>\n";	
						}
					} else {
						if (_allVMentions[i]->getEventID() != _allVMentions[j]->getEventID()) {
							_correct_no_link++;
						} else {
							_missed++;							
							resultStream << L"<font color=\"purple\">MISSED</font> (";
							resultStream << _allVMentions[i]->getEventType() << "): <br>\n";
							printHTMLEventMention(_allVMentions[i], resultStream);
							printHTMLEventMention(_allVMentions[j], resultStream);
							_model->printHTMLDebugInfo(_observation, 0, resultStream);
							_model->printHTMLDebugInfo(_observation, link_index, resultStream);
							resultStream << "<br>\n";
						}
					}
				}
			}
		}


		for (int i = 0; i < _num_documents; i++) {
			delete _docTheories[i];
		}
	}
	
	printRoundRobinStatistics(resultStream);
	resultStream.close();

	delete[] _docTheories;

}

void StatEventLinker::printHTMLEventMention(EventMention *vm, UTF8OutputStream &out) {
	out << vm->getAnchorNode()->getHeadWord();
	out << L" (";
	for (int i = 0; i < vm->getNArgs(); i++) {
		out << vm->getNthArgRole(i) << ": ";
		out << vm->getNthArgMention(i)->getNode()->toTextString();
		if (i != vm->getNArgs() - 1)
			out << ", ";
	}
	out << ")<br>\n";
}

void StatEventLinker::loadTrainingDataFromList(const char *listfile) {

	_num_documents = TrainingLoader::countDocumentsInFileList(listfile);
	_docTheories = _new DocTheory * [_num_documents];

	boost::scoped_ptr<UTF8InputStream> fileListStream_scoped_ptr(UTF8InputStream::build(listfile));
	UTF8InputStream& fileListStream(*fileListStream_scoped_ptr);	
	UTF8Token token;
	
	int index = 0;
	while (!fileListStream.eof()) {
		fileListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		loadTrainingData(token.chars(), index);
	}

}

void StatEventLinker::addTrainingDataToModel() {
	for (int doc = 0; doc < _num_documents; doc++) {
		_observation->setEntitySet(_docTheories[doc]->getEntitySet());
		int n_vmentions = fillAllVMentionsArray(_docTheories[doc]);
		for (int i = 0; i < n_vmentions; i++) {
			for (int j = i + 1; j < n_vmentions; j++) {
				if (_allVMentions[i]->getEventType() != _allVMentions[j]->getEventType())
					continue;
				_observation->setEventMentions(_allVMentions[i], _allVMentions[j]);
				if (_allVMentions[i]->getEventID() == _allVMentions[j]->getEventID())
					_model->addToTraining(_observation, _tagSet->getLinkTagIndex());
				else _model->addToTraining(_observation, _tagSet->getNoneTagIndex());
			}
		}
	}
}

int StatEventLinker::fillAllVMentionsArray(DocTheory *docTheory) {
	EventSet *vset = docTheory->getEventSet();
	int n_vmentions = 0;
	for (int i = 0; i < vset->getNEvents(); i++) {
		Event::LinkedEventMention *lem = vset->getEvent(i)->getEventMentions();
		while (lem != 0) {
			_allVMentions[n_vmentions++] = lem->eventMention;
			lem = lem->next;
		}
	}
	return n_vmentions;
}

void StatEventLinker::loadTrainingData(const wchar_t *filename, int& index) {
	char state_file_name_str[501];
	StringTransliterator::transliterateToEnglish(state_file_name_str, filename, 500);
	SessionLogger::info("SERIF") << "Loading data from " << state_file_name_str << "\n";
	
	StateLoader *stateLoader = _new StateLoader(state_file_name_str);
	int num_docs = TrainingLoader::countDocumentsInFile(filename);

	wchar_t state_tree_name[100];
	wcscpy(state_tree_name, L"DocTheory following stage: doc-relations-events");

	for (int i = 0; i < num_docs; i++) {
		_docTheories[index] = _new DocTheory(static_cast<Document*>(0));
		_docTheories[index]->loadFakedDocTheory(stateLoader, state_tree_name);
		_docTheories[index]->resolvePointers(stateLoader);
		index++;
	}
}

void StatEventLinker::getLinkScores(EntitySet *entitySet,
									std::vector<EventMention*> &vMentions, 
									int n_vmentions, 
									std::vector< std::vector<float> > &scores)
{
	_observation->setEntitySet(entitySet);
	int link_index = _tagSet->getLinkTagIndex();
	std::vector<float> empty_vector; //empty vector to populate scores with
	scores.resize((size_t)n_vmentions, empty_vector); //make sure the vector is large enough
	for (int i = 0; i < n_vmentions; i++) {
		scores[i].resize((size_t)n_vmentions,0);
		for (int j = i + 1; j < n_vmentions; j++) {
			if (vMentions[i]->getEventType() != vMentions[j]->getEventType()) {
				scores[i][j] = 0;
				continue;		
			}
			_observation->setEventMentions(vMentions[i], vMentions[j]);
			_model->decodeToDistribution(_observation, _tagScores, _tagSet->getNTags());
			scores[i][j] = (float) _tagScores[link_index];
		}
	}
}

void StatEventLinker::dumpTrainingParameters(UTF8OutputStream &out) {

	DTFeature::recordDate(out);

	out << L"Parameters:\n";
	DTFeature::recordParamForConsistency(Symbol(L"event_link_tag_set_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"event_link_features_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"word_cluster_bits_file"), out);

	DTFeature::recordParamForReference(Symbol(L"event_training_file_list"), out);
	DTFeature::recordParamForReference(Symbol(L"event_link_trainer_percent_held_out"), out);
	DTFeature::recordParamForReference(Symbol(L"event_link_trainer_max_iterations"), out);
	DTFeature::recordParamForReference(Symbol(L"event_link_trainer_gaussian_variance"), out);
	DTFeature::recordParamForReference(Symbol(L"event_link_trainer_min_likelihood_delta"), out);
	DTFeature::recordParamForReference(Symbol(L"event_link_trainer_stop_check_frequency"), out);
	DTFeature::recordParamForReference(Symbol(L"event_link_trainer_pruning_cutoff"), out);
	out << L"\n";

}


void StatEventLinker::resetRoundRobinStatistics() {
	_correct_link = 0;
	_correct_no_link = 0;
	_missed = 0;
	_spurious = 0;
}

void StatEventLinker::printRoundRobinStatistics(UTF8OutputStream &out) {

	double recall = (double) _correct_link / (_missed + _correct_link);
	double precision = (double) _correct_link / (_spurious + _correct_link);

	out << L"CORRECT: " << _correct_link + _correct_no_link << L" ";
	out << L"(links: " << _correct_link << L", non-links: ";
	out << _correct_no_link << L")<br>\n";
	out << L"MISSED: " << _missed << L"<br>\n";
	out << L"SPURIOUS: " << _spurious << L"<br>\n";
	out << L"RECALL: " << recall << L"<br>\n";
	out << L"PRECISION: " << precision << L"<br><br>\n\n";

}
