// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "docRelationsEvents/RelationTimexArgFinder.h"
#include "docRelationsEvents/RelationTimexArgFeatureTypes.h"
#include "docRelationsEvents/RelationTimexArgFeatureType.h"
#include "docRelationsEvents/RelationTimexArgObservation.h"
#include "common/UnexpectedInputException.h"
#include "common/StringTransliterator.h"
#include "theories/RelMention.h"
#include "theories/RelMentionSet.h"
#include "theories/Token.h"
#include "common/ParamReader.h"
#include "common/HeapChecker.h"
#include "maxent/MaxEntModel.h"
#include "discTagger/DTFeatureTypeSet.h"
#include "discTagger/DTTagSet.h"
#include "theories/TokenSequence.h"
#include "theories/Parse.h"
#include "theories/NPChunkTheory.h"
#include "theories/MentionSet.h"
#include "theories/ValueMentionSet.h"
#include "theories/PropositionSet.h"
#include "theories/RelMentionSet.h"
#include "wordClustering/WordClusterTable.h"
#include "state/TrainingLoader.h"
#include "morphAnalysis/MorphologicalAnalyzer.h"
#include <boost/scoped_ptr.hpp>

RelationTimexArgFinder::RelationTimexArgFinder(int mode_) : MODE(mode_),
	_tagSet(0),	_tagScores(0), _featureTypes(0), _weights(0),
    _model(0), _secondPassWeights(0), _secondPassModel(0), _morphAnalysis(0)
{
	_observation = _new RelationTimexArgObservation();
	RelationTimexArgFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();

	if (MODE == TRAIN && ParamReader::isParamTrue("train_second_pass_relation_time_model"))
		MODE = TRAIN_SECOND_PASS;

	// DEBUGGING
	std::string buffer = ParamReader::getParam("relation_time_debug_file");
	DEBUG = false;
	if (!buffer.empty()) {
		DEBUG = true;
		_debugStream.open(buffer.c_str());
	}

	// TAG SET
	std::string tag_set_file = ParamReader::getRequiredParam("relation_time_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);
	_tagScores = _new double[_tagSet->getNTags()];

	// FEATURES
	std::string features_file = ParamReader::getRequiredParam("relation_time_features_file");
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), RelationTimexArgFeatureType::modeltype);

	_use_two_pass_model = ParamReader::isParamTrue("use_two_pass_time_arg_model");
	if (_use_two_pass_model || MODE == TRAIN_SECOND_PASS) {
		std::string second_pass_features_file =	ParamReader::getRequiredParam("relation_time_second_pass_features_file");
		_secondPassFeatureTypes = _new DTFeatureTypeSet(second_pass_features_file.c_str(), 
			RelationTimexArgFeatureType::modeltype);
	}


	if (MODE == DECODE) {

		// RECALL THRESHOLD
		_recall_threshold = ParamReader::getRequiredFloatParam("relation_time_recall_threshold");

		// MODEL FILE
		std::string model_file = ParamReader::getParam("relation_time_model_file");
		if (!model_file.empty()) {
			_weights = _new DTFeature::FeatureWeightMap(50000);
			DTFeature::readWeights(*_weights, model_file.c_str(), RelationTimexArgFeatureType::modeltype);
			_model = _new MaxEntModel(_tagSet, _featureTypes, _weights);

			if (_use_two_pass_model) {
				std::string buffer = model_file + ".second-pass";
				_secondPassWeights = _new DTFeature::FeatureWeightMap(50000);
				DTFeature::readWeights(*_secondPassWeights, buffer.c_str(), RelationTimexArgFeatureType::modeltype);
				_secondPassModel = _new MaxEntModel(_tagSet, _secondPassFeatureTypes, _secondPassWeights);
			} 
			
		} else {
			model_file = ParamReader::getParam("relation_time_round_robin_setup");
			if (model_file.empty()) {
				throw UnexpectedInputException("RelationTimexArgFinder::RelationTimexArgFinder()",
					"Neither 'relation_time_model_file' nor 'event_round_robin_setup' specified");
			} else {
				_weights = 0;
				_model = 0;
				_secondPassWeights = 0;
				_secondPassModel = 0;
			}
		}

	} else if (MODE == TRAIN || MODE == TRAIN_SECOND_PASS) {

		// PERCENT HELD OUT
		int percent_held_out = 
			ParamReader::getRequiredIntParam("relation_time_maxent_trainer_percent_held_out");
		if (percent_held_out < 0 || percent_held_out > 50) 
			throw UnexpectedInputException("RelationTimexArgFinder::RelationTimexArgFinder()",
			"Parameter 'relation_time_maxent_trainer_percent_held_out' must be between 0 and 50");

		// MAX NUMBER OF ITERATIONS (STOPPING CONDITION)
		int max_iterations = 
			ParamReader::getRequiredIntParam("relation_time_maxent_trainer_max_iterations");

		// GAUSSIAN PRIOR VARIANCE
		double variance = ParamReader::getRequiredFloatParam("relation_time_maxent_trainer_gaussian_variance");

		// MIN CHANGE IN LIKELIHOOD (STOPPING CONDITION)
		double likelihood_delta = ParamReader::getOptionalFloatParamWithDefaultValue("relation_time_maxent_trainer_gaussian_variance", .0001);

		// FREQUENCY OF STOPPING CONDITION CHECKS (NUM ITERATIONS)
		int stop_check_freq = ParamReader::getOptionalIntParamWithDefaultValue("relation_time_maxent_trainer_stop_check_frequency",1);

		if (MODE == TRAIN) {
			_weights = _new DTFeature::FeatureWeightMap(50000);
			_model = _new MaxEntModel(_tagSet, _featureTypes, _weights, 
				MaxEntModel::SCGIS, percent_held_out, max_iterations, variance,
				likelihood_delta, stop_check_freq, "", "");
		} else {
			// load first-pass model
			std::string model_file = ParamReader::getRequiredParam("relation_time_1p_model_file");
			_weights = _new DTFeature::FeatureWeightMap(50000);
			DTFeature::readWeights(*_weights, model_file.c_str(), RelationTimexArgFeatureType::modeltype);
			_model = _new MaxEntModel(_tagSet, _featureTypes, _weights);
			
			// create empty second-pass model
			_secondPassWeights = _new DTFeature::FeatureWeightMap(50000);
			_secondPassModel = _new MaxEntModel(_tagSet, _secondPassFeatureTypes, _secondPassWeights, 
				MaxEntModel::SCGIS, percent_held_out, max_iterations, variance,
				likelihood_delta, stop_check_freq, "", "");			
		}
	}

	_morphAnalysis = MorphologicalAnalyzer::build();
}


RelationTimexArgFinder::~RelationTimexArgFinder() {
	delete _tagSet;
	delete _featureTypes;
	delete _weights;
	delete _model;
	delete _secondPassWeights;
	delete _secondPassModel;
	delete _morphAnalysis;

	delete [] _tagScores;
	delete _observation;
}

void RelationTimexArgFinder::cleanup() {
	delete _observation;
	_observation = 0;
}

void RelationTimexArgFinder::train() {
	std::string file_list = ParamReader::getRequiredParam("relation_time_training_file_list");

	TrainingLoader *trainingLoader = _new TrainingLoader(file_list.c_str(), L"doc-relations-events");
	HeapChecker::checkHeap("TL created");
	int max_sentences = trainingLoader->getMaxSentences();

	_tokenSequences = _new TokenSequence *[max_sentences];
	_parses = _new Parse *[max_sentences];
	_secondaryParses = _new Parse *[max_sentences];
	_npChunks = _new NPChunkTheory* [max_sentences];
	_mentionSets = _new MentionSet * [max_sentences];
	_valueMentionSets = _new ValueMentionSet * [max_sentences];
	_propSets = _new PropositionSet * [max_sentences];
	_relationMentionSets = _new RelMentionSet * [max_sentences];

	int num_sentences = loadTrainingData(trainingLoader);
	delete trainingLoader;

	SessionLogger::info("SERIF") << "done with loading\n";

	train(_tokenSequences, _parses, _valueMentionSets, _mentionSets,
		 _propSets, _relationMentionSets, num_sentences);
	
	for (int i = 0; i < num_sentences; i++) {
		delete _tokenSequences[i];
		delete _parses[i];
		delete _secondaryParses[i];
		delete _npChunks[i];
		delete _mentionSets[i];
		delete _valueMentionSets[i];
		delete _propSets[i];
		delete _relationMentionSets[i];
	}

	delete[] _tokenSequences;
	delete[] _parses;
	delete[] _secondaryParses;
	delete[] _npChunks;
	delete[] _mentionSets;
	delete[] _valueMentionSets;
	delete[] _propSets;
	delete[] _relationMentionSets;

	SessionLogger::info("SERIF") << "done with training\n";
}

void RelationTimexArgFinder::roundRobin() {

	std::string setup_file = ParamReader::getRequiredParam("relation_time_round_robin_setup");
	std::string output_file = ParamReader::getRequiredParam("relation_time_round_robin_results");

	UTF8OutputStream resultStream;

	std::string str(output_file.c_str());
	str += ".html";
	resultStream.open(str.c_str());		

	UTF8Token modelToken;
	UTF8Token messageToken;
	int max_sentences = 0;

	boost::scoped_ptr<UTF8InputStream> countStream_scoped_ptr(UTF8InputStream::build(setup_file.c_str()));
	UTF8InputStream& countStream(*countStream_scoped_ptr);
	while (!countStream.eof()) {
		countStream >> messageToken;
		if (wcscmp(messageToken.chars(), L"") == 0)
			break;
		countStream >> modelToken;
		if (wcscmp(modelToken.chars(), L"") == 0)
			break;
		TrainingLoader *trainingLoader = _new TrainingLoader(messageToken.chars(),L"doc-relations-events", true);
		int batch_max_sentences = trainingLoader->getMaxSentences();
		delete trainingLoader;
		max_sentences = (batch_max_sentences > max_sentences) ? batch_max_sentences : max_sentences;	
	}
	countStream.close();

	_tokenSequences = _new TokenSequence *[max_sentences];
	_parses = _new Parse *[max_sentences];
	_secondaryParses = _new Parse *[max_sentences];
	_npChunks = _new NPChunkTheory* [max_sentences];
	_mentionSets = _new MentionSet * [max_sentences];
	_valueMentionSets = _new ValueMentionSet * [max_sentences];
	_propSets = _new PropositionSet * [max_sentences];
	_relationMentionSets = _new RelMentionSet * [max_sentences];

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

		TrainingLoader *trainingLoader = _new TrainingLoader(messageToken.chars(),L"doc-relations-events", true);
		int num_sentences = loadTrainingData(trainingLoader);
		delete trainingLoader;

		char char_str[501];
		StringTransliterator::transliterateToEnglish(char_str, modelToken.chars(), 500);
		replaceModel(char_str);
		decode(_tokenSequences, _parses, _valueMentionSets, _mentionSets,
  			_propSets, _relationMentionSets, num_sentences, resultStream);


		for (int i = 0; i < num_sentences; i++) {
			delete _tokenSequences[i];
			delete _parses[i];
			delete _secondaryParses[i];
			delete _npChunks[i];
			delete _valueMentionSets[i];
			delete _mentionSets[i];
			delete _propSets[i];
			delete _relationMentionSets[i];
		}
	}

	printRoundRobinStatistics(resultStream);
	resultStream.close();

	delete[] _tokenSequences;
	delete[] _parses;
	delete[] _secondaryParses;
	delete[] _npChunks;
	delete[] _mentionSets;
	delete[] _valueMentionSets;
	delete[] _propSets;
	delete[] _relationMentionSets;

}

void RelationTimexArgFinder::devtest() {
	std::string file_list = ParamReader::getRequiredParam("relation_time_devtest_file_list");

	std::string output_stream = ParamReader::getRequiredParam("relation_time_devtest_output");
	UTF8OutputStream outputStream;
	outputStream.open(output_stream.c_str());

	TrainingLoader *trainingLoader = _new TrainingLoader(file_list.c_str(), L"doc-relations-events");
	HeapChecker::checkHeap("TL created");
	int max_sentences = trainingLoader->getMaxSentences();

	_tokenSequences = _new TokenSequence *[max_sentences];
	_parses = _new Parse *[max_sentences];
	_secondaryParses = _new Parse *[max_sentences];
	_npChunks = _new NPChunkTheory* [max_sentences];
	_mentionSets = _new MentionSet * [max_sentences];
	_valueMentionSets = _new ValueMentionSet * [max_sentences];
	_propSets = _new PropositionSet * [max_sentences];
	_relationMentionSets = _new RelMentionSet * [max_sentences];

	int num_sentences = loadTrainingData(trainingLoader);
	delete trainingLoader;

	SessionLogger::info("SERIF") << "done with loading\n";

	decode(_tokenSequences, _parses, _valueMentionSets, _mentionSets,
		 _propSets, _relationMentionSets, num_sentences, outputStream);

	printRoundRobinStatistics(outputStream);
	
	for (int i = 0; i < num_sentences; i++) {
		delete _tokenSequences[i];
		delete _parses[i];
		delete _secondaryParses[i];
		delete _npChunks[i];
		delete _mentionSets[i];
		delete _valueMentionSets[i];
		delete _propSets[i];
		delete _relationMentionSets[i];
	}

	delete[] _tokenSequences;
	delete[] _parses;
	delete[] _secondaryParses;
	delete[] _npChunks;
	delete[] _mentionSets;
	delete[] _valueMentionSets;
	delete[] _propSets;
	delete[] _relationMentionSets;

	outputStream.close();

	SessionLogger::info("SERIF") << "done with devtest\n";
}

void RelationTimexArgFinder::replaceModel(char *model_file) {
	delete _model;
	delete _weights;
	_weights = _new DTFeature::FeatureWeightMap(50000);
	DTFeature::readWeights(*_weights, model_file, RelationTimexArgFeatureType::modeltype);
	_model = _new MaxEntModel(_tagSet, _featureTypes, _weights);

	if (_use_two_pass_model) {
		delete _secondPassWeights;
		delete _secondPassModel;
		char buffer[550];
		sprintf(buffer, "%s.second-pass", model_file);	
		_secondPassWeights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_secondPassWeights, buffer, RelationTimexArgFeatureType::modeltype);
		_secondPassModel = _new MaxEntModel(_tagSet, _secondPassFeatureTypes, _secondPassWeights);			
	}
}

// this is for stand-alone use only -- essentially for development use
void RelationTimexArgFinder::decode(TokenSequence **tokenSequences, Parse **parses, 
		ValueMentionSet **valueMentionSets, MentionSet **mentionSets, PropositionSet **propSets, 
		RelMentionSet **relsets, int nsentences, UTF8OutputStream& resultStream)
{
	if (MODE != DECODE || _model == 0)
		return;

	for (int i = 0; i < nsentences; i++) {
		for (int relment = 0; relment < relsets[i]->getNRelMentions(); relment++) {
			RelMention *key_rm = relsets[i]->getRelMention(relment);
			RelMention *test_rm = _new RelMention(*key_rm);	
			test_rm->resetTimeArgument();
			attachTimeArgument(test_rm, tokenSequences[i], valueMentionSets[i], 
				parses[i], mentionSets[i], propSets[i]);


			const ValueMention *key_ment = key_rm->getTimeArgument();
			const ValueMention *test_ment = test_rm->getTimeArgument();

			if (key_ment != 0 || test_ment != 0) {
				// print relation
				resultStream << L"<h3>";
				resultStream << key_rm->getLeftMention()->getHead()->toString(0);
				resultStream << L" and ";
				resultStream << key_rm->getRightMention()->getHead()->toString(0); 
				resultStream << L" (" << key_rm->getType().to_string() << L")</h3>\n";

				for (int tok = 0; tok < tokenSequences[i]->getNTokens(); tok++) {
					if (tok == key_rm->getLeftMention()->getHead()->getStartToken())
						resultStream << L"<u><b>";
					if (tok == key_rm->getRightMention()->getHead()->getStartToken())
						resultStream << L"<u><b>";
					resultStream << tokenSequences[i]->getToken(tok)->getSymbol().to_string();
					if (tok == key_rm->getLeftMention()->getHead()->getEndToken())
						resultStream << L"</u></b>";
					if (tok == key_rm->getRightMention()->getHead()->getEndToken())
						resultStream << L"</u></b>";
					resultStream << L" ";
				}
				resultStream << L"<br><br>\n";

				bool matched = false;

				if (test_ment == key_ment) {
					matched = true;
					if (key_rm->getTimeRole() == test_rm->getTimeRole()) {
						resultStream << L"<font color=\"red\">CORRECT</font>: ";
						resultStream << key_ment->toString(tokenSequences[i]) << L" (<b>";
						resultStream << key_rm->getTimeRole().to_string() << L"</b>)<br>\n";
						printDebugScores(key_rm, key_ment, resultStream);
						resultStream << "<br>\n";
						_correct_args++;
					} else {
						resultStream << L"<font color=\"green\">WRONG TYPE (should be <b>";
						resultStream << key_rm->getTimeRole().to_string();
						resultStream << "</b>)</font>: ";
						resultStream << key_ment->toString(tokenSequences[i]);
						resultStream << L" (<b>" << test_rm->getTimeRole() << L"</b>)<br>\n";
						printDebugScores(key_rm, key_ment, resultStream);
						resultStream << L"<br>\n";
						_wrong_type++;
					}
				}
				if (!matched) {
					if (key_ment != 0 && test_ment == 0) {
						resultStream << L"<font color=\"purple\">MISSED</font>: ";
						resultStream << key_ment->toString(tokenSequences[i]) << L" (<b>";
						resultStream << key_rm->getTimeRole() << L"</b>)<br>\n";
						printDebugScores(key_rm, key_ment, resultStream);
						resultStream << "<br>\n";
						_missed++;
					} 
					else if (key_ment == 0 && test_ment != 0) {
						resultStream << L"<font color=\"blue\">SPURIOUS</font>: ";
						resultStream << test_ment->toString(tokenSequences[i]) << L" (<b>";
						resultStream << test_rm->getTimeRole() << L"</b>)<br>\n";
						printDebugScores(test_rm, test_ment, resultStream);
						resultStream << "<br>\n";
						_spurious++;
					}
					else if (key_ment != 0 && test_ment != 0) {
						resultStream << L"<font color=\"green\">WRONG ARG (should be <b>";
						resultStream << key_ment->toString(tokenSequences[i]);
						resultStream << L" " << key_rm->getTimeRole() << L"</b>)</font>: ";
						resultStream << test_ment->toString(tokenSequences[i]);
						resultStream << L" (<b>" << test_rm->getTimeRole() << L"</b>)<br>\n";
						printDebugScores(key_rm, key_ment, resultStream);
						resultStream << L"<br>\n";
						_wrong_type++;
					}
					resultStream << L"<br>";
				}
			}
		}
	}

}

void RelationTimexArgFinder::printDebugScores(RelMention *rm, const ValueMention *arg,
										   UTF8OutputStream& stream) 
{
	if (!_observation)
		_observation = _new RelationTimexArgObservation();
	_observation->setRelation(rm);
	_observation->setCandidateArgument(arg);

	int best_tag;
	_model->decodeToDistribution(_observation, _tagScores, _tagSet->getNTags(), &best_tag);
	
	int best = 0;
	int second_best = 0;
	int third_best = 0;
	double best_score = -100000;
	double second_best_score = -100000;
	double third_best_score = -100000;
	for (int i = 0; i < _tagSet->getNTags(); i++) {
		if (_tagScores[i] > best_score) {
			third_best = second_best;
			third_best_score = second_best_score;
			second_best = best;
			second_best_score = best_score;
			best = i;
			best_score = _tagScores[i];
		} else if (_tagScores[i] > second_best_score) {
			third_best = second_best;
			third_best_score = second_best_score;
			second_best = i;			
			second_best_score = _tagScores[i];
		} else if (_tagScores[i] > third_best_score) {
			third_best = i;
			third_best_score = _tagScores[i];
		} 
	}
	stream << _tagSet->getTagSymbol(best).to_string() << L": " << _tagScores[best] << L"<br>\n";
	stream << "<font size=1>\n";
	_model->printHTMLDebugInfo(_observation, best, stream);
	stream << "</font>\n";
	stream << _tagSet->getTagSymbol(second_best).to_string() << L": " << _tagScores[second_best] << L"<br>\n";
	stream << "<font size=1>\n";
	_model->printHTMLDebugInfo(_observation, second_best, stream);
	stream << "</font>\n";
	stream << _tagSet->getTagSymbol(third_best).to_string() << L": " << _tagScores[third_best] << L"<br>\n";
	stream << "<font size=1>\n";
	_model->printHTMLDebugInfo(_observation, third_best, stream);
	stream << "</font>\n";
}


void RelationTimexArgFinder::train(TokenSequence **tokenSequences, Parse **parses, 
		ValueMentionSet **valueMentionSets, MentionSet **mentionSets, PropositionSet **propSets, 
		RelMentionSet **relsets, int nsentences) 
{
	if (!_observation)
		_observation = _new RelationTimexArgObservation();
	if (MODE != TRAIN && MODE != TRAIN_SECOND_PASS)
		return;
	
	// PRUNING
	int pruning = ParamReader::getRequiredIntParam("relation_time_maxent_trainer_pruning_cutoff");
	
	// MODEL FILE
	std::string model_file = ParamReader::getRequiredParam("relation_time_model_file");

	for (int i = 0; i < nsentences; i++) {
		_observation->resetForNewSentence(tokenSequences[i], parses[i], 
			mentionSets[i], valueMentionSets[i], propSets[i]);
		for (int rment = 0; rment < relsets[i]->getNRelMentions(); rment++) {
			RelMention *rm = relsets[i]->getRelMention(rment);
			RelMention *new_rm = rm;
			if (MODE == TRAIN_SECOND_PASS) {
				new_rm = _new RelMention (*rm);
				new_rm->resetTimeArgument();
				attachTimeArgument(_model, new_rm, tokenSequences[i], valueMentionSets[i], 
					parses[i], mentionSets[i], propSets[i]);
			}
			_observation->setRelation(new_rm);
			
			const ValueMention *keyMention = rm->getTimeArgument();
			
			for (int val_id = 0; val_id < valueMentionSets[i]->getNValueMentions(); val_id++) {
				ValueMention *valueMention = valueMentionSets[i]->getValueMention(val_id);
				// we only care about timex values
				if (!valueMention->isTimexValue())
					continue;
				_observation->setCandidateArgument(valueMention);

				Symbol roleSym = Symbol();
				int role = _tagSet->getNoneTagIndex();

				if (keyMention == valueMention)
					roleSym = rm->getTimeRole();
				if (!roleSym.is_null()) {
					role = _tagSet->getTagIndex(roleSym);
					if (role == -1) {
						char c[500];
						sprintf(c, "ERROR: Invalid time argument role in training file: %s", 
							roleSym.to_debug_string());
						throw UnexpectedInputException("RelationTimexArgumentFinder::train()", c);
					}
				}
				if (MODE == TRAIN_SECOND_PASS) {
					if (new_rm->getTimeArgument() == valueMention) {
						Symbol newRoleSym = new_rm->getTimeRole();
						_secondPassModel->addToTraining(_observation, role);
					}
				} else _model->addToTraining(_observation, role);	

			}
		}
	}

	char output_file[550];
	
	if (MODE == TRAIN_SECOND_PASS) {
		_secondPassModel->deriveModel(pruning);
		sprintf(output_file, "%s.second-pass", model_file.c_str());	
	} else {
        _model->deriveModel(pruning);
		sprintf(output_file, "%s", model_file.c_str());
	}

	UTF8OutputStream out;
	out.open(output_file);

	if (out.fail()) {
		throw UnexpectedInputException("RelationTimexArgFinder::train()",
			"Could not open model file for writing");
	}

	dumpTrainingParameters(out);
	if (MODE == TRAIN_SECOND_PASS)
		DTFeature::writeWeights(*_secondPassWeights, out);
	else DTFeature::writeWeights(*_weights, out);
	out.close();
  	
	// we are now set to decode
	MODE = DECODE;

}

void RelationTimexArgFinder::attachTimeArguments(RelMentionSet *relMentionSet, const TokenSequence *tokens,
		ValueMentionSet *valueMentionSet, Parse *parse, 
		MentionSet *mentionSet, PropositionSet *propSet)
{
	for (int i = 0; i < relMentionSet->getNRelMentions(); i++) {
		attachTimeArgument(relMentionSet->getRelMention(i), tokens,
							valueMentionSet, parse, mentionSet, propSet);
	}
}

void RelationTimexArgFinder::attachTimeArgument(RelMention *relMention, const TokenSequence *tokens,
		ValueMentionSet *valueMentionSet, Parse *parse, 
		MentionSet *mentionSet, PropositionSet *propSet)
{
	attachTimeArgument(_model, relMention, tokens, valueMentionSet, parse, mentionSet, propSet);
	if (_use_two_pass_model)
		attachTimeArgument(_secondPassModel, relMention, tokens, valueMentionSet, 
		parse, mentionSet, propSet);
}


void RelationTimexArgFinder::attachTimeArgument(MaxEntModel *model,
		RelMention *relMention, const TokenSequence *tokens,
		ValueMentionSet *valueMentionSet, Parse *parse, 
		MentionSet *mentionSet, PropositionSet *propSet)
{
	if (!_observation)
		_observation = _new RelationTimexArgObservation();
	_observation->resetForNewSentence(tokens, parse, mentionSet, valueMentionSet, propSet);
	_observation->setRelation(relMention);

	if (DEBUG) _debugStream << relMention->toString() << L"\n";
	
	_potentialTimeArgument = 0;
	double best_score = 0.0;

	// attach all possible time arguments to the relation mention
	for (int val_id = 0; val_id < valueMentionSet->getNValueMentions(); val_id++) {
		ValueMention *valueMention = valueMentionSet->getValueMention(val_id);
		if (!valueMention->isTimexValue())
			continue;
		_observation->setCandidateArgument(valueMention);
		addTagsForCandidateValue(model, valueMention);
	}

	// filter those arguments
	// how about for now we allow only one for each label (and of course,
	//  one for each mention!)
	while (_potentialTimeArgument != 0) {
		
		// only add this if we don't already have one 
		if (relMention->getTimeArgument() == 0) {
			relMention->setTimeArgument(_potentialTimeArgument->role, 
										_potentialTimeArgument->valueMention,
										_potentialTimeArgument->score);

			if (DEBUG) {
				_debugStream << "Adding \"";
				_debugStream << _potentialTimeArgument->valueMention->toString(tokens);
				_debugStream << "\" as " << _potentialTimeArgument->role << "\n";
			}

		}
		Symbol role = _potentialTimeArgument->role;
		const ValueMention *valueMention = _potentialTimeArgument->valueMention;
		PotentialTimeArgument *iter = _potentialTimeArgument;
		while (iter->next != 0) {
			if (DEBUG) {
				_debugStream << "Dumping \"";
				_debugStream << valueMention->toString(tokens);
				_debugStream  << "\" as " << iter->next->role << "\n";
			}
			PotentialTimeArgument *temp = iter->next;
			iter->next = iter->next->next;
			temp->next = 0;
			delete temp;
		}
		PotentialTimeArgument *temp = _potentialTimeArgument;
		_potentialTimeArgument = _potentialTimeArgument->next;
		temp->next = 0;
		delete temp;
	}
	
}

int RelationTimexArgFinder::addTagsForCandidateValue(MaxEntModel *model, const ValueMention *value) {
	int ntags_found = 0;
	int best_tag;
	model->decodeToDistribution(_observation, _tagScores, _tagSet->getNTags(), &best_tag);
	for (int tag = 0; tag < _tagSet->getNTags(); tag++) {
		if (tag == _tagSet->getNoneTagIndex())
			continue;
		if (_tagScores[tag] > _recall_threshold) {
			PotentialTimeArgument *newArgument = 
				_new PotentialTimeArgument(_tagSet->getTagSymbol(tag), value, 
				(float) _tagScores[tag]);
			addPotentialArgument(newArgument);
			ntags_found++;
			if (DEBUG) {
				_debugStream << _tagSet->getTagSymbol(tag) << L": ";
				_debugStream << L"(UNPRINTED TIMEX)" << L" (";
				_debugStream << _tagScores[tag] << L")\n";
			}
		}
	}
	return ntags_found;
}

void RelationTimexArgFinder::addPotentialArgument(PotentialTimeArgument *newArgument) {
	if (_potentialTimeArgument == 0) {
		_potentialTimeArgument = newArgument;
		return;
	} else if (newArgument->score > _potentialTimeArgument->score) {        
		newArgument->next = _potentialTimeArgument;
		_potentialTimeArgument = newArgument;
		return;
	} else {
		PotentialTimeArgument *iter = _potentialTimeArgument;
		while (iter->next != 0) {
			if (newArgument->score > iter->next->score) {
				newArgument->next = iter->next;
				iter->next = newArgument;
				return;
			}
			iter = iter->next;
		}
		iter->next = newArgument;
		return;
	}
}

void RelationTimexArgFinder::dumpTrainingParameters(UTF8OutputStream &out) {

	DTFeature::recordDate(out);

	out << L"Parameters:\n";
	DTFeature::recordParamForConsistency(Symbol(L"relation_time_tag_set_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"relation_time_features_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"word_cluster_bits_file"), out);	
	DTFeature::recordParamForConsistency(Symbol(L"lc_word_cluster_bits_file"), out);

	DTFeature::recordParamForReference(Symbol(L"relation_time_training_file_list"), out);
	DTFeature::recordParamForReference(Symbol(L"relation_time_maxent_trainer_percent_held_out"), out);
	DTFeature::recordParamForReference(Symbol(L"relation_time_maxent_trainer_max_iterations"), out);
	DTFeature::recordParamForReference(Symbol(L"relation_time_maxent_trainer_gaussian_variance"), out);
	DTFeature::recordParamForReference(Symbol(L"relation_time_maxent_trainer_min_likelihood_delta"), out);
	DTFeature::recordParamForReference(Symbol(L"relation_time_maxent_trainer_stop_check_frequency"), out);
	DTFeature::recordParamForReference(Symbol(L"relation_time_maxent_trainer_pruning_cutoff"), out);
	out << L"\n";

}


void RelationTimexArgFinder::resetRoundRobinStatistics() {
	_correct_args = 0;
	_correct_non_args = 0;
	_wrong_type = 0;
	_missed = 0;
	_spurious = 0;
}

void RelationTimexArgFinder::printRoundRobinStatistics(UTF8OutputStream &out) {

	double recall = (double) _correct_args / (_missed + _wrong_type + _correct_args);
	double precision = (double) _correct_args / (_spurious + _wrong_type + _correct_args);

	out << L"CORRECT: " << _correct_args + _correct_non_args << L" ";
	out << L"(arguments: " << _correct_args << L", non-arguments: ";
	out << _correct_non_args << L")<br>\n";
	out << L"MISSED: " << _missed << L"<br>\n";
	out << L"SPURIOUS: " << _spurious << L"<br>\n";
	out << L"WRONG TYPE: " << _wrong_type << L"<br><br>\n\n";
	out << L"RECALL: " << recall << L"<br>\n";
	out << L"PRECISION: " << precision << L"<br><br>\n\n";

}

int RelationTimexArgFinder::loadTrainingData(TrainingLoader *trainingLoader) {

	int max_sentences = trainingLoader->getMaxSentences();
	int i;
	for (i = 0; i < max_sentences; i++) {
		SentenceTheory *theory = trainingLoader->getNextSentenceTheory();
		if (theory == 0)
			return i;
		_tokenSequences[i] = theory->getTokenSequence();
		_tokenSequences[i]->gainReference();
		_parses[i] = theory->getPrimaryParse();
		_parses[i]->gainReference();
		//have to add a reference to the np chunk theory, because it may be 
		//the source of the parse, if the reference isn't gained, the parse 
		//will be deleted when result is deleted
		_npChunks[i] = theory->getNPChunkTheory();
		if(_npChunks[i] != 0){
			_npChunks[i]->gainReference();
			// is this what we want?
			_secondaryParses[i] = theory->getFullParse();
			_secondaryParses[i]->gainReference();
		} else _secondaryParses[i] = 0;
		_mentionSets[i] = theory->getMentionSet();
		_mentionSets[i]->gainReference();
		_valueMentionSets[i] = theory->getValueMentionSet();
		_valueMentionSets[i]->gainReference();
		_propSets[i] = theory->getPropositionSet();
		_propSets[i]->gainReference();
		_propSets[i]->fillDefinitionsArray();
		_relationMentionSets[i] = theory->getRelMentionSet();
		_relationMentionSets[i]->gainReference();
	}

	return i;
}
