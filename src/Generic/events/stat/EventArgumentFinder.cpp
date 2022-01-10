// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/stat/EventArgumentFinder.h"
#include "Generic/events/stat/EventAAFeatureTypes.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/Sexp.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/Token.h"
#include "Generic/common/ParamReader.h"
#include "Generic/maxent/MaxEntModel.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/P1Decoder.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/common/version.h"

//#ifdef ENGLISH_LANGUAGE
//#define USE_FAKED_MENTION_TYPES true
//#else
//#define USE_FAKED_MENTION_TYPES false
//#endif

#include "Generic/distributionalKnowledge/DistributionalKnowledgeTable.h"	// distributional knowledge

EventArgumentFinder::EventArgumentFinder(int mode_) : MODE(mode_)
{

	EventAAFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();

	// distributional knowledge
	bool useDistributionalKnowledge = ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_distributional_knowledge", false);
	if(useDistributionalKnowledge) {
		DistributionalKnowledgeTable::ensureInitialized();
	}


	// DEBUGGING
	std::string buffer = ParamReader::getParam("event_aa_debug_file");
	DEBUG = (!buffer.empty());
	if (DEBUG) _debugStream.open(buffer.c_str());

	// TAG SET
	std::string tag_set_file = ParamReader::getRequiredParam("event_aa_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);
	_tagScores = _new double[_tagSet->getNTags()];

	_gpeFacLocChoices[0] = _tagSet->getTagIndex(Symbol(L"Place"));
	_gpeFacLocChoices[1] = _tagSet->getTagIndex(Symbol(L"Destination"));
	_gpeFacLocChoices[2] = _tagSet->getTagIndex(Symbol(L"Origin"));

	_perOrgChoices[0] = _tagSet->getTagIndex(Symbol(L"Agent"));
	_perOrgChoices[1] = _tagSet->getTagIndex(Symbol(L"Defendant"));
	_perOrgChoices[2] = _tagSet->getTagIndex(Symbol(L"Plaintiff"));
	_perOrgChoices[3] = _tagSet->getTagIndex(Symbol(L"Prosecutor"));
	_perOrgChoices[4] = _tagSet->getTagIndex(Symbol(L"Entity"));
	_perOrgChoices[5] = _tagSet->getTagIndex(Symbol(L"Attacker"));
	_perOrgChoices[6] = _tagSet->getTagIndex(Symbol(L"Giver"));
	_perOrgChoices[7] = _tagSet->getTagIndex(Symbol(L"Recipient"));
	_perOrgChoices[8] = _tagSet->getTagIndex(Symbol(L"Beneficiary"));
	_perOrgChoices[9] = _tagSet->getTagIndex(Symbol(L"Buyer"));
	_perOrgChoices[10] = _tagSet->getTagIndex(Symbol(L"Seller"));

	// FEATURES
	std::string features_file = ParamReader::getRequiredParam("event_aa_features_file");
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), EventAAFeatureType::modeltype);

	_attach_event_only_values = ParamReader::isParamTrue("use_preexisting_event_only_values");
	
	_model = 0;
	_weights = 0;
	_p1Model = 0;
	_p1Weights = 0;

	// split classifiers
	_directModel = 0;
	_sharedModel = 0;
	_unconnectedModel = 0;
	_directWeights = 0;
	_sharedWeights = 0;
	_unconnectedWeights = 0;
	_directFeatureTypes = 0;
	_sharedFeatureTypes = 0;
	_unconnectedFeatureTypes = 0;

	_use_split_classifiers = ParamReader::isParamTrue("event_aa_use_split_classifiers");
	
	if(_use_split_classifiers) {
		std::string directFeaturesFile = ParamReader::getParam("event_aa_direct_features_file");
		_directFeatureTypes = _new DTFeatureTypeSet(directFeaturesFile.c_str(), EventAAFeatureType::modeltype);
		std::string sharedFeaturesFile = ParamReader::getParam("event_aa_shared_features_file");
		_sharedFeatureTypes = _new DTFeatureTypeSet(sharedFeaturesFile.c_str(), EventAAFeatureType::modeltype);
		std::string unconnectedFeaturesFile = ParamReader::getParam("event_aa_unconnected_features_file");
		_unconnectedFeatureTypes = _new DTFeatureTypeSet(unconnectedFeaturesFile.c_str(), EventAAFeatureType::modeltype);
	}

	if (MODE == DECODE) {

		// RECALL THRESHOLD
		_recall_threshold = ParamReader::getRequiredFloatParam("event_aa_recall_threshold");

		// SECOND ARG RECALL THRESHOLD
		_second_arg_recall_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("event_aa_second_arg_recall_threshold", 1.0);

		// split classifiers
		if(_use_split_classifiers) {
			std::string directModelFile = ParamReader::getParam("event_aa_direct_model_file");
			if(!directModelFile.empty()) {
				_directWeights = _new DTFeature::FeatureWeightMap(50000);
				DTFeature::readWeights(*_directWeights, directModelFile.c_str(), EventAAFeatureType::modeltype);
				_directModel = _new MaxEntModel(_tagSet, _directFeatureTypes, _directWeights);
			}
			std::string sharedModelFile = ParamReader::getParam("event_aa_shared_model_file");
			if(!sharedModelFile.empty()) {
				_sharedWeights = _new DTFeature::FeatureWeightMap(50000);
				DTFeature::readWeights(*_sharedWeights, sharedModelFile.c_str(), EventAAFeatureType::modeltype);
				_sharedModel = _new MaxEntModel(_tagSet, _sharedFeatureTypes, _sharedWeights);
			}
			std::string unconnectedModelFile = ParamReader::getParam("event_aa_unconnected_model_file");
			if(!unconnectedModelFile.empty()) {
				_unconnectedWeights = _new DTFeature::FeatureWeightMap(50000);
				DTFeature::readWeights(*_unconnectedWeights, unconnectedModelFile.c_str(), EventAAFeatureType::modeltype);
				_unconnectedModel = _new MaxEntModel(_tagSet, _unconnectedFeatureTypes, _unconnectedWeights);
			}
		}

		// MODEL FILE
		std::string model_file = ParamReader::getParam("event_aa_model_file");
		if (!model_file.empty()) 
		{
			_weights = _new DTFeature::FeatureWeightMap(50000);
			DTFeature::readWeights(*_weights, model_file.c_str(), EventAAFeatureType::modeltype);
			_model = _new MaxEntModel(_tagSet, _featureTypes, _weights);

			model_file = ParamReader::getParam("event_aa_p1_model_file");
			if (!model_file.empty()) {
				_p1Weights = _new DTFeature::FeatureWeightMap(50000);
				DTFeature::readWeights(*_p1Weights, model_file.c_str(), EventAAFeatureType::modeltype);
				_p1Model = _new P1Decoder(_tagSet, _featureTypes, _p1Weights);
				_p1Model->setUndergenPercentage(.2);
			} 
			
		} else if (!ParamReader::hasParam("event_round_robin_setup")) {

			throw UnexpectedInputException("EventArgumentFinder::EventArgumentFinder()",
				"Neither parameter 'event_aa_model' nor 'event_round_robin_setup' specified");

		} else {
			_weights = 0;
			_model = 0;
		}

	} else if (MODE == TRAIN_BOTH || MODE == TRAIN_MAXENT || 
		MODE == TRAIN_P1 ) 
	{
		
		const std::string train_vector_file = ParamReader::getParam("training_vectors_file");
		if (train_vector_file != "") {
			SessionLogger::info("SERIF") << "Dumping feature vectors to " << train_vector_file;
		}


		if (MODE == TRAIN_P1 || MODE == TRAIN_BOTH) {
			_p1Weights = _new DTFeature::FeatureWeightMap(50000);
			_p1Model = _new P1Decoder(_tagSet, _featureTypes, _p1Weights, 0, true);
			// if we are training both models, we don't want them logging 
			// feature vectors to the same log file, so we let the Maxent model
			// do it ~ rgabbard
			if (MODE != TRAIN_BOTH) {
				_p1Model->setLogFile(train_vector_file);
			}
		} 

		if (MODE == TRAIN_BOTH || MODE == TRAIN_MAXENT) { 
			// PERCENT HELD OUT
			int percent_held_out = 
				ParamReader::getRequiredIntParam("aa_maxent_trainer_percent_held_out");
			if (percent_held_out < 0 || percent_held_out > 50) 
				throw UnexpectedInputException("EventArgumentFinder::EventArgumentFinder()",
				"Parameter 'aa_maxent_trainer_percent_held_out' must be between 0 and 50");

			// MAX NUMBER OF ITERATIONS (STOPPING CONDITION)
			int max_iterations = 
				ParamReader::getRequiredIntParam("aa_maxent_trainer_max_iterations");

			// GAUSSIAN PRIOR VARIANCE
			double variance = ParamReader::getRequiredFloatParam("aa_maxent_trainer_gaussian_variance");

			// MIN CHANGE IN LIKELIHOOD (STOPPING CONDITION)
			double likelihood_delta = ParamReader::getOptionalFloatParamWithDefaultValue("aa_maxent_trainer_min_likelihood_delta", .0001);

			// FREQUENCY OF STOPPING CONDITION CHECKS (NUM ITERATIONS)
			int stop_check_freq = ParamReader::getOptionalIntParamWithDefaultValue("aa_maxent_trainer_stop_check_frequency",1);

			if (MODE == TRAIN_MAXENT || MODE == TRAIN_BOTH) {
				_weights = _new DTFeature::FeatureWeightMap(50000);
				_model = _new MaxEntModel(_tagSet, _featureTypes, _weights, 
					MaxEntModel::SCGIS, percent_held_out, max_iterations, variance,
					likelihood_delta, stop_check_freq, train_vector_file.c_str(), "");

				// split classifiers
				if(_use_split_classifiers) {
					_directWeights = _new DTFeature::FeatureWeightMap(50000);
					_directModel = _new MaxEntModel(_tagSet, _directFeatureTypes, _directWeights, 
						MaxEntModel::SCGIS, percent_held_out, max_iterations, variance, 
						likelihood_delta, stop_check_freq, train_vector_file.c_str(), "");
					_sharedWeights = _new DTFeature::FeatureWeightMap(50000);
					_sharedModel = _new MaxEntModel(_tagSet, _sharedFeatureTypes, _sharedWeights, 
						MaxEntModel::SCGIS, percent_held_out, max_iterations, variance, 
						likelihood_delta, stop_check_freq, train_vector_file.c_str(), "");
					_unconnectedWeights = _new DTFeature::FeatureWeightMap(50000);
					_unconnectedModel = _new MaxEntModel(_tagSet, _unconnectedFeatureTypes, _unconnectedWeights, 
						MaxEntModel::SCGIS, percent_held_out, max_iterations, variance, 
						likelihood_delta, stop_check_freq, train_vector_file.c_str(), "");
				}
	
			} 
		}       
	}
}


EventArgumentFinder::~EventArgumentFinder() {
	if (_model) {
		delete _model;
	}

	// split classifiers
	if(_directModel) {
		delete _directModel;
	}
	if(_sharedModel) {
		delete _sharedModel;
	}
	if(_unconnectedModel) {
		delete _unconnectedModel;
	}
}

void EventArgumentFinder::replaceModel(char *model_file) {
	delete _model;
	delete _weights;
	_weights = _new DTFeature::FeatureWeightMap(50000);
	DTFeature::readWeights(*_weights, model_file, EventAAFeatureType::modeltype);
	_model = _new MaxEntModel(_tagSet, _featureTypes, _weights);
}

// this is for stand-alone use only -- essentially for development use
void EventArgumentFinder::decode(TokenSequence **tokenSequences, Parse **parses, 
		ValueMentionSet **valueMentionSets, MentionSet **mentionSets, PropositionSet **propSets, 
		EventMentionSet **esets, int nsentences, UTF8OutputStream& resultStream)
{
	if (MODE != DECODE || _model == 0)
		return;

	for (int i = 0; i < nsentences; i++) {
		for (int ement = 0; ement < esets[i]->getNEventMentions(); ement++) {
			EventMention *key_em = esets[i]->getEventMention(ement);
			EventMention *test_em = _new EventMention(*key_em);	
			test_em->resetArguments();
			attachArguments(test_em, tokenSequences[i], valueMentionSets[i], 
				parses[i], mentionSets[i], propSets[i]);

			// print event
			resultStream << L"<h3>";
			resultStream << key_em->getAnchorNode()->getHeadWord().to_string() << L" (";
			resultStream << key_em->getEventType().to_string() << L")</h3>\n";

			for (int tok = 0; tok < tokenSequences[i]->getNTokens(); tok++) {
				if (tok == key_em->getAnchorNode()->getStartToken())
					resultStream << L"<u><b>";
				resultStream << tokenSequences[i]->getToken(tok)->getSymbol().to_string();
				if (tok == key_em->getAnchorNode()->getEndToken())
					resultStream << L"</u></b>";
				resultStream << L" ";
			}
			resultStream << L"<br><br>\n";
			
			int key_arg, test_arg;
			for (key_arg = 0; key_arg < key_em->getNArgs(); key_arg++) {
				const Mention *key_ment = key_em->getNthArgMention(key_arg);
				bool matched = false;
				EventAAObservation_ptr keyMentionArgObservation = boost::make_shared<EventAAObservation>();
				keyMentionArgObservation->initializeEventAAObservation(tokenSequences[i], valueMentionSets[i], 
											parses[i], mentionSets[i], propSets[i], key_em, key_ment);

				for (test_arg = 0; test_arg < test_em->getNArgs(); test_arg++) {
					if (test_em->getNthArgRole(test_arg).is_null())
						continue;
					if (test_em->getNthArgMention(test_arg) == key_ment) {
						matched = true;
						if (key_em->getNthArgRole(key_arg) == test_em->getNthArgRole(test_arg)) {
							printDebugResult(keyMentionArgObservation.get(), CORRECT, resultStream, key_em->getNthArgRole(key_arg));
							_correct_args++;
						} else {
							printDebugResult(keyMentionArgObservation.get(), CORRECT, resultStream, key_em->getNthArgRole(key_arg),  test_em->getNthArgRole(test_arg));
							_wrong_type++;
						}
						test_em->changeNthRole(test_arg, Symbol());   //Change this to NULL so that it does not get printed as SPURIUOS below
						break;
					}
				}
				if (!matched) {
					printDebugResult(keyMentionArgObservation.get(), MISSING, resultStream, key_em->getNthArgRole(key_arg));
					_missed++;							
				}
			}
			for (test_arg = 0; test_arg < test_em->getNArgs(); test_arg++) {
				if (test_em->getNthArgRole(test_arg).is_null())  //Args that were matched above, were reset to have role = Symbol()
					continue;
				const Mention *test_ment = test_em->getNthArgMention(test_arg);
				EventAAObservation_ptr testMentionArgObservation = boost::make_shared<EventAAObservation>();
				testMentionArgObservation->initializeEventAAObservation(tokenSequences[i], valueMentionSets[i], 
											parses[i], mentionSets[i], propSets[i], test_em, test_ment);
				printDebugResult(testMentionArgObservation.get(), SPURIOUS, resultStream, test_em->getNthArgRole(test_arg));
				_spurious++;
			}
			resultStream << L"<br>";

			// NOW DO THE SAME FOR VALUE ARGS
			for (key_arg = 0; key_arg < key_em->getNValueArgs(); key_arg++) {
				const ValueMention *key_ment = key_em->getNthArgValueMention(key_arg);
				bool matched = false;
				EventAAObservation_ptr keyMentionArgObservation = boost::make_shared<EventAAObservation>();
				keyMentionArgObservation->initializeEventAAObservation(tokenSequences[i], valueMentionSets[i], 
											parses[i], mentionSets[i], propSets[i], key_em, key_ment);
				for (test_arg = 0; test_arg < test_em->getNValueArgs(); test_arg++) {
					if (test_em->getNthArgValueRole(test_arg).is_null())
						continue;
					if (test_em->getNthArgValueMention(test_arg) == key_ment) {
						matched = true;
						if (key_em->getNthArgValueRole(key_arg) == test_em->getNthArgValueRole(test_arg)) {
							printDebugResult(keyMentionArgObservation.get(), CORRECT, resultStream, key_em->getNthArgValueRole(key_arg));
							_correct_args++;
						} else {
							printDebugResult(keyMentionArgObservation.get(), WRONG_TYPE, resultStream, key_em->getNthArgValueRole(key_arg), test_em->getNthArgValueRole(test_arg));
							_wrong_type++;
						}
						test_em->changeNthValueRole(test_arg, Symbol());
						break;
					}
				}
				if (!matched) {
					printDebugResult(keyMentionArgObservation.get(), MISSING, resultStream, key_em->getNthArgValueRole(key_arg));
					_missed++;							
				}
			}
			for (test_arg = 0; test_arg < test_em->getNValueArgs(); test_arg++) {
				if (test_em->getNthArgValueRole(test_arg).is_null())
					continue;

				const ValueMention *test_ment = test_em->getNthArgValueMention(test_arg);
				EventAAObservation_ptr testMentionArgObservation = boost::make_shared<EventAAObservation>();
				testMentionArgObservation->initializeEventAAObservation(tokenSequences[i], valueMentionSets[i], 
											parses[i], mentionSets[i], propSets[i], test_em, test_ment);

				printDebugResult(testMentionArgObservation.get(), SPURIOUS, resultStream, test_em->getNthArgValueRole(test_arg));
				_spurious++;
			}
			resultStream << L"<br>";
		}
	}

}

void EventArgumentFinder::printDebugResult(EventAAObservation* observation, int result, UTF8OutputStream& stream, Symbol role, Symbol other_role)
{
		std::string color = "black";
		std::string label = "UNKNOWN";
		if(result == CORRECT){
			color = "red";
			label = "CORRECT";
		}
		else if(result == MISSING){
			color = "purple";
			label = "MISSING";
		}
		else if(result == SPURIOUS){
			color = "blue";
			label = "SPURIOUS";
		}
		if(result == WRONG_TYPE){
			color = "green";
			label = "WRONG TYPE";
		}
		std::wstring argumentString = L"-UNKNOWN-";
		if(observation->isValue()){
			argumentString = observation->getCandidateValueMention()->toString(observation->getTokenSequence());
		}
		else{
			argumentString = observation->getCandidateArgumentMention()->getNode()->toTextString();
		}
		
		stream << L"<font color=\""<<color<<"\">"<<label<<"</font>: ";
		stream << argumentString << L" (<b>";
		stream << role ;
		if(other_role != L"") stream <<" System Role: "<<other_role;
		stream<< L"</b>)<br>\n";
		printDebugScores(observation, stream);
		stream << "<br>\n";
}
void EventArgumentFinder::printDebugScores(EventAAObservation* observation,   UTF8OutputStream& stream) 
{
	int best_tag;
	MaxEntModel* selectedModel = getMaxEntModelToUse(observation);
	selectedModel->decodeToDistribution(observation, _tagScores, _tagSet->getNTags(), &best_tag);
	
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
	_model->printHTMLDebugInfo(observation, best, stream);
	stream << "</font>\n";
	stream << _tagSet->getTagSymbol(second_best).to_string() << L": " << _tagScores[second_best] << L"<br>\n";
	stream << "<font size=1>\n";
	_model->printHTMLDebugInfo(observation, second_best, stream);
	stream << "</font>\n";
	stream << _tagSet->getTagSymbol(third_best).to_string() << L": " << _tagScores[third_best] << L"<br>\n";
	stream << "<font size=1>\n";
	_model->printHTMLDebugInfo(observation, third_best, stream);
	stream << "</font>\n";
}

void EventArgumentFinder::train(TokenSequence **tokenSequences, Parse **parses, 
		ValueMentionSet **valueMentionSets, MentionSet **mentionSets, PropositionSet **propSets, 
		EventMentionSet **esets, int nsentences) 
{
	if (MODE == DECODE)
		return;

	int pruning = 0;
	if (MODE == TRAIN_MAXENT || MODE == TRAIN_BOTH) {
		pruning = ParamReader::getRequiredIntParam("aa_maxent_trainer_pruning_cutoff");
	}
	int n_epochs = 1;
	if (MODE == TRAIN_P1 || MODE == TRAIN_BOTH) {		
		n_epochs = ParamReader::getRequiredIntParam("aa_p1_trainer_epochs");
	}

			
	
	// MODEL FILE
	std::string model_file = ParamReader::getRequiredParam("event_aa_model_file");

	// split classifiers
	std::string directModelFile, sharedModelFile, unconnectedModelFile;
	if(_use_split_classifiers) {
		directModelFile = ParamReader::getParam("event_aa_direct_model_file");
		sharedModelFile = ParamReader::getParam("event_aa_shared_model_file");
		unconnectedModelFile = ParamReader::getParam("event_aa_unconnected_model_file");
	}

	for (int epoch = 0; epoch < n_epochs; epoch++) {
		SessionLogger::LogMessageMaker msg = SessionLogger::info("SERIF");
        msg << "Epoch "<< epoch << "\n";
		for (int i = 0; i < nsentences; i++) {
			for (int ement = 0; ement < esets[i]->getNEventMentions(); ement++) {
				EventMention *em = esets[i]->getEventMention(ement);

				for (int val_id = 0; val_id < valueMentionSets[i]->getNValueMentions(); val_id++) {
					ValueMention *valueMention = valueMentionSets[i]->getValueMention(val_id);
					SessionLogger::info("SERIF") << valueMention->toCasedTextString(tokenSequences[i]);
					if (!_attach_event_only_values && valueMention->getFullType().isForEventsOnly())
						continue;
					

					EventAAObservation_ptr observation = boost::make_shared<EventAAObservation>();
					observation->initializeEventAAObservation(tokenSequences[i], valueMentionSets[i], parses[i], mentionSets[i],  propSets[i], em, valueMention);

					// distributional knowledge
					if(ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_distributional_knowledge", false)) {
						observation->setAADistributionalKnowledge();	
					}

					Symbol roleSym = em->getRoleForValueMention(valueMention);
					int role = validateAndReturnRoleType(roleSym, valueMention->isTimexValue());

					if (epoch == 0 && (MODE == TRAIN_MAXENT || MODE == TRAIN_BOTH)) {						
						MaxEntModel* selectedModel = getMaxEntModelToUse(observation.get());
						selectedModel->addToTraining(observation.get(), role);
					}
					if (MODE == TRAIN_P1 || MODE == TRAIN_BOTH) {
						_p1Model->train(observation.get(), role);
					}
				}
				for (int ment_id = 0; ment_id < mentionSets[i]->getNMentions(); ment_id++) {
					const Mention *mention = mentionSets[i]->getMention(ment_id);
					SessionLogger::info("SERIF") << mention->toCasedTextString();
					// we only want real, typed mentions
					if (!mention->getEntityType().isRecognized()) continue;
					if (mention->getMentionType() != Mention::NAME &&
						mention->getMentionType() != Mention::DESC &&
						mention->getMentionType() != Mention::PRON &&
						mention->getMentionType() != Mention::PART)
						continue;

					EventAAObservation_ptr observation = boost::make_shared<EventAAObservation>();
					observation->initializeEventAAObservation(tokenSequences[i], valueMentionSets[i], parses[i], mentionSets[i],  propSets[i], em, mention);

					// distributional knowledge
					if(ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_distributional_knowledge", false)) {
						observation->setAADistributionalKnowledge();	
					}

					Symbol roleSym = em->getRoleForMention(mention);
					int role = validateAndReturnRoleType(roleSym);

					if (epoch == 0 && (MODE == TRAIN_MAXENT || MODE == TRAIN_BOTH)) {
						MaxEntModel* selectedModel = getMaxEntModelToUse(observation.get());
						selectedModel->addToTraining(observation.get(), role);
					}
					if (MODE == TRAIN_P1 || MODE == TRAIN_BOTH) {
						_p1Model->train(observation.get(), role);
					}
				}
			}

			if ((MODE == TRAIN_P1 || MODE == TRAIN_BOTH) && i % 16 == 0) {
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
	}
	if (MODE == TRAIN_MAXENT || MODE == TRAIN_BOTH) {
        std::string output_file = model_file + ".maxent";
		_model->deriveModel(pruning);
		UTF8OutputStream max_out(output_file.c_str());
		if (max_out.fail()) {
			throw UnexpectedInputException("EventArgumentFinder::train()",
				"Could not open model file for writing");
		}		
		dumpTrainingParameters(max_out);
		DTFeature::writeWeights(*_weights, max_out);
		max_out.close();

		// split classifiers
		if(_use_split_classifiers) {
	        std::string directOutputFile = directModelFile + ".maxent";
			_directModel->deriveModel(pruning);
			UTF8OutputStream directMaxOut(directOutputFile.c_str());
			if (directMaxOut.fail()) {
				throw UnexpectedInputException("EventArgumentFinder::train()", "Could not open direct model file for writing");
			}		
			dumpTrainingParameters(directMaxOut);
			DTFeature::writeWeights(*_directWeights, directMaxOut);
			directMaxOut.close();

	        std::string sharedOutputFile = sharedModelFile + ".maxent";
			_sharedModel->deriveModel(pruning);
			UTF8OutputStream sharedMaxOut(sharedOutputFile.c_str());
			if (sharedMaxOut.fail()) {
				throw UnexpectedInputException("EventArgumentFinder::train()", "Could not open shared model file for writing");
			}		
			dumpTrainingParameters(sharedMaxOut);
			DTFeature::writeWeights(*_sharedWeights, sharedMaxOut);
			sharedMaxOut.close();

	        std::string unconnectedOutputFile = unconnectedModelFile + ".maxent";
			_unconnectedModel->deriveModel(pruning);
			UTF8OutputStream unconnectedMaxOut(unconnectedOutputFile.c_str());
			if (unconnectedMaxOut.fail()) {
				throw UnexpectedInputException("EventArgumentFinder::train()", "Could not open unconnected model file for writing");
			}		
			dumpTrainingParameters(unconnectedMaxOut);
			DTFeature::writeWeights(*_unconnectedWeights, unconnectedMaxOut);
			unconnectedMaxOut.close();
		}
	}
	if (MODE == TRAIN_P1 || MODE == TRAIN_BOTH) {
		std::string output_file = model_file + ".p1";
		UTF8OutputStream p1_out(output_file.c_str());
		if (p1_out.fail()) {
			throw UnexpectedInputException("EventArgumentFinder::train()",
				"Could not open model file for writing");
		}		
		dumpTrainingParameters(p1_out);
		DTFeature::writeSumWeights(*_p1Weights, p1_out);
		p1_out.close();	
	}
  	
	// we are now set to decode
	MODE = DECODE;

}

MaxEntModel* EventArgumentFinder::getMaxEntModelToUse(const EventAAObservation *observation) const{

	MaxEntModel* selectedModel = _model;
	if(_use_split_classifiers && !observation->isValue()){
		if(observation->isDirectProp())
			selectedModel = _directModel;
		else if(observation->isSharedProp())
			selectedModel = _sharedModel;
		else if(observation->isUnconnectedProp())
			selectedModel = _unconnectedModel;
	}
	return selectedModel;
}

void EventArgumentFinder::classifyObservationWithMaxEnt(EventAAObservation* observation){
	int best_tag = 0;
	// split classifiers
	MaxEntModel* selectedModel = getMaxEntModelToUse(observation);
	selectedModel->decodeToDistribution(observation, _tagScores, _tagSet->getNTags(), &best_tag);
}

void EventArgumentFinder::collectPotentialArguments(const TokenSequence *tokens,
		const ValueMentionSet *valueMentionSet, const Parse *parse, 
		const MentionSet *mentionSet, PropositionSet *propSet,
		const EventMention *vMention)
{

	_potentialEventArgument = 0;
	// attach all possible arguments to the event mention
	for (int ment_id = 0; ment_id < mentionSet->getNMentions(); ment_id++) {
		const Mention *mention = mentionSet->getMention(ment_id);



		// we only want real, typed mentions
		if (!mention->getEntityType().isRecognized()) continue;
		if (mention->getMentionType() != Mention::NAME &&
			mention->getMentionType() != Mention::DESC &&
			mention->getMentionType() != Mention::PRON &&
			mention->getMentionType() != Mention::LIST &&
			mention->getMentionType() != Mention::PART)
			continue;

		// don't attach the same mention twice (for when we're doing a second pass)
		if (!vMention->getRoleForMention(mention).is_null())
			continue;
	
		EventAAObservation_ptr observation = boost::make_shared<EventAAObservation>();
		observation->initializeEventAAObservation(tokens, valueMentionSet, parse, mentionSet, propSet, vMention, mention);

		// distributional knowledge
		if(ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_distributional_knowledge", false)) {
			observation->setAADistributionalKnowledge();	
		}

		classifyObservationWithMaxEnt(observation.get());
		//if we found a label using the first set of models, dont do type backoff
		if(addBasicTags(mention, observation.get()) > 0)
			continue;

		if (_p1Model != 0 && (!observation->getCandidateRoleInCP().is_null() ||
							  !observation->getCandidateRoleInTriggerProp().is_null()))
		{
			Symbol p1Tag = _p1Model->decodeToSymbol(observation.get());
			if (p1Tag != _tagSet->getNoneTag()) {
				PotentialEventArgument *newArgument = 
					_new PotentialEventArgument(p1Tag, mention, .5);
				addPotentialArgument(newArgument);
				if (DEBUG) {
					_debugStream << p1Tag << L": ";
					_debugStream << mention->getNode()->toTextString() << L" (P1)\n";
					_debugStream << "NONE: \n";
					_p1Model->printDebugInfo(observation.get(), _tagSet->getNoneTagIndex(), _debugStream);
					_debugStream << p1Tag << ": \n";
					_p1Model->printDebugInfo(observation.get(), _tagSet->getTagIndex(p1Tag), _debugStream);
					
				}
			}
		}

//		if (!USE_FAKED_MENTION_TYPES)
		if (!SerifVersion::isEnglish())
			continue;
		if (mention->getEntityType().matchesPER() || 
			mention->getEntityType().matchesORG())
		{
			if (mention->getEntityType().matchesPER())
				observation->setCandidateType(EntityType::getORGType().getName());
			else if (mention->getEntityType().matchesORG())
				observation->setCandidateType(EntityType::getPERType().getName());

			classifyObservationWithMaxEnt(observation.get());	
			addFakedTags(observation.get(), mention, _perOrgChoices, 11);
		}
		else if (mention->getEntityType().matchesFAC() ||
			mention->getEntityType().matchesLOC() ||
			(mention->getEntityType().matchesGPE() && mention->getRoleType().matchesLOC()))
		{
			if (!mention->getEntityType().matchesGPE()) {
				observation->setCandidateType(EntityType::getGPEType().getName());
				classifyObservationWithMaxEnt(observation.get());	
				addFakedTags(observation.get(), mention, _gpeFacLocChoices, 3);
			} 
			if (!mention->getEntityType().matchesLOC()) {
				observation->setCandidateType(EntityType::getLOCType().getName());
				classifyObservationWithMaxEnt(observation.get());	
				addFakedTags(observation.get(), mention, _gpeFacLocChoices, 3);
			} 
		} 
	}

	for (int val_id = 0; val_id < valueMentionSet->getNValueMentions(); val_id++) {
		ValueMention *valueMention = valueMentionSet->getValueMention(val_id);
		if (!_attach_event_only_values && valueMention->getFullType().isForEventsOnly())
			continue;


		EventAAObservation_ptr observation = boost::make_shared<EventAAObservation>();
		observation->initializeEventAAObservation(tokens, valueMentionSet, parse, mentionSet, propSet, vMention, valueMention);


		// distributional knowledge
		if(ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_distributional_knowledge", false)) {
			observation->setAADistributionalKnowledge();	
		}
		int best_tag = 0;
		classifyObservationWithMaxEnt(observation.get());
		addBasicTags(valueMention, observation.get());
	}

	
}
void EventArgumentFinder::attachArguments(
		EventMention *vMention, const TokenSequence *tokens,
		ValueMentionSet *valueMentionSet, Parse *parse, 
		MentionSet *mentionSet, PropositionSet *propSet)
{
	//decode, collecting all potential arguments and their associated scores, 
	//potential arguments are stored in _potentialEventArgument
	collectPotentialArguments(tokens, valueMentionSet, parse, mentionSet, propSet, vMention);
	if (DEBUG) _debugStream << vMention->toString() << L"\n";
	
	
	// filter those arguments
	// how about for now we allow only one for each label (and of course,
	//  one for each mention!)
	while (_potentialEventArgument != 0) {

		// check to make sure we haven't already added this mention with the 
		// same role (for example, as a member of a LIST)
		if (!(vMention->getRoleForMention(_potentialEventArgument->mention) == _potentialEventArgument->role) &&
			!(vMention->getRoleForValueMention(_potentialEventArgument->valueMention) == _potentialEventArgument->role))
		{
			if (_potentialEventArgument->mention != 0) {				
				if (_potentialEventArgument->mention->getMentionType() == Mention::LIST) {
					const Mention *childMention = _potentialEventArgument->mention->getChild();
					while (childMention != 0) {
						if (childMention->getEntityType() == 
							_potentialEventArgument->mention->getEntityType()) 
						{
							// don't add child mention if it's already attached 
							// with this same role
							if (!(vMention->getRoleForMention(childMention) == 
								  _potentialEventArgument->role))
							{
								vMention->addArgument(_potentialEventArgument->role, 
									childMention, _potentialEventArgument->score);
							}
						}
						childMention = childMention->getNext();
					}
				} else vMention->addArgument(_potentialEventArgument->role, 
					_potentialEventArgument->mention, _potentialEventArgument->score);
			} else vMention->addValueArgument(_potentialEventArgument->role, 
				_potentialEventArgument->valueMention, _potentialEventArgument->score);

			if (DEBUG) {
				_debugStream << "Adding \"";
				if (_potentialEventArgument->mention != 0)
					_debugStream << _potentialEventArgument->mention->getNode()->toTextString();
				else _debugStream << _potentialEventArgument->valueMention->toString(tokens);
				_debugStream << "\" as " << _potentialEventArgument->role << "\n";
			}

		}
		Symbol role = _potentialEventArgument->role;
		const Mention *mention = _potentialEventArgument->mention;
		const ValueMention *valueMention = _potentialEventArgument->valueMention;
		PotentialEventArgument *iter = _potentialEventArgument;
		while (iter->next != 0) {
			// don't allow multiple roles for the same mention
			if ((iter->next->mention == mention && mention != 0) ||
				(iter->next->valueMention == valueMention && valueMention != 0))
			{
				if (DEBUG) {
					_debugStream << "Dumping \"";
					if (mention != 0)
						_debugStream << mention->getNode()->toTextString();
					else _debugStream << valueMention->toString(tokens);
					_debugStream  << "\" as " << iter->next->role << "\n";
				}
				PotentialEventArgument *temp = iter->next;
				iter->next = iter->next->next;
				temp->next = 0;
				delete temp;
			} 
			// only allow multiple mentions in the same role if they 
			// pass the _second_arg_recall_threshold
			else if ((iter->next->role == role) && 
					 (iter->next->score < _second_arg_recall_threshold)) 
			{
				if (DEBUG) {
					_debugStream << "Dumping \"";
					if (iter->next->mention != 0)
						_debugStream << iter->next->mention->getNode()->toTextString();
					else _debugStream << iter->next->valueMention->toString(tokens);
					_debugStream  << "\" as " << iter->next->role << "\n";
				}
				PotentialEventArgument *temp = iter->next;
				iter->next = iter->next->next;
				temp->next = 0;
				delete temp;
			} else {
				if (DEBUG) {
					_debugStream << "Keeping \"";
					if (iter->next->mention != 0)
						_debugStream << iter->next->mention->getNode()->toTextString();
					else _debugStream << iter->next->valueMention->toString(tokens);
					_debugStream  << "\" as " << iter->next->role;
					if ((iter->next->role == role) && 
						(iter->next->score >= _second_arg_recall_threshold))
					{ 
						_debugStream << " (shares role, but passes second arg recall threshold)";
					}
					_debugStream << "\n";
				}
				iter = iter->next;
			}
		}
		PotentialEventArgument *temp = _potentialEventArgument;
		_potentialEventArgument = _potentialEventArgument->next;
		temp->next = 0;
		delete temp;
	}
	
}

void EventArgumentFinder::addFakedTags(const EventAAObservation* observation, const Mention *mention, int *choices, int nchoices) {

	for (int i = 0; i < 3; i++) {
		int tag = choices[i];
        if (_tagScores[tag] > _recall_threshold) {
			PotentialEventArgument *newArgument = 
				_new PotentialEventArgument(_tagSet->getTagSymbol(tag), mention, 
				(float) _tagScores[tag]);
			addPotentialArgument(newArgument);
			if (DEBUG) {
				_debugStream << "FAKED (with type ";
				_debugStream << observation->getCandidateType() << ") "; 
				_debugStream << _tagSet->getTagSymbol(tag) << L": ";
				_debugStream << mention->getNode()->toTextString() << L" (";
				_debugStream << _tagScores[tag] << L")\n";
			}
		}
	}
}
int EventArgumentFinder::addBasicTags(const Mention *mention, EventAAObservation* observation) {
	int ntags_found = 0;
	for (int tag = 0; tag < _tagSet->getNTags(); tag++) {
		if (tag == _tagSet->getNoneTagIndex())
			continue;
		if (_tagScores[tag] > _recall_threshold) {
			PotentialEventArgument *newArgument = 
				_new PotentialEventArgument(_tagSet->getTagSymbol(tag), mention, 
				(float) _tagScores[tag]);
			addPotentialArgument(newArgument);
			ntags_found++;
			if (DEBUG) {
				_debugStream << _tagSet->getTagSymbol(tag) << L": ";
				_debugStream << mention->getNode()->toTextString() << L" (";
				_debugStream << _tagScores[tag] << L")\n";
				MaxEntModel* selectedModel = getMaxEntModelToUse(observation);
				selectedModel->printDebugInfo(observation, tag, _debugStream); 
			}
		}
	}
	if (ntags_found != 0 && DEBUG){
		MaxEntModel* selectedModel = getMaxEntModelToUse(observation);
		selectedModel->printDebugInfo(observation, _tagSet->getNoneTagIndex(), _debugStream);
	}
	
	return ntags_found;
}
int EventArgumentFinder::addBasicTags(const ValueMention *vMention,  EventAAObservation* observation) {
	int ntags_found = 0;
	for (int tag = 0; tag < _tagSet->getNTags(); tag++) {
		if (tag == _tagSet->getNoneTagIndex())
			continue;
		if (_tagScores[tag] > _recall_threshold) {
			PotentialEventArgument *newArgument = 
				_new PotentialEventArgument(_tagSet->getTagSymbol(tag), vMention, 
				(float) _tagScores[tag]);
			addPotentialArgument(newArgument);
			ntags_found++;
			if (DEBUG) {
				_debugStream << _tagSet->getTagSymbol(tag) << L": ";
				_debugStream << L"(UNPRINTED TIMEX)" << L" (";
				_debugStream << _tagScores[tag] << L")\n";
				MaxEntModel* selectedModel = getMaxEntModelToUse(observation);
				selectedModel->printDebugInfo(observation, tag, _debugStream);
			}
		}
	}

	if (ntags_found != 0 && DEBUG){
		MaxEntModel* selectedModel = getMaxEntModelToUse(observation);
		selectedModel->printDebugInfo(observation, _tagSet->getNoneTagIndex(), _debugStream);
	}
	return ntags_found;
}





void EventArgumentFinder::addPotentialArgument(PotentialEventArgument *newArgument) {
	if (_potentialEventArgument == 0) {
		_potentialEventArgument = newArgument;
		return;
	} else if (newArgument->score > _potentialEventArgument->score) {        
		newArgument->next = _potentialEventArgument;
		_potentialEventArgument = newArgument;
		return;
	} else {
		PotentialEventArgument *iter = _potentialEventArgument;
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

void EventArgumentFinder::dumpTrainingParameters(UTF8OutputStream &out) {

	DTFeature::recordDate(out);

	out << L"Parameters:\n";
	DTFeature::recordParamForConsistency(Symbol(L"event_aa_tag_set_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"event_aa_features_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"word_cluster_bits_file"), out);

	DTFeature::recordParamForReference(Symbol(L"event_training_file_list"), out);
	DTFeature::recordParamForReference(Symbol(L"aa_maxent_trainer_percent_held_out"), out);
	DTFeature::recordParamForReference(Symbol(L"aa_maxent_trainer_max_iterations"), out);
	DTFeature::recordParamForReference(Symbol(L"aa_maxent_trainer_gaussian_variance"), out);
	DTFeature::recordParamForReference(Symbol(L"aa_maxent_trainer_min_likelihood_delta"), out);
	DTFeature::recordParamForReference(Symbol(L"aa_maxent_trainer_stop_check_frequency"), out);
	DTFeature::recordParamForReference(Symbol(L"aa_maxent_trainer_pruning_cutoff"), out);
	DTFeature::recordParamForReference(Symbol(L"aa_p1_trainer_epochs"), out);
	out << L"\n";

}


void EventArgumentFinder::resetRoundRobinStatistics() {
	_correct_args = 0;
	_correct_non_args = 0;
	_wrong_type = 0;
	_missed = 0;
	_spurious = 0;
}

void EventArgumentFinder::printRoundRobinStatistics(UTF8OutputStream &out) {

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

int EventArgumentFinder::validateAndReturnRoleType(Symbol roleSym, bool isTimex) const{
	int role = _tagSet->getNoneTagIndex();
	if (!roleSym.is_null()) {
		role = _tagSet->getTagIndex(roleSym);
		if (isTimex)
			role = _tagSet->getTagIndex(Symbol(L"Time-Within"));
		if (role == -1) {
			char c[500];
			sprintf(c, "ERROR: Invalid event argument type in training file: %s", 
				roleSym.to_debug_string());
			throw UnexpectedInputException("EventArgumentFinder::train()", c);
		}
	}
	return role;

}

//////////////

// helper function to generate a vector of <Mention, event-aa-role> pairs
std::vector<EventArgumentFinder::MentionRolePair> EventArgumentFinder::getMentionRolePairs(EventMentionSet *eset) {
	std::vector<MentionRolePair> results;

	for(int emIndex=0; emIndex<eset->getNEventMentions(); emIndex++) {
		EventMention *em = eset->getEventMention(emIndex);
		for(int argIndex=0; argIndex<em->getNArgs(); argIndex++) {
			const Mention *arg = em->getNthArgMention(argIndex);
			Symbol role = em->getNthArgRole(argIndex);
			results.push_back( std::pair<const Mention*, Symbol>(arg, role) );
		}
	}
	return results;
}

// helper function to generate a vector of <ValueMention, event-aa-role> pairs
std::vector<EventArgumentFinder::ValueMentionRolePair> EventArgumentFinder::getValueMentionRolePairs(EventMentionSet *eset) {
	std::vector<ValueMentionRolePair> results;

	for(int emIndex=0; emIndex<eset->getNEventMentions(); emIndex++) {
		EventMention *em = eset->getEventMention(emIndex);
		for(int argIndex=0; argIndex<em->getNValueArgs(); argIndex++) {
			const ValueMention *arg = em->getNthArgValueMention(argIndex);
			Symbol role = em->getNthArgValueRole(argIndex);
			results.push_back( std::pair<const ValueMention*, Symbol>(arg, role) );
		}
	}
	return results;
}

// given two vectors of MentionRolePair, one gold/key and the other predicted, calculate the number of matches
void EventArgumentFinder::tabulateStatistics(const std::vector<MentionRolePair>& key, const std::vector<MentionRolePair>& test) {
	int c = 0;

	for(unsigned keyIndex=0; keyIndex<key.size(); keyIndex++) {
		const Mention *keyMention = key[keyIndex].first;
		Symbol keyRole = key[keyIndex].second;

		for(unsigned testIndex=0; testIndex<test.size(); testIndex++) {
			if(keyMention == test[testIndex].first) {
				if(keyRole == test[testIndex].second) {
					c += 1;
					break;
				}
			}
		}
	}

	_correct_args += c;
	_correct_entity_mention += c;
	_missed += (static_cast<int>(key.size()) - c);
	_missed_entity_mention += (static_cast<int>(key.size()) - c);
	_spurious += (static_cast<int>(test.size()) - c);
	_spurious_entity_mention += (static_cast<int>(test.size()) - c);
}

void EventArgumentFinder::tabulateStatistics(const std::vector<ValueMentionRolePair>& key, const std::vector<ValueMentionRolePair>& test) {
	int c = 0;

	for(unsigned keyIndex=0; keyIndex<key.size(); keyIndex++) {
		const ValueMention *keyMention = key[keyIndex].first;
		Symbol keyRole = key[keyIndex].second;

		for(unsigned testIndex=0; testIndex<test.size(); testIndex++) {
			if(keyMention == test[testIndex].first) {
				if(keyRole == test[testIndex].second) {
					c += 1;
					break;
				}
			}
		}
	}

	_correct_args += c;
	_correct_value_mention += c;
	_missed += (static_cast<int>(key.size()) - c);
	_missed_value_mention += (static_cast<int>(key.size()) - c);
	_spurious += (static_cast<int>(test.size()) - c);
	_spurious_value_mention += (static_cast<int>(test.size()) - c);
}

// the 'eset' parameter represent gold triggers. I will go through each (gold) trigger, reset/clear-out its arguments,
// and then invoke attachArguments to predict/attach arguments to the trigger. Hence, I am performing event-aa with gold triggers
void EventArgumentFinder::devTestDecode(TokenSequence *tokenSequence, Parse *parse, ValueMentionSet *valueMentionSet, 
		MentionSet *mentionSet, PropositionSet *propSet, EventMentionSet *eset) 
{
	if (MODE != DECODE || _model == 0)
		return;

	std::vector<MentionRolePair> keyMentionRolePairs = getMentionRolePairs(eset);
	std::vector<ValueMentionRolePair> keyValueMentionRolePairs = getValueMentionRolePairs(eset);

	EventMentionSet *testEset = _new EventMentionSet(parse);

	for (int ement = 0; ement < eset->getNEventMentions(); ement++) {
		EventMention *key_em = eset->getEventMention(ement);
		EventMention *test_em = _new EventMention(*key_em);	
		test_em->resetArguments();
		attachArguments(test_em, tokenSequence, valueMentionSet, parse, mentionSet, propSet);
		testEset->takeEventMention(test_em);
	}

	std::vector<MentionRolePair> testMentionRolePairs = getMentionRolePairs(testEset);
	std::vector<ValueMentionRolePair> testValueMentionRolePairs = getValueMentionRolePairs(testEset);

	tabulateStatistics(keyMentionRolePairs, testMentionRolePairs);
	tabulateStatistics(keyValueMentionRolePairs, testValueMentionRolePairs);

	delete testEset;
}

// Performing event-aa with predicted triggers. 'keyEset' are gold triggers, while 'testEset' are predicted triggers
void EventArgumentFinder::devTestDecode(TokenSequence *tokenSequence, Parse *parse, ValueMentionSet *valueMentionSet, 
		MentionSet *mentionSet, PropositionSet *propSet, EventMentionSet *keyEset, EventMentionSet *testEset) 
{
	if (MODE != DECODE || _model == 0)
		return;

	std::vector<MentionRolePair> keyMentionRolePairs = getMentionRolePairs(keyEset);
	std::vector<ValueMentionRolePair> keyValueMentionRolePairs = getValueMentionRolePairs(keyEset);

	for (int ement = 0; ement < testEset->getNEventMentions(); ement++) {
		EventMention *test_em = testEset->getEventMention(ement);
		test_em->resetArguments();
		attachArguments(test_em, tokenSequence, valueMentionSet, parse, mentionSet, propSet);
	}

	std::vector<MentionRolePair> testMentionRolePairs = getMentionRolePairs(testEset);
	std::vector<ValueMentionRolePair> testValueMentionRolePairs = getValueMentionRolePairs(testEset);

	tabulateStatistics(keyMentionRolePairs, testMentionRolePairs);
	tabulateStatistics(keyValueMentionRolePairs, testValueMentionRolePairs);
}

void EventArgumentFinder::resetAllStatistics() {
	_correct_args = 0;
	_correct_non_args = 0;
	_wrong_type = 0;
	_missed = 0;
	_spurious = 0;

	_correct_entity_mention = 0;
	_wrong_entity_mention = 0;
	_missed_entity_mention = 0;
	_spurious_entity_mention = 0;

	_correct_value_mention = 0;
	_wrong_value_mention = 0;
	_missed_value_mention = 0;
	_spurious_value_mention = 0;
}

void EventArgumentFinder::printRecPrecF1(UTF8OutputStream& out) {
	int R = _correct_args + _wrong_type + _missed;
	int P = _correct_args + _wrong_type + _spurious;
	double recall = (double) _correct_args / R;
	double precision = (double) _correct_args / P;
	double f1 = (2 * precision * recall) / (precision + recall);
	out << L"all_mention: R=" << _correct_args << L"/" << R << L"=" << recall << L" P=" << _correct_args << L"/" << P << L"=" << precision << L" F1=" << f1 << L"\n";

	R = _correct_entity_mention + _wrong_entity_mention + _missed_entity_mention;
	P = _correct_entity_mention + _wrong_entity_mention + _spurious_entity_mention;
	recall = (double) _correct_entity_mention / R;
	precision = (double) _correct_entity_mention / P;
	f1 = (2 * precision * recall) / (precision + recall);
	out << L"entity_mention: R=" << _correct_entity_mention << L"/" << R << L"=" << recall << L" P=" << _correct_entity_mention << L"/" << P << L"=" << precision << L" F1=" << f1 << L"\n";

	R = _correct_value_mention + _wrong_value_mention + _missed_value_mention;
	P = _correct_value_mention + _wrong_value_mention + _spurious_value_mention;
	recall = (double) _correct_value_mention / R;
	precision = (double) _correct_value_mention / P;
	f1 = (2 * precision * recall) / (precision + recall);
	out << L"value_mention: R=" << _correct_value_mention << L"/" << R << L"=" << recall << L" P=" << _correct_value_mention << L"/" << P << L"=" << precision << L" F1=" << f1 << L"\n";
}

