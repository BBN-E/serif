// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "boost/regex.hpp"

#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/Parse.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discourseRel/DiscourseRelFeatureType.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/maxent/MaxEntModel.h"
#include "Generic/discTagger/P1Decoder.h"
#include "Generic/discourseRel/DiscourseRelFeatureTypes.h"
#include "Generic/discourseRel/DiscourseRelObservation.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/state/TrainingLoader.h"
#include "Generic/state/StateLoader.h"

#include "Generic/common/SymbolUtilities.h"
#include "Generic/discourseRel/PennDiscourseTreebank.h"
#include "Generic/discourseRel/TargetConnectives.h"
#include "Generic/discourseRel/DiscourseRelTrainer.h"
#include "Generic/discourseRel/ConnectiveCorpusStatistics.h"
#include "Generic/discourseRel/CrossSentRelation.h"
#include "Generic/discourseRel/StopWordFilter.h"


#include <boost/algorithm/string/predicate.hpp>
#include <boost/scoped_ptr.hpp>

// this is intended for development mode, when one might want to save
// the model after each iteration
#define PRINT_EVERY_EPOCH 0

//const float targetLoadingFactor = static_cast<float>(0.7);


DiscourseRelTrainer::DiscourseRelTrainer()
	: MODE(TRAIN), MODEL_TYPE(MAX_ENT),
	  TRAIN_SOURCE(PTB_PARSES), 
	  _featureTypes(0), _tagSet(0), _tagScores(0), 
	  _maxEntDecoder(0), _maxEntWeights(0),
	  _p1Decoder(0), _p1Weights(0),  DEBUG(false),
	  _num_documents(0), _docTheories(0)

{
	DiscourseRelFeatureTypes::ensureFeatureTypesInstantiated(); 
	// Functions potentially useful in the future
	// WordClusterTable::ensureInitializedFromParamFile();
	
	
	std::string buffer = ParamReader::getParam("discourse_rel_train_debug");
	// DEBUGGING
	if (!buffer.empty()) {
		DEBUG = true;
		_debugStream.open(buffer.c_str());
	}
	
	// Discourse Rel MODEL TYPE -- MaxEnt or P1
	// We only implemented MaxEnt model so far 
	std::string mtype = ParamReader::getRequiredParam("discourse_rel_model_type");
	if (mtype == "maxent" || mtype == "MAXENT"){
		MODEL_TYPE = MAX_ENT;
	}else if (mtype == "p1" || mtype == "P1") {
		MODEL_TYPE = P1;
		cerr<<"p1 model\n";
	}else if (mtype == "both" || mtype == "BOTH") {
		MODEL_TYPE = BOTH;
	}else if (mtype == "p1_ranking" || mtype == "P1_RANKING"){
		MODEL_TYPE = P1_RANKING;
		cerr<<"p1 ranking model\n";
	}else {
		throw UnrecoverableException("DiscourseRelTrainer::DiscourseRelTrainer()",
			"Parameter 'discourse_rel_model_type' not recognized");
	}

	//training mode 
	MODE = TRAIN;
	
	if (ParamReader::isParamTrue("discourse_rel_crossvalidation")) {
		MODE = CV;
		_cross_validation_fold = ParamReader::getRequiredIntParam("cross_validation_fold");
	}

	if (ParamReader::isParamTrue("discourse_rel_dataanalysis")) {
		MODE = ANALY;
	}

	//task
	TASK = CROSS_SENT_ALL;  // default
	std::string task_str = ParamReader::getParam("discourse_rel_task");
	if (!task_str.empty()) {
		if (boost::iequals(task_str, "cross-sent-all")) {
			TASK = CROSS_SENT_ALL;
		}else if (boost::iequals(task_str, "explicit-select")) {
			TASK = EXPLICIT_SELECT;
			// Connective Dict FILE
			_connectives_file = ParamReader::getRequiredParam("discourse_rel_connectives_file");
		}else if (boost::iequals(task_str, "explicit-all")){
			TASK = EXPLICIT_ALL;
		}else if (boost::iequals(task_str, "cross-sent-implicit")){
			TASK = CROSS_SENT_IMPLICIT;
		}else if (boost::iequals(task_str, "cross-sent-select")) {
			TASK = CROSS_SENT_SELECT;
		}
	}

	// SOURCE OF TRAINING DATA -- state-files or aug-parses
	// need to modify later ##
	std::string source = ParamReader::getRequiredParam("discourse_rel_train_source");
	if (boost::iequals(source, "ptb-parses"))
		TRAIN_SOURCE = PTB_PARSES;
	else if (boost::iequals(source, "auto-parses"))
		TRAIN_SOURCE = AUTO_PARSES;
	else
		throw UnexpectedInputException("DiscourseRelTrainer()::DiscourseRelTrainer()", 
			"Invalid parameter value for 'discourse_rel_train_source'.  Must be 'ptb-parses' or 'auto-parses'.");

	std::string tag_set_file;
	std::string features_file;

	if (TASK == CROSS_SENT_ALL || TASK == CROSS_SENT_SELECT || TASK == CROSS_SENT_IMPLICIT){
		tag_set_file = ParamReader::getRequiredParam("discourse_rel_tag_set_crossSent_file");
		features_file = ParamReader::getRequiredParam("discourse_rel_features_crossSent_file");

		// TRAINING DATA FILE
		_training_file_list = ParamReader::getRequiredParam("discourse_rel_training_file_crossSent_list");
	
		// PDTB DATA FILE
		_pdtb_file_list = ParamReader::getRequiredParam("discourse_rel_pdtb_file_crossSent_list");

		// STOP WORD File
		_stopword_file = ParamReader::getRequiredParam("discourse_rel_stopword_file");

	}else if (TASK == EXPLICIT_ALL || TASK == EXPLICIT_SELECT){
		tag_set_file = ParamReader::getRequiredParam("discourse_rel_tag_set_explicit_file");
		features_file = ParamReader::getRequiredParam("discourse_rel_features_explicit_file");

		// TRAINING DATA FILE
		_training_file_list = ParamReader::getRequiredParam("discourse_rel_training_file_explicit_list");
	
		// PDTB DATA FILE
		_pdtb_file_list = ParamReader::getRequiredParam("discourse_rel_pdtb_file_explicit_list");
	}
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);
	_tagScores = _new double[_tagSet->getNTags()];
	cerr<<"after reading the tag set\n";
	
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), DiscourseRelFeatureType::modeltype);
	cerr<<"after read the feature file\n";

	// MODEL FILE NAME
	_model_file = ParamReader::getRequiredParam("discourse_rel_model_file");
	
}

DiscourseRelTrainer::~DiscourseRelTrainer() {
	
	cerr<<"in delete"<<endl;
	delete _featureTypes;
	cerr<<"in delete1"<<endl;
	delete _tagSet;
	cerr<<"in delete2"<<endl;
	delete _tagScores;
	//	cerr<<"in delete3"<<endl;
	//	delete _observation;

	//cerr<<"in delete5"<<endl;
	//delete [] _observations;
	
	/*
	cerr<<"in delete3"<<endl;
	for (int j = 0; j < _num_documents; j++)
		delete _stateFileNameList[j];
	delete [] _stateFileNameList;
	*/

}

void DiscourseRelTrainer::train() {
	_p1Weights = 0;
	_p1Decoder = 0;
	_maxEntWeights = 0;
	_maxEntDecoder = 0;
	_epochs = 1;

	if (MODEL_TYPE == BOTH) {
		throw UnexpectedInputException("DiscourseRelTrainer::train()",
									 "You can't train both types of models at once.");
	} else if (MODEL_TYPE == MAX_ENT) {

		// TRAIN MODE
		std::string param_mode = ParamReader::getRequiredParam("maxent_trainer_mode");
		if (param_mode == "GIS")
			_mode = MaxEntModel::GIS;
		else if (param_mode == "SCGIS")
			_mode = MaxEntModel::SCGIS;
		else
			throw UnexpectedInputException("DiscourseRelTrainer::train()",
							"Invalid setting for parameter 'maxent_trainer_mode'");

		// PRUNING
		_pruning = ParamReader::getRequiredIntParam("maxent_trainer_pruning_cutoff");

		// PERCENT HELD OUT
		_percent_held_out = ParamReader::getRequiredIntParam("maxent_trainer_percent_held_out");
		if (_percent_held_out < 0 || _percent_held_out > 50)
			throw UnexpectedInputException("DiscourseRelTrainer::train()",
				"Parameter 'maxent_trainer_percent_held_out' must be between 0 and 50");

		// MAX NUMBER OF ITERATIONS (STOPPING CONDITION)
		_max_iterations = ParamReader::getRequiredIntParam("maxent_trainer_max_iterations");

		// GAUSSIAN PRIOR VARIANCE
		_variance = ParamReader::getRequiredFloatParam("maxent_trainer_gaussian_variance");

		// MIN CHANGE IN LIKELIHOOD (STOPPING CONDITION)
		_likelihood_delta = ParamReader::getOptionalFloatParamWithDefaultValue("maxent_min_likelihood_delta", .0001);

		// FREQUENCY OF STOPPING CONDITION CHECKS (NUM ITERATIONS)
		_stop_check_freq = ParamReader::getOptionalIntParamWithDefaultValue("maxent_stop_check_frequency", 1);

		_train_vector_file = ParamReader::getParam("maxent_train_vector_file");
		_test_vector_file = ParamReader::getParam("maxent_test_vector_file");

		const char* train_vf = 0;
		if (!_train_vector_file.empty())
			train_vf = _train_vector_file.c_str();
		const char* test_vf = 0;
		if (!_test_vector_file.empty())
			test_vf = _test_vector_file.c_str();

		_maxEntWeights = _new DTFeature::FeatureWeightMap();
		_maxEntDecoder = _new MaxEntModel(_tagSet, _featureTypes, _maxEntWeights,
								   _mode, _percent_held_out, _max_iterations, _variance,
								   _likelihood_delta, _stop_check_freq,
								   train_vf, test_vf);


	} else if (MODEL_TYPE == P1 || MODEL_TYPE == P1_RANKING) {
		_epochs = ParamReader::getRequiredIntParam("discourse_rel_trainer_epochs");
		_p1Weights = _new DTFeature::FeatureWeightMap();
		if(MODEL_TYPE == P1){
			cerr<<"P1 decoder used\n";
			_p1Decoder = _new P1Decoder(_tagSet, _featureTypes, _p1Weights);
		}else {// RANKING
			cerr<<"P1_RANKING decoder used\n";
			/* need to figure out later
			DTFeatureTypeSet *NoneFeaturetypes = _new DTFeatureTypeSet(1);
			NoneFeaturetypes->addFeatureType(DiscourseRelFeatureType::modeltype,Symbol(L"ment-hw")); //##
			DTFeatureTypeSet **_featureTypesArr = _new DTFeatureTypeSet*[_tagSet->getNTags()];
			_featureTypesArr[_tagSet->getNoneTagIndex()] = NoneFeaturetypes;
			_featureTypesArr[_tagSet->getTagIndex(DescLinkFeatureFunctions::getLinkSymbol())] 
				= _featureTypes;
			_p1Decoder = _new P1Decoder(_tagSet, _featureTypesArr, _p1Weights);
			*/
		}

	}

	if (MODEL_TYPE == MAX_ENT) {
		if (MODE == TRAIN){
			if ( TRAIN_SOURCE == PTB_PARSES){
				trainMaxEntPTBParses();  
			}else{
				trainMaxEntAutoParses(); 
			}
		}else if (MODE == CV){
			if ( TRAIN_SOURCE == PTB_PARSES){
				cvMaxEntPTBParses();  
			}else{
				cvMaxEntAutoParses();
			}
		}else if (MODE == ANALY){
			dataAnalysis ();
		}

	} else if (MODEL_TYPE == P1 || MODEL_TYPE == P1_RANKING) {
		if ( TRAIN_SOURCE == PTB_PARSES){
			trainP1PTBParses();
		}else{
			trainP1AutoParses();
		}
	}
	// this isn't really necessary in practice, but helpful for memory leak
	// detection
	/*if (_p1Weights != 0) {
		for (DTFeature::FeatureWeightMap::iterator iter = _p1Weights->begin();
			iter != _p1Weights->end(); ++iter)
		{
			(*iter).first->deallocate();
		}
	}
	*/

	if (_maxEntWeights != 0){
			delete _maxEntWeights;	
		}
		if (_maxEntDecoder != 0){
			delete _maxEntDecoder;
		}
	cerr<<"exiting train()"<<endl;
}	

void DiscourseRelTrainer::trainMaxEntAutoParses() {
	loadTrainingDataFromList(_training_file_list.c_str());
	
	TargetConnectives::loadConnDict(_connectives_file.c_str());
	//PennDiscourseTreebank::loadDataFromPDTBFileList (_pdtb_file_list);
	PennDiscourseTreebank::loadDataFromPDTBFileList (_pdtb_file_list.c_str(), TargetConnectives::getConnDict());

	vector<string>::iterator iterFileName = _stateFileNameList.begin();
	
	//for (int i = _num_documents - 1; i >= 0; i--) {
	for (int i = 0; i < _num_documents; i++) {
		string DocId = *iterFileName;
		iterFileName ++;
		processDocument(_docTheories[i], DocId);
	}

	// train the model
	for (int i=0; i< (int)_keys.size(); i++){
		_maxEntDecoder->addToTraining(_observations[i], _keys[i]);
	}
	SessionLogger::info("SERIF") << "Deriving model...\n";
	_maxEntDecoder->deriveModel(_pruning);
	
	writeWeights();
	
	for (int j = 0; j < _num_documents; j++)
		delete _docTheories[j];
	delete [] _docTheories;

	_num_documents = 0;
	_docTheories = 0;

	PennDiscourseTreebank::finalize();
	TargetConnectives::finalize();
	StopWordFilter::finalize();
}

void DiscourseRelTrainer::trainMaxEntPTBParses() {
	/*refer to DTCorefTrainer */
}

void DiscourseRelTrainer::cvMaxEntAutoParses() {
	std::string buffer = ParamReader::getRequiredParam("discourse_rel_cv_out");
	_cvStream.open(buffer.c_str());
	_featDebugStream.open("C:\\DiscourseRel\\English\\models\\debug\\richFeature.debug");
	loadTrainingDataFromList(_training_file_list.c_str());
	
	if (TASK == EXPLICIT_ALL){
		PennDiscourseTreebank::loadDataFromPDTBFileList (_pdtb_file_list.c_str());
	}else if (TASK == EXPLICIT_SELECT){
		TargetConnectives::loadConnDict(_connectives_file.c_str());
		PennDiscourseTreebank::loadDataFromPDTBFileList (_pdtb_file_list.c_str(), TargetConnectives::getConnDict());
	}else if (TASK == CROSS_SENT_ALL){
		PennDiscourseTreebank::loadCrossSentDataFromPDTBFileList (_pdtb_file_list.c_str());
		StopWordFilter::loadStopWordDict(_stopword_file.c_str());
		StopWordFilter::initFilteredWordDict();
	}

	vector<string>::iterator iterFileName = _stateFileNameList.begin();
		
	if (TASK == EXPLICIT_ALL || TASK == EXPLICIT_SELECT ){
		for (int i = 0; i < _num_documents; i++) {
			string DocId = *iterFileName;
			iterFileName ++;
			processDocument(_docTheories[i], DocId);
		}
	}else if (TASK == CROSS_SENT_ALL ){
		for (int i = 0; i < _num_documents; i++){
			string DocId = *iterFileName;
			iterFileName ++;
			processDocument_crossSentRel(_docTheories[i], DocId);
		}
	}

	// train the model
	if (_cross_validation_fold == 0){
		SessionLogger::info("SERIF") << "there is no held out data for DEVTEST mode !!";
	}else if (TASK == EXPLICIT_ALL || TASK == EXPLICIT_SELECT){
		int totalInsts = (int)_keys.size();
		int numOfEvalSets = _cross_validation_fold;
		
		// eval the MaxEnt model
		int totalfP = 0;  //false positive
		int totalfN = 0;
		int pInsts = 0;  // positive instances
		int nInsts = 0;  // negative instances
		if (_maxEntWeights != 0){
			delete _maxEntWeights;	
		}
		if (_maxEntDecoder != 0){
			delete _maxEntDecoder;
		}
		for (int round = 0 ; round < numOfEvalSets ; round ++){
			_maxEntWeights = _new DTFeature::FeatureWeightMap();

			const char* train_vf = 0;
			if (!_train_vector_file.empty())
				train_vf = _train_vector_file.c_str();
			const char* test_vf = 0;
			if (!_test_vector_file.empty())
				test_vf = _test_vector_file.c_str();

			_maxEntDecoder = _new MaxEntModel(_tagSet, _featureTypes, _maxEntWeights,
								   _mode, _percent_held_out, _max_iterations, _variance,
								   _likelihood_delta, _stop_check_freq,
								   train_vf, test_vf);


			map<Symbol, int> *_connFreqDict = _new map<Symbol, int>;
			vector<DiscourseRelObservation> _evalObservations;
			vector<int> _evalKeys;
			for (int i=0; i< (int)_keys.size(); i++){
				DiscourseRelObservation obs = *dynamic_cast<DiscourseRelObservation*>(_observations[i]);
				Symbol lcWord = obs.getLCWord();
				int wordIndex=0; 
				map<Symbol, int>::iterator myIterator = _connFreqDict->find(lcWord);

				if(myIterator != _connFreqDict->end())
				{					
					wordIndex = myIterator->second;
					(*_connFreqDict)[lcWord]++;
				}else{
					(*_connFreqDict)[lcWord]=1;
				}
				
				if (wordIndex%numOfEvalSets == round){
					_evalObservations.push_back(obs);
					_evalKeys.push_back(_keys[i]);
				}else{
					_maxEntDecoder->addToTraining(&obs, _keys[i]);	
				}
			}

			delete _connFreqDict;
			_connFreqDict = 0;

			// train the MaxEnt model
			SessionLogger::info("SERIF") << "Deriving model...\n";
			_maxEntDecoder->deriveModel(_pruning);
	
			writeWeights();
			
			// eval the MaxEnt model
			int falsePositive = 0;
			int falseNegative = 0;
			int evalInsts = 0;
			int correct = 0;
			int positiveInsts = 0;
			int negativeInsts = 0;

			_cvStream << "round " << round << ":\n";
			for (int i = 0; i < (int) _evalKeys.size(); i++){
				int hypothesis;
				_maxEntDecoder->decodeToDistribution(&(_evalObservations[i]), _tagScores, 
															 _tagSet->getNTags(), &hypothesis);
				double this_score = _tagScores[hypothesis];
				const SynNode *root=  _evalObservations[i].getRootOfContextTree();
				Symbol lcword=_evalObservations[i].getLCWord();

				evalInsts ++;
				
				if (hypothesis == _evalKeys[i]){
					correct ++;
					if (_evalKeys[i] == 1){
						positiveInsts ++;
					}else{
						negativeInsts ++;
					}

					//cout << "correct answer ! \n";
					_cvStream << lcword << " -- correct answer --" << hypothesis << "(model, gold)\n";
					_cvStream << root->toPrettyParse(3) << "\n";

					printFeatures(_cvStream, &(_evalObservations[i]), _evalKeys[i]);

				}else{
					if (_evalKeys[i] == 1){
						positiveInsts ++;
						falseNegative ++;
					}else{
						negativeInsts ++;
						falsePositive ++;
					}
					//cout << "wrong answer ! \n";
					_cvStream << lcword << " -- wrong answer --" << hypothesis <<"(model) vs. " << _evalKeys[i] <<"(gold)\n";
					_cvStream << root->toPrettyParse(3) << "\n";

					printFeatures(_cvStream, &(_evalObservations[i]), _evalKeys[i]);
				}
			}
			totalfP += falsePositive;  //false positive
			totalfN += falseNegative;
			pInsts += positiveInsts;  // positive instances
			nInsts += negativeInsts;  // negative instances
			_cvStream << "round " << round << ":  positive --" << positiveInsts << " negative --" << negativeInsts << "\n" ;
			_cvStream << "round " << round << ":  false positive --" << falsePositive << " false negative --" << falseNegative << "  acc: " << (float)correct/evalInsts << "\n\n";
		
			delete _maxEntWeights;
			delete _maxEntDecoder;
		}
		_cvStream << "total:  insts --" << totalInsts << " positive --" << pInsts << " negative --" << nInsts << "\n";
		_cvStream << "total:  false positive --" << totalfP << " false negative --" << totalfN << "  acc: " << (float)(totalInsts-totalfP-totalfN)/totalInsts << "\n\n";
		
		_maxEntWeights = 0;
		_maxEntDecoder = 0;
	}else if (TASK == CROSS_SENT_ALL){
		int totalInsts = (int)_keys.size();
		int numOfEvalSets = _cross_validation_fold;
		
		// eval the MaxEnt model
		int totalfP = 0;  //false positive
		int totalfN = 0;
		int pInsts = 0;  // positive instances
		int nInsts = 0;  // negative instances
			
		map<string, IntIntIntInt> *_resultbyTypeDict = _new  map<string, IntIntIntInt>;
			


		if (_maxEntWeights != 0){
			delete _maxEntWeights;	
		}
		if (_maxEntDecoder != 0){
			delete _maxEntDecoder;
		}
		for (int round = 0 ; round < numOfEvalSets ; round ++){
			_maxEntWeights = _new DTFeature::FeatureWeightMap();
			
			const char* train_vf = 0;
			if (!_train_vector_file.empty())
				train_vf = _train_vector_file.c_str();
			const char* test_vf = 0;
			if (!_test_vector_file.empty())
				test_vf = _test_vector_file.c_str();

			_maxEntDecoder = _new MaxEntModel(_tagSet, _featureTypes, _maxEntWeights,
								   _mode, _percent_held_out, _max_iterations, _variance,
								   _likelihood_delta, _stop_check_freq,
								   train_vf, test_vf);

			map<string, int> *_disRelFreqDict = _new map<string, int>;
			vector<DiscourseRelObservation> _evalObservations;
			vector<int> _evalKeys;
			for (int i=0; i< (int)_keys.size(); i++){
				DiscourseRelObservation obs = *dynamic_cast<DiscourseRelObservation*>(_observations[i]);
				//CrossSentRelation * _crossSentRel = _keysDetails[i];
				//string relType = _crossSentRel->getRelType();
				string relType = _relTypes[i];
				int typeFreq=0; 
				map<string, int>::iterator myIterator = _disRelFreqDict->find(relType);

				if(myIterator != _disRelFreqDict->end())
				{					
					typeFreq = myIterator->second;
					(*_disRelFreqDict)[relType]++;
				}else{
					(*_disRelFreqDict)[relType]=1;
				}
				
				if (typeFreq%numOfEvalSets == round){
					_evalObservations.push_back(obs);
					_evalKeys.push_back(_keys[i]);
				}else{
					SessionLogger::info("SERIF") << i << endl;
					_maxEntDecoder->addToTraining(&obs, _keys[i]);	
				}
			}

			//output distribution information for relation types
			_cvStream << "Frequency distribution on Relation Types: \n";
			map<string, int>::iterator myIterator = _disRelFreqDict->begin();
			string relType;
			int typeFreq; 
			while (myIterator != _disRelFreqDict->end()){
				relType = (* myIterator).first;
				typeFreq = (* myIterator).second;
				_cvStream << relType.c_str() << "    :" << typeFreq << "\n";
				myIterator ++;
			}
			_cvStream << "**********************************\n\n";
			delete _disRelFreqDict;
			_disRelFreqDict = 0;

			// train the MaxEnt model
			SessionLogger::info("SERIF") << "Deriving model...\n";
			_maxEntDecoder->deriveModel(_pruning);
	
			writeWeights();
			
			// eval the MaxEnt model
			int falsePositive = 0;
			int falseNegative = 0;
			int evalInsts = 0;
			int correct = 0;
			int positiveInsts = 0;
			int negativeInsts = 0;
			
			_cvStream << "round " << round << ":\n";
			for (int i = 0; i < (int) _evalKeys.size(); i++){
				int hypothesis;
				_maxEntDecoder->decodeToDistribution(&(_evalObservations[i]), _tagScores, 
															 _tagSet->getNTags(), &hypothesis);
				double this_score = _tagScores[hypothesis];
				wstring sentPairContext =  _evalObservations[i].getSentPair();
				
				evalInsts ++;
				string relType = _relTypes[i];
				map<string, IntIntIntInt>::iterator myIterator = _resultbyTypeDict->find(relType);

				if(myIterator == _resultbyTypeDict->end()){
					(*_resultbyTypeDict)[relType].correctPos=0;
					(*_resultbyTypeDict)[relType].correctNeg=0;
					(*_resultbyTypeDict)[relType].falsePos=0;
					(*_resultbyTypeDict)[relType].falseNeg=0;
				}
		
				if (hypothesis == _evalKeys[i]){
					correct ++;
					if (_evalKeys[i] == 1){
						positiveInsts ++;
						(*_resultbyTypeDict)[relType].correctPos++;
					}else{
						negativeInsts ++;
						(*_resultbyTypeDict)[relType].correctNeg++;
					}

					//cout << "correct answer ! \n";
					_cvStream << " -- correct answer --" << hypothesis << "(model, gold)\n";
					_cvStream << sentPairContext << "\n";

					printFeatures(_cvStream, &(_evalObservations[i]), _evalKeys[i]);

				}else{
					if (_evalKeys[i] == 1){
						positiveInsts ++;
						falseNegative ++;
						(*_resultbyTypeDict)[relType].falseNeg++;
					}else{
						negativeInsts ++;
						falsePositive ++;
						(*_resultbyTypeDict)[relType].falsePos++;
					}
					//cout << "wrong answer ! \n";
					_cvStream << " -- wrong answer --" << hypothesis <<"(model) vs. " << _evalKeys[i] <<"(gold)\n";
					_cvStream << sentPairContext << "\n";

					printFeatures(_cvStream, &(_evalObservations[i]), _evalKeys[i]);
				}
			}
			totalfP += falsePositive;  //false positive
			totalfN += falseNegative;
			pInsts += positiveInsts;  // positive instances
			nInsts += negativeInsts;  // negative instances
			_cvStream << "round " << round << ":  positive --" << positiveInsts << " negative --" << negativeInsts << "\n" ;
			_cvStream << "round " << round << ":  false positive --" << falsePositive << " false negative --" << falseNegative << "  acc: " << (float)correct/evalInsts << "\n\n";
			
			_cvStream << "**********************************\n\n";



			delete _maxEntWeights;
			delete _maxEntDecoder;

		}

		_cvStream << "total:  insts --" << totalInsts << " positive --" << pInsts << " negative --" << nInsts << "\n";
		_cvStream << "total:  false positive --" << totalfP << " false negative --" << totalfN << "  acc: " << (float)(totalInsts-totalfP-totalfN)/totalInsts << "\n\n";


		_cvStream << "\nResult Distribution on Relation Types: \n";
		map<string, IntIntIntInt>::iterator myIterator = _resultbyTypeDict->begin();
		string relType;
		
		while (myIterator != _resultbyTypeDict->end()){
			relType = (* myIterator).first;
			IntIntIntInt results = (* myIterator).second;
			_cvStream << relType.c_str()  << " :\n";
			_cvStream << "    correct positive: " << results.correctPos << "\n";
			_cvStream << "    correct negative: " << results.correctNeg << "\n";
			_cvStream << "    false positive: " << results.falsePos << "\n";
			_cvStream << "    false negative: " << results.falseNeg << "\n\n";

			myIterator ++;
		}
		delete _resultbyTypeDict;
		_resultbyTypeDict = 0;
		_maxEntWeights = 0;
		_maxEntDecoder = 0;
	}
	for (int j = 0; j < _num_documents; j++)
		delete _docTheories[j];
	delete [] _docTheories;


	StopWordFilter::showFilteredWords(_cvStream);

	_num_documents = 0;
	_docTheories = 0;


	PennDiscourseTreebank::finalize();
	TargetConnectives::finalize();
	StopWordFilter::finalize();
}



void DiscourseRelTrainer::cvMaxEntPTBParses() {
}
void DiscourseRelTrainer::trainP1AutoParses() {
	/*refer to DTCorefTrainer */
}

void DiscourseRelTrainer::trainP1PTBParses() {
	/*refer to DTCorefTrainer */
}

void DiscourseRelTrainer::trainEpochAutoParses() {
	/*refer to DTCorefTrainer */

}


void DiscourseRelTrainer::devTestMaxEntPTBParses() {
	/*refer to DTCorefTrainer */
}
void DiscourseRelTrainer::devTestMaxEntAutoParses() {
	/*refer to DTCorefTrainer */
}

void DiscourseRelTrainer::loadTrainingDataFromList(const char *listfile) {

	_num_documents = countDocumentsInFileList(listfile);
	_docTheories = _new DocTheory * [_num_documents];

	boost::scoped_ptr<UTF8InputStream> fileListStream_scoped_ptr(UTF8InputStream::build(listfile));
	UTF8InputStream& fileListStream(*fileListStream_scoped_ptr);	
	UTF8Token token;
	
	// in our case, one state file only contains information for one document
	// so state_file_name = _document_name
	//_stateFileNameList = (char **)malloc( _num_documents * sizeof(char *));
    
	int index = 0;
	while (!fileListStream.eof()) {
		fileListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		loadTrainingData(token.chars(), index);
	}

}


void DiscourseRelTrainer::loadTrainingData(const wchar_t *filename, int& index) {
	char state_file_name_str[501];
	StringTransliterator::transliterateToEnglish(state_file_name_str, filename, 500);
	SessionLogger::info("SERIF") << "Loading data from " << state_file_name_str << "\n";
	
	/* keep doc name */
	boost::regex expression("WSJ_(\\d+)-state-");
	string fullDocName = state_file_name_str;
	string::const_iterator start, end; 
	start = fullDocName.begin(); 
	end = fullDocName.end(); 
    boost::match_results<string::const_iterator> what; 
	boost::match_flag_type flags = boost::match_default;		

	if (regex_search(start, end, what, expression, flags)) { 
		// what[0] contains the string pattern matching the whole reg expression
		// what[1] contains the doc id name 
		std::string docId = string(what[1].first, what[1].second);
		//_stateFileNameList[index]= (char *) malloc(strlen(docId.c_str()) + 1);
		//strcpy(_stateFileNameList[index], docId.c_str());
		_stateFileNameList.push_back(docId);
	}

	StateLoader *stateLoader = _new StateLoader(state_file_name_str);
	int num_docs = TrainingLoader::countDocumentsInFile(filename);

	wchar_t state_tree_name[100];
	wcscpy(state_tree_name, L"DocTheory following stage: doc-values");
	
	for (int i = 0; i < num_docs; i++) {
		_docTheories[index] = _new DocTheory(static_cast<Document*>(0));
		_docTheories[index]->loadFakedDocTheory(stateLoader, state_tree_name);
		_docTheories[index]->resolvePointers(stateLoader);
		index++;
	}
	
	delete stateLoader;

}



void DiscourseRelTrainer::processSentence(string docName, string sentIndex, const TokenSequence *tokens, Parse *parse){
	SessionLogger::info("SERIF") << "handling file: "+docName+"  sentence: "+sentIndex+"\n";

	for (int tok=0; tok < tokens->getNTokens(); tok ++){
		char wordBuffer[501];
		Symbol lowerCaseWordSymbol=SymbolUtilities::lowercaseSymbol(tokens->getToken(tok)->getSymbol());
		StringTransliterator::transliterateToEnglish(wordBuffer, lowerCaseWordSymbol.to_string(), 500);	
		string word = wordBuffer;
		DiscourseRelObservation* _observation= _new DiscourseRelObservation();
		if (TASK == EXPLICIT_ALL || TargetConnectives::isInConnDict(word)){
			_observation->populate(tok, tokens, parse, false);
		
			/* temporarily used, may change later */
			std::ostringstream oss;
			oss << tok ;
			string SynNodeId = oss.str();
			int answer = 0;
		
			
			Symbol goldLabel = PennDiscourseTreebank::getLabelofExpConnective(docName, word, sentIndex, SynNodeId);
			if (goldLabel != Symbol(L"-EMPTY-")){
				answer = _tagSet->getTagIndex(goldLabel);
			}else{
				SessionLogger::info("SERIF") << word + " is not used as a connective\n";
				answer = _tagSet->getNoneTagIndex();
			}

			_observations.push_back(_observation);
			_keys.push_back(answer);

			/*
			if (MODEL_TYPE == MAX_ENT) {
				_maxEntDecoder->addToTraining(_observation, answer);
			}
			*/
		}
		delete _observation;
	}
}

void DiscourseRelTrainer::processSentence_crossSent(string docName, string sentIndex, int sent){
	SessionLogger::info("SERIF") << "handling file: "+docName+"  sentence: "+sentIndex+"\n";
	DiscourseRelObservation* _observation= _new DiscourseRelObservation();
	if (TASK == CROSS_SENT_ALL){
		// may need to modify later
		_observation->populateWithWords(sent, _tokenSequences, _parses);
		_observation->populateWithWordNet(sent, _tokenSequences, _parses);
		//_observation->populateWithNonStopWords(sent, _tokenSequences, _parses);
		//_observation->populateWithWordNetUsingStopWordLs(sent, _tokenSequences, _parses);
		_observation->populate();
		_observation->populateWithRichFeatures(sent, _tokenSequences, _entitySets, _mentionSets, _parses, _propSets, _featDebugStream);
		
		/* temporarily used, may change later */
		
		int answer = 0;
		Symbol goldLabel = PennDiscourseTreebank::getLabelofCrossSentRel(docName, sentIndex);
		if (goldLabel != Symbol(L"-EMPTY-")){
			vector<CrossSentRelation>* disRel=PennDiscourseTreebank::getCrossSentRel(docName, sentIndex);
			answer = _tagSet->getTagIndex(goldLabel);
			for (int i=0; i< (int)(* disRel).size(); i++){
				_observations.push_back(_observation);
				_keys.push_back(answer);
				_relTypes.push_back((* disRel)[i].getRelType());
				//_keysDetails.push_back(&(* disRel)[i])
			}
		}else{
			SessionLogger::info("SERIF") << sentIndex + " has no relation with its succussor\n";
			answer = _tagSet->getNoneTagIndex();
			_observations.push_back(_observation);
			_keys.push_back(answer);
			//_keysDetails.push_back(0);		
			_relTypes.push_back("-NONE-");
		}
			 
		//Symbol goldLabel = PennDiscourseTreebank::getLabelofCrossSentRel(docName, sentIndex);
		//if (goldLabel != Symbol(L"-EMPTY-")){
		//	answer = _tagSet->getTagIndex(goldLabel);
		//}else{
		//	std::cout << sentIndex + " has no relation with its successor\n";
		//	answer = _tagSet->getNoneTagIndex();
		//}

		//_observations.push_back(* _observation);
		//_keys.push_back(answer);
	}

	delete _observation;

}
		
void DiscourseRelTrainer::processDocument(DocTheory* docTheory, string docName) {

	for (int sent = 0; sent < docTheory->getNSentences(); sent++) {
		SessionLogger::info("SERIF") << "Process sentence " << sent
			 << "/" << docTheory->getNSentences()
			 << "...";
		SentenceTheory *sTheory = docTheory->getSentenceTheory(sent);
		
		/* temporarily used, may change later */
		std::ostringstream oss;
		oss<< sent;
		string sentIndex = oss.str();
		processSentence(docName, sentIndex, sTheory->getTokenSequence(), sTheory->getPrimaryParse());
	}
		
}	


void DiscourseRelTrainer::processDocument_crossSentRel(DocTheory* docTheory, string docName) {
	// initialize data structures such as proptree, parse, mentions ...
	int num_sentences = docTheory->getNSentences();
	//_docIds = _new Symbol[num_sentences];
	//_documentTopics = _new Symbol[num_sentences];
	//_secondaryParses = _new Parse *[num_sentences];
	//_npChunks = _new NPChunkTheory* [num_sentences];
	//_valueMentionSets = _new ValueMentionSet * [num_sentences];
	_tokenSequences = _new TokenSequence *[num_sentences];
	_parses = _new const Parse *[num_sentences];
	_mentionSets = _new MentionSet * [num_sentences];
	_propSets = _new const PropositionSet * [num_sentences];
	_entitySets = _new EntitySet * [num_sentences];
	//_eventMentionSets = _new EventMentionSet * [num_sentences];
	
	for (int sent = 0; sent < num_sentences ; sent++) {
		_tokenSequences[sent] = docTheory->getSentenceTheory(sent)->getTokenSequence();
		//_tokenSequences[sent]->gainReference();
		// ignore count reference method for our case now
		_parses[sent] = docTheory->getSentenceTheory(sent)->getPrimaryParse();
		_mentionSets[sent] = docTheory->getSentenceTheory(sent)->getMentionSet();
		_propSets[sent] = docTheory->getSentenceTheory(sent)->getPropositionSet();
		_entitySets[sent] = docTheory->getSentenceTheory(sent)->getEntitySet();
		//_propSets[sent]->fillDefinitionsArray();
		//_eventMentionSets[sent] = docTheory->getEventMentionSet();
	}

	for (int sent = 0; sent < num_sentences-1 ; sent++) {
		SessionLogger::info("SERIF") << "Process sentence " << sent
			 << "/" << docTheory->getNSentences()
			 << "...";

		/* temporarily used, may change later */
		std::ostringstream oss;
		oss<< sent;
		string sentIndex = oss.str();
		
		processSentence_crossSent(docName, sentIndex, sent);
	}

	//delete[] _docIds;
	//delete[] _documentTopics;
	//delete[] _secondaryParses;
	//delete[] _npChunks;
	//delete[] _valueMentionSets;
	delete[] _tokenSequences;
	delete[] _parses;
	delete[] _mentionSets;
	delete[] _propSets;
	delete[] _entitySets;
	//delete[] _eventMentionSets;
		
}	



/*void DiscourseRelTrainer::printDebugScores(int mentionID, int entityID, 
									  int hobbs_distance,
									  UTF8OutputStream& stream) {
	DiscourseRelObservation *observation = static_cast<DiscourseRelObservation*>(_observations[1]);
	observation->populate(mentionID, entityID, hobbs_distance);
	
	if (MODEL_TYPE == MAX_ENT) {
		int best_tag;
		_maxEntDecoder->decodeToDistribution(observation, _tagScores, _tagSet->getNTags(), &best_tag);
	
		int best = 0;
		int second_best = 0;
		double best_score = -100000;
		double second_best_score = -100000;
		for (int i = 0; i < _tagSet->getNTags(); i++) {
			if (_tagScores[i] > best_score) {
				second_best = best;
				second_best_score = best_score;
				best = i;
				best_score = _tagScores[i];
			} else if (_tagScores[i] > second_best_score) {
				second_best = i;			
				second_best_score = _tagScores[i];
			}
		}
		stream << _tagSet->getTagSymbol(best).to_string() << L": " << _tagScores[best] << L"<br>\n";
		stream << "<font size=1>\n";
		_maxEntDecoder->printHTMLDebugInfo(observation, best, stream);
		stream << "</font>\n";
		stream << _tagSet->getTagSymbol(second_best).to_string() << L": " << _tagScores[second_best] << L"<br>\n";
		stream << "<font size=1>\n";
		_maxEntDecoder->printHTMLDebugInfo(observation, second_best, stream);
		stream << "</font>\n";
	}
	else if (MODEL_TYPE == P1 || MODEL_TYPE == P1_RANKING) {
		decodeToP1Distribution(observation);

		int best = 0;
		int second_best = 0;
		double best_score = -100000;
		double second_best_score = -100000;
		for (int i = 0; i < _tagSet->getNTags(); i++) {
			if (_tagScores[i] > best_score) {
				second_best = best;
				second_best_score = best_score;
				best = i;
				best_score = _tagScores[i];
			} else if (_tagScores[i] > second_best_score) {
				second_best = i;			
				second_best_score = _tagScores[i];
			}
		}
		stream << _tagSet->getTagSymbol(best).to_string() << L": " << _tagScores[best] << L"<br>\n";
		stream << "<font size=1>\n";
		_p1Decoder->printHTMLDebugInfo(observation, best, stream);
		stream << "</font>\n";
		stream << _tagSet->getTagSymbol(second_best).to_string() << L": " << _tagScores[second_best] << L"<br>\n";
		stream << "<font size=1>\n";
		_p1Decoder->printHTMLDebugInfo(observation, second_best, stream);
		stream << "</font>\n";
	}

}



*/
/*void DiscourseRelTrainer::decodeToP1Distribution(DiscourseRelObservation *observation) {
	for (int i = 0; i < _tagSet->getNTags(); i++) {
		_tagScores[i] = _p1Decoder->getScore(observation, i);
	}
}

*/
void DiscourseRelTrainer::writeWeights(int epoch) {
	if (epoch != -1 && PRINT_EVERY_EPOCH == 0)
		return;

	UTF8OutputStream out;
	std::string modeltype;
	char file[600];
	if (MODEL_TYPE == MAX_ENT) {
		if (epoch == -1)
			sprintf(file, "%s-maxent", _model_file.c_str());
		else
			sprintf(file, "%s-maxent-epoch-%d", _model_file.c_str(), epoch);
	} else if (MODEL_TYPE == P1) {
		if (epoch == -1)
			sprintf(file, "%s-p1", _model_file.c_str());
		else
			sprintf(file, "%s-p1-epoch-%d", _model_file.c_str(), epoch);
	} else if (MODEL_TYPE == P1_RANKING) {
		if (epoch == -1)
			sprintf(file, "%s-rank", _model_file.c_str());
		else
			sprintf(file, "%s-rank-epoch-%d", _model_file.c_str(), epoch);
	}

	out.open(file);

	if (out.fail()) {
		throw UnexpectedInputException("DiscourseRelTrainer::writeWeights()",
			"Could not open model file for writing");
	}

	dumpTrainingParameters(out);
	if (MODEL_TYPE == MAX_ENT)
		DTFeature::writeWeights(*_maxEntWeights, out);
	else if (MODEL_TYPE == P1 || MODEL_TYPE == P1_RANKING)
		DTFeature::writeSumWeights(*_p1Weights, out);
	out.close();

}


void DiscourseRelTrainer::dumpTrainingParameters(UTF8OutputStream& out) {

	DTFeature::recordDate(out);

	out << L"Parameters:\n";
	
	DTFeature::recordParamForReference(Symbol(L"discourse_rel_training_file"), out);
	DTFeature::recordParamForReference(Symbol(L"discourse_rel_training_list_mode"), out);
	DTFeature::recordParamForConsistency(Symbol(L"word_cluster_bits_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"lc_word_cluster_bits_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"discourse_rel_tag_set_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"discourse_rel_features_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"discourse_rel_model_type"), out);

	DTFeature::recordParamForReference(Symbol(L"discourse_rel_train_source"), out);
	DTFeature::recordParamForReference(Symbol(L"wordnet_subtypes"), out);
	DTFeature::recordParamForReference(Symbol(L"desc_head_subtypes"), out);
	DTFeature::recordParamForReference(Symbol(L"partitive_headword_list"), out);
	DTFeature::recordParamForReference(Symbol(L"desc_types"), out);
	DTFeature::recordParamForReference(Symbol(L"word_net_dictionary_path"), out);
	DTFeature::recordParamForReference(Symbol(L"discourse_rel_model_type"), out);
	if (MODEL_TYPE == MAX_ENT) {
		DTFeature::recordParamForReference(Symbol(L"maxent_trainer_percent_held_out"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_trainer_max_iterations"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_trainer_gaussian_variance"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_trainer_min_likelihood_delta"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_trainer_stop_check_frequency"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_trainer_pruning_cutoff"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_trainer_mode"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_train_vector_file"), out);
		DTFeature::recordParamForReference(Symbol(L"maxent_test_vector_file"), out);
	} else if (MODEL_TYPE == P1 || MODEL_TYPE == P1_RANKING) {
		DTFeature::recordParamForReference(Symbol(L"discourse_rel_trainer_epochs"), out);
	}

	out << L"\n";

}


int DiscourseRelTrainer::countDocumentsInFileList(const char *filename) {

	boost::scoped_ptr<UTF8InputStream> tempFileListStream_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& tempFileListStream(*tempFileListStream_scoped_ptr);
	UTF8Token token;
	int num_documents = 0;
	while (!tempFileListStream.eof()) {
		tempFileListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		num_documents += 1;
	}
	tempFileListStream.close();

	SessionLogger::info("SERIF") << "\n" << num_documents << " documents in " << filename << "\n\n";
	return num_documents;
}

void DiscourseRelTrainer::printFeatures(UTF8OutputStream& out, DTObservation *_obs, int _key){
	DTFeature *featureBuffer[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	DTState state(_tagSet->getTagSymbol(_key), Symbol(), Symbol(), 0, std::vector<DTObservation*>(1, _obs));
	wchar_t str1[1000];
	wstring str=str1;
	//for (int i = 0; i < _featureTypes->getNFeaturesTypes(); i++) {
	for (int i = 2; i < _featureTypes->getNFeaturesTypes(); i++) {
		int n_features = _featureTypes->getFeatureType(i)->extractFeatures(state, featureBuffer);
		for (int j = 0; j < n_features; j++) {
			featureBuffer[j]->toString(str);
			out << "f[" << i << "] -- " << _featureTypes->getFeatureType(i)->getName() << " :  " << str << L"\n";		
		}
	}
	out << "\n\n";
}

// count positive/negative instances for each connective
// baseline accuracy for all and each connective
void DiscourseRelTrainer::dataAnalysis() {
	UTF8OutputStream _dataAnalyStream;

	std::string buffer = ParamReader::getRequiredParam("discourse_rel_data_analysis_out");
	_dataAnalyStream.open(buffer.c_str());

	loadTrainingDataFromList(_training_file_list.c_str());
	
	TargetConnectives::loadConnDict(_connectives_file.c_str());
	PennDiscourseTreebank::loadDataFromPDTBFileList (_pdtb_file_list.c_str(), TargetConnectives::getConnDict());

	vector<string>::iterator iterFileName = _stateFileNameList.begin();
	
	for (int i = 0; i < _num_documents; i++) {
		string DocId = *iterFileName;
		iterFileName ++;
		processDocument(_docTheories[i], DocId);
	}

	map<Symbol, ConnectiveCorpusStatistics> *_connStatisticsDict = _new map<Symbol, ConnectiveCorpusStatistics>;
	
	for (int i=0; i< (int)_keys.size(); i++){
		DiscourseRelObservation obs = *dynamic_cast<DiscourseRelObservation*>(_observations[i]);
		Symbol lcWord = obs.getLCWord();
		
		map<Symbol, ConnectiveCorpusStatistics>::iterator myIterator = _connStatisticsDict->find(lcWord);
		if(myIterator == _connStatisticsDict->end()){
			ConnectiveCorpusStatistics *ccs= _new ConnectiveCorpusStatistics(lcWord);
			(*_connStatisticsDict)[lcWord]=(*ccs);
		}
		(*_connStatisticsDict)[lcWord].readOneSample(_keys[i]);

	}
	double aveBaseline = 0;
	int totalSamples = 0;
	for(map<Symbol, ConnectiveCorpusStatistics>::iterator iter = _connStatisticsDict->begin(); iter != _connStatisticsDict->end(); iter++)
    {
		ConnectiveCorpusStatistics ccs = iter->second;
		int numOfNegSamples= ccs.totalNegativeSamples();
		int numOfPosSamples= ccs.totalPositiveSamples();
		int numOfSamples = ccs.totalSamples();
		double baseline=0;
		if (numOfNegSamples > numOfPosSamples){
			baseline = 1.0*numOfNegSamples/numOfSamples;
			aveBaseline += numOfNegSamples;
		}else{
			baseline = 1.0*numOfPosSamples/numOfSamples;
			aveBaseline += numOfPosSamples;
		}
		totalSamples += numOfSamples;
		_dataAnalyStream << "connective: " << (*iter).first << "\n positive samples--" << numOfPosSamples << "\n negative samples--" << numOfNegSamples << "\n baseline accuracy --" << baseline << "\n\n" ;    
    }
	aveBaseline /= totalSamples;
	_dataAnalyStream << "average baseline: " << aveBaseline << "\n\n" ;    
	
	delete _connStatisticsDict;
	_connStatisticsDict = 0;

	for (int j = 0; j < _num_documents; j++)
		delete _docTheories[j];
	delete [] _docTheories;


	_num_documents = 0;
	_docTheories = 0;


	PennDiscourseTreebank::finalize();
	TargetConnectives::finalize();
	StopWordFilter::finalize();

}
