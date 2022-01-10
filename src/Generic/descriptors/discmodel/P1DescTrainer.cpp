// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/state/StateLoader.h"
#include "Generic/theories/SentenceTheoryBeam.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/P1Decoder.h"
#include "Generic/maxent/MaxEntModel.h"
#include "Generic/descriptors/discmodel/P1DescTrainer.h"
#include "Generic/descriptors/discmodel/P1DescFeatureTypes.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/NPChunkTheory.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/common/NgramScoreTable.h"

#include "Generic/common/NationalityRecognizer.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"
#include "Generic/theories/PropositionSet.h"

#include <iostream>
#include <stdio.h>
#include <boost/scoped_ptr.hpp>


using namespace std;


P1DescTrainer::P1DescTrainer()
	: _featureTypes(0), _tagSet(0),
	  _p1Decoder(0), _maxentDecoder(0), 
	  _weights(0), _num_sentences(0), 
	  _trainingLoader(0), _morphAnalysis(0)
{
	//arabic needs the MorphologicalAnalyzer to load state files
	_morphAnalysis = MorphologicalAnalyzer::build();
	// MODEL TYPE
	std::string model_type = ParamReader::getRequiredParam("desc_model_type");
	if (model_type == "P1") {
		_model_type = P1;
	}
	else if (model_type == "MAXENT") {
		_model_type = MAXENT;
	}
	else {
		std::string error = "Parameter 'desc_model_type' must be set to 'P1' or 'MAXENT'";
		throw UnexpectedInputException("P1DescTrainer::P1DescTrainer()", error.c_str());
	}

	// TASK (descriptor classification or premod-classification)
	std::string buffer = ParamReader::getRequiredParam("p1_desc_task");
	if	(buffer == "desc-classify"){
		_task = DESC_CLASSIFY;
	}
	else if	(buffer == "premod-classify"){
		_task = PREMOD_CLASSIFY;
	}
	else if(buffer == "desc-premod-classify") {
		_task = DESC_PREMOD_CLASSIFY;
	}
	else if(buffer == "pronoun-classify") {
		_task = PRONOUN_CLASSIFY;
	}
	else {
		throw UnexpectedInputException("P1DescTrainer::P1DescTrainer()",
			"Parameter 'p1_desc_task' should be set to 'desc-classify', "
			"'premod-classify', 'desc-premod-classify' or 'pronoun-classify'.");
	}

	_use_wordnet = ParamReader::getRequiredTrueFalseParam("p1_desc_use_wordnet");
	P1DescFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();
	_observation = _new DescriptorObservation(_use_wordnet);
	_num_sentences = 0;

	// TAG SET
	std::string tag_set_file;
	if (_task == PREMOD_CLASSIFY) {
		tag_set_file = ParamReader::getRequiredParam("p1_nom_premod_tag_set_file");
	} else if (_task == PRONOUN_CLASSIFY) {
		tag_set_file = ParamReader::getRequiredParam("p1_pronoun_type_tag_set_file");
	} else {
		tag_set_file = ParamReader::getRequiredParam("p1_desc_tag_set_file");
	}
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);

	// FEATURES
	std::string features_file;
	if (_task == PREMOD_CLASSIFY) {
		features_file = ParamReader::getRequiredParam("p1_nom_premod_features_file");
	} else if (_task == PRONOUN_CLASSIFY) {
		features_file = ParamReader::getRequiredParam("p1_pronoun_type_features_file");
	} else {
		features_file = ParamReader::getRequiredParam("p1_desc_features_file");
	}
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), P1DescFeatureType::modeltype);

	// TRAINING ONLY FEATURES (not devtest)
	if (!ParamReader::isParamTrue("p1_desc_devtest")) {
		
		if (_model_type == P1) {			
			_epochs = ParamReader::getRequiredIntParam("p1_trainer_epochs");	
			_seed_features = ParamReader::getRequiredTrueFalseParam(L"p1_trainer_seed_features");
			_add_hyp_features = ParamReader::getRequiredTrueFalseParam(L"p1_trainer_add_hyp_features");
			_weightsum_granularity = ParamReader::getRequiredIntParam("p1_trainer_weightsum_granularity");
		}
		else {
			_epochs = 1;
			_seed_features = false;
			_add_hyp_features = false;
			_weightsum_granularity = 0;
		}

		if (_model_type == MAXENT) {
			// PERCENT HELD OUT
			_percent_held_out = ParamReader::getRequiredIntParam("maxent_trainer_percent_held_out");
			if (_percent_held_out < 0 || _percent_held_out > 50) 
				throw UnexpectedInputException("P1DescTrainer::P1DescTrainer()",
				"Parameter 'p1_trainer_percent_held_out' must be between 0 and 50");

			// MAX NUMBER OF ITERATIONS (STOPPING CONDITION)
			_max_iterations = ParamReader::getRequiredIntParam("maxent_trainer_max_iterations");

			// GAUSSIAN PRIOR VARIANCE
			_variance = ParamReader::getRequiredFloatParam("maxent_trainer_gaussian_variance");
			
			// MIN CHANGE IN LIKELIHOOD (STOPPING CONDITION)
			_likelihood_delta = ParamReader::getOptionalFloatParamWithDefaultValue("maxent_trainer_min_likelihood_delta", .0001);

			// FREQUENCY OF STOPPING CONDITION CHECKS (NUM ITERATIONS)
			_stop_check_freq = ParamReader::getOptionalIntParamWithDefaultValue("maxent_trainer_stop_check_frequency", 1);

		}
		else {
			_percent_held_out = 0;
			_max_iterations = 100;
			_variance = 0;
			_likelihood_delta = .0001;
			_stop_check_freq = 1;
		}
	}

	// MODEL FILE NAME
	if (_task == PREMOD_CLASSIFY) {
		_model_file = ParamReader::getRequiredParam("p1_nom_premod_model_file");
	} else if (_task == PRONOUN_CLASSIFY) {
		_model_file = ParamReader::getRequiredParam("p1_pronoun_type_model_file");
	} else {
		_model_file = ParamReader::getRequiredParam("p1_desc_model_file");
	}

	// DEBUGGING FILES
	_training_vectors_file = ParamReader::getParam("training_vectors_file");

	if (_training_vectors_file != "") {
		SessionLogger::info("SERIF") << "got training vectors param" << _training_vectors_file << std::endl;
	}


	// TRAINING DATA FILE LIST
	_training_file_basename = ParamReader::getRequiredParam("p1_training_file_list");

	// BEAM WIDTH
	// JSM: this is needed for Chinese - beam isn't always 1
	_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("beam_width", 1);
	
	//Loading all of the training data at once can make you run out of memory.  Avoid this by 
	//splitting your training-list into several training lists and loading each list separately
	//This will slow down training, because each batch needs to be loaded once every epoch
	_n_training_batches = ParamReader::getOptionalIntParamWithDefaultValue("training_split", 1);
	if (_n_training_batches >= MAX_BATCHES) {
		throw UnexpectedInputException("P1DescTrainer::P1DescTrainer()",
			"Parameter 'training_split' is too large");
	}
	if (_n_training_batches > 1) {
		//rather than creating lists of lists, make training subfiles during training
		_subtraining_file_prefix = _model_file + "-subfile-list";
		makeTrainingSubfiles(_training_file_basename.c_str(), _subtraining_file_prefix.c_str());
	}

	// CREATE WEIGHTS TABLE
	_weights = _new DTFeature::FeatureWeightMap(50000);

	_correct = 0;
	_missed = 0;
	_spurious = 0;
	_wrong_type = 0;
}

P1DescTrainer::~P1DescTrainer() {
	delete _featureTypes;
	delete _tagSet;
	delete _morphAnalysis;
}

int P1DescTrainer::loadBatchTrainingData(const char* file_list, int batch_num){
	//if there are existing parses, etc delete them
	//if there is an existing training loader delete it
	if (_trainingLoader != 0)
		delete _trainingLoader;
	for (int i = 0; i < _num_sentences; i++) {
		delete _parses[i];
		delete _npChunks[i];
		delete _mentionSets[i];
		delete _propSets[i];
		_parses[i] = 0;
		_npChunks[i] = 0;
		_mentionSets[i] = 0;
		_propSets[i] = 0;
	}
	char buffer[1000];
	sprintf(buffer, "%s.%d.txt", file_list, batch_num);
	_num_sentences = loadTrainingData(buffer, _n_sent_per_batch[batch_num]);
	SessionLogger::info("SERIF") << "Batch: " << batch_num << " has " << _num_sentences
			  << " sentences " << std::endl;
	_n_sent_per_batch[batch_num] = _num_sentences;
	return _num_sentences;
}

int P1DescTrainer::loadTrainingData(const char* file_list, int n_sentences) {
	if (n_sentences == 0) {
		try {
			_trainingLoader = _new TrainingLoader(file_list, L"doc-relations-events");
		}catch(UnexpectedInputException e){
			_trainingLoader = _new TrainingLoader(file_list, L"sent-level-end");
		}
	}
	else {
		try {
			_trainingLoader = _new TrainingLoader(file_list, L"doc-relations-events", n_sentences);
		}catch (UnexpectedInputException e){
			_trainingLoader = _new TrainingLoader(file_list, L"sent-level-end", n_sentences);
		}
	}
	int max_sentences = _trainingLoader->getMaxSentences();
	int i;
	_parses = _new Parse *[max_sentences];
	_npChunks = _new NPChunkTheory *[max_sentences];
	_mentionSets = _new MentionSet *[max_sentences];
	_propSets = _new PropositionSet *[max_sentences];
	int nwords = 0;
	for (i = 0; i < max_sentences; i++) {
		SentenceTheory *theory = _trainingLoader->getNextSentenceTheory();
		if (theory == 0)
			break;
		//std::cerr<<"sentenceL "<<i<<" wordcount: "<<theory->getTokenSequence()->getNTokens()<<std::endl;
		nwords += theory->getTokenSequence()->getNTokens();

		_parses[i] = theory->getPrimaryParse();
		_parses[i]->gainReference();
		//have to add a reference to the np chunk theory, because it may be 
		//the source of the parse, if the reference isn't gained, the parse 
		//will be deleted when result is deleted
		_npChunks[i] = theory->getNPChunkTheory();
		if (_npChunks[i] != 0) {
			_npChunks[i]->gainReference();
		}
		_mentionSets[i] = theory->getMentionSet();
		_mentionSets[i]->gainReference();

		_propSets[i] = theory->getPropositionSet();
		_propSets[i]->gainReference();
	}
	SessionLogger::info("SERIF") << "total tokens: " << nwords << std::endl;
	return i;
}

void P1DescTrainer::devTest() {
	std::string buffer = ParamReader::getRequiredParam("p1_desc_devtest_out");
	_devTestStream.open(buffer.c_str());
	_num_sentences = loadTrainingData(_training_file_basename.c_str());

	std::string model_name = "";
	if (_model_type == P1)
		model_name = _model_file + ".p1";
	if (_model_type == MAXENT)
		model_name = _model_file + ".maxent";
	DTFeature::readWeights(*_weights, model_name.c_str(), P1DescFeatureType::modeltype);

	if (_model_type == P1) 
		_p1Decoder = _new P1Decoder(_tagSet, _featureTypes, _weights, _add_hyp_features);
	else if (_model_type == MAXENT) 
		_maxentDecoder = _new MaxEntModel(_tagSet, _featureTypes, _weights);

	std::ostringstream ostr;
	for (int i = 0; i < _num_sentences; i++) {
		if (i % 10 == 0) {
			ostr << i << " \r";
		}
		walkThroughSentence(i, DEVTEST);
	}
	ostr << "\n";
	SessionLogger::info("SERIF") << ostr.str();

	double recall = (double) _correct / (_missed + _wrong_type + _correct);
	double precision = (double) _correct / (_spurious + _wrong_type + _correct);

	_devTestStream << "CORRECT: " << _correct << "<br>\n";
	_devTestStream << "MISSED: " << _missed << "<br>\n";
	_devTestStream << "SPURIOUS: " << _spurious << "<br>\n";
	_devTestStream << "WRONG TYPE: " << _wrong_type << "<br><br>\n";
	_devTestStream << "RECALL: " << recall << "<br>\n";
	_devTestStream << "PRECISION: " << precision << "<br>\n";

}

void P1DescTrainer::train() {
	if (_training_vectors_file != "") {
		SessionLogger::info("SERIF") << "Vectors will be written to " << _training_vectors_file << "\n";
	}
	if (_model_type == P1) {
		_p1Decoder = _new P1Decoder(_tagSet, _featureTypes, _weights, _add_hyp_features);
		if (_training_vectors_file != "") {
			_p1Decoder->setLogFile(_training_vectors_file);
		}
	}
	else if (_model_type == MAXENT) {
		_maxentDecoder = _new MaxEntModel(_tagSet, _featureTypes, _weights, 
										MaxEntModel::SCGIS, _percent_held_out, 
										_max_iterations, _variance, _likelihood_delta, 
										_stop_check_freq, 
										_training_vectors_file.empty()?0:_training_vectors_file.c_str(),
										_heldout_vectors_file.empty()?0:_heldout_vectors_file.c_str());
	}

	if (_model_type == P1 && _seed_features && !_add_hyp_features) {
		SessionLogger::info("SERIF") << "Seeding weight table with all features from training set...\n";
		for (int j = 0; j < _n_training_batches; j++) {
			if (_n_training_batches == 1) {
				_num_sentences = loadTrainingData(_training_file_basename.c_str()); //load from original list
			}
			else {
				_num_sentences = loadBatchTrainingData(_subtraining_file_prefix.c_str(), j); //load from model list, saved in outdir
			}
			for (int i = 0; i < _num_sentences; i++) {
				walkThroughSentence(i, ADD_FEATURES);
			}	
		}
		for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
			iter != _weights->end(); ++iter)
		{
			(*iter).second.addToSum();
		}
		writeWeights(0);
	}

	SessionLogger::info("SERIF") << "\n";
	//checkHWConsistency();
	for (int epoch = 0; epoch < _epochs; epoch++) {
		SessionLogger::info("SERIF") << "Epoch " << epoch + 1 << "...\n";
		for(int batch = 0; batch < _n_training_batches; batch++) {
			SessionLogger::info("SERIF") << "\tBatch: " << batch << " n batches total: " << _n_training_batches << std::endl;
			if (_n_training_batches == 1) {
				if (epoch == 0) {
					SessionLogger::info("SERIF") << "load files with only 1 batch" << std::endl;
					_num_sentences = loadTrainingData(_training_file_basename.c_str());
					//checkHWConsistency();
				}
			}
			else {
				SessionLogger::info("SERIF") << "load files every time" << std::endl;
				_num_sentences = loadBatchTrainingData(_subtraining_file_prefix.c_str(), batch);
			}
			trainEpoch();
		}
		writeWeights(epoch+1);
	}

	// if _weightsum_granularity == 0, we didn't do weight averaging,
	// so we must transfer the weights to the weightsums now, so they
	// get printed out by writeWeights()
	if (_model_type == P1 && _weightsum_granularity == 0) {
		for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
			iter != _weights->end(); ++iter)
		{
			(*iter).second.addToSum();
		}
	}

	writeWeights();

	// this isn't really necessary in practice, but helpful for memory leak
	// detection
	for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
		 iter != _weights->end(); ++iter)
	{
		(*iter).first->deallocate();
	}
	delete _weights;
	delete _p1Decoder;
	delete _maxentDecoder;
}


void P1DescTrainer::checkHWConsistency(){
	NgramScoreTable* hw_table = _new NgramScoreTable(1, _num_sentences * 10);
	NgramScoreTable* hw_tag_table = _new NgramScoreTable(2, _num_sentences * 10);
	UTF8OutputStream uos1;
	UTF8OutputStream uos2;
	uos1.open("hw_consistency_multtypes.txt");
	uos2.open("hw_singletype.txt");
	
	//put all mention hws and hw + entity types in score tables
	Symbol ngram[2];

	for (int i = 0; i < _num_sentences; i++) {
		int nments = _mentionSets[i]->getNMentions();
		int ncorrect = 0;
		int n_valid_ments = 0;
		int n_uniq_valid_ments = 0;
		for (int j = 0; j < nments; j++) {
			const Mention *ment = _mentionSets[i]->getMention(j);
			const SynNode *node = ment->getNode();
			if (isInTrainingSet(ment)) {
				n_valid_ments++;
				ngram[0] = node->getHeadWord();
				int type = getEntType(ment, _mentionSets[i]);
				ngram[1] = EntityType::getType(type).getName();
				if(hw_table->lookup(ngram) <= 0){
					n_uniq_valid_ments++;
				}
				hw_table->add(ngram);
				hw_tag_table->add(ngram);
				
			}
		}
	}
	int n_types =0;
	Symbol types[8];
	float type_counts[8];
	//iterate through the table of head_words
	int nent = EntityType::getNTypes();

	NgramScoreTable::Table::iterator it;
	for(it = hw_table->get_start(); it != hw_table->get_end() ; ++it){
		
		Symbol this_word = (*it).first[0];
		double total_count = (*it).second;
		n_types =0;
		for(int i = 0; i < nent; i++){
			ngram[0] = this_word;
			ngram[1] = EntityType::getType(i).getName();
			float count = hw_tag_table->lookup(ngram);
			if(count >0){
				types[n_types] = ngram[1];
				type_counts[n_types] = count;
				n_types++;
			}
		}
		if(n_types == 0){
			SessionLogger::err("SERIF")<<"no matching type for :"<<this_word.to_debug_string()<<std::endl;
		}
		else if(n_types == 1){
			if((types[0] != Symbol(L"OTH") ) &&(types[0] != Symbol(L"NONE"))){
				uos2<<this_word<<" \t"<<types[0]<<" \t"<<total_count<<"\n";
			}
		}
		else{
			uos1<<"Problem HW: "<<this_word<<" \t"<<total_count;
			for(int j =0; j< n_types; j++){
				uos1<<" \t"<<types[j]<<" \t"<<type_counts[j];
			}
			uos1<<"\n";
			for (int i = 0; i < _num_sentences; i++) {
				int nments = _mentionSets[i]->getNMentions();
				int ncorrect = 0;
				int n_valid_ments = 0;
				int n_uniq_valid_ments = 0;
				for (int j = 0; j < nments; j++) {
					const Mention *ment = _mentionSets[i]->getMention(j);
					const SynNode *node = ment->getNode();
					if(node == 0){
						continue;
					}
					if (isInTrainingSet(ment)) {
						if(node->getHeadWord() == this_word){
							uos1 <<"Mention: "<<ment->getIndex()<<" "<<
								ment->getTypeString(ment->getMentionType())
								<<" "<<ment->getEntityType().getName().to_debug_string()<<"\n";
							uos1<<"Matching Mention in Node: "<<node->getID()<<"\n";
							if(node->getParent() != 0){
								uos1 << node->getParent()->toDebugString(0).c_str()<<"\n";
							}
							else{
								uos1 << node->toDebugString(0).c_str()<<"\n";
							}

						}
					}
				}
			}
			uos1<<"\n";
		}
			
	}
	uos1.close();
	uos2.close();
	delete hw_table;
	delete hw_tag_table;


}

void P1DescTrainer::trainEpoch() {
	double sumPercentCorrect = 0;
	for (int i = 0; i < _num_sentences; i++) {
		if (i % 1000 == 0) {
			//cout << i << "\n";
			if (i == 0) 
				SessionLogger::info("SERIF") << i << "\n";
			else
				SessionLogger::info("SERIF") << i << ": " << 100 * ((double) sumPercentCorrect/i) << "%\n";
		}
		
		sumPercentCorrect += walkThroughSentence(i, TRAIN);

		// if _weightsum_granularity == 0, we don't do weight averaging
		if (_model_type == P1 && _weightsum_granularity > 0 && i % _weightsum_granularity == 0) {
			for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
				iter != _weights->end(); ++iter)
			{
				(*iter).second.addToSum();
			}
		}
	}
	SessionLogger::info("SERIF") << "Final - nsent: " << _num_sentences << " : " 
		 << 100*((double) sumPercentCorrect/_num_sentences) << "%\n";
}

double P1DescTrainer::walkThroughSentence(int index, Mode mode) {

	if (mode == DEVTEST) {
		_devTestStream << _mentionSets[index]->getParse()->getRoot()->toTextString() << "<br>\n";
	}

	int nments = _mentionSets[index]->getNMentions();
	int ncorrect = 0;
	int nvalid = 0;
	for (int i = 0; i < nments; i++) {
		const Mention *ment = _mentionSets[index]->getMention(i);

		if (isInTrainingSet(ment)) {
			nvalid++;

			_observation->populate(_mentionSets[index], ment->getNode(), _propSets[index]);

			int answer = getEntType(ment, _mentionSets[index]);
			if (mode == TRAIN) {
				if (_model_type == P1) {
					Symbol prediction = _p1Decoder->decodeToSymbol(_observation);
					bool correct = _p1Decoder->train(_observation, answer);
					if (correct) {
						//if((ment->getEntityType().getName() != Symbol(L"OTH") ) &&
						//	(ment->getEntityType().getName() != Symbol(L"NONE"))){
						//	std::cout<<"Correct for: "<<_observation->getNode()->getHeadWord().to_debug_string()<<" "<<
						//	ment->getEntityType().getName()<<std::endl;
						//	}
						ncorrect++;
					}
					else{
						//std::cout<<"Wrong for: "<<_observation->getNode()->getHeadWord().to_debug_string()<<" "<<
						//	ment->getEntityType().getName()<<" predicted: "<<prediction.to_debug_string()<<
						//	std::endl;
					}
				}
				else if (_model_type == MAXENT) {
					_maxentDecoder->addToTraining(_observation, answer);
				}
			} 
			else if (mode == ADD_FEATURES && _model_type == P1) {
				_p1Decoder->addFeatures(_observation, answer, 0);
			} 
			else if (mode == DEVTEST) {
				Symbol correctAnswer = _tagSet->getTagSymbol(answer);
				Symbol hypothesis;
				if (_model_type == P1)
					hypothesis = _p1Decoder->decodeToSymbol(_observation);
				else if (_model_type == MAXENT)
					hypothesis = _maxentDecoder->decodeToSymbol(_observation);
				//use a Nationality hack to make sure that Nationality HWs are 
				//treated as PER-Desc.  This occurs in the DescRecognizer, but
				//shows up as errors in the devtest
				if (NationalityRecognizer::isNamePersonDescriptor(ment->node)) {
					hypothesis = Symbol(L"PER");
				}
				
				if (correctAnswer != _tagSet->getNoneTag() ||
					hypothesis != _tagSet->getNoneTag())
				{
					_devTestStream << ment->getNode()->getHeadWord().to_string();
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
	if (mode == TRAIN) {
		if (nvalid == 0) 
			return 0;
		else {
			//std::cout<<"temp: "<<(double)ncorrect/nvalid<<std::endl;
			return (double)ncorrect/nvalid;
		}
	}
	return 0;
}


void P1DescTrainer::writeWeights(int epoch) {
	UTF8OutputStream out;
	
	if (_model_type == P1) {
		if (epoch != -1) {
			char file[600];
			sprintf(file, "%s-epoch-%d.p1", _model_file.c_str(), epoch);
			out.open(file);
		} else {
			char file[550];
			sprintf(file, "%s.p1", _model_file.c_str());
			out.open(file);
		}

		if (out.fail()) {
			throw UnexpectedInputException("P1DescTrainer::writeWeights()",
				"Could not open model file for writing");
		}

		dumpTrainingParameters(out);
		DTFeature::writeSumWeights(*_weights, out);
		out.close();
	}
	else if (_model_type == MAXENT) {
		if (epoch == -1) {
			int pruning = ParamReader::getRequiredIntParam("maxent_trainer_pruning_cutoff");
			_maxentDecoder->deriveModel(pruning);
			char file[550];
			sprintf(file, "%s.maxent", _model_file.c_str());
			out.open(file);
			if (out.fail()) {
				throw UnexpectedInputException("P1DescTrainer::writeWeights()",
					"Could not open model file for writing");
			}
			dumpTrainingParameters(out);
			DTFeature::writeWeights(*_weights, out);
			out.close();
		}
	}
}

void P1DescTrainer::dumpTrainingParameters(UTF8OutputStream &out) {

	DTFeature::recordDate(out);

	out << L"Parameters:\n";

	DTFeature::recordFileListForReference(Symbol(L"p1_training_file_list"), out);
		
	if (_task == PREMOD_CLASSIFY) {
		DTFeature::recordParamForConsistency(Symbol(L"p1_nom_premod_tag_set_file"), out);
		DTFeature::recordParamForConsistency(Symbol(L"p1_nom_premod_features_file"), out);
	} else if (_task == PRONOUN_CLASSIFY) {
		DTFeature::recordParamForConsistency(Symbol(L"p1_pronoun_type_tag_set_file"), out);
		DTFeature::recordParamForConsistency(Symbol(L"p1_pronoun_type_features_file"), out);
	} else {
		DTFeature::recordParamForConsistency(Symbol(L"p1_desc_tag_set_file"), out);
		DTFeature::recordParamForConsistency(Symbol(L"p1_desc_features_file"), out);
	}
	DTFeature::recordParamForConsistency(Symbol(L"word_cluster_bits_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"lc_word_cluster_bits_file"), out);
	DTFeature::recordParamForReference(Symbol(L"wordnet_level_start"), out);
	DTFeature::recordParamForReference(Symbol(L"wordnet_level_interval"), out);

	DTFeature::recordParamForConsistency(Symbol(L"p1_desc_use_alt_models"), out);
	DTFeature::recordParamForReference(Symbol(L"alternative_p1_desc_model_file"), out);

	DTFeature::recordParamForReference(Symbol(L"desc_model_type"), out);
	DTFeature::recordParamForReference(Symbol(L"p1_desc_task"), out);
	DTFeature::recordParamForReference(Symbol(L"p1_desc_use_wordnet"), out);
	DTFeature::recordParamForReference(Symbol(L"pdesc_rare_hw_list"), out);
	DTFeature::recordParamForReference(Symbol(L"p1_trainer_epochs"), out);
	DTFeature::recordParamForReference(Symbol(L"p1_trainer_seed_features"), out);
	DTFeature::recordParamForReference(Symbol(L"p1_trainer_add_hyp_features"), out);
	DTFeature::recordParamForReference(Symbol(L"p1_trainer_weightsum_granularity"), out);
	DTFeature::recordParamForReference(Symbol(L"maxent_trainer_percent_held_out"), out);
	DTFeature::recordParamForReference(Symbol(L"maxent_trainer_max_iterations"), out);
	DTFeature::recordParamForReference(Symbol(L"maxent_trainer_gaussian_variance"), out);
	DTFeature::recordParamForReference(Symbol(L"maxent_trainer_min_likelihood_delta"), out);
	DTFeature::recordParamForReference(Symbol(L"maxent_trainer_stop_check_frequency"), out);
	DTFeature::recordParamForReference(Symbol(L"maxent_trainer_pruning_cutoff"), out);
	out << L"\n";

}

bool P1DescTrainer::isInTrainingSet(const Mention *mention) {
	if (_task == DESC_CLASSIFY) {
		return (mention->getMentionType() == Mention::DESC &&
				!NodeInfo::isNominalPremod(mention->getNode()));
	}
	else if (_task == PREMOD_CLASSIFY) {
		return (mention->getMentionType() == Mention::DESC &&
				NodeInfo::isNominalPremod(mention->getNode()));
	}
	else if (_task == DESC_PREMOD_CLASSIFY) { 
		return (mention->getMentionType() == Mention::DESC);
	}
	else { // _task == PRONOUN_CLASSIFY
		return (mention->getMentionType() == Mention::PRON); 
	}
}

int P1DescTrainer::getEntType(const Mention* ment, const MentionSet* mentSet){
	int answer = _tagSet->getNoneTagIndex();
	if (ment->getEntityType().isRecognized())
		answer = _tagSet->getTagIndex(ment->getEntityType().getName());
	return answer;

}

void P1DescTrainer::makeTrainingSubfiles(const char* training_list, const char* sublist_prefix){
	UTF8OutputStream uos_array[MAX_BATCHES];
	char buffer[1000];
	int i;
	for(i =0; i< _n_training_batches; i++){
		sprintf(buffer, "%s.%d.txt", sublist_prefix, i);
		uos_array[i].open(buffer);
		_n_sent_per_batch[i] = 0;
	}
	boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& uis(*uis_scoped_ptr);
	UTF8Token token;
	uis.open(training_list);
	while(!uis.eof()){
		for(i = 0; i < _n_training_batches; i++){
			if(uis.eof()){
				break;
			}
			uis >> token;
			if (wcscmp(token.chars(), L"") == 0)
				break;
			uos_array[i] << token.chars() <<"\n"; 
		}
	}
	for(i =0; i <_n_training_batches; i++){
		uos_array[i].close();
	}
}
