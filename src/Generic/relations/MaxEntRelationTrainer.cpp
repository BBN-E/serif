// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/FeatureModule.h"
#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/theories/Token.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/relations/xx_RelationUtilities.h"
#include "Generic/relations/MaxEntRelationTrainer.h"
#include "Generic/relations/discmodel/P1RelationFeatureTypes.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/DTRelationSet.h"
#include "Generic/relations/discmodel/DTRelSentenceInfo.h"
#include "Generic/relations/HighYield/HYInstanceSet.h"
#include "Generic/maxent/MaxEntModel.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/TrainingLoader.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/NPChunkTheory.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"

#include "Generic/docRelationsEvents/DocEventHandler.h"

#include <iostream>
#include <stdio.h>
#include <boost/scoped_ptr.hpp>

using namespace std;


MaxEntRelationTrainer::MaxEntRelationTrainer()
	: _featureTypes(0), _tagSet(0),
	  _decoder(0), _weights(0), _num_sentences(0),
	   _use_high_yield_annotation(false),
	  _highYieldPool(0), _highYieldAnnotation(0),
	  _do_not_cache_sentences(false),
  	  _filter_mode(false),
	  _set_secondary_parse(true)
{
	FeatureModule::load();
	std::vector<std::string> modules = FeatureModule::loadedFeatureModules();
	if (!modules.empty()) {
		cout << "  Using Modules:";
		BOOST_FOREACH(std::string name, modules) {
			cout << " " << name;
		}
		cout << "\n";
	}
	P1RelationFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();
	_observation = _new RelationObservation();

	//Initialize the Morphological Analyzer,
	//Because Arabic needs it to load state files
	_morphAnalysis = MorphologicalAnalyzer::build();

	// TAG SET
	std::string tag_set_file = ParamReader::getRequiredParam("maxent_relation_tag_set_file");

	_filter_mode = ParamReader::isParamTrue("maxent_relation_filter_mode");
	if (_filter_mode) {
		std::string filter_tag_set_file = ParamReader::getRequiredParam("maxent_relation_filter_tag_set_file");
		_tagSet = _new DTTagSet(filter_tag_set_file.c_str(), false, false);
		_filterTag = _tagSet->getTagSymbol(_tagSet->getNTags() - 1);
	} else {
		_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);
	}

	// FEATURES
	std::string features_file = ParamReader::getRequiredParam("maxent_relation_features_file");
	if (_filter_mode) {
		std::string filter_features_file = ParamReader::getRequiredParam("maxent_relation_filter_features_file");
		_featureTypes = _new DTFeatureTypeSet(filter_features_file.c_str(), P1RelationFeatureType::modeltype);
	}
	else {
		_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), P1RelationFeatureType::modeltype);
	}


	// TRAINING ONLY FEATURES (not devtest)
	if (!ParamReader::isParamTrue("maxent_relation_devtest"))	{
		// TRAIN MODE
		std::string param_mode = ParamReader::getRequiredParam("maxent_trainer_mode");
		if (param_mode == "GIS")
			_mode = MaxEntModel::GIS;
		else if (param_mode == "SCGIS")
			_mode = MaxEntModel::SCGIS;
		else
			throw UnexpectedInputException("MaxEntRelationTrainer::MaxEntRelationTrainer()",
							"Invalid setting for parameter 'maxent_trainer_mode'");

		// PRUNING
		_pruning = ParamReader::getRequiredIntParam("maxent_trainer_pruning_cutoff");

		// PERCENT HELD OUT
		_percent_held_out = ParamReader::getRequiredIntParam("maxent_trainer_percent_held_out");
		if (_percent_held_out < 0 || _percent_held_out > 50)
			throw UnexpectedInputException("MaxEntRelationTrainer::MaxEntRelationTrainer()",
				"Parameter 'maxent_trainer_percent_held_out' must be between 0 and 50");

		// MAX NUMBER OF ITERATIONS (STOPPING CONDITION)
		_max_iterations = ParamReader::getRequiredIntParam("maxent_trainer_max_iterations");

		// GAUSSIAN PRIOR VARIANCE
		_variance = ParamReader::getRequiredFloatParam("maxent_trainer_gaussian_variance");

		// MIN CHANGE IN LIKELIHOOD (STOPPING CONDITION)
		_likelihood_delta = ParamReader::getOptionalFloatParamWithDefaultValue("maxent_min_likelihood_delta", .0001);

		// FREQUENCY OF STOPPING CONDITION CHECKS (NUM ITERATIONS)
		_stop_check_freq = ParamReader::getOptionalIntParamWithDefaultValue("maxent_stop_check_frequency",1);

		_train_vector_file = ParamReader::getParam("maxent_train_vector_file");
		_test_vector_file = ParamReader::getParam("maxent_test_vector_file");
	}



	// MODEL FILE	
	_model_file = ParamReader::getRequiredParam("maxent_relation_model_file");
	if (_filter_mode) {
		_filter_model_file = ParamReader::getRequiredParam("maxent_relation_filter_model_file");
	}

	// TRAINING DATA LIST
	std::string file_list = ParamReader::getRequiredParam("maxent_training_file_list");

	// BEAM WIDTH -- this needs to be the same as in training, or the stateloader can't work
	_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("beam_width", 1);

	//This is required to be set before loading data
	_set_secondary_parse = ParamReader::getOptionalTrueFalseParamWithDefaultVal("maxent_set_secondary_parse", true);

	// LOAD DATA
	_do_not_cache_sentences = ParamReader::isParamTrue("do_not_cache_sentences");
	_trainingLoader = _new TrainingLoader(file_list.c_str(), L"doc-relations-events");
	if (_do_not_cache_sentences) {
		_sentenceInfo = _new DTRelSentenceInfo*;
	} else {
		_sentenceInfo = _new DTRelSentenceInfo *[_trainingLoader->getMaxSentences()];
	}
	_num_sentences = loadTrainingData();

	// CREATE WEIGHTS TABLE
	_weights = _new DTFeature::FeatureWeightMap(50000);

	_correct = 0;
	_missed = 0;
	_spurious = 0;
	_wrong_type = 0;

	// ADD HIGH YIELD ANNOTATION
	if (ParamReader::isParamTrue("maxent_use_high_yield_annotation"))
		_use_high_yield_annotation = true;

	if (_use_high_yield_annotation) {
		// READ IN STATE FILES FOR HIGH YIELD POOL
		std::string high_yield_pool = ParamReader::getRequiredParam("maxent_high_yield_pool_list");
		_highYieldPool = _new HYInstanceSet(_tagSet);
		_highYieldPool->addDataFromSerifFileList(high_yield_pool.c_str(), _beam_width);

		// READ IN ACTUAL HIGH YIELD ANNOTATION
		std::string high_yield_annotation_list = ParamReader::getRequiredParam("maxent_high_yield_annotation_list");
		_highYieldAnnotation = _new HYInstanceSet(_tagSet);
		boost::scoped_ptr<UTF8InputStream> tempFileListStream_scoped_ptr(UTF8InputStream::build(high_yield_annotation_list.c_str()));
		UTF8InputStream& tempFileListStream(*tempFileListStream_scoped_ptr);
		UTF8Token token;
		while (!tempFileListStream.eof()) {
			tempFileListStream >> token;
			if (wcscmp(token.chars(), L"") == 0)
				break;
			_highYieldAnnotation->addDataFromAnnotationFile(token.chars(), *_highYieldPool);
		}
		tempFileListStream.close();
	}

}

MaxEntRelationTrainer::~MaxEntRelationTrainer() {
	delete _featureTypes;
	delete _tagSet;
	if (_highYieldPool != 0) {
		_highYieldPool->deleteObjects();
		delete _highYieldPool;
	}
	if (_highYieldAnnotation != 0) {
		// I think this just points to objects also in 
		// _highYieldPool, so we don't need to delete both.
		//_highYieldAnnotation->deleteObjects(); 
		delete _highYieldAnnotation;
	}
}


int MaxEntRelationTrainer::loadTrainingData() {
	int max_sentences = _trainingLoader->getMaxSentences();
	int i;

	if (_do_not_cache_sentences) return max_sentences; //This is an upperbound of the actual number of the annotated sentences.

	for (i = 0; i < max_sentences; i++) {
		SentenceTheory *theory = _trainingLoader->getNextSentenceTheory();
		const DocTheory* dt = _trainingLoader->getCurrentDocTheory();
		if (theory == 0)
			return i;
		_sentenceInfo[i] = _new DTRelSentenceInfo(1);
		_sentenceInfo[i]->parses[0] = theory->getPrimaryParse();
		_sentenceInfo[i]->parses[0]->gainReference();
		_sentenceInfo[i]->npChunks[0] = theory->getNPChunkTheory();
		if (_sentenceInfo[i]->npChunks[0] != 0)
			_sentenceInfo[i]->npChunks[0]->gainReference();
		//make sure this isnt double counting when fullParse == primaryParse
		if (_set_secondary_parse) {
			_sentenceInfo[i]->secondaryParses[0] = theory->getFullParse();
			_sentenceInfo[i]->secondaryParses[0]->gainReference();
		} else {
			_sentenceInfo[i]->secondaryParses[0] = 0;
		}
		_sentenceInfo[i]->mentionSets[0] = theory->getMentionSet();
		_sentenceInfo[i]->mentionSets[0]->gainReference();
		_sentenceInfo[i]->valueMentionSets[0] = theory->getValueMentionSet();
		_sentenceInfo[i]->valueMentionSets[0]->gainReference();
		_sentenceInfo[i]->propSets[0] = theory->getPropositionSet();
		_sentenceInfo[i]->propSets[0]->gainReference();
		_sentenceInfo[i]->propSets[0]->fillDefinitionsArray();
		_sentenceInfo[i]->relSets[0] =
			_new DTRelationSet(_sentenceInfo[i]->mentionSets[0]->getNMentions(),
			theory->getRelMentionSet(), _tagSet->getNoneTag());		
		_sentenceInfo[i]->entitySets[0] = _trainingLoader->getCurrentEntitySet();
		_sentenceInfo[i]->entitySets[0]->gainReference();
		_sentenceInfo[i]->documentTopic = DocEventHandler::assignTopic(dt);
	}
	return i;
}

SentenceTheory* MaxEntRelationTrainer::loadNextTrainingSentence() {
	SentenceTheory *theory = _trainingLoader->getNextSentenceTheory();
	const DocTheory* dt = _trainingLoader->getCurrentDocTheory();
	if (theory == 0)
		return 0;
	_sentenceInfo[0] = _new DTRelSentenceInfo(1);
	_sentenceInfo[0]->parses[0] = theory->getPrimaryParse();
	_sentenceInfo[0]->parses[0]->gainReference();
	_sentenceInfo[0]->npChunks[0] = theory->getNPChunkTheory();
	if (_sentenceInfo[0]->npChunks[0] != 0)
		_sentenceInfo[0]->npChunks[0]->gainReference();
	//make sure this isnt double counting when fullParse == primaryParse
	if (_set_secondary_parse) {
		_sentenceInfo[0]->secondaryParses[0] = theory->getFullParse();
		_sentenceInfo[0]->secondaryParses[0]->gainReference();
	} else {
		_sentenceInfo[0]->secondaryParses[0] = 0;
	}
	_sentenceInfo[0]->mentionSets[0] = theory->getMentionSet();
	_sentenceInfo[0]->mentionSets[0]->gainReference();
	_sentenceInfo[0]->valueMentionSets[0] = theory->getValueMentionSet();
	_sentenceInfo[0]->valueMentionSets[0]->gainReference();
	_sentenceInfo[0]->propSets[0] = theory->getPropositionSet();
	_sentenceInfo[0]->propSets[0]->gainReference();
	_sentenceInfo[0]->propSets[0]->fillDefinitionsArray();
	_sentenceInfo[0]->relSets[0] =
		_new DTRelationSet(_sentenceInfo[0]->mentionSets[0]->getNMentions(), 
		theory->getRelMentionSet(), _tagSet->getNoneTag());		
	_sentenceInfo[0]->entitySets[0] = _trainingLoader->getCurrentEntitySet();
	_sentenceInfo[0]->entitySets[0]->gainReference();
	_sentenceInfo[0]->documentTopic = DocEventHandler::assignTopic(dt);

	return theory;
}

void MaxEntRelationTrainer::unloadSentence(DTRelSentenceInfo* sentenceInfo) {
	sentenceInfo->parses[0]->loseReference();
	if (sentenceInfo->npChunks[0] != 0)
		sentenceInfo->npChunks[0]->loseReference();
	if (sentenceInfo->secondaryParses[0] != 0)
		sentenceInfo->secondaryParses[0]->loseReference();
	sentenceInfo->mentionSets[0]->loseReference();
	sentenceInfo->valueMentionSets[0]->loseReference();
	sentenceInfo->propSets[0]->loseReference();
	delete sentenceInfo->relSets[0];
	sentenceInfo->entitySets[0]->loseReference();

	delete sentenceInfo;
}

void MaxEntRelationTrainer::devTest() {
	std::string param = ParamReader::getRequiredParam("maxent_relation_devtest_out");
	_devTestStream.open(param.c_str());

	if (_filter_mode) {
		DTFeature::readWeights(*_weights, _filter_model_file.c_str(), P1RelationFeatureType::modeltype);
	} else {
		DTFeature::readWeights(*_weights, _model_file.c_str(), P1RelationFeatureType::modeltype);
	}
	_decoder = _new MaxEntModel(_tagSet, _featureTypes, _weights);

	for (int i = 0; i < _num_sentences; i++) {
		if (i % 10 == 0)
			std::cerr << i << "\r";
		walkThroughSentence(i, DEVTEST);
	}

	double recall = (double) _correct / (_missed + _wrong_type + _correct);
	double precision = (double) _correct / (_spurious + _wrong_type + _correct);

	_devTestStream << "CORRECT: " << _correct << "<br>\n";
	_devTestStream << "MISSED: " << _missed << "<br>\n";
	_devTestStream << "SPURIOUS: " << _spurious << "<br>\n";
	_devTestStream << "WRONG TYPE: " << _wrong_type << "<br>\n<br>\n";
	_devTestStream << "RECALL: " << recall << "<br>\n";
	_devTestStream << "PRECISION: " << precision << "<br>\n";

}

void MaxEntRelationTrainer::train() {

	const char* train_vf = 0;
	if (!_train_vector_file.empty())
		train_vf = _train_vector_file.c_str();
	const char* test_vf = 0;
	if (!_test_vector_file.empty())
		test_vf = _test_vector_file.c_str();

	_decoder = _new MaxEntModel(_tagSet, _featureTypes, _weights,
								   _mode, _percent_held_out, _max_iterations, _variance,
								   _likelihood_delta, _stop_check_freq,
								   train_vf, test_vf);

	if (!_do_not_cache_sentences) {
		for (int i = 0; i < _num_sentences; i++)
			walkThroughSentence(i, TRAIN);
	} else {
		SentenceTheory *theory;
		while ((theory = loadNextTrainingSentence()) != 0) {
			walkThroughSentence(0, TRAIN);
			unloadSentence(_sentenceInfo[0]);
		}
	}

	if (_use_high_yield_annotation)
		walkThroughHighYieldAnnotation(TRAIN);

	_decoder->deriveModel(_pruning);

	writeWeights();

	// this isn't really necessary in practice, but helpful for memory leak
	// detection
	for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
		 iter != _weights->end(); ++iter)
	{
		(*iter).first->deallocate();
	}
	delete _weights;
	delete _decoder;
}

void MaxEntRelationTrainer::walkThroughSentence(int index, int mode) {
	const MentionSet *mset = _sentenceInfo[index]->mentionSets[0];
	_observation->resetForNewSentence(_sentenceInfo[index]);
	int nmentions = mset->getNMentions();
	int n_comparisons = 0;

	if (mode == DEVTEST) {
		_devTestStream << "Sentence " << index << ": ";
		_devTestStream << mset->getParse()->getRoot()->toTextString() << "<br>\n";
	}

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

			int correct_answer = _tagSet->getTagIndex(getModelTagSymbol(_sentenceInfo[index]->relSets[0]->getRelation(i,j)));
			Symbol correct_symbol = getModelTagSymbol(_sentenceInfo[index]->relSets[0]->getRelation(i,j));

			n_comparisons++;
			if (correct_answer == -1) {
				char error[150];
				sprintf(error, "unknown relation type in training: %s",
					_sentenceInfo[index]->relSets[0]->getRelation(i,j).to_debug_string());
				throw UnexpectedInputException("MaxEntRelationTrainer::trainSentence()", error);
			}
			if (mode == DEVTEST && _decoder->DEBUG) {
				_decoder->_debugStream << "Sentence " << index << ":" << n_comparisons << "\n";
				_decoder->_debugStream << "RELATION: " << _tagSet->getTagSymbol(correct_answer).to_string() << "\n";
				_decoder->_debugStream << "MENTIONS:\n";
				_decoder->_debugStream << _observation->getMention1()->getNode()->toTextString() << L"\n";
				_decoder->_debugStream << _observation->getMention2()->getNode()->toTextString() << L"\n";
			}
			/* Uncomment this to debug output relation head words and predicates
			if (mode == DEVTEST && _decoder->DEBUG) {
				_decoder->_debugStream << _tagSet->getTagSymbol(correct_answer).to_string() << L" ";
				if (_sentenceInfo[index]->relSets[0]->hasReversedRelation(i,j)) {
					_decoder->_debugStream << mset->getMention(j)->getNode()->getHeadWord().to_string() << L" ";
					_decoder->_debugStream << mset->getMention(i)->getNode()->getHeadWord().to_string() << L" ";
				}
				else {
					_decoder->_debugStream << mset->getMention(i)->getNode()->getHeadWord().to_string() << L" ";
					_decoder->_debugStream << mset->getMention(j)->getNode()->getHeadWord().to_string() << L" ";
				}
				RelationPropLink *link = _observation->getPropLink();
				if (link !=0 && !link->isEmpty() && !link->isNested()) {
					Proposition *prop = link->getTopProposition();
					if (prop != 0 && !prop->getPredSymbol().is_null()) {
 						_decoder->_debugStream << prop->getPredSymbol().to_string() << L" ";
						_decoder->_debugStream << prop->getPredTypeString(prop->getPredType());
					}
				}
				_decoder->_debugStream << "\n";

			}*/

			if (mode == TRAIN) {

				_decoder->addToTraining(_observation, correct_answer);

			} else if (mode == DEVTEST) {
				Symbol correctAnswer = getModelTagSymbol(correct_symbol);
				Symbol hypothesis = _decoder->decodeToSymbol(_observation);
				/*if (correctAnswer != _tagSet->getNoneTag() &&
					_observation->getPropLink()->isEmpty() &&
					mset->getMention(i)->getMentionType() != Mention::PART &&
					mset->getMention(j)->getMentionType() != Mention::PART)
				{
					_devTestStream << L"<b>Answer without prop link (";
					for (int th = 1; th < 10; th++) {
						if (!_observation->getNthPropLink(th)->isEmpty()) {
							_devTestStream << "X";
						}
					}
					_devTestStream << "):</b> ";
				}*/
				if (correctAnswer != _tagSet->getNoneTag() ||
					hypothesis != _tagSet->getNoneTag())
				{
					_devTestStream << n_comparisons << ": ";
					_devTestStream << mset->getMention(i)->getNode()->getHeadWord().to_string();
					_devTestStream << L" & ";
					_devTestStream << mset->getMention(j)->getNode()->getHeadWord().to_string();
					_devTestStream << L": ";
					_devTestStream << hypothesis.to_string() << L" ";
					if (correctAnswer == hypothesis) {
						_devTestStream << L"<font color=\"red\">CORRECT</font><br>\n";
						_correct++;
					} else if (correctAnswer == _tagSet->getNoneTag()) {
						_devTestStream << L"<font color=\"blue\">SPURIOUS</font><br>\n";
						_spurious++;
					} else if (hypothesis == _tagSet->getNoneTag()) {
						_devTestStream << L"<font color=\"purple\">MISSING (";
						_devTestStream << correctAnswer.to_string();
						_devTestStream << ")</font><br>\n";
						_missed++;
					} else if (correctAnswer != hypothesis) {
						_devTestStream << L"<font color=\"green\">WRONG TYPE (";
						_devTestStream << correctAnswer.to_string();
						_devTestStream << ")</font><br>\n";
						_wrong_type++;
					}
				}
			}
		}
	}
	if (mode == DEVTEST) {
		_devTestStream << "<br>\n";
	}

}

void MaxEntRelationTrainer::walkThroughHighYieldAnnotation(int mode) {
	int n_comparisons = 0;

	for (vector<HYRelationInstance*>::iterator iter = _highYieldAnnotation->begin();
		iter < _highYieldAnnotation->end();
		++iter)
	{
		HYRelationInstance* instance = *iter;
		
		if (!instance->isValid()) 
			continue;

		DTRelSentenceInfo* sentenceInfo = instance->getSentenceInfo();
		const MentionSet *mset = sentenceInfo->mentionSets[0];
		int i = instance->getFirstIndex();
		int j = instance->getSecondIndex();
		Symbol relation = instance->getRelationSymbol();
		_observation->resetForNewSentence(sentenceInfo);

		if (!RelationUtilities::get()->validRelationArgs(mset->getMention(i), mset->getMention(j)))
				continue;

		if (mode == DEVTEST) {
			_devTestStream << "Sentence ?? " << ": ";
			_devTestStream << mset->getParse()->getRoot()->toTextString() << "<br>\n";
		}

		_observation->populate(i, j);

		int correct_answer = _tagSet->getTagIndex(instance->getRelationSymbol());
		Symbol correct_symbol = instance->getRelationSymbol();
		
		n_comparisons++;
		if (correct_answer == -1) {
			char error[150];
			sprintf(error, "unknown relation type in training: %s",
				sentenceInfo->relSets[0]->getRelation(i,j).to_debug_string());
			throw UnexpectedInputException("MaxEntRelationTrainer::trainSentence()", error);
		}
		if (mode == DEVTEST && _decoder->DEBUG) {
			_decoder->_debugStream << "Sentence ?? " << ":" << n_comparisons << "\n";
			_decoder->_debugStream << "RELATION: " << _tagSet->getTagSymbol(correct_answer).to_string() << "\n";
			_decoder->_debugStream << "MENTIONS:\n";
			_decoder->_debugStream << _observation->getMention1()->getNode()->toTextString() << L"\n";
			_decoder->_debugStream << _observation->getMention2()->getNode()->toTextString() << L"\n";
		}
		/* Uncomment this to debug output relation head words and predicates
		if (mode == DEVTEST && _decoder->DEBUG) {
			_decoder->_debugStream << _tagSet->getTagSymbol(correct_answer).to_string() << L" ";
			if (_sentenceInfo[index]->relSets[0]->hasReversedRelation(i,j)) {
				_decoder->_debugStream << mset->getMention(j)->getNode()->getHeadWord().to_string() << L" ";
				_decoder->_debugStream << mset->getMention(i)->getNode()->getHeadWord().to_string() << L" ";
			}
			else {
				_decoder->_debugStream << mset->getMention(i)->getNode()->getHeadWord().to_string() << L" ";
				_decoder->_debugStream << mset->getMention(j)->getNode()->getHeadWord().to_string() << L" ";
			}
			RelationPropLink *link = _observation->getPropLink();
			if (link !=0 && !link->isEmpty() && !link->isNested()) {
				Proposition *prop = link->getTopProposition();
				if (prop != 0 && !prop->getPredSymbol().is_null()) {
 					_decoder->_debugStream << prop->getPredSymbol().to_string() << L" ";
					_decoder->_debugStream << prop->getPredTypeString(prop->getPredType());
				}
			}
			_decoder->_debugStream << "\n";

		}*/

		if (mode == TRAIN) {

			_decoder->addToTraining(_observation, correct_answer);

		} else if (mode == DEVTEST) {
			Symbol correctAnswer = correct_symbol;
			Symbol hypothesis = _decoder->decodeToSymbol(_observation);
			/*if (correctAnswer != _tagSet->getNoneTag() &&
				_observation->getPropLink()->isEmpty() &&
				mset->getMention(i)->getMentionType() != Mention::PART &&
				mset->getMention(j)->getMentionType() != Mention::PART)
			{
				_devTestStream << L"<b>Answer without prop link (";
				for (int th = 1; th < 10; th++) {
					if (!_observation->getNthPropLink(th)->isEmpty()) {
						_devTestStream << "X";
					}
				}
				_devTestStream << "):</b> ";
			}*/
			if (correctAnswer != _tagSet->getNoneTag() ||
				hypothesis != _tagSet->getNoneTag())
			{
				_devTestStream << n_comparisons << ": ";
				_devTestStream << mset->getMention(i)->getNode()->getHeadWord().to_string();
				_devTestStream << L" & ";
				_devTestStream << mset->getMention(j)->getNode()->getHeadWord().to_string();
				_devTestStream << L": ";
				_devTestStream << hypothesis.to_string() << L" ";
				if (correctAnswer == hypothesis) {
					_devTestStream << L"<font color=\"red\">CORRECT</font><br>\n";
					_correct++;
				} else if (correctAnswer == _tagSet->getNoneTag()) {
					_devTestStream << L"<font color=\"blue\">SPURIOUS</font><br>\n";
					_spurious++;
				} else if (hypothesis == _tagSet->getNoneTag()) {
					_devTestStream << L"<font color=\"purple\">MISSING (";
					_devTestStream << correctAnswer.to_string();
					_devTestStream << ")</font><br>\n";
					_missed++;
				} else if (correctAnswer != hypothesis) {
					_devTestStream << L"<font color=\"green\">WRONG TYPE (";
					_devTestStream << correctAnswer.to_string();
					_devTestStream << ")</font><br>\n";
					_wrong_type++;
				}
			}
		}
	}
	if (mode == DEVTEST) {
		_devTestStream << "<br>\n";
	}

}

void MaxEntRelationTrainer::writeWeights() {
	UTF8OutputStream out;
	if (_filter_mode) {
		out.open(_filter_model_file.c_str());
	} else {
		out.open(_model_file.c_str());
	}

	if (out.fail()) {
		throw UnexpectedInputException("MaxEntRelationTrainer::writeWeights()",
			"Could not open model file for writing");
	}

	dumpTrainingParameters(out);
	DTFeature::writeWeights(*_weights, out);
	out.close();

}

void MaxEntRelationTrainer::dumpTrainingParameters(UTF8OutputStream &out) {

	DTFeature::recordDate(out);

	out << L"Parameters:\n";
	
	DTFeature::recordFileListForReference(Symbol(L"maxent_training_file_list"), out);

	DTFeature::recordParamForConsistency(Symbol(L"maxent_relation_tag_set_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"maxent_relation_features_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"word_cluster_bits_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"lc_word_cluster_bits_file"), out);

	DTFeature::recordParamForReference(Symbol(L"word_net_dictionary_path"), out);
	DTFeature::recordParamForReference(Symbol(L"wordnet_level_start"), out);
	DTFeature::recordParamForReference(Symbol(L"wordnet_level_interval"), out);

	DTFeature::recordParamForReference(Symbol(L"maxent_trainer_percent_held_out"), out);
	DTFeature::recordParamForReference(Symbol(L"maxent_trainer_max_iterations"), out);
	DTFeature::recordParamForReference(Symbol(L"maxent_trainer_gaussian_variance"), out);
	DTFeature::recordParamForReference(Symbol(L"maxent_trainer_min_likelihood_delta"), out);
	DTFeature::recordParamForReference(Symbol(L"maxent_trainer_stop_check_frequency"), out);
	DTFeature::recordParamForReference(Symbol(L"maxent_trainer_pruning_cutoff"), out);
	DTFeature::recordParamForReference(Symbol(L"maxent_trainer_mode"), out);
	DTFeature::recordParamForReference(Symbol(L"maxent_train_vector_file"), out);
	DTFeature::recordParamForReference(Symbol(L"maxent_test_vector_file"), out);
	DTFeature::recordParamForReference(Symbol(L"relation_finder_use_alt_models"), out);
	DTFeature::recordParamForConsistency(Symbol(L"relation_mention_dist_cutoff"), out);
	DTFeature::recordParamForReference(Symbol(L"allow_relations_within_distance"), out);

	if (_filter_mode) {
		DTFeature::recordParamForReference(Symbol(L"maxent_relation_filter_mode"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_relation_filter_tag_set_file"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_relation_filter_features_file"), out);
	}

	if (_use_high_yield_annotation) {
		DTFeature::recordParamForReference(Symbol(L"maxent_high_yield_pool_list"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_high_yield_annotation_list"), out);
	}
	
	out << L"\n";

}

Symbol MaxEntRelationTrainer::getModelTagSymbol(const Symbol symbol) {
	if (_filter_mode) {
		if (symbol != _tagSet->getNoneTag()) {
			return _filterTag;
		}
	}
	return symbol;
}
