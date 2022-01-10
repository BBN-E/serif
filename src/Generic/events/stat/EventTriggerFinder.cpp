// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/stat/EventTriggerFinder.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerFeatureTypes.h"
#include "Generic/events/stat/EventTriggerSentence.h"
#include "Generic/events/stat/PotentialEventMention.h"
#include "Generic/events/stat/EventTriggerObservation.h"
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
#include "Generic/theories/NodeInfo.h"

EventTriggerFinder::EventTriggerFinder(int mode_) : MODE(mode_)
{
	resetRoundRobinStatistics();
	_observation = _new EventTriggerObservation();
	EventTriggerFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();


	// DEBUGGING OUTPUT
	std::string buffer = ParamReader::getParam("event_trigger_debug");
	DEBUG = (!buffer.empty());
	if (DEBUG) _debugStream.open(buffer.c_str());

	// TAG SET
	std::string tag_set_file = ParamReader::getRequiredParam("event_trigger_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);
	_tagScores = _new double[_tagSet->getNTags()];
	_tagFound = _new bool[_tagSet->getNTags()];

	// FEATURES
	std::string features_file = ParamReader::getRequiredParam("event_trigger_features_file");
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), EventTriggerFeatureType::modeltype);

	_use_p1_model = ParamReader::getRequiredTrueFalseParam("et_use_p1_model");
	_use_maxent_model = ParamReader::getRequiredTrueFalseParam("et_use_maxent_model");

	_documentTopic = Symbol();

	if (MODE == DECODE) {

		// RECALL THRESHOLD
		const string decode_dump_file = ParamReader::getParam("decode_dump_file");
		if (decode_dump_file != "") {
			SessionLogger::info("SERIF") << "Dumping feature vectors to " << decode_dump_file;
		}

		_recall_threshold = ParamReader::getRequiredFloatParam("event_trigger_recall_threshold");
		_probModelSet = _new ETProbModelSet(_tagSet, _tagScores, _recall_threshold);

		_p1Weights = 0;
		_maxentWeights = 0;
		_p1Model = 0;		
		_maxentModel = 0;
		_probModelSet->clearModels();

		if (_use_p1_model) {
			_p1_overgen_percentage = (float) ParamReader::getRequiredFloatParam("et_p1_recall_threshold");
		} else _p1_overgen_percentage = 0;

		// MODEL FILE
		std::string model_file = ParamReader::getParam("event_trigger_model_file");
		if (!model_file.empty())
		{
			_probModelSet->loadModels(model_file.c_str());

			if (_use_p1_model) {
				std::string buffer = model_file + ".p1";
				_p1Weights = _new DTFeature::FeatureWeightMap(50000);
				DTFeature::readWeights(*_p1Weights, buffer.c_str(), EventTriggerFeatureType::modeltype);
				
				_p1Model = _new P1Decoder(_tagSet, _featureTypes, _p1Weights, 
					_p1_overgen_percentage, false);
			} 

			if (_use_maxent_model) {
				std::string buffer = model_file + ".maxent";
				_maxentWeights = _new DTFeature::FeatureWeightMap(50000);
				DTFeature::readWeights(*_maxentWeights, buffer.c_str(), EventTriggerFeatureType::modeltype);
				_maxentModel = _new MaxEntModel(_tagSet, _featureTypes, _maxentWeights);
			} 

		} else if (!ParamReader::hasParam("event_round_robin_setup")) {

			throw UnexpectedInputException("EventTriggerFinder::EventTriggerFinder()",
				"Neither parameter 'event_trigger_model' nor 'event_round_robin_setup' specified");

		} 

	} else if (MODE == TRAIN) {
		const string train_vector_file = ParamReader::getParam("training_vectors_file");
		if (train_vector_file != "") {
			SessionLogger::info("SERIF") << "Dumping feature vectors to " << train_vector_file;
		}

		if (_use_p1_model) {
			_p1Weights = _new DTFeature::FeatureWeightMap(50000);
			_p1Model = _new P1Decoder(_tagSet, _featureTypes, _p1Weights, true);
			if (train_vector_file != "") {
				_p1Model->setLogFile(train_vector_file);
			}
			// SEED FEATURES
			_seed_features = ParamReader::getRequiredTrueFalseParam("et_p1_trainer_seed_features");
		} else {
			_p1Model = 0;
			_p1Weights = 0;
			_seed_features = 0;
		}

		if (_use_maxent_model) {
			// PERCENT HELD OUT
			int percent_held_out = 
				ParamReader::getRequiredIntParam("et_maxent_trainer_percent_held_out");
			if (percent_held_out < 0 || percent_held_out > 50) 
				throw UnexpectedInputException("EventTriggerFinder::EventTriggerFinder()",
				"Parameter 'et_maxent_trainer_percent_held_out' must be between 0 and 50");

			// MAX NUMBER OF ITERATIONS (STOPPING CONDITION)
			int max_iterations = 
				ParamReader::getRequiredIntParam("et_maxent_trainer_max_iterations");

			// GAUSSIAN PRIOR VARIANCE
			double variance = ParamReader::getRequiredFloatParam("et_maxent_trainer_gaussian_variance");

			// MIN CHANGE IN LIKELIHOOD (STOPPING CONDITION)
			double likelihood_delta = ParamReader::getOptionalFloatParamWithDefaultValue("et_maxent_trainer_min_likelihood_delta", .0001);

			// FREQUENCY OF STOPPING CONDITION CHECKS (NUM ITERATIONS)
			int stop_check_freq = ParamReader::getOptionalIntParamWithDefaultValue("et_maxent_trainer_stop_check_frequency",1);

			_maxentWeights = _new DTFeature::FeatureWeightMap(50000);
			_maxentModel = _new MaxEntModel(_tagSet, _featureTypes, _maxentWeights, 
				MaxEntModel::SCGIS, percent_held_out, max_iterations, variance,
				likelihood_delta, stop_check_freq, train_vector_file.c_str(), "");

		} else {
			_maxentModel = 0;
			_maxentWeights = 0;
		}

		_probModelSet = _new ETProbModelSet(_tagSet, _tagScores, 0);
		_probModelSet->createEmptyModels();

	}
}

EventTriggerFinder::~EventTriggerFinder() {
	delete _probModelSet;
	delete _p1Model;
	delete _p1Weights;
	delete _maxentModel;
	delete _maxentWeights;
	delete _tagSet;
	delete _featureTypes;
	delete _observation;
	delete [] _tagScores;	
}

void EventTriggerFinder::replaceModel(char *model_file) {

	delete _p1Weights;
	delete _p1Model;
	delete _maxentWeights;
	delete _maxentModel;

	std::string model_file_str(model_file);

	if (_use_p1_model) {
		std::string buffer = model_file_str + ".p1";
		_p1Weights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_p1Weights, buffer.c_str(), EventTriggerFeatureType::modeltype);
		_p1Model = _new P1Decoder(_tagSet, _featureTypes, _p1Weights, 
			_p1_overgen_percentage, false);
	} else {
		_p1Model = 0;
		_p1Weights = 0;
	}
	
	if (_use_maxent_model) {
		std::string buffer = model_file_str + ".maxent";
		_maxentWeights = _new DTFeature::FeatureWeightMap(50000);
		DTFeature::readWeights(*_maxentWeights, buffer.c_str(), EventTriggerFeatureType::modeltype);
		_maxentModel = _new MaxEntModel(_tagSet, _featureTypes, _maxentWeights);
	} else {
		_maxentModel = 0;
		_maxentWeights = 0;
	}

	_probModelSet->deleteModels();
	_probModelSet->loadModels(model_file);
}

void EventTriggerFinder::roundRobinDecode(TokenSequence **tokenSequences, Parse **parses, 
								ValueMentionSet **valueMentionSets, MentionSet **mentionSets, 
								PropositionSet **propSets, EventMentionSet **esets, 
								int nsentences,
								UTF8OutputStream& resultStream,
								UTF8OutputStream& HRresultStream,
								UTF8OutputStream& keyStream,
								UTF8OutputStream& htmlStream) 
{
	if (MODE != DECODE)
		return;

	int eventTypes[MAX_SENTENCE_TOKENS];
	int nEventArgs[MAX_SENTENCE_TOKENS];
	EventMention *tmpEvents[MAX_TOKEN_TRIGGERS];

	for (int i = 0; i < nsentences; i++) {
		const TokenSequence *tseq = tokenSequences[i];
		if (tseq->getSentenceNumber() == 0)
			_documentTopic = DocEventHandler::assignTopic(tokenSequences, parses, i, nsentences);
		for (int j = 0; j < tseq->getNTokens(); j++) {
			eventTypes[j] = 0;
			nEventArgs[j] = 0;
		}
		for (int ement = 0; ement < esets[i]->getNEventMentions(); ement++) {
			EventMention *em = esets[i]->getEventMention(ement);
			if (_tagSet->getTagIndex(em->getEventType()) == -1) {
				throw UnexpectedInputException("EventTriggerFinder::roundRobinDecode()",
					"Invalid event type in training file");
			}			
			eventTypes[em->getAnchorNode()->getStartToken()] = _tagSet->getTagIndex(em->getEventType());
			nEventArgs[em->getAnchorNode()->getStartToken()] = em->getNArgs();
		}

		for (int k = 0; k < tseq->getNTokens(); k++) {
			int correct_answer = eventTypes[k];
			if (correct_answer != _tagSet->getNoneTagIndex()) {
				keyStream << L"<ENAMEX TYPE=\"" 
						  << _tagSet->getTagSymbol(correct_answer).to_string() << L"\">";
			}
			keyStream << tseq->getToken(k)->getSymbol().to_string();
			if (correct_answer != _tagSet->getNoneTagIndex()) {
				keyStream << L"</ENAMEX>";
			}
			keyStream << L" ";
			
			//Decode to get label for token
			int n_system_answers = 0;
			processToken(k, tseq, parses[i], mentionSets[i],
				propSets[i], tmpEvents, n_system_answers, MAX_TOKEN_TRIGGERS);

			// Return answer with highest score
			int system_answer = _tagSet->getNoneTagIndex();
			float this_score, system_score = 0;
			for (int m = 0; m < n_system_answers; m++) {
				this_score = exp(tmpEvents[m]->getScore());
				if (this_score > system_score) {
					system_score = this_score;
					system_answer = _tagSet->getTagIndex(tmpEvents[m]->getEventType());
				}
			}

			for (int n = 0; n < n_system_answers; n++) 
				delete tmpEvents[n];
		
			/*
			// Combine model results
			int p1_answer = _tagSet->getNoneTagIndex();
			int maxent_answer = _tagSet->getNoneTagIndex();
			if (_use_p1_model)
				p1_answer = _p1Model->decodeToInt(_observation);
			if (_use_maxent_model)
				maxent_answer = decodeToMaxentDistribution();
			int pm_answer = _probModelSet->getAnswer(_observation);
			
			int system_answer = pm_answer;
			float system_score = 0;
			if (system_answer == _tagSet->getNoneTagIndex())
				system_answer = p1_answer;
			if (system_answer == _tagSet->getNoneTagIndex())
				system_answer = maxent_answer;
			*/

			//Update calculating recall, precision during round robbin
			updateRRCounts(correct_answer, system_answer, nEventArgs[k]);
			
			//Print to HTML output file with correct/missing/spurious 
			if (correct_answer == system_answer) {
				if (correct_answer != _tagSet->getNoneTagIndex()) {
					printHTMLSentence(htmlStream, tseq, k, correct_answer, system_answer, system_score);

				}
			} else if (correct_answer == _tagSet->getNoneTagIndex()) {
				printHTMLSentence(htmlStream, tseq, k, correct_answer, system_answer, system_score);
				
			} else if (system_answer == _tagSet->getNoneTagIndex()) {
				htmlStream<<"<b>Number of Correct Event Args: "<<nEventArgs[k]<<"</b><br>\n";
				printHTMLSentence(htmlStream, tseq, k, correct_answer, system_answer, system_score);
				htmlStream<<"<b>CorrectFeatures: <br></b>\n";
				_p1Model->printHTMLDebugInfo(_observation, correct_answer, htmlStream);
				htmlStream<<"<br>\n<b>NoneFeatures: </b><br>\n";
				_p1Model->printHTMLDebugInfo(_observation, _tagSet->getNoneTagIndex(), htmlStream);
				
			} else {
				printHTMLSentence(htmlStream, tseq, k, correct_answer, system_answer, system_score);

			}
			//Print enamex Style output
			if (system_answer != _tagSet->getNoneTagIndex()) {
				resultStream << L"<ENAMEX TYPE=\"" 
							 << _tagSet->getTagSymbol(system_answer).to_string() << L"\">";
			}
			resultStream << tseq->getToken(k)->getSymbol().to_string();
			if (system_answer != _tagSet->getNoneTagIndex()) {
				resultStream << L"</ENAMEX>";
			}
			resultStream << L" ";			
		}
		resultStream << L"\n\n";
		HRresultStream << L"\n\n";
		keyStream << L"\n\n";

	}

}
void EventTriggerFinder::updateRRCounts(int correct, int ans, int nEventArgs ){
	//which model was used for this decoding
	bool use_sub_for_this_instance = false;
	bool use_obj_for_this_instance = false;
	bool use_only_confident_p1 = false;
	selectModels(use_sub_for_this_instance, use_obj_for_this_instance, use_only_confident_p1);
	std::set<int> known_tags = _p1Model->featureInTable(_observation, Symbol(L"stemmed-word")); //Symbol(L"lc-word"));

	if(correct == ans){
		if(correct != _tagSet->getNoneTagIndex()){
			_scoringCounts[CORRECT][COUNT_ALL]++;
			_scoringCounts[CORRECT][COUNT_BY_ARG]+=nEventArgs;
			if (use_sub_for_this_instance || use_obj_for_this_instance) _scoringCounts[CORRECT][COUNT_PM]++;
			if (known_tags.size() > 0) _scoringCounts[CORRECT][COUNT_KNOWN]++;
			if (known_tags.find(correct) != known_tags.end()) _scoringCounts[CORRECT][COUNT_KNOWN_CORRECT]++;
		}
	}
	else if(correct == _tagSet->getNoneTagIndex()){
		_scoringCounts[SPURIOUS][COUNT_ALL]++;
		_scoringCounts[SPURIOUS][COUNT_BY_ARG]+=nEventArgs;
		if (use_sub_for_this_instance || use_obj_for_this_instance) _scoringCounts[SPURIOUS][COUNT_PM]++;
		if (known_tags.size() > 0) _scoringCounts[SPURIOUS][COUNT_KNOWN]++;
		if (known_tags.find(correct) != known_tags.end()) _scoringCounts[SPURIOUS][COUNT_KNOWN_CORRECT]++;
	}
	else if (ans == _tagSet->getNoneTagIndex()) {
		_scoringCounts[MISSING][COUNT_ALL]++;
		_scoringCounts[MISSING][COUNT_BY_ARG]+=nEventArgs;
		if (use_sub_for_this_instance || use_obj_for_this_instance) _scoringCounts[MISSING][COUNT_PM]++;
		if (known_tags.size() > 0) _scoringCounts[MISSING][COUNT_KNOWN]++;
		if (known_tags.find(correct) != known_tags.end()) _scoringCounts[MISSING][COUNT_KNOWN_CORRECT]++;
	}
	else{
		_scoringCounts[WRONG_TYPE][COUNT_ALL]++;
		_scoringCounts[WRONG_TYPE][COUNT_BY_ARG]+=nEventArgs;
		if (use_sub_for_this_instance || use_obj_for_this_instance) _scoringCounts[WRONG_TYPE][COUNT_PM]++;
		if (known_tags.size() > 0) _scoringCounts[WRONG_TYPE][COUNT_KNOWN]++;
		if (known_tags.find(correct) != known_tags.end()) _scoringCounts[WRONG_TYPE][COUNT_KNOWN_CORRECT]++;
	}
}
void EventTriggerFinder::printHTMLSentence(UTF8OutputStream &out, const TokenSequence *tseq, 
										   int index, int correct_answer, int system_answer,
										   float system_score) 
{
	for (int tok = 0; tok < tseq->getNTokens(); tok++) {
		if (tok == index) {
			out << L"<u><b>";
			if (system_answer == correct_answer) 
				out << L"<font color=\"red\">";
			else if (correct_answer == _tagSet->getNoneTagIndex())
				out << L"<font color=\"blue\">";
			else if (system_answer == _tagSet->getNoneTagIndex())
				out << L"<font color=\"purple\">";
			else out << L"<font color=\"green\">";
		}
		out << tseq->getToken(tok)->getSymbol().to_string();
		if (DEBUG) _debugStream << tseq->getToken(tok)->getSymbol().to_string();
		if (tok == index) {
			if (system_answer == correct_answer)
				out << L" (" << _tagSet->getTagSymbol(correct_answer) << L")";
			else if (correct_answer == _tagSet->getNoneTagIndex())
				out << L" (SPURIOUS: " << _tagSet->getTagSymbol(system_answer) << L")";			
			else if (system_answer == _tagSet->getNoneTagIndex())
				out << L" (MISSING: " << _tagSet->getTagSymbol(correct_answer) << L")";
			else {
				out << L" (WRONG TYPE: " << _tagSet->getTagSymbol(system_answer);
				out << L" should be " << _tagSet->getTagSymbol(correct_answer) << L")";
			}
			out << L"</font></u></b>";
		}
		out << L" ";
		if (DEBUG) _debugStream << L" ";
	}
	//_probModelSet->decodeToDistribution(_observation, ETProbModelSet::WORD);
	//out << "(system score: " << _tagScores[system_answer] << ")";
	out << L"<br><br>\n";
	if (DEBUG) _debugStream << L"\n\n";

	out << L"Overall System Score = " << system_score << L"<br><br>\n";
	bool use_sub_for_this_instance = false;
	bool use_obj_for_this_instance = false;
	bool use_only_confident_p1 = false;
	selectModels(use_sub_for_this_instance, use_obj_for_this_instance, use_only_confident_p1);
	out<<"Use Sub: "<<use_sub_for_this_instance	<<" Use Obj: "<<use_obj_for_this_instance
		<<" Use Only ConfP1: "<<use_only_confident_p1<<"<br>\n";

	if (_use_p1_model) 
		printP1DebugScores(out);
	if (_use_maxent_model)
		printMaxentDebugScores(out);
	_probModelSet->printPMDebugScores(_observation, out);
}

void EventTriggerFinder::selectAnnotation(Symbol *docIds, Symbol *docTopics,
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

bool EventTriggerFinder::isHighValueAnnotation(int tok, const TokenSequence *tokens, 
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

	/*
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

	}*/

	delete [] best_tag;
	delete [] second_best_tag;
	delete [] best_score;
	delete [] second_best_score;


	return result;
}

void EventTriggerFinder::printAnnotationSentence(UTF8OutputStream& out,
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

void EventTriggerFinder::printP1DebugScores(UTF8OutputStream& out) {
	int best = 0;
	int second_best = 0;
	int third_best = 0;
	double best_score = -100000;
	double second_best_score = -100000;
	double third_best_score = -100000;

	decodeToP1Distribution();
	int p1tag = _p1Model->decodeToInt(_observation, _tagScores);

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
		_debugStream << L"P1 Top Outcomes: " << "\n----------------\n";
		_p1Model->printDebugInfo(_observation, best, _debugStream);
		_debugStream << L"\n";
		_p1Model->printDebugInfo(_observation, second_best, _debugStream);
		_debugStream << L"\n";
		_p1Model->printDebugInfo(_observation, third_best, _debugStream);
		_debugStream << L"\n";
	}
	out << L"ETP1Model<br>\n";
	
	out << _tagSet->getTagSymbol(best).to_string() << L": " << _tagScores[best];
	if(p1tag == best)
		out<<" --- Selected";
	if(isConfidentP1Tag(best))
		out<<" --- Confident";
	out << L"<br>\n";
	out << _tagSet->getTagSymbol(second_best).to_string() << L": " << _tagScores[second_best];
	if(p1tag == second_best)
		out<<" --- Selected";
	out << L"<br>\n";
	out << _tagSet->getTagSymbol(third_best).to_string() << L": " << _tagScores[third_best] << L"<br>\n";
	out << "<br>\n";
}

void EventTriggerFinder::printMaxentDebugScores(UTF8OutputStream& out) {
	int best = 0;
	int second_best = 0;
	int third_best = 0;
	double best_score = -100000;
	double second_best_score = -100000;
	double third_best_score = -100000;

	_maxentModel->decodeToDistribution(_observation, _tagScores, _tagSet->getNTags());

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

void EventTriggerFinder::decodeToP1Distribution() {
	for (int i = 0; i < _tagSet->getNTags(); i++) {
		_tagScores[i] = _p1Model->getScore(_observation, i);
	}
}

int EventTriggerFinder::decodeToMaxentDistribution() {
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

void EventTriggerFinder::resetRoundRobinStatistics() {
	for(int i = 0; i < 4; i++){
		for(int j = 0; j < 5; j++){
			_scoringCounts[i][j] = 0;
		}
	}

	_correct = 0;
	_wrong_type = 0;
	_missed = 0;
	_spurious = 0;
}

void EventTriggerFinder::printRoundRobinStatistics(UTF8OutputStream &out) {
	for(int j = 0; j < 5; j++){
		double recall = (double) _scoringCounts[CORRECT][j] / (_scoringCounts[CORRECT][j] + _scoringCounts[MISSING][j] +_scoringCounts[WRONG_TYPE][j]);
		double precision = (double) _scoringCounts[CORRECT][j] / (_scoringCounts[CORRECT][j] + _scoringCounts[SPURIOUS][j] +_scoringCounts[WRONG_TYPE][j]);
		if(j == COUNT_ALL) out <<"** REGULAR: \n";
		else if(j == COUNT_BY_ARG) out <<"** ARG_WEIGHTED: \n";
		else if(j == COUNT_PM) out <<"** PROB_MOD_ACTIVE: \n";
		else if(j == COUNT_KNOWN) out <<"** KNOWN: \n";
		else if(j == COUNT_KNOWN_CORRECT) out <<"** KNOWN_CORRECT: \n";
		out <<"    CORRECT: "<<_scoringCounts[CORRECT][j]<<"\n";
		out <<"    MISSED: "<<_scoringCounts[MISSING][j]<<"\n";
		out <<"    SPURIOUS: "<<_scoringCounts[SPURIOUS][j]<<"\n";
		out <<"    WRONG_TYPE: "<<_scoringCounts[WRONG_TYPE][j]<<"\n";
		out <<"    *RECALL: " << recall << L"\n";
		out <<"    *PRECISION: " << precision << L"\n\n";	

	}
}

void EventTriggerFinder::printRecPrecF1(UTF8OutputStream& out) {
	int R = _missed + _wrong_type + _correct;
	int P = _spurious + _wrong_type + _correct;
	double recall = (double) _correct / R;
	double precision = (double) _correct / P;
	double f1 = (2 * precision * recall) / (precision + recall);
	out << L"trigger: R=" << _correct << L"/" << R << L"=" << recall << L" P=" << _correct << L"/" << P << L"=" << precision << L" F1=" << f1 << L"\n";
}

void EventTriggerFinder::train(TokenSequence **tokenSequences, Parse **parses, 
								ValueMentionSet **valueMentionSets, MentionSet **mentionSets, 
								PropositionSet **propSets, EventMentionSet **esets, 
								Symbol *docTopics, int nsentences) 
{
	if (MODE != TRAIN)
		return;

	// MODEL FILE
	std::string model_file = ParamReader::getRequiredParam("event_trigger_model_file");

	int n_epochs = 1;
	bool print_every_epoch = false;
	if (_use_p1_model) {
		n_epochs = ParamReader::getRequiredIntParam("et_p1_epochs");
		if (ParamReader::isParamTrue("em_p1_print_every_epoch"))
			print_every_epoch = true;		

		if (_seed_features) {
			SessionLogger::info("SERIF") << "Seeding weight table with all features from training set...\n";
			for (int i = 0; i < nsentences; i++) {
				_documentTopic = docTopics[i];
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
		int count=0;
		for (int i = 0; i < nsentences; i++) {
			_documentTopic = docTopics[i];
			count=count+tokenSequences[i]->getNTokens();
			msg << tokenSequences[i]->getNTokens() <<"\n";
				msg << count << "\n";
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
		if (epoch == 0) {
			_probModelSet->deriveModels();
			_probModelSet->printModels(model_file.c_str());
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
		int pruning = ParamReader::getRequiredIntParam("et_maxent_trainer_pruning_cutoff");
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

void EventTriggerFinder::walkThroughTrainingSentence(int mode, int epoch, 
													 const TokenSequence *tseq, 
													 const Parse *parse, 
													 const ValueMentionSet *valueMentionSet,
													 MentionSet *mentionSet, 
													 const PropositionSet *propSet, 
													 const EventMentionSet *eset) 
{
	int eventTypes[MAX_SENTENCE_TOKENS];

	for (int j = 0; j < tseq->getNTokens(); j++) {
		eventTypes[j] = 0;
	}

	for (int ement = 0; ement < eset->getNEventMentions(); ement++) {
		EventMention *em = eset->getEventMention(ement);
		if (_tagSet->getTagIndex(em->getEventType()) == -1) {
			char c[500];
			sprintf( c, "ERROR: Invalid event type in training file: %s", em->getEventType().to_debug_string());
			throw UnexpectedInputException("EventTriggerFinder::walkThroughTrainingSentence()", c);
		}			
		eventTypes[em->getAnchorNode()->getStartToken()] = _tagSet->getTagIndex(em->getEventType());	
	}

	for (int k = 0; k < tseq->getNTokens(); k++) {
		_observation->populate(k, tseq, parse, mentionSet, 
			propSet, true);
		_observation->setDocumentTopic(_documentTopic);
		if (_use_p1_model) {
			if (mode == TRAIN) {
				_p1Model->train(_observation, eventTypes[k]);
			} else if (mode == ADD_FEATURES) {
				_p1Model->addFeatures(_observation, eventTypes[k]);
		
			} 
		}
		if (epoch == 0) {
			if (_use_maxent_model)
				_maxentModel->addToTraining(_observation, eventTypes[k]);
			_probModelSet->addTrainingEvent(_observation, 
				_tagSet->getTagSymbol(eventTypes[k]));					
		}
	}
}

void EventTriggerFinder::dumpP1TrainingParameters(UTF8OutputStream &out, int epoch) {

	DTFeature::recordDate(out);

	out << L"Parameters:\n";
	DTFeature::recordParamForConsistency(Symbol(L"event_trigger_tag_set_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"event_trigger_features_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"word_cluster_bits_file"), out);

	DTFeature::recordParamForReference(Symbol(L"event_training_file_list"), out);
	DTFeature::recordParamForReference(Symbol(L"et_p1_epochs"), out);
	out << "actual_epoch_output_here " << epoch << "\n";
	out << L"\n";

}

void EventTriggerFinder::dumpMaxentTrainingParameters(UTF8OutputStream &out) {

	DTFeature::recordDate(out);

	out << L"Parameters:\n";
	DTFeature::recordParamForConsistency(Symbol(L"event_trigger_tag_set_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"event_trigger_features_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"word_cluster_bits_file"), out);

	DTFeature::recordParamForReference(Symbol(L"event_training_file_list"), out);
	DTFeature::recordParamForReference(Symbol(L"et_maxent_trainer_percent_held_out"), out);
	DTFeature::recordParamForReference(Symbol(L"et_maxent_trainer_max_iterations"), out);
	DTFeature::recordParamForReference(Symbol(L"et_maxent_trainer_gaussian_variance"), out);
	DTFeature::recordParamForReference(Symbol(L"et_maxent_trainer_min_likelihood_delta"), out);
	DTFeature::recordParamForReference(Symbol(L"et_maxent_trainer_stop_check_frequency"), out);
	DTFeature::recordParamForReference(Symbol(L"et_maxent_trainer_pruning_cutoff"), out);
	out << L"\n";

}


int EventTriggerFinder::processSentence(const TokenSequence *tokens, 
										const Parse *parse, 
										MentionSet *mentionSet, 
										const PropositionSet *propSet, 
										EventMention **potentials,
										int max)
{
	int n_potential_events = 0;
	if (DEBUG) {
		_debugStream << parse->getRoot()->toTextString() << "\n";
	}
	for (int tok = 0; tok < tokens->getNTokens(); tok++) {
		if (n_potential_events >= max)
			break;
		processToken(tok, tokens, parse, mentionSet, propSet, 
								potentials, n_potential_events, max);
	}

	return n_potential_events;

}

void EventTriggerFinder::processToken(int tok, const TokenSequence *tokens, 
									 const Parse *parse, MentionSet *mentionSet, 
									 const PropositionSet *propSet,
									 EventMention **potentials, 
									 int& n_potential_events,
									 int max_events) 
{
	
	_observation->populate(tok, tokens, parse, mentionSet, propSet, true);
	_observation->setDocumentTopic(_documentTopic);

	for (int tag = 1; tag < _tagSet->getNTags(); tag++) {
		_tagFound[tag] = false;
	}
	int p1tag = 0;
	bool use_sub_for_this_instance = false;
	bool use_obj_for_this_instance = false;
	bool use_only_confident_p1 = false;
	selectModels(use_sub_for_this_instance, use_obj_for_this_instance, use_only_confident_p1);

	std::set<int> knownP1Tags;
	if(_use_p1_model)
		knownP1Tags = _p1Model->featureInTable(_observation, Symbol(L"lc-word"));
	for (int modelnum = 0; modelnum < _probModelSet->getNModels() + 2; modelnum++) {
		if (n_potential_events >= max_events)
			break;
		
		// do actual decoding
		if (modelnum < _probModelSet->getNModels() && _probModelSet->useModel(modelnum)) {
			if (modelnum == ETProbModelSet::SUB && !use_sub_for_this_instance)
				continue;
			if (modelnum == ETProbModelSet::OBJ && !use_obj_for_this_instance)
				continue;
			_probModelSet->decodeToDistribution(_observation, modelnum);				
		} else if (modelnum == _probModelSet->getNModels() && _use_maxent_model) {
			decodeToMaxentDistribution();
		} else if (modelnum == _probModelSet->getNModels() + 1 && _use_p1_model) {
			decodeToP1Distribution();
			p1tag = _p1Model->decodeToInt(_observation, _tagScores);
		} else continue;

		bool tok_printed = false;

		for (int tag = 1; tag < _tagSet->getNTags(); tag++) {
			if (n_potential_events >= max_events)
				break;
			
			if (!(modelnum < _probModelSet->getNModels() && _tagScores[tag] > log(_recall_threshold)) &&   //prob model condition
				!(modelnum == _probModelSet->getNModels() && _tagScores[tag] > _recall_threshold) &&	//max_ent condition
				!(modelnum == _probModelSet->getNModels() + 1 && tag == p1tag))	//P1 condition
				continue;

			if (modelnum == _probModelSet->getNModels() + 1 && use_only_confident_p1) {
				if(!isConfidentP1Tag(tag))
					continue;
			}

			if (DEBUG) {
				if (!tok_printed) {
					_debugStream << "Model " << modelnum << " -- ";
					_debugStream << tokens->getToken(tok)->getSymbol() << ":\n";
					_debugStream << "NONE: " << _tagScores[0] << "\n";
					tok_printed = true;
				}
				_debugStream << _tagSet->getTagSymbol(tag) << ": " << _tagScores[tag] << "\n";
			}
			if (_tagFound[tag])
				continue;
			_tagFound[tag] = true;

			potentials[n_potential_events] = _new EventMention(tokens->getSentenceNumber(), n_potential_events);
			potentials[n_potential_events]->setEventType(_tagSet->getTagSymbol(tag));

			const SynNode *tNode = parse->getRoot()->getNthTerminal(tok);
			// we want the preterminal node, since that's what we would get if we were
			// going from a proposition to a node
			if (tNode->getParent() != 0)
				tNode = tNode->getParent();
			potentials[n_potential_events]->setAnchor(tNode, propSet);
			
			// set score
			setScore(potentials[n_potential_events], modelnum);

			n_potential_events++;
		}
	}
}

float EventTriggerFinder::setScore(EventMention *eventMention, int modelnum) {
	
	eventMention->setScore(0);
	//eventMention->_simple_score = 0;

	for (int model = 0; model < _probModelSet->getNModels(); model++) {
		if (_probModelSet->useModel(model)) {
			// due to new way we're combining models, comment this out..
			//float prob = (float) _probModelSet->getProbability(_observation,
			//		eventMention->getEventType(), modelnum);
			//eventMention->addToScore(prob);
			//if (modelnum == ETProbModelSet::WORD)
			//	eventMention->_simple_score = prob;
			//if (modelnum == ETProbModelSet::WORD_PRIOR)
			//	eventMention->_score_with_prior = prob;
		}
	}

	if (eventMention->getScore() == 0 && _use_maxent_model) {
		decodeToMaxentDistribution();
		int index = _tagSet->getTagIndex(eventMention->getEventType());
		eventMention->addToScore((float) _tagScores[index]);
	}

	if (eventMention->getScore() == 0 && _use_p1_model) {
		float none_prob = (float) _p1Model->getScore(_observation, _tagSet->getNoneTagIndex());
		if (none_prob < 0)
			none_prob = 0;
		float prob = (float) _p1Model->getScore(_observation,
			_tagSet->getTagIndex(eventMention->getEventType()));
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

	/*
	if (modelnum < _probModelSet->getNModels() && _probModelSet->useModel(modelnum)) {
		// due to new way we're combining models, comment this out..
		float prob = (float) _probModelSet->getProbability(_observation,
				eventMention->getEventType(), modelnum);
		eventMention->addToScore(prob);
		//if (modelnum == ETProbModelSet::WORD)
		//	eventMention->_simple_score = prob;
		//if (modelnum == ETProbModelSet::WORD_PRIOR)
		//	eventMention->_score_with_prior = prob;
	}
	

	if (modelnum == _probModelSet->getNModels() && _use_maxent_model) {
		decodeToMaxentDistribution();
		int index = _tagSet->getTagIndex(eventMention->getEventType());
		eventMention->addToScore((float) _tagScores[index]);
	}

	if (modelnum == _probModelSet->getNModels() + 1 && _use_p1_model) {
		float none_prob = (float) _p1Model->getScore(_observation, _tagSet->getNoneTagIndex());
		if (none_prob < 0)
			none_prob = 0;
		float prob = (float) _p1Model->getScore(_observation,
			_tagSet->getTagIndex(eventMention->getEventType()));
		if (prob < 0)
			prob = 1;
		float score = 0;
		if (prob + none_prob != 0) 
			score = prob / (none_prob + prob);
		float log_score = 0;
		if (score != 0)
			log_score = log(score);
		eventMention->addToScore(log_score);
	}*/

	return eventMention->getScore();
}

bool EventTriggerFinder::isLikelyTrigger(const SynNode* aNode){
	return true;


	const SynNode* anchorHead = aNode->getHeadPreterm();
	if(LanguageSpecificFunctions::isNounPOS(anchorHead->getTag())){
		if(NodeInfo::isNominalPremod(aNode)){ 
			return false;	
		}
		else{
			return true;
		}
	}
	else if(LanguageSpecificFunctions::isVerbPOS(anchorHead->getTag())){
		return true;
	}
	else{
		return false;
	}
}
bool EventTriggerFinder::isConfidentP1Tag(int tag){
	double none_score = _tagScores[_tagSet->getNoneTagIndex()];
	double tag_score = _tagScores[tag];
	if (tag_score < none_score)
		return false;
	if ((tag_score - none_score) / tag_score < .2)
		return false;
	return true;
}
void EventTriggerFinder::selectModels(bool& use_sub, bool& use_obj, bool& use_only_confident_P1){
	if(_probModelSet->preferModel(_observation, ETProbModelSet::SUB)){
		use_sub = true;
		use_only_confident_P1 = true;
	}
	if(_probModelSet->preferModel(_observation, ETProbModelSet::OBJ)){
		use_obj = true;
		use_only_confident_P1 = true;
	}
}


EventMentionSet* EventTriggerFinder::devTestDecode(const TokenSequence *tseq, Parse *parse, ValueMentionSet *valueMentionSet, MentionSet *mentionSet, 
						PropositionSet *propSet, EventMentionSet *eset, int nsentences)
{

	EventMentionSet *result = _new EventMentionSet(parse);

	if (MODE != DECODE)
		return 0;

	int eventTypes[MAX_SENTENCE_TOKENS];

	for (int j = 0; j < tseq->getNTokens(); j++) {
		eventTypes[j] = 0;
	}
	for (int ement = 0; ement < eset->getNEventMentions(); ement++) {
		EventMention *em = eset->getEventMention(ement);
		if (_tagSet->getTagIndex(em->getEventType()) == -1) {
			throw UnexpectedInputException("EventTriggerFinder::devTestDecode()", "Invalid event type in training file");
		}			
		eventTypes[em->getAnchorNode()->getStartToken()] = _tagSet->getTagIndex(em->getEventType());
	}

	for (int k = 0; k < tseq->getNTokens(); k++) {
		int correct_answer = eventTypes[k];
		EventMention *tmpEvents[MAX_TOKEN_TRIGGERS];

		int n_system_answers = 0;
		processToken(k, tseq, parse, mentionSet, propSet, tmpEvents, n_system_answers, MAX_TOKEN_TRIGGERS);

		// Return answer with highest score
		int system_answer = _tagSet->getNoneTagIndex();
		float this_score, system_score = 0;
		int systemEventIndex = -1;		// points to the highest scoring event mention
		for (int m = 0; m < n_system_answers; m++) {
			this_score = exp(tmpEvents[m]->getScore());
			if (this_score > system_score) {
				system_score = this_score;
				system_answer = _tagSet->getTagIndex(tmpEvents[m]->getEventType());
				systemEventIndex = m;
			}
		}

		if( (systemEventIndex != -1) && (system_answer != _tagSet->getNoneTagIndex()) ) {
			result->takeEventMention(tmpEvents[systemEventIndex]);	// use this predicted event mention to do event-aa prediction
			for (int n=0; n<n_system_answers; n++) {
				if(n != systemEventIndex)
					delete tmpEvents[n];
			}
		}
		else {
			for (int n=0; n<n_system_answers; n++) 
				delete tmpEvents[n];
		}	
		
		if (correct_answer == system_answer) {
			if (correct_answer != _tagSet->getNoneTagIndex()) {
				_correct++;
			}
		} else if (correct_answer == _tagSet->getNoneTagIndex()) {
			_spurious++;
		} else if (system_answer == _tagSet->getNoneTagIndex()) {
			_missed++;
		} else {
			_wrong_type++;
		}
	}

	return result;
}

