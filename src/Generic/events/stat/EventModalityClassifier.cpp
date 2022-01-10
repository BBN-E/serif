// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/stat/EventModalityClassifier.h"
#include "Generic/events/stat/EventModalityFeatureType.h"
#include "Generic/events/stat/EventModalityFeatureTypes.h"
//#include "events/stat/EventTriggerSentence.h"
#include "Generic/events/stat/PotentialEventMention.h"
#include "Generic/events/stat/EventModalityObservation.h"
#include "Generic/events/stat/ETProbModelSet.h"
#include "Generic/docRelationsEvents/DocEventHandler.h"
#include "Generic/maxent/MaxEntModel.h"
#include "Generic/discTagger/P1Decoder.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/Sexp.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/EventMention.h"

// Two Layer Model using P1 models has not been refined yet (may have bugs)

EventModalityClassifier::EventModalityClassifier(int mode_) : MODE(mode_)
{
	resetRoundRobinStatistics();
	_observation = EventModalityObservation::build();
	EventModalityFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();


	// DEBUGGING OUTPUT
	std::string buffer = ParamReader::getParam("event_modality_debug");
	DEBUG = (!buffer.empty());
	if (DEBUG) _debugStream.open(buffer.c_str());

	// TAG SET
	std::string tag_set_file = ParamReader::getRequiredParam("event_modality_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);
	_tagScores = _new double[_tagSet->getNTags()];
	_tagFound = _new bool[_tagSet->getNTags()];

	// FEATURES
	_featureTypes = 0; 
	_featureTypes_layer1 = 0;
	_featureTypes_layer2 = 0;
	if (MODE == TRAIN_TWO_LAYER || MODE == DEVTEST_TWO_LAYER || MODE == DECODE_TWO_LAYER){
		std::string features_file = ParamReader::getRequiredParam("event_modality_features_file_layer1");
		_featureTypes_layer1 = _new DTFeatureTypeSet(features_file.c_str(), EventModalityFeatureType::modeltype);
		features_file = ParamReader::getRequiredParam("event_modality_features_file_layer2");
		_featureTypes_layer2 = _new DTFeatureTypeSet(features_file.c_str(), EventModalityFeatureType::modeltype);
	}else{
		std::string features_file = ParamReader::getRequiredParam("event_modality_features_file");
		_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), EventModalityFeatureType::modeltype);
	}

	_use_p1_model = ParamReader::getRequiredTrueFalseParam("em_use_p1_model");
	_use_maxent_model = ParamReader::getRequiredTrueFalseParam("em_use_maxent_model");

	if (MODE == DECODE || MODE == DEVTEST) {
		_recall_threshold = ParamReader::getRequiredFloatParam("event_modality_recall_threshold");

		_p1Weights = 0;
		_maxentWeights = 0;
		_p1Model = 0;		
		_maxentModel = 0;
	 
		if (_use_p1_model) {
			_p1_overgen_percentage = (float) ParamReader::getRequiredFloatParam("em_p1_recall_threshold");
		} else _p1_overgen_percentage = 0;	

		// MODEL FILE
		std::string model_file = ParamReader::getParam("event_modality_model_file");
		if (!model_file.empty()) 
		{
			//_probModelSet->loadModels(model_file);

			if (_use_p1_model) {
				std::string buffer = model_file + ".p1";
				_p1Weights = _new DTFeature::FeatureWeightMap(50000);
				DTFeature::readWeights(*_p1Weights, buffer.c_str(), EventModalityFeatureType::modeltype);
				
				_p1Model = _new P1Decoder(_tagSet, _featureTypes, _p1Weights, 
					_p1_overgen_percentage, false);
			} 

			if (_use_maxent_model) {
				std::string buffer = model_file + ".maxent";
				_maxentWeights = _new DTFeature::FeatureWeightMap(50000);
				DTFeature::readWeights(*_maxentWeights, buffer.c_str(), EventModalityFeatureType::modeltype);
				_maxentModel = _new MaxEntModel(_tagSet, _featureTypes, _maxentWeights);
			} 

		} else if (!ParamReader::hasParam("event_round_robin_setup")) {

			throw UnexpectedInputException("EventModalityClassifier::EventModalityClassifier()",
				"Neither parameter 'event_modality_model' nor 'event_round_robin_setup' specified");

		} 

	}else if (MODE == DECODE_TWO_LAYER || MODE == DEVTEST_TWO_LAYER) {
		_recall_threshold = ParamReader::getRequiredFloatParam("event_modality_recall_threshold");

		_p1Weights_layer1 = 0;
		_maxentWeights_layer1 = 0;
		_p1Model_layer1 = 0;		
		_maxentModel_layer1 = 0;

	 
		_p1Weights_layer2 = 0;
		_maxentWeights_layer2 = 0;
		_p1Model_layer2 = 0;		
		_maxentModel_layer2 = 0;

		if (_use_p1_model) {
			_p1_overgen_percentage = (float) ParamReader::getRequiredFloatParam("em_p1_recall_threshold");
		} else _p1_overgen_percentage = 0;
	


		// MODEL FILE
		std::string model_file = ParamReader::getParam("event_modality_model_file_layer1");
		if (!model_file.empty()) 
		{
			if (_use_p1_model) {
				std::string buffer = model_file + ".p1";
				_p1Weights_layer1 = _new DTFeature::FeatureWeightMap(50000);
				DTFeature::readWeights(*_p1Weights_layer1, buffer.c_str(), EventModalityFeatureType::modeltype);
				
				_p1Model_layer1 = _new P1Decoder(_tagSet, _featureTypes_layer1, _p1Weights_layer1, 
					_p1_overgen_percentage, false);
			} 

			if (_use_maxent_model) {
				std::string buffer = model_file + ".maxent";
				_maxentWeights_layer1 = _new DTFeature::FeatureWeightMap(50000);
				DTFeature::readWeights(*_maxentWeights_layer1, buffer.c_str(), EventModalityFeatureType::modeltype);
				_maxentModel_layer1 = _new MaxEntModel(_tagSet, _featureTypes_layer1, _maxentWeights_layer1);
			} 

		} else if (!ParamReader::hasParam("event_round_robin_setup")) {

			throw UnexpectedInputException("EventModalityClassifier::EventModalityClassifier()",
				"Neither parameter 'event_modality_model' nor 'event_round_robin_setup' specified");

		} 

		model_file = ParamReader::getParam("event_modality_model_file_layer2");
		if (!model_file.empty()) 
		{
			if (_use_p1_model) {
				std::string buffer = model_file + ".p1";
				_p1Weights_layer2 = _new DTFeature::FeatureWeightMap(50000);
				DTFeature::readWeights(*_p1Weights_layer2, buffer.c_str(), EventModalityFeatureType::modeltype);
				
				_p1Model_layer2 = _new P1Decoder(_tagSet, _featureTypes_layer2, _p1Weights_layer2, 
					_p1_overgen_percentage, false);
			} 

			if (_use_maxent_model) {
				std::string buffer = model_file + ".maxent";
				_maxentWeights_layer2 = _new DTFeature::FeatureWeightMap(50000);
				DTFeature::readWeights(*_maxentWeights_layer2, buffer.c_str(), EventModalityFeatureType::modeltype);
				_maxentModel_layer2 = _new MaxEntModel(_tagSet, _featureTypes_layer2, _maxentWeights_layer2);
			} 

		} else if (!ParamReader::hasParam("event_round_robin_setup")) {

			throw UnexpectedInputException("EventModalityClassifier::EventModalityClassifier()",
				"Neither parameter 'event_modality_model' nor 'event_round_robin_setup' specified");

		} 


	}else if (MODE == TRAIN) {

		if (_use_p1_model) {
			_p1Weights = _new DTFeature::FeatureWeightMap(50000);
			_p1Model = _new P1Decoder(_tagSet, _featureTypes, _p1Weights, true);
			// SEED FEATURES
			_seed_features = ParamReader::getRequiredTrueFalseParam("em_p1_trainer_seed_features");
		} else {
			_p1Model = 0;
			_p1Weights = 0;
			_seed_features = 0;
		}

		if (_use_maxent_model) {
			// PERCENT HELD OUT
			int percent_held_out = 
				ParamReader::getRequiredIntParam("em_maxent_trainer_percent_held_out");
			if (percent_held_out < 0 || percent_held_out > 50) 
				throw UnexpectedInputException("EventModalityClassifier::EventModalityClassifier()",
				"Parameter 'em_maxent_trainer_percent_held_out' must be between 0 and 50");

			// MAX NUMBER OF ITERATIONS (STOPPING CONDITION)
			int max_iterations = 
				ParamReader::getRequiredIntParam("em_maxent_trainer_max_iterations");

			// GAUSSIAN PRIOR VARIANCE
			double variance = ParamReader::getRequiredFloatParam("em_maxent_trainer_gaussian_variance");

			// MIN CHANGE IN LIKELIHOOD (STOPPING CONDITION)
			double likelihood_delta = ParamReader::getOptionalFloatParamWithDefaultValue("em_maxent_trainer_min_likelihood_delta", .0001);

			// FREQUENCY OF STOPPING CONDITION CHECKS (NUM ITERATIONS)
			int stop_check_freq = ParamReader::getOptionalIntParamWithDefaultValue("em_maxent_trainer_stop_check_frequency",1);

			_maxentWeights = _new DTFeature::FeatureWeightMap(50000);
			_maxentModel = _new MaxEntModel(_tagSet, _featureTypes, _maxentWeights, 
				MaxEntModel::SCGIS, percent_held_out, max_iterations, variance,
				likelihood_delta, stop_check_freq, "", "");

		} else {
			_maxentModel = 0;
			_maxentWeights = 0;
		}

		//_probModelSet = _new ETProbModelSet(_tagSet, _tagScores, 0);
		//_probModelSet->createEmptyModels();

	} else if (MODE == TRAIN_TWO_LAYER) {

		if (_use_p1_model) {
			_p1Weights_layer1 = _new DTFeature::FeatureWeightMap(50000);
			_p1Model_layer1 = _new P1Decoder(_tagSet, _featureTypes_layer1, _p1Weights, true);
			_p1Weights_layer2 = _new DTFeature::FeatureWeightMap(50000);
			_p1Model_layer2 = _new P1Decoder(_tagSet, _featureTypes_layer2, _p1Weights, true);
			// SEED FEATURES
			_seed_features = ParamReader::getRequiredTrueFalseParam("em_p1_trainer_seed_features");
		} else {
			_p1Model_layer1 = 0;
			_p1Weights_layer1 = 0;
			_p1Model_layer2 = 0;
			_p1Weights_layer2 = 0;
			_seed_features = 0;
		}

		if (_use_maxent_model) {
			// PERCENT HELD OUT
			int percent_held_out = 
				ParamReader::getRequiredIntParam("em_maxent_trainer_percent_held_out");
			if (percent_held_out < 0 || percent_held_out > 50) 
				throw UnexpectedInputException("EventModalityClassifier::EventModalityClassifier()",
				"Parameter 'em_maxent_trainer_percent_held_out' must be between 0 and 50");

			// MAX NUMBER OF ITERATIONS (STOPPING CONDITION)
			int max_iterations = 
				ParamReader::getRequiredIntParam("em_maxent_trainer_max_iterations");

			// GAUSSIAN PRIOR VARIANCE
			double variance = ParamReader::getRequiredFloatParam("em_maxent_trainer_gaussian_variance");

			// MIN CHANGE IN LIKELIHOOD (STOPPING CONDITION)
			double likelihood_delta = ParamReader::getOptionalFloatParamWithDefaultValue("em_maxent_trainer_min_likelihood_delta", .0001);

			// FREQUENCY OF STOPPING CONDITION CHECKS (NUM ITERATIONS)
			int stop_check_freq = ParamReader::getOptionalIntParamWithDefaultValue("em_maxent_trainer_stop_check_frequency",1);

			_maxentWeights_layer1 = _new DTFeature::FeatureWeightMap(50000);
			_maxentModel_layer1 = _new MaxEntModel(_tagSet, _featureTypes_layer1, _maxentWeights_layer1, 
				MaxEntModel::SCGIS, percent_held_out, max_iterations, variance,
				likelihood_delta, stop_check_freq, "", "");
			_maxentWeights_layer2 = _new DTFeature::FeatureWeightMap(50000);
			_maxentModel_layer2 = _new MaxEntModel(_tagSet, _featureTypes_layer2, _maxentWeights_layer2, 
				MaxEntModel::SCGIS, percent_held_out, max_iterations, variance,
				likelihood_delta, stop_check_freq, "", "");


		} else {
			_maxentModel_layer1 = 0;
			_maxentWeights_layer1 = 0;
			_maxentModel_layer2 = 0;
			_maxentWeights_layer2 = 0;

		}

		//_probModelSet = _new ETProbModelSet(_tagSet, _tagScores, 0);
		//_probModelSet->createEmptyModels();

	}
}

EventModalityClassifier::~EventModalityClassifier() {
	//delete _probModelSet;
	delete _p1Model;
	delete _p1Weights;
	delete _maxentModel;
	delete _maxentWeights;
	delete _tagSet;
	delete _featureTypes;
	delete _observation;
	delete [] _tagScores;	


	delete _p1Model_layer1;
	delete _p1Weights_layer1;
	delete _maxentModel_layer1;
	delete _maxentWeights_layer1;
	delete _featureTypes_layer1;

	delete _p1Model_layer2;
	delete _p1Weights_layer2;
	delete _maxentModel_layer2;
	delete _maxentWeights_layer2;
	delete _featureTypes_layer2;

	EventModalityObservation::finalize();
}

void EventModalityClassifier::replaceModel(char *model_file) {

	delete _p1Weights;
	delete _p1Model;
	delete _maxentWeights;
	delete _maxentModel;

	if (_use_p1_model) {
		char buffer[550];
		sprintf(buffer, "%s.p1", model_file);
		_p1Weights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_p1Weights, buffer, EventModalityFeatureType::modeltype);
		_p1Model = _new P1Decoder(_tagSet, _featureTypes, _p1Weights, 
			_p1_overgen_percentage, false);
	} else {
		_p1Model = 0;
		_p1Weights = 0;
	}
	
	if (_use_maxent_model) {
		char buffer[550];
		sprintf(buffer, "%s.maxent", model_file);
		_maxentWeights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_maxentWeights, buffer, EventModalityFeatureType::modeltype);
		_maxentModel = _new MaxEntModel(_tagSet, _featureTypes, _maxentWeights);
	} else {
		_maxentModel = 0;
		_maxentWeights = 0;
	}

	_probModelSet->deleteModels();
	_probModelSet->loadModels(model_file);
}

void EventModalityClassifier::setModality(const TokenSequence *tseq, Parse *parse, 
								ValueMentionSet *valueMentionSet, MentionSet *mentionSet, 
								PropositionSet *propSet, EventMention *em)
{
	EventMention *tmpEvents[MAX_MODALITIES];
	int tokPos = em->getAnchorNode()->getStartToken();
	int n_system_answers = processToken(tokPos, tseq, parse, mentionSet,
				propSet, tmpEvents, MAX_MODALITIES);

	int system_answer = _tagSet->getTagIndex(tmpEvents[0]->getModalityType());
	float this_score;
	float system_score = exp(tmpEvents[0]->getScore());
	for (int m = 1; m < n_system_answers; m++) {
		this_score = exp(tmpEvents[m]->getScore()); 
		if (this_score > system_score) {
			system_score = this_score;
			system_answer = _tagSet->getTagIndex(tmpEvents[m]->getModalityType());
		}
	}
	
	// current SERIF setting (0 - Asserted; 1 - Non-asserted)
	// system_answer (1 - Asserted; 2 - Non-asserted)
	// need to align them
	if (system_answer == 1)
		em->setModality(Modality::ASSERTED);
	else if (system_answer == 2)
		em->setModality(Modality::OTHER);
	else
		throw InternalInconsistencyException("EventModalityClassifier::setModality()",
											"Invalid system answer value.");

	for (int n = 0; n < n_system_answers; n++) 
		delete tmpEvents[n];
			
	if (DEBUG){
		if (_use_p1_model) 
			printP1DebugScores(_debugStream);
		if (_use_maxent_model)
			printMaxentDebugScores(_debugStream);
	}
}

void EventModalityClassifier::decode(TokenSequence **tokenSequences, Parse **parses, 
								ValueMentionSet **valueMentionSets, MentionSet **mentionSets, 
								PropositionSet **propSets, EventMentionSet **esets, 
								int nsentences,	UTF8OutputStream& htmlStream) 
{
	//if (MODE != DECODE)
	if (MODE != DECODE && MODE != DEVTEST && MODE != DEVTEST_TWO_LAYER && MODE != DECODE_TWO_LAYER)
		return;


	int modalityTypes[MAX_SENTENCE_TOKENS];
	EventMention *tmpEvents[MAX_MODALITIES];

	for (int i = 0; i < nsentences; i++) {
		const TokenSequence *tseq = tokenSequences[i];
		// print debug information
		_htmlStream << L" sentence " << i << ": " << parses[i]->getRoot()->toTextString() << L"<br><br>\n";
		

		for (int j = 0; j < tseq->getNTokens(); j++) {
			modalityTypes[j] = 0;   
		}
		
		for (int ement = 0; ement < esets[i]->getNEventMentions(); ement++) {
			EventMention *em = esets[i]->getEventMention(ement);

			if (_tagSet->getTagIndex(em->getModalityType()) == -1) {
				throw UnexpectedInputException("EventModalityClassifier::decode()",
					"Invalid modality type in training file");
			}			
			modalityTypes[em->getAnchorNode()->getStartToken()] = _tagSet->getTagIndex(em->getModalityType());
		}

		for (int k = 0; k < tseq->getNTokens(); k++) {
			int correct_answer = modalityTypes[k];
			
			if (correct_answer != _tagSet->getNoneTagIndex()) {
				_htmlStream << L" trigger: " << tseq->getToken(k)->getSymbol() << L"<br><br>\n";
				

				int n_system_answers = processToken(k, tseq, parses[i], mentionSets[i],
				propSets[i], tmpEvents, MAX_MODALITIES);  
				

				// Return answer with highest score
				int system_answer = _tagSet->getTagIndex(tmpEvents[0]->getModalityType());
				float this_score;
				//float system_score = exp(tmpEvents[0]->getScore());
				float system_score = tmpEvents[0]->getScore();
				for (int m = 1; m < n_system_answers; m++) {
					//this_score = exp(tmpEvents[m]->getScore()); 
					this_score = tmpEvents[m]->getScore(); 
					if (this_score > system_score) {
						system_score = this_score;
						system_answer = _tagSet->getTagIndex(tmpEvents[m]->getModalityType());
					}
				}
			
				for (int n = 0; n < n_system_answers; n++) 
					delete tmpEvents[n];
			
			
				

				if (correct_answer == system_answer) {
					_correct++;
					printHTMLSentence(htmlStream, tseq, k, correct_answer, system_answer, system_score);
				}else{
					_wrong_type++;
					printHTMLSentence(htmlStream, tseq, k, correct_answer, system_answer, system_score);
				}
				
			}
		}
	}

}

/*
void EventModalityClassifier::decode(TokenSequence **tokenSequences, Parse **parses, 
								ValueMentionSet **valueMentionSets, MentionSet **mentionSets, 
								PropositionSet **propSets, EventMentionSet **esets, 
								int nsentences,
								UTF8OutputStream& resultStream,
								UTF8OutputStream& keyStream,
								UTF8OutputStream& htmlStream) 
{
	//if (MODE != DECODE)
	if (MODE != DECODE && MODE != DEVTEST)
		return;

	int modalityTypes[MAX_SENTENCE_TOKENS];
	EventMention *tmpEvents[MAX_MODALITIES];

	for (int i = 0; i < nsentences; i++) {
		const TokenSequence *tseq = tokenSequences[i];
	
		for (int j = 0; j < tseq->getNTokens(); j++) {
			modalityTypes[j] = 0;   
		}
		
		for (int ement = 0; ement < esets[i]->getNEventMentions(); ement++) {
			EventMention *em = esets[i]->getEventMention(ement);

			if (_tagSet->getTagIndex(em->getModalityType()) == -1) {
				throw UnexpectedInputException("EventModalityClassifier::train()",
					"Invalid modality type in training file");
			}			
			modalityTypes[em->getAnchorNode()->getStartToken()] = _tagSet->getTagIndex(em->getModalityType());
		}

		for (int k = 0; k < tseq->getNTokens(); k++) {
			int correct_answer = modalityTypes[k];
			
			if (correct_answer != _tagSet->getNoneTagIndex()) {
				keyStream << L"<ENAMEX TYPE=\"" 
						  << _tagSet->getTagSymbol(correct_answer).to_string() << L"\">";
			
				keyStream << tseq->getToken(k)->getSymbol().to_string();
				keyStream << L"</ENAMEX>";
			
				//keyStream << L" ";
				keyStream << L"\n";
			
				int n_system_answers = processToken(k, tseq, parses[i], mentionSets[i],
				propSets[i], tmpEvents, MAX_MODALITIES);  

				// Return answer with highest score
				int system_answer = _tagSet->getTagIndex(tmpEvents[0]->getModalityType());
				float this_score;
				float system_score = exp(tmpEvents[0]->getScore());
				for (int m = 1; m < n_system_answers; m++) {
					this_score = exp(tmpEvents[m]->getScore()); 
					if (this_score > system_score) {
						system_score = this_score;
						system_answer = _tagSet->getTagIndex(tmpEvents[m]->getModalityType());
					}
				}
			
				for (int n = 0; n < n_system_answers; n++) 
					delete tmpEvents[n];
			
			
				if (correct_answer == system_answer) {
					_correct++;
					printHTMLSentence(htmlStream, tseq, k, correct_answer, system_answer, system_score);
				}else{
					_wrong_type++;
					printHTMLSentence(htmlStream, tseq, k, correct_answer, system_answer, system_score);
				}

				resultStream << L"<ENAMEX TYPE=\"" 
							 << _tagSet->getTagSymbol(system_answer).to_string() << L"\">";
				resultStream << tseq->getToken(k)->getSymbol().to_string();
				resultStream << L"</ENAMEX>";
			
				resultStream << L" ";			
			}
		}
		resultStream << L"\n\n";
		keyStream << L"\n\n";
	}

}
*/

void EventModalityClassifier::printHTMLSentence(UTF8OutputStream &out, const TokenSequence *tseq, 
										   int index, int correct_answer, int system_answer,
										   float system_score) 
{
	for (int tok = 0; tok < tseq->getNTokens(); tok++) {
		if (tok == index) {
			out << L"<u><b>";
			if (system_answer == correct_answer) 
				out << L"<font color=\"red\">";
			else out << L"<font color=\"green\">";
		}
		out << tseq->getToken(tok)->getSymbol().to_string();
		if (DEBUG) _debugStream << tseq->getToken(tok)->getSymbol().to_string();
		if (tok == index) {
			if (system_answer == correct_answer)
				out << L" (" << _tagSet->getTagSymbol(correct_answer) << L")";
			else {
				out << L" (WRONG TYPE: " << _tagSet->getTagSymbol(system_answer);
				out << L" should be " << _tagSet->getTagSymbol(correct_answer) << L")";
			}
			out << L"</font></u></b>";
		}
		out << L" ";
		if (DEBUG) _debugStream << L" ";
	}
	
	out << L"<br><br>\n";
	if (DEBUG) _debugStream << L"\n\n";

	out << L"Overall System Score = " << system_score << L"<br><br>\n";

	if (_use_p1_model) {
		printP1DebugScores(out);
		
		out << L"Features extracted for: " << _tagSet->getTagSymbol(system_answer) << L"<br>";
		
		if (MODE == DEVTEST_TWO_LAYER || MODE == DECODE_TWO_LAYER){
			if (_observation->hasRichFeatures()){
				_p1Model_layer1->printHTMLDebugInfo(_observation, system_answer, out);
				out << L"<br>";
				if (correct_answer != system_answer){
					out << L"Features for correct Answer: " << _tagSet->getTagSymbol(correct_answer) << L"<br>";
					_p1Model_layer1->printHTMLDebugInfo(_observation, correct_answer, out);
					out << L"<br>";
				}
			}else{
				_p1Model_layer2->printHTMLDebugInfo(_observation, system_answer, out);
				out << L"<br>";
				if (correct_answer != system_answer){
					out << L"Features for correct Answer: " << _tagSet->getTagSymbol(correct_answer) << L"<br>";
					_p1Model_layer2->printHTMLDebugInfo(_observation, correct_answer, out);
					out << L"<br>";
				}
			}
		}else{
			_p1Model->printHTMLDebugInfo(_observation, system_answer, out);
			out << L"<br>";
			if (correct_answer != system_answer){
				out << L"Features for correct Answer: " << _tagSet->getTagSymbol(correct_answer) << L"<br>";
				_p1Model->printHTMLDebugInfo(_observation, correct_answer, out);
				out << L"<br>";
			}
		}

	}
	if (_use_maxent_model){
		printMaxentDebugScores(out);
		out << L"Features extracted for: " << _tagSet->getTagSymbol(system_answer) << L"<br>";
		
		if (MODE == DEVTEST_TWO_LAYER || MODE == DECODE_TWO_LAYER){
			int _classifierNo = _observation->getClassifierNo();
			if ( _classifierNo == 1){
				_maxentModel_layer1->printHTMLDebugInfo(_observation, system_answer, out);
				out << L"<br>";
				if (correct_answer != system_answer){
					out << L"Features for correct Answer: " << _tagSet->getTagSymbol(correct_answer) << L"<br>";
					_maxentModel_layer1->printHTMLDebugInfo(_observation, correct_answer, out);
					out << L"<br>";
				}
			}else if (_classifierNo == 2){
				_maxentModel_layer2->printHTMLDebugInfo(_observation, system_answer, out);
				out << L"<br>";
				if (correct_answer != system_answer){
					out << L"Features for correct Answer: " << _tagSet->getTagSymbol(correct_answer) << L"<br>";
					_maxentModel_layer2->printHTMLDebugInfo(_observation, correct_answer, out);
					out << L"<br>";
				}
			}
		}else{
			_maxentModel->printHTMLDebugInfo(_observation, system_answer, out);
			out << L"<br>";
			if (correct_answer != system_answer){
				out << L"Features for correct Answer: " << _tagSet->getTagSymbol(correct_answer) << L"<br>";
				_maxentModel->printHTMLDebugInfo(_observation, correct_answer, out);
				out << L"<br>";
			}
		}
	}
}

/*
void EventModalityClassifier::selectAnnotation(Symbol *docIds, Symbol *docTopics,
								TokenSequence **tokenSequences, 
								Parse **parses, ValueMentionSet **valueMentionSets, 
								MentionSet **mentionSets, PropositionSet **propSets, 
								EventMentionSet **esets, int nsentences,
								UTF8OutputStream& annotationStream) 
{
	if (MODE != DECODE)
		return;

	int n_selected = 0;

	for (int i = 0; i < nsentences; i++) {

		const TokenSequence *tseq = tokenSequences[i];
		_documentTopic = docTopics[i];
		
		for (int k = 0; k < tseq->getNTokens(); k++) {
			if (isHighValueAnnotation(k, tseq, parses[i], mentionSets[i], propSets[i])) {
				printAnnotationSentence(annotationStream, docIds, tokenSequences, i, nsentences);
				n_selected++;
				break;
			}
		}
	}

	if (DEBUG)
		_debugStream << n_selected << " out of " << nsentences << " sentences selected\n";
}

*/

/*
bool EventModalityClassifier::isHighValueAnnotation(int tok, const TokenSequence *tokens, 
				const Parse *parse, MentionSet *mentionSet, const PropositionSet *propSet)
{
	bool result = false;
	int p1tag = 0;

	_observation->populate(tok, tokens, parse, mentionSet, propSet, true);
	_observation->setDocumentTopic(_documentTopic);

	double sub_lambda = 0;
	if (_probModelSet->useModel(ETProbModelSet::SUB))
		sub_lambda = _probModelSet->getLambdaForFullHistory(_observation, 
			_tagSet->getNoneTag(), ETProbModelSet::SUB);
	double obj_lambda = 0;
	if (_probModelSet->useModel(ETProbModelSet::OBJ))
		obj_lambda = _probModelSet->getLambdaForFullHistory(_observation, 
			_tagSet->getNoneTag(), ETProbModelSet::OBJ);
	double word_lambda = 0;
	if (_probModelSet->useModel(ETProbModelSet::WORD))
		word_lambda = _probModelSet->getLambdaForFullHistory(_observation,
			_tagSet->getNoneTag(), ETProbModelSet::WORD);

	if (word_lambda > .8 && !_observation->getLCWord().is_null()) {
		return false;
	}

	
	if (DEBUG) {
		char lambda_str[10];
		_snprintf(lambda_str, 9, "%f", word_lambda);
		lambda_str[9] = '\0';
		_debugStream << "\n*** TOKEN = " << tokens->getToken(tok)->getSymbol().to_string();
		_debugStream << " lambda = " << lambda_str << " ***\n";
	}

	bool use_sub_for_this_instance = false;
	bool use_obj_for_this_instance = false;
	bool use_word_for_this_instance = false;
	bool use_only_confident_p1 = false;
	
	if (sub_lambda > .8 && !_observation->getSubjectOfTrigger().is_null()) {
		if (DEBUG) _debugStream << "Subject Model is already confident\n";
		use_sub_for_this_instance = true;
		use_only_confident_p1 = true;
//		return false;
	}

	if (obj_lambda > .8 && !_observation->getObjectOfTrigger().is_null()) {
		if (DEBUG) _debugStream << "Object Model is already confident\n";
		use_obj_for_this_instance = true;
		use_only_confident_p1 = true;
//		return false;
	}

	
	if (word_lambda > .8 && !_observation->getLCWord().is_null()) {
		if (DEBUG) _debugStream << "Word Model is already confident\n";
		use_word_for_this_instance = true;
		use_only_confident_p1 = true;
//		return false;
	}
	
	int *best_tag = _new int[_probModelSet->getNModels() + 2];
	int *second_best_tag = _new int[_probModelSet->getNModels() + 2];
	double *best_score = _new double[_probModelSet->getNModels() + 2];
	double *second_best_score = _new double[_probModelSet->getNModels() + 2];
	double best_system_tag = _tagSet->getNoneTagIndex();
	double best_system_score = 0;

	for (int modelnum = 0; modelnum < _probModelSet->getNModels() + 2; modelnum++) {
		
		if (modelnum < _probModelSet->getNModels() && _probModelSet->useModel(modelnum)) {
			//if (modelnum == ETProbModelSet::SUB && !use_sub_for_this_instance)
			//	continue;
			//if (modelnum == ETProbModelSet::OBJ && !use_obj_for_this_instance)
			//	continue;
			_probModelSet->decodeToDistribution(_observation, modelnum);				
		} else if (modelnum == _probModelSet->getNModels() && _use_maxent_model) {
			decodeToMaxentDistribution();
		} else if (modelnum == _probModelSet->getNModels() + 1 && _use_p1_model) {
			decodeToP1Distribution();
			p1tag = _p1Model->decodeToInt(_observation);
		} else continue;

		
		int tag;
		for (tag = 1; tag < _tagSet->getNTags(); tag++) {
			
			if (!(modelnum < _probModelSet->getNModels() &&
				_tagScores[tag] > log(_recall_threshold)) &&
				!(modelnum == _probModelSet->getNModels() && 
				_tagScores[tag] > _recall_threshold) &&
				!(modelnum == _probModelSet->getNModels() + 1 && tag == p1tag))
				continue;

			if (modelnum == _probModelSet->getNModels() + 1 && use_only_confident_p1) {
				double none_score = _tagScores[_tagSet->getNoneTagIndex()];
				double tag_score = _tagScores[tag];
				if (tag_score < none_score)
					continue;
				if ((tag_score - none_score) / tag_score < .2)
					continue;
			}

			result = true;
			if (DEBUG) {
				_debugStream << "Model " << modelnum << " -- ";
				_debugStream << tokens->getToken(tok)->getSymbol() << ":\n";
				_debugStream << "NONE: " << _tagScores[0] << "\n";
				_debugStream << _tagSet->getTagSymbol(tag) << ": " << _tagScores[tag] << "\n";
			}

			if (_use_p1_model) {
				float none_prob = (float) _p1Model->getScore(_observation, _tagSet->getNoneTagIndex());
				if (none_prob < 0)
					none_prob = 0;
				float prob = (float) _p1Model->getScore(_observation, tag);
				if (prob < 0)
					prob = 1;
				float score = 0;
				if (prob + none_prob != 0) 
					score = prob / (none_prob + prob);

				if (DEBUG) _debugStream << "OVERALL SYSTEM SCORE: " << score << "\n";

				if (score > best_system_score) {
					best_system_score = score;
					best_system_tag = tag;
				}
			}

		}

		best_tag[modelnum] = 0;
		best_score[modelnum] = -20000;
		second_best_tag[modelnum] = 0;
		second_best_score[modelnum] = -20000;
		for (tag = 1; tag < _tagSet->getNTags(); tag++) {
			if (_tagScores[tag] > best_score[modelnum]) {
				second_best_tag[modelnum] = best_tag[modelnum];
				second_best_score[modelnum] = best_score[modelnum];
				best_tag[modelnum] = tag;
				best_score[modelnum] = _tagScores[tag];
			}
			else if (_tagScores[tag] > second_best_score[modelnum]) {
				second_best_tag[modelnum] = tag;
				second_best_score[modelnum] = _tagScores[tag];
			}
		}
	}

	if (_use_p1_model) {
		int p1_index = _probModelSet->getNModels() + 1;
		float none_prob = (float) _p1Model->getScore(_observation, _tagSet->getNoneTagIndex());
		if (none_prob < 0)
			none_prob = 0;
		float prob = (float) best_score[p1_index];
		if (prob < 0)
			prob = 1;
		float score = 0;
		if (prob + none_prob != 0) 
			score = prob / (none_prob + prob);
		// Strong positive P1 score for a relatively unseen word
		if (score > 0.5 && _probModelSet->useModel(ETProbModelSet::WORD) && word_lambda < 0.5) {
			result =  true;
			if (DEBUG) {
				_debugStream << _observation->getLCWord().to_string() << " shows a high score for ";
				_debugStream << _tagSet->getTagSymbol(best_tag[p1_index]).to_string();
				_debugStream << " and was relatively unseen.\n";
			}
		}
		// Somewhat likely according to P1 and either first or second choice of SUB and OBJ 
		else if (score < 0.5 && //score > 0.25 && 
				 _probModelSet->useModel(ETProbModelSet::OBJ) && 
				 !_observation->getObjectOfTrigger().is_null() &&
			     (best_tag[ETProbModelSet::OBJ] == best_tag[p1_index] ||
				 second_best_tag[ETProbModelSet::OBJ] == best_tag[p1_index]) &&
				 _probModelSet->useModel(ETProbModelSet::SUB) &&
				 !_observation->getSubjectOfTrigger().is_null() &&
			     (best_tag[ETProbModelSet::SUB] == best_tag[p1_index] ||
				 second_best_tag[ETProbModelSet::SUB] == best_tag[p1_index]))
		{
			result = true;
			if (DEBUG) {
				//_debugStream << _observation->getLCWord().to_string() << " shows an okay score for ";
				_debugStream << _observation->getLCWord().to_string() << " shows its best score for ";
				_debugStream << _tagSet->getTagSymbol(best_tag[p1_index]).to_string();
				_debugStream << ", which is reinforced by the SUB and OBJ models.\n";
			}
		}

	}

	delete [] best_tag;
	delete [] second_best_tag;
	delete [] best_score;
	delete [] second_best_score;


	return result;
}
*/

void EventModalityClassifier::printAnnotationSentence(UTF8OutputStream& out,
												 Symbol *docIds,
												 TokenSequence **tokenSequences,
											     int index, int n_sentences)
{
	const TokenSequence *tseq = tokenSequences[index];
	int tok;

	out << "================================================\n";

	if (tseq->getSentenceNumber() > 0) {
		const TokenSequence *prior = tokenSequences[index-1];
		for (tok = 0; tok < prior->getNTokens(); tok++) 
			out << prior->getToken(tok)->getSymbol().to_string() << " ";
		out << "\n\n";
	}
	
	out << "<S ID=\"" << docIds[index].to_string() << "-" << tseq->getSentenceNumber() << "\">";
	for (tok = 0; tok < tseq->getNTokens(); tok++) 
		out << tseq->getToken(tok)->getSymbol().to_string() << " ";
	out << "</S>\n\n";

	if (index < n_sentences - 1 && tokenSequences[index+1]->getSentenceNumber() != 0) {
		const TokenSequence *next = tokenSequences[index+1];
		for (tok = 0; tok < next->getNTokens(); tok++) 
			out << next->getToken(tok)->getSymbol().to_string() << " ";
	}
	out << "\n";
}

void EventModalityClassifier::printP1DebugScores(UTF8OutputStream& out) {
	int best = 0;
	int second_best = 0;
	int third_best = 0;
	double best_score = -100000;
	double second_best_score = -100000;
	double third_best_score = -100000;

	if (MODE == DEVTEST_TWO_LAYER || MODE == DECODE_TWO_LAYER){
		decodeToP1Distribution_2layer();
	}else{
		decodeToP1Distribution();
	}

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
	if (DEBUG) {
		if (MODE == DEVTEST_TWO_LAYER || MODE == DECODE_TWO_LAYER){
			if (_observation->hasRichFeatures()){
				_debugStream << L"P1 Top Outcomes: " << "\n----------------\n";
				_p1Model_layer1->printDebugInfo(_observation, best, _debugStream);
				_debugStream << L"\n";
				_p1Model_layer1->printDebugInfo(_observation, second_best, _debugStream);
				_debugStream << L"\n";
				_p1Model_layer1->printDebugInfo(_observation, third_best, _debugStream);
				_debugStream << L"\n";
			}else{
				_debugStream << L"P1 Top Outcomes: " << "\n----------------\n";
				_p1Model_layer2->printDebugInfo(_observation, best, _debugStream);
				_debugStream << L"\n";
				_p1Model_layer2->printDebugInfo(_observation, second_best, _debugStream);
				_debugStream << L"\n";
				_p1Model_layer2->printDebugInfo(_observation, third_best, _debugStream);
				_debugStream << L"\n";
			}
		}else{
			_debugStream << L"P1 Top Outcomes: " << "\n----------------\n";
			_p1Model->printDebugInfo(_observation, best, _debugStream);
			_debugStream << L"\n";
			_p1Model->printDebugInfo(_observation, second_best, _debugStream);
			_debugStream << L"\n";
			_p1Model->printDebugInfo(_observation, third_best, _debugStream);
			_debugStream << L"\n";
		}
	}
	out << _tagSet->getTagSymbol(best).to_string() << L": " << _tagScores[best] << L"<br>\n";
	out << _tagSet->getTagSymbol(second_best).to_string() << L": " << _tagScores[second_best] << L"<br>\n";
	out << _tagSet->getTagSymbol(third_best).to_string() << L": " << _tagScores[third_best] << L"<br>\n";
	out << "<br>\n";
}

void EventModalityClassifier::printMaxentDebugScores(UTF8OutputStream& out) {
	int best = 0;
	int second_best = 0;
	int third_best = 0;
	double best_score = -100000;
	double second_best_score = -100000;
	double third_best_score = -100000;
	
	if (MODE == DEVTEST_TWO_LAYER || MODE == DECODE_TWO_LAYER){
		int _classifierNo = _observation->getClassifierNo();
		if (_classifierNo == 1){
			_maxentModel_layer1->decodeToDistribution(_observation, _tagScores, _tagSet->getNTags());
		}else if (_classifierNo == 2){
			_maxentModel_layer2->decodeToDistribution(_observation, _tagScores, _tagSet->getNTags());
		}
	}else{
		_maxentModel->decodeToDistribution(_observation, _tagScores, _tagSet->getNTags());
	}

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
	//if (DEBUG){
	if (DEBUG && ! (MODE == DEVTEST_TWO_LAYER || MODE == DECODE_TWO_LAYER)) {
		_debugStream << L"Maxent Top Outcomes: " << "\n--------------------\n";
		_maxentModel->printDebugInfo(_observation, best, _debugStream);
		_debugStream << L"\n";
		_maxentModel->printDebugInfo(_observation, second_best, _debugStream);
		_debugStream << L"\n";
		_maxentModel->printDebugInfo(_observation, third_best, _debugStream);
		_debugStream << L"\n";
	}
	out << _tagSet->getTagSymbol(best).to_string() << L": " << _tagScores[best] << L"<br>\n";
	out << _tagSet->getTagSymbol(second_best).to_string() << L": " << _tagScores[second_best] << L"<br>\n";
	out << _tagSet->getTagSymbol(third_best).to_string() << L": " << _tagScores[third_best] << L"<br>\n";
	out << "<br>\n";
}

void EventModalityClassifier::decodeToP1Distribution() {
	for (int i = 0; i < _tagSet->getNTags(); i++) {
		_tagScores[i] = _p1Model->getScore(_observation, i);
	}
}


void EventModalityClassifier::decodeToP1Distribution_2layer() {
	if (_observation->hasRichFeatures()){
		for (int i = 0; i < _tagSet->getNTags(); i++) {
			_tagScores[i] = _p1Model_layer1->getScore(_observation, i);
		}
	}else{
		for (int i = 0; i < _tagSet->getNTags(); i++) {
			_tagScores[i] = _p1Model_layer2->getScore(_observation, i);
		}
	}
}

int EventModalityClassifier::decodeToMaxentDistribution() {
	int maxent_answer;
	_maxentModel->decodeToDistribution(_observation, _tagScores, _tagSet->getNTags(),
		&maxent_answer);
	if (maxent_answer != _tagSet->getNoneTagIndex())
		return maxent_answer;

	double best_score = -20000;
	int best_tag = 0;
	double second_best_score = -20000;
	int second_best_tag;
	for (int i = 0; i < _tagSet->getNTags(); i++) {
		if (_tagScores[i] > best_score) {
			second_best_score = best_score;
			second_best_tag = best_tag;
			best_score = _tagScores[i];
			best_tag = i;
		} else if (_tagScores[i] > second_best_score) {
			second_best_score = _tagScores[i];
			second_best_tag = i;
		}
	}
	if (best_tag == _tagSet->getNoneTagIndex() && second_best_score > _recall_threshold)
		return second_best_tag;
	else return best_tag;
}


int EventModalityClassifier::decodeToMaxentDistribution_2layer() {
	int maxent_answer;

	// option 1  -- use two layers independantly
	/*
	if (_observation->hasRichFeatures()){
		_maxentModel_layer1->decodeToDistribution(_observation, _tagScores, _tagSet->getNTags(),
		&maxent_answer);
	}else{
		_maxentModel_layer2->decodeToDistribution(_observation, _tagScores, _tagSet->getNTags(),
		&maxent_answer);
	}
	*/

	// option 2 
	if (_observation->hasRichFeatures()){
		_maxentModel_layer1->decodeToDistribution(_observation, _tagScores, _tagSet->getNTags(),
		&maxent_answer);
		if (_tagSet->getTagIndex(Symbol(L"Asserted")) == maxent_answer){
			_maxentModel_layer2->decodeToDistribution(_observation, _tagScores, _tagSet->getNTags(),
		&maxent_answer);
			_observation->assignClassifier(2);
		}else{
			_observation->assignClassifier(1);
		}
	}else{
		_maxentModel_layer2->decodeToDistribution(_observation, _tagScores, _tagSet->getNTags(),
		&maxent_answer);
		_observation->assignClassifier(2);
	}

	

	if (maxent_answer != _tagSet->getNoneTagIndex())
		return maxent_answer;

	double best_score = -20000;
	int best_tag = 0;
	double second_best_score = -20000;
	int second_best_tag;
	for (int i = 0; i < _tagSet->getNTags(); i++) {
		if (_tagScores[i] > best_score) {
			second_best_score = best_score;
			second_best_tag = best_tag;
			best_score = _tagScores[i];
			best_tag = i;
		} else if (_tagScores[i] > second_best_score) {
			second_best_score = _tagScores[i];
			second_best_tag = i;
		}
	}
	if (best_tag == _tagSet->getNoneTagIndex() && second_best_score > _recall_threshold)
		return second_best_tag;
	else return best_tag;
}


void EventModalityClassifier::resetRoundRobinStatistics() {
	_correct = 0;
	_wrong_type = 0;
}

void EventModalityClassifier::printRoundRobinStatistics(UTF8OutputStream &out) {
	double precision = (double) _correct / (_wrong_type + _correct);


	out << L"REGULAR\n\n";
	out << L"CORRECT: " << _correct << L"\n";
	out << L"WRONG TYPE: " << _wrong_type << L"\n\n";
	out << L"PRECISION: " << precision << L"\n\n";

}



// add for devTest 2008-01-22
void EventModalityClassifier::devTest(TokenSequence **tokenSequences, Parse **parses, 
								ValueMentionSet **valueMentionSets, MentionSet **mentionSets, 
								PropositionSet **propSets, EventMentionSet **esets, 
								int nsentences) {

	resetRoundRobinStatistics();
	
	std::string param = ParamReader::getRequiredParam("em_devtest_out");
	
	/*_devTestStream.open(param);
	char keyparam[500];
	strcpy (keyparam,param);
	strcat (keyparam,".key");
	_keyStream.open(keyparam);
	*/

	std::string HTMLparam = param + ".html";
	_htmlStream.open(HTMLparam.c_str());
	

	//decode(tokenSequences, parses, valueMentionSets, mentionSets, propSets, esets, nsentences,
	//	   _devTestStream, _keyStream, _htmlStream);
	
	
	decode(tokenSequences, parses, valueMentionSets, mentionSets, propSets, esets, nsentences,
		   _htmlStream);
	
	//printRoundRobinStatistics(_devTestStream);
	printRoundRobinStatistics(_htmlStream);
	//_devTestStream.close();
	_htmlStream.close();
	//_keyStream.close();
	
}

void EventModalityClassifier::train(TokenSequence **tokenSequences, Parse **parses, 
								ValueMentionSet **valueMentionSets, MentionSet **mentionSets, 
								PropositionSet **propSets, EventMentionSet **esets, 
								int nsentences) 
{
	if (MODE != TRAIN)
		return;

	// MODEL FILE
	std::string model_file = ParamReader::getRequiredParam("event_modality_model_file");

	int n_epochs = 1;
	bool print_every_epoch = false;
	if (_use_p1_model) {
		n_epochs = ParamReader::getRequiredIntParam("em_p1_epochs");
		if (ParamReader::isParamTrue("em_p1_print_every_epoch"))
			print_every_epoch = true;		

		if (_seed_features) {
			SessionLogger::info("SERIF") << "Seeding weight table with all features from training set...\n";
			for (int i = 0; i < nsentences; i++) {
				walkThroughTrainingSentence(ADD_FEATURES, -1, tokenSequences[i], parses[i],
											valueMentionSets[i], mentionSets[i],
											propSets[i], esets[i]);
			}
			if (_use_p1_model) {
				for (DTFeature::FeatureWeightMap::iterator iter = _p1Weights->begin();
					iter != _p1Weights->end(); ++iter)
				{
					(*iter).second.addToSum();
				}
			}
			if (print_every_epoch) {
				std::string buffer = model_file + "-epoch-0.p1";
				UTF8OutputStream p1_out(buffer.c_str());
				dumpP1TrainingParameters(p1_out, 0);
				DTFeature::writeSumWeights(*_p1Weights, p1_out, true);
				p1_out.close();
			}
		}
	}
	
	for (int epoch = 0; epoch < n_epochs; epoch++) {
        SessionLogger::LogMessageMaker msg = SessionLogger::info("SERIF");
		msg << "Epoch " << epoch + 1 << "\n";
		for (int i = 0; i < nsentences; i++) {
			walkThroughTrainingSentence(TRAIN, epoch, tokenSequences[i], parses[i],
										valueMentionSets[i], mentionSets[i],
										propSets[i], esets[i]);
			if (_use_p1_model && i % 16 == 0) {
				for (DTFeature::FeatureWeightMap::iterator iter = _p1Weights->begin();
					iter != _p1Weights->end(); ++iter)
				{
					(*iter).second.addToSum();
				}
			}
			if (i % 10 == 0) {
				msg << ".";
			}

		}
		if (print_every_epoch) {
			std::stringstream buffer;
			buffer << model_file << "-epoch-" << epoch+1 << ".p1";
			UTF8OutputStream p1_out(buffer.str().c_str());
			dumpP1TrainingParameters(p1_out, epoch + 1);
			DTFeature::writeSumWeights(*_p1Weights, p1_out);
			p1_out.close();
		}
	
	}

	SessionLogger::info("SERIF") << "Deriving models\n";
			
	if (_use_p1_model) {
		std::string buffer = model_file + ".p1";
		UTF8OutputStream p1_out(buffer.c_str());
		dumpP1TrainingParameters(p1_out, n_epochs);
		DTFeature::writeSumWeights(*_p1Weights, p1_out);
		p1_out.close();
	}

	if (_use_maxent_model) {
		int pruning = ParamReader::getRequiredIntParam("em_maxent_trainer_pruning_cutoff");
		_maxentModel->deriveModel(pruning);		
		std::string buffer = model_file + ".maxent";
		UTF8OutputStream maxent_out(buffer.c_str());
		dumpMaxentTrainingParameters(maxent_out);
		DTFeature::writeWeights(*_maxentWeights, maxent_out);
		maxent_out.close();
	}

	// we are now set to decode
	MODE = DECODE;
}



void EventModalityClassifier::train_2layer(TokenSequence **tokenSequences, Parse **parses, 
								ValueMentionSet **valueMentionSets, MentionSet **mentionSets, 
								PropositionSet **propSets, EventMentionSet **esets, 
								int nsentences) 
{
	if (MODE != TRAIN_TWO_LAYER)
		return;

	// MODEL FILE
	std::string model_file1 = ParamReader::getRequiredParam("event_modality_model_file_layer1");
	std::string model_file2 = ParamReader::getRequiredParam("event_modality_model_file_layer2");
	
	int n_epochs = 1;
	bool print_every_epoch = false;
	if (_use_p1_model) {
		n_epochs = ParamReader::getRequiredIntParam("em_p1_epochs");
		if (ParamReader::isParamTrue("em_p1_print_every_epoch"))
			print_every_epoch = true;		

		if (_seed_features) {
			SessionLogger::info("SERIF") << "Seeding weight table with all features from training set...\n";
			for (int i = 0; i < nsentences; i++) {
				walkThroughTrainingSentence_twoLayer(ADD_FEATURES, -1, tokenSequences[i], parses[i],
											valueMentionSets[i], mentionSets[i],
											propSets[i], esets[i]);
			}
			if (_use_p1_model) {
				for (DTFeature::FeatureWeightMap::iterator iter = _p1Weights_layer1->begin();
					iter != _p1Weights_layer1->end(); ++iter)
				{
					(*iter).second.addToSum();
				}

				for (DTFeature::FeatureWeightMap::iterator iter = _p1Weights_layer2->begin();
					iter != _p1Weights_layer2->end(); ++iter)
				{
					(*iter).second.addToSum();
				}

			}
			if (print_every_epoch) {
				std::string buffer = model_file1 + "-epoch-0.p1";
				UTF8OutputStream p1_out_layer1(buffer.c_str());
				dumpP1TrainingParameters(p1_out_layer1, 0);
				DTFeature::writeSumWeights(*_p1Weights_layer1, p1_out_layer1, true);
				p1_out_layer1.close();
				
				buffer = model_file2 + "-epoch-0.p1";
				UTF8OutputStream p1_out_layer2(buffer.c_str());
				dumpP1TrainingParameters(p1_out_layer2, 0);
				DTFeature::writeSumWeights(*_p1Weights_layer2, p1_out_layer2, true);
				p1_out_layer2.close();


			}
		}
	}
	
	for (int epoch = 0; epoch < n_epochs; epoch++) {
        SessionLogger::LogMessageMaker msg = SessionLogger::info("SERIF");
		msg << "Epoch " << epoch + 1 << "\n";
		for (int i = 0; i < nsentences; i++) {
			walkThroughTrainingSentence_twoLayer(TRAIN, epoch, tokenSequences[i], parses[i],
										valueMentionSets[i], mentionSets[i],
										propSets[i], esets[i]);
			if (_use_p1_model && i % 16 == 0) {
				for (DTFeature::FeatureWeightMap::iterator iter = _p1Weights_layer1->begin();
					iter != _p1Weights_layer1->end(); ++iter)
				{
					(*iter).second.addToSum();
				}

				for (DTFeature::FeatureWeightMap::iterator iter = _p1Weights_layer2->begin();
					iter != _p1Weights_layer2->end(); ++iter)
				{
					(*iter).second.addToSum();
				}

			}

			if (i % 10 == 0) {
				msg << ".";
			}

		}
		if (print_every_epoch) {
			std::stringstream buffer1;
			buffer1 << model_file1 << "-epoch-" << epoch+1 << ".p1";
			UTF8OutputStream p1_out_layer1(buffer1.str().c_str());
			dumpP1TrainingParameters(p1_out_layer1, epoch + 1);
			DTFeature::writeSumWeights(*_p1Weights_layer1, p1_out_layer1);
			p1_out_layer1.close();				
			
			std::stringstream buffer2;
			buffer2 << model_file2 << "-epoch-" << epoch+1 << ".p1";
			UTF8OutputStream p1_out_layer2(buffer2.str().c_str());
			dumpP1TrainingParameters(p1_out_layer2, epoch + 1);
			DTFeature::writeSumWeights(*_p1Weights_layer2, p1_out_layer2);
			p1_out_layer2.close();
		}
	}

	SessionLogger::info("SERIF") << "Deriving models\n";
			
	if (_use_p1_model) {
		std::string buffer = model_file1 + ".p1";
		UTF8OutputStream p1_out_layer1(buffer.c_str());
		dumpP1TrainingParameters(p1_out_layer1, n_epochs);
		DTFeature::writeSumWeights(*_p1Weights_layer1, p1_out_layer1);
		p1_out_layer1.close();

		buffer = model_file2 + ".p1";
		UTF8OutputStream p1_out_layer2(buffer.c_str());
		dumpP1TrainingParameters(p1_out_layer2, n_epochs);
		DTFeature::writeSumWeights(*_p1Weights_layer2, p1_out_layer2);
		p1_out_layer2.close();

	}

	if (_use_maxent_model) {
		int pruning = ParamReader::getRequiredIntParam("em_maxent_trainer_pruning_cutoff");
		_maxentModel_layer1->deriveModel(pruning);
		_maxentModel_layer2->deriveModel(pruning);
		std::string buffer = model_file1 + ".maxent";
		UTF8OutputStream maxent_out_layer1(buffer.c_str());
		dumpMaxentTrainingParameters(maxent_out_layer1);
		DTFeature::writeWeights(*_maxentWeights_layer1, maxent_out_layer1);
		maxent_out_layer1.close();

		buffer = model_file2 + ".maxent";
		UTF8OutputStream maxent_out_layer2(buffer.c_str());
		dumpMaxentTrainingParameters(maxent_out_layer2);
		DTFeature::writeWeights(*_maxentWeights_layer2, maxent_out_layer2);
		maxent_out_layer2.close();

	}

	// we are now set to decode
	MODE = DECODE;
}


void EventModalityClassifier::walkThroughTrainingSentence(int mode, int epoch, 
													 const TokenSequence *tseq, 
													 const Parse *parse, 
													 const ValueMentionSet *valueMentionSet,
													 MentionSet *mentionSet, 
													 const PropositionSet *propSet, 
													 const EventMentionSet *eset) 
{
	int modalityTypes[MAX_SENTENCE_TOKENS];

	for (int j = 0; j < tseq->getNTokens(); j++) {
		modalityTypes[j] = 0;   // 0 means no classification for modality
	}

	for (int ement = 0; ement < eset->getNEventMentions(); ement++) {
		EventMention *em = eset->getEventMention(ement);
		if (_tagSet->getTagIndex(em->getModalityType()) == -1) {
			char c[500];
			sprintf( c, "ERROR: Invalid event modality type in training file: %s", em->getModalityType().to_debug_string());
			throw UnexpectedInputException("EventModalityClassifier::walkThroughTrainingSentence()", c);
		}			
		modalityTypes[em->getAnchorNode()->getStartToken()] = _tagSet->getTagIndex(em->getModalityType());
	}

	for (int k = 0; k < tseq->getNTokens(); k++) {
		if (modalityTypes[k] > 0){
			_observation->populate(k, tseq, parse, mentionSet, 
				propSet, true);
			
			if (_use_p1_model) {
				if (mode == TRAIN) {
					_p1Model->train(_observation, modalityTypes[k]);
				} else if (mode == ADD_FEATURES) {
					_p1Model->addFeatures(_observation, modalityTypes[k]);
		
				} 
			}
			if (epoch == 0) {
				if (_use_maxent_model)
					_maxentModel->addToTraining(_observation, modalityTypes[k]);
							
			}
			
		}
	}
}


void EventModalityClassifier::walkThroughTrainingSentence_twoLayer(int mode, int epoch, 
													 const TokenSequence *tseq, 
													 const Parse *parse, 
													 const ValueMentionSet *valueMentionSet,
													 MentionSet *mentionSet, 
													 const PropositionSet *propSet, 
													 const EventMentionSet *eset) 
{
	int modalityTypes[MAX_SENTENCE_TOKENS];

	for (int j = 0; j < tseq->getNTokens(); j++) {
		modalityTypes[j] = 0;   // 0 means no classification for modality
	}

	for (int ement = 0; ement < eset->getNEventMentions(); ement++) {
		EventMention *em = eset->getEventMention(ement);
		if (_tagSet->getTagIndex(em->getModalityType()) == -1) {
			char c[500];
			sprintf( c, "ERROR: Invalid event modality type in training file: %s", em->getModalityType().to_debug_string());
			throw UnexpectedInputException("EventModalityClassifier::walkThroughTrainingSentence_twoLayer()", c);
		}			
		modalityTypes[em->getAnchorNode()->getStartToken()] = _tagSet->getTagIndex(em->getModalityType());
	}

	for (int k = 0; k < tseq->getNTokens(); k++) {
		if (modalityTypes[k] > 0){
			_observation->populate(k, tseq, parse, mentionSet, 
				propSet, true);
			
			if (_use_p1_model) {
				if (mode == TRAIN) {
					if (_observation->hasRichFeatures()){
						_p1Model_layer1->train(_observation, modalityTypes[k]);
					}else{
						_p1Model_layer2->train(_observation, modalityTypes[k]);
					}
				} else if (mode == ADD_FEATURES) {
					if (_observation->hasRichFeatures()){
						_p1Model_layer1->addFeatures(_observation, modalityTypes[k]);
					}else{
						_p1Model_layer2->addFeatures(_observation, modalityTypes[k]);
					}
				} 
			}
			if (epoch == 0) {
				if (_use_maxent_model){
					// option 1
					/*
					if (_observation->hasRichFeatures()){
						_maxentModel_layer1->addToTraining(_observation, modalityTypes[k]);
					}else{
						_maxentModel_layer2->addToTraining(_observation, modalityTypes[k]);
					}
					*/

					// option 2
					if (_observation->hasRichFeatures()){
						_maxentModel_layer1->addToTraining(_observation, modalityTypes[k]);
					}
					_maxentModel_layer2->addToTraining(_observation, modalityTypes[k]);
				}
			
			}
			
		}
	}
}



void EventModalityClassifier::dumpP1TrainingParameters(UTF8OutputStream &out, int epoch) {

	DTFeature::recordDate(out);

	out << L"Parameters:\n";
	DTFeature::recordParamForConsistency(Symbol(L"event_modality_tag_set_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"event_modality_features_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"word_cluster_bits_file"), out);

	DTFeature::recordParamForReference(Symbol(L"event_training_file_list"), out);
	DTFeature::recordParamForReference(Symbol(L"em_p1_epochs"), out);
	out << "actual_epoch_output_here " << epoch << "\n";
	out << L"\n";

}

void EventModalityClassifier::dumpMaxentTrainingParameters(UTF8OutputStream &out) {

	DTFeature::recordDate(out);

	out << L"Parameters:\n";
	DTFeature::recordParamForConsistency(Symbol(L"event_modality_tag_set_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"event_modality_features_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"word_cluster_bits_file"), out);

	DTFeature::recordParamForReference(Symbol(L"event_training_file_list"), out);
	DTFeature::recordParamForReference(Symbol(L"em_maxent_trainer_percent_held_out"), out);
	DTFeature::recordParamForReference(Symbol(L"em_maxent_trainer_max_iterations"), out);
	DTFeature::recordParamForReference(Symbol(L"em_maxent_trainer_gaussian_variance"), out);
	DTFeature::recordParamForReference(Symbol(L"em_maxent_trainer_min_likelihood_delta"), out);
	DTFeature::recordParamForReference(Symbol(L"em_maxent_trainer_stop_check_frequency"), out);
	DTFeature::recordParamForReference(Symbol(L"em_maxent_trainer_pruning_cutoff"), out);
	out << L"\n";

}

// for modality classification, the classifier only classifies event trigger positions
int EventModalityClassifier::processSentence(const TokenSequence *tokens, 
										const Parse *parse, 
										MentionSet *mentionSet, 
										const PropositionSet *propSet, 
										EventMention **potentials,
										int max)
{
	int n_found = 0;
	if (DEBUG) {
		_debugStream << parse->getRoot()->toTextString() << "\n";
	}
	for (int tok = 0; tok < tokens->getNTokens(); tok++) {
		if (n_found >= max)
			break;
		n_found += processToken(tok, tokens, parse, mentionSet, propSet, 
								potentials + n_found, max - n_found);
	}

	return n_found;

}

//current implementation doesn't allow combination of P1 and maxEnt models
int EventModalityClassifier::processToken(int tok, const TokenSequence *tokens, 
									 const Parse *parse, MentionSet *mentionSet, 
									 const PropositionSet *propSet,
									 EventMention **esets, int max) 
{
	int n_found = 0;

	_observation->populate(tok, tokens, parse, mentionSet, propSet, true);

	for (int tag = 1; tag < _tagSet->getNTags(); tag++) {
		_tagFound[tag] = false;
	}
	int p1tag = 0;

	
	if (_use_maxent_model) {
		if (MODE == DEVTEST_TWO_LAYER || MODE == DECODE_TWO_LAYER){
			decodeToMaxentDistribution_2layer();
		}else{
			decodeToMaxentDistribution();
		}
	} else if (_use_p1_model) {
		if (MODE == DEVTEST_TWO_LAYER || MODE == DECODE_TWO_LAYER){
			decodeToP1Distribution_2layer();
			if (_observation->hasRichFeatures()){
				p1tag = _p1Model_layer1->decodeToInt(_observation, _tagScores);
			}else{
				p1tag = _p1Model_layer2->decodeToInt(_observation, _tagScores);
			}
		}else{
			decodeToP1Distribution();
			p1tag = _p1Model->decodeToInt(_observation, _tagScores);
		}
		
		
	} 

	bool tok_printed = false;
	for (int tag = 1; tag < _tagSet->getNTags(); tag++) {
		if (n_found >= max)
			break;
			
		if (    !(_use_maxent_model && 
				_tagScores[tag] > _recall_threshold) &&
				!(_use_p1_model && tag == p1tag))
			continue;

		if (_use_p1_model) {
			double none_score = _tagScores[_tagSet->getNoneTagIndex()];
			double tag_score = _tagScores[tag];
			if (tag_score < none_score)
				continue;
			if ((tag_score - none_score) / tag_score < .2)
				continue;
		}

		if (DEBUG) {
			if (!tok_printed) {
				_debugStream << "Model -- ";
				_debugStream << tokens->getToken(tok)->getSymbol() << ":\n";
				_debugStream << "NONE: " << _tagScores[0] << "\n";
				tok_printed = true;
			}
			_debugStream << _tagSet->getTagSymbol(tag) << ": " << _tagScores[tag] << "\n";
			}
		if (_tagFound[tag])
			continue;
		_tagFound[tag] = true;

		esets[n_found] = _new EventMention(tokens->getSentenceNumber(), n_found);
		esets[n_found]->setModalityType(_tagSet->getTagSymbol(tag));

		const SynNode *tNode = parse->getRoot()->getNthTerminal(tok);
		// we want the preterminal node, since that's what we would get if we were
		// going from a proposition to a node
		if (tNode->getParent() != 0)
			tNode = tNode->getParent();
		esets[n_found]->setAnchor(tNode, propSet);
			
		// set score
		setScore(esets[n_found]);

		n_found++;
		
	}
	return n_found;
}






float EventModalityClassifier::setScore(EventMention *eventMention) {
	
	eventMention->setScore(0);

	if (eventMention->getScore() == 0 && _use_maxent_model) {
		if (MODE == DEVTEST_TWO_LAYER || MODE == DECODE_TWO_LAYER){
			decodeToMaxentDistribution_2layer();
		}else{
			decodeToMaxentDistribution();
		}
		int index = _tagSet->getTagIndex(eventMention->getModalityType());
		eventMention->addToScore((float) _tagScores[index]);
	}

	if (eventMention->getScore() == 0 && _use_p1_model) {
		float none_prob;
		if (MODE == DEVTEST_TWO_LAYER || MODE == DECODE_TWO_LAYER){
			if (_observation->hasRichFeatures()){
				none_prob = (float) _p1Model_layer1->getScore(_observation, _tagSet->getNoneTagIndex());
			}else{
				none_prob = (float) _p1Model_layer2->getScore(_observation, _tagSet->getNoneTagIndex());
			}
		}else{
			none_prob = (float) _p1Model->getScore(_observation, _tagSet->getNoneTagIndex());
		}

		if (none_prob < 0)
			none_prob = 0;

		float prob;
		if (MODE == DEVTEST_TWO_LAYER || MODE == DECODE_TWO_LAYER){
			if (_observation->hasRichFeatures()){
				prob = (float) _p1Model_layer1->getScore(_observation,
				_tagSet->getTagIndex(eventMention->getModalityType()));
			}else{
				prob = (float) _p1Model_layer2->getScore(_observation,
				_tagSet->getTagIndex(eventMention->getModalityType()));
			}
		}else{
			prob = (float) _p1Model->getScore(_observation,
				_tagSet->getTagIndex(eventMention->getModalityType()));
		}

		if (prob < 0)
			prob = 1;
		float score = 0;
		if (prob + none_prob != 0) 
			score = prob / (none_prob + prob);
		float log_score = 0;
		if (score != 0)
			log_score = log(score);
		eventMention->addToScore(log_score);
	}

	return eventMention->getScore();
}

