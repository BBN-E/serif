// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/limits.h"
#include "common/ParamReader.h"
#include "common/UnexpectedInputException.h"
#include "common/UTF8InputStream.h"
#include "common/UTF8OutputStream.h"
#include "common/UTF8Token.h"
#include "theories/Token.h"
#include "WordClustering/WordClusterTable.h"
#include "WordClustering/WordClusterClass.h"
#include "names/IdFWordFeatures.h"
#include "discTagger/DTTagSet.h"
#include "discTagger/DTFeatureTypeSet.h"
#include "names/discmodel/PIdFFeatureType.h"
#include "discTagger/PDecoder.h"
#include "names/discmodel/TokenObservation.h"
#include "names/discmodel/PIdFFeatureTypes.h"
#include "names/discmodel/PIdFSentence.h"
#include "names/discmodel/PIdFDecoder.h"
#include "names/discmodel/PIdFTrainer.h"

#include <iostream>
#include <stdio.h>
#include <boost/scoped_ptr.hpp>

using namespace std;

#define PRINT_AFTER_EVERY_EPOCH false


PIdFTrainer::PIdFTrainer()
	: _featureTypes(0), _tagSet(0), _wordFeatures(0),
	  _decoder(0), _weights(0), _firstSentBlock(0), _lastSentBlock(0),
	  _curSentBlock(0), _cur_sent_no(0), _min_tot(1), _min_change(0)
{
	PIdFFeatureTypes::ensureFeatureTypesInstantiated();

	// Read parameters

	Symbol outputMode = ParamReader::getParam(L"pidf_trainer_output_mode");
	if (outputMode == Symbol(L"taciturn")) {
		_output_mode = TACITURN_OUTPUT;
	}
	else if (outputMode == Symbol(L"verbose")) {
		_output_mode = VERBOSE_OUTPUT;
	}
	else {
		throw UnexpectedInputException("PIdFTrainer::PIdFTrainer()",
			"Parameter 'pidf_trainer_output_mode' not specified");
	}
	
			
	Symbol seedFeatures =
		ParamReader::getParam(L"pidf_trainer_seed_features");
	if (seedFeatures == Symbol(L"true")) {
		_seed_features = true;
	}
	else if (seedFeatures == Symbol(L"false")) {
		_seed_features = false;
	}
	else {
		throw UnexpectedInputException("PIdFTrainer::PIdFTrainer()",
			"Parameter 'pidf_trainer_seed_fetures' not specified");
	}

	Symbol addHypFeatures =
		ParamReader::getParam(L"pidf_trainer_add_hyp_features");
	if (addHypFeatures == Symbol(L"true")) {
		_add_hyp_features = true;
	}
	else if (addHypFeatures == Symbol(L"false")) {
		_add_hyp_features = false;
	}
	else {
		throw UnexpectedInputException("PIdFTrainer::PIdFTrainer()",
			"Parameter 'pidf_trainer_add_hyp_features' not specified");
	}

	char param_epochs[10];
	if (!ParamReader::getParam("pidf_trainer_epochs",param_epochs, 10))	{
		throw UnexpectedInputException("PIdFTrainer::PIdFTrainer()",
			"Parameter 'pidf_trainer_epochs' not specified");
	}
	_epochs = atoi(param_epochs);
	if (ParamReader::getParam("pidf_trainer_min_tot",param_epochs, 10))	{
		_min_tot = atof(param_epochs);
	}
	if (ParamReader::getParam("pidf_trainer_min_change",param_epochs, 10))	{
		_min_change = atof(param_epochs);
		_min_change = _min_change/100;
	}
	SessionLogger::info("SERIF")<<"MinTot: "<<_min_tot<<std::endl;
	SessionLogger::info("SERIF")<<"MinChange: "<<_min_change<<std::endl;


	char param_weightsum_granularity[10];
	if (!ParamReader::getParam("pidf_trainer_weightsum_granularity",param_weightsum_granularity, 10))	{
		throw UnexpectedInputException("PIdFTrainer::PIdFTrainer()",
			"Parameter 'pidf_trainer_weightsum_granularity' not specified");
	}
	_weightsum_granularity = atoi(param_weightsum_granularity);

	if (!ParamReader::getParam("pidf_training_file",_training_file,									 500))	{
		throw UnexpectedInputException("PIdFTrainer::PIdFTrainer()",
			"Parameter 'pidf_training_file' not specified");
	}
	Symbol isfilelist = ParamReader::getParam(Symbol(L"pidf_traingingfile_is_list"));
	if(isfilelist == Symbol(L"true")){
		_traininglist = true;
	}
	else{
		_traininglist = false;
	}
	char nsent[10];
	if(ParamReader::getParam("pidf_ntrainsent",nsent, 10)){
		_nTrainSent = atoi(nsent);
	}
	else{
		_nTrainSent = 0;
	}
	if (!ParamReader::getParam("pidf_model_file",_model_file,									 500))	{
		throw UnexpectedInputException("PIdFTrainer::PIdFTrainer()",
			"Parameter 'pidf_model_file' not specified");
	}
	char history_buffer[500];
	strcpy(history_buffer, _model_file);
	strcat(history_buffer, ".hist.txt");
	_historyStream.open(history_buffer);
	if(_historyStream.fail()){
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::PIdFSimActiveLearningTrainer()",
			"Cant open history stream");
	}

	char features_file[500];
	if (!ParamReader::getParam("pidf_features_file",features_file,									 500))	{
		throw UnexpectedInputException("PIdFTrainer::PIdFTrainer()",
			"Parameter 'pidf_features_file' not specified");
	}
	_featureTypes = _new DTFeatureTypeSet(features_file, PIdFFeatureType::modeltype);

	char tag_set_file[500];
	if (!ParamReader::getParam("pidf_tag_set_file",tag_set_file,									 500))	{
		throw UnexpectedInputException("PIdFTrainer::PIdFTrainer()",
			"Parameter 'pidf_tag_set_file' not specified");
	}
	char interleave_tags[500];
	if (!ParamReader::getParam("pidf_interleave_tags",interleave_tags,									 500))	{
		throw UnexpectedInputException("PIdFDecoder::PIdFTrainder()",
			"Parameter 'pidf_interleave_tags' not specified");
	}
	if(strcmp(interleave_tags, "true") == 0){
		_interleave_tags = true;
	}
	else{
		_interleave_tags = false;
	}
	_tagSet = _new DTTagSet(tag_set_file, true, true, _interleave_tags);
	char learn_transitions[10];
	if (!ParamReader::getParam("pidf_learn_transitions",learn_transitions,									 10))	{
		throw UnexpectedInputException("PIdFDecoder::PIdFSimActiveLearningTrainer()",
			"Parameter 'pidf_learn_transitions' not specified");
	}
	if(strcmp(learn_transitions, "true") == 0){
		_learn_transitions_from_training = true;
	}
	else{
		_learn_transitions_from_training = false;
	}

	_wordFeatures = IdFWordFeatures::build();

	WordClusterTable::ensureInitializedFromParamFile();
}

PIdFTrainer::PIdFTrainer(
	enum output_mode_e output_mode, bool seed_features, bool add_hyp_features,
	int weightsum_granularity, int epochs, const char *features_file,
	const char *tag_set_file, const char *word_clusters_file,
	IdFWordFeatures *wordFeatures, const char *model_file)
	: _featureTypes(0), _tagSet(0), _decoder(0), _weights(0),
	  _output_mode(output_mode), _seed_features(seed_features),
	  _add_hyp_features(add_hyp_features),
	  _weightsum_granularity(weightsum_granularity), _epochs(epochs),
	  _wordFeatures(wordFeatures), _firstSentBlock(0), _lastSentBlock(0),
	  _curSentBlock(0), _cur_sent_no(0), _min_tot(1), _min_change(0)
{
	PIdFFeatureTypes::ensureFeatureTypesInstantiated();

	_featureTypes = _new DTFeatureTypeSet(features_file, PIdFFeatureType::modeltype);

	_tagSet = _new DTTagSet(tag_set_file, true, true);

	if (word_clusters_file != 0)
		WordClusterTable::initTable(word_clusters_file);

	strncpy(_model_file, model_file, 500);
}

PIdFTrainer::~PIdFTrainer() {
	delete _featureTypes;
	delete _wordFeatures;
	delete _tagSet;
}


void PIdFTrainer::addTrainingSentence(PIdFSentence &sentence) {
	if (_lastSentBlock == 0) {
		_firstSentBlock = _new SentenceBlock();
		_lastSentBlock = _firstSentBlock;
	}
	else if (_lastSentBlock->n_sentences == SentenceBlock::BLOCK_SIZE) {
		_lastSentBlock->next = _new SentenceBlock();
		_lastSentBlock = _lastSentBlock->next;
	}

	_lastSentBlock->sentences[_lastSentBlock->n_sentences++].populate(
																sentence);
}

void PIdFTrainer::addTrainingSentencesFromTrainingFile(const char *file) {
	if(_traininglist){
		addTrainingSentencesFromTrainingFileList(file);
		return;
	}

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	if (file == 0)
		in.open(_training_file);
	else
		in.open(file);

	if (in.fail()) {
		throw UnexpectedInputException(
			"PIdFTrainer::addTrainingSentencesFromFile()",
			"Unable to open training file.");
	}

	cout << "Reading training sentences from disk...\n";

	PIdFSentence idfSentence(_tagSet, MAX_SENTENCE_TOKENS);

	int sentence_n = 0;
	while (idfSentence.readTrainingSentence(in)) {
		if((_nTrainSent <= 0)){
			addTrainingSentence(idfSentence);
		}
		else{
			if(sentence_n < _nTrainSent){
				addTrainingSentence(idfSentence);
			}
			else{
				break;
			}
		}

		sentence_n++;

		if (sentence_n % 1000 == 0) {
			cout << sentence_n << "\n";
			cout.flush();
		}
		else if (_output_mode == VERBOSE_OUTPUT) {
			cout << sentence_n << "              \r";
		}
	}
	cout <<"Training on: "<<sentence_n<<" sentences"<<std::endl;
	cout << "\n";
}

void PIdFTrainer::train() {
	if(_learn_transitions_from_training){
		_tagSet->resetSuccessorTags();
		_tagSet->resetPredecessorTags();
		seekToFirstSentence();
		while (moreSentences()) {
			PIdFSentence *sentence = getNextSentence();
			Symbol prevTag = _tagSet->getStartTag();
			for(int i =0; i< sentence->getLength(); i++){
				Symbol nextTag = _tagSet->getTagSymbol(sentence->getTag(i));
				_tagSet->addTransition(prevTag, nextTag);
				prevTag = nextTag;
			}
			_tagSet->addTransition(prevTag, _tagSet->getEndTag());
		}

	}
	_weights = _new DTFeature::FeatureWeightMap(500009);
	_decoder = _new PDecoder(_tagSet, _featureTypes, _weights,
							 _add_hyp_features);

	if (_seed_features) {
		cout << "Seeding weight table with all features from training set...\n";
		cout.flush();
		addTrainingFeatures();
	}
	double prevtot=0;
	double tot =0;
	tot = trainEpoch();
	for (int epoch = 1; epoch < _epochs; epoch++) {
		cout << "Starting epoch " << epoch + 1 << "...\n";
		_historyStream << "Starting epoch " << epoch + 1 << "...\n";
		cout.flush();
		prevtot = tot;
		tot = trainEpoch();
		if(tot >= _min_tot){
			SessionLogger::info("SERIF")<<"breaking b/c tot: "<<tot<<" >= "<<_min_tot<<std::endl;
			break;
		}
		if((tot-prevtot) < _min_change){
			SessionLogger::info("SERIF")<<"breaking b/c change: "<<tot-prevtot<<" >= "<<_min_change<<std::endl;

			break;
		}
		_historyStream.flush();
		if (PRINT_AFTER_EVERY_EPOCH)
			writeWeights(epoch);
	}
	writeTransitions();


	writeWeights();
	_historyStream.close();


//	cerr << "Press enter to free weight tables and decoder...\n";
//	getchar();

	// this isn't really necessary in practice, but helpful for memory leak
	// detection
	for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
		 iter != _weights->end(); ++iter)
	{
		(*iter).first->deallocate();
	}
	delete _decoder;
	delete _weights;

//	cerr << "Press enter to continue...\n";
//	getchar();
}

void PIdFTrainer::addTrainingFeatures() {
	DTObservation *observations[MAX_SENTENCE_TOKENS+2];
	TokenObservation obsArray[MAX_SENTENCE_TOKENS+2];
	for (int i = 0; i < MAX_SENTENCE_TOKENS+2; i++)
		observations[i] = &obsArray[i];
	int tags[MAX_SENTENCE_TOKENS+2];

	Token blankToken(0, 0, Symbol(L"NULL"));
	Symbol blankLCSymbol = Symbol(L"NULL");
	Symbol blankWordFeatures = Symbol(L"NULL");
	WordClusterClass blankWordClass = WordClusterClass::nullCluster();

	seekToFirstSentence();
	int sentence_n = 0;
	while (moreSentences()) {
		PIdFSentence *sentence = getNextSentence();
		int n_observations = sentence->getLength() + 2;

		static_cast<TokenObservation*>(observations[0])->populate(
			blankToken, blankLCSymbol, blankWordFeatures, blankWordClass, 0, 0);
		for (int i = 0; i < sentence->getLength(); i++) {
			PIdFDecoder::populateObservation(
				static_cast<TokenObservation*>(observations[i+1]),
				_wordFeatures, sentence->getWord(i), i == 0);
		}
		static_cast<TokenObservation*>(observations[n_observations - 1])
			->populate(blankToken, blankLCSymbol, blankWordFeatures,
					   blankWordClass, 0, 0);

		tags[0] = _tagSet->getStartTagIndex();
		for (int j = 0; j < sentence->getLength(); j++)
			tags[j+1] = sentence->getTag(j);
		tags[n_observations-1] = _tagSet->getEndTagIndex();

		_decoder->addFeatures(n_observations, observations, tags, 1);

		sentence_n++;

		if (sentence_n % 1000 == 0) {
			cout << sentence_n << ": " << (int) _weights->size() << "\n";
/*				<< (int)_weights->get_path_length() << "\r";
				<< (int)_weights->get_num_lookup_eqs() << "/"
				<< (int)_weights->get_num_lookups() << "\r"; */
			cout.flush();
		}
		else if (_output_mode == VERBOSE_OUTPUT) {
			cout << sentence_n << "              \r";
		}
	}

	cout << "\n";
}

double PIdFTrainer::trainEpoch() {
	DTObservation *observations[MAX_SENTENCE_TOKENS+2];
	TokenObservation obsArray[MAX_SENTENCE_TOKENS+2];
	for (int i = 0; i < MAX_SENTENCE_TOKENS+2; i++)
		observations[i] = &obsArray[i];
	int tags[MAX_SENTENCE_TOKENS+2];

	Token blankToken(0, 0, Symbol(L"NULL"));
	Symbol blankLCSymbol = Symbol(L"NULL");
	Symbol blankWordFeatures = Symbol(L"NULL");
	WordClusterClass blankWordClass = WordClusterClass::nullCluster();

	seekToFirstSentence();
	int sentence_n = 0;
	int n_correct = 0;
	int total_ncorrect = 0;
	while (moreSentences()) {
		PIdFSentence *sentence = getNextSentence();

		int n_observations = sentence->getLength() + 2;

		static_cast<TokenObservation*>(observations[0])->populate(
			blankToken, blankLCSymbol, blankWordFeatures, blankWordClass, 0, 0);
		for (int i = 0; i < sentence->getLength(); i++) {
			PIdFDecoder::populateObservation(
				static_cast<TokenObservation*>(observations[i+1]),
				_wordFeatures, sentence->getWord(i), i == 0);
		}
		static_cast<TokenObservation*>(observations[n_observations - 1])
			->populate(blankToken, blankLCSymbol, blankWordFeatures,
					   blankWordClass, 0, 0);

		tags[0] = _tagSet->getStartTagIndex();
		for (int j = 0; j < sentence->getLength(); j++)
			tags[j+1] = sentence->getTag(j);
		tags[n_observations-1] = _tagSet->getEndTagIndex();

		bool correct = _decoder->train(n_observations, observations, tags, 1);
		if (correct){
			n_correct++;
			total_ncorrect++;
		}

		sentence_n++;

		if (sentence_n % 1000 == 0) {
			cout << sentence_n << ": " << n_correct
				 << " (" << 100*n_correct/1000 << "%)"
				 << "; " << (int) _weights->size() << "      \n";
			_historyStream << sentence_n << ": " << n_correct
				 << " (" << 100*n_correct/1000 << "%)"
				 << "; " << (int) _weights->size() << "      \n";

/*				<< (int)_weights->get_path_length() << "\r";
				<< (int)_weights->get_num_lookup_eqs() << "/"
				<< (int)_weights->get_num_lookups() << "\r"; */
			cout.flush();
			n_correct = 0;
		}
		else if (_output_mode == VERBOSE_OUTPUT) {
			cout << sentence_n << ": " << n_correct
				 << " (" << 100*n_correct/(sentence_n % 1000) << "%)"
				 << "              \r";
		}

		// every so-often, add weights to _weightSums
		if (sentence_n % _weightsum_granularity == 0) {
			for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
				 iter != _weights->end(); ++iter)
			{
				(*iter).second.addToSum();
			}
		}
	}
	cout <<"final: "<< sentence_n << ": " << total_ncorrect
		 << " (" << 100*((double)total_ncorrect/sentence_n)<< "%)"
		 << "; " << (int) _weights->size() << "      \n";
		cout.flush();
	_historyStream <<"final: "<< sentence_n << ": " << total_ncorrect
		 << " (" << 100*((double)total_ncorrect/sentence_n)<< "%)"
		 << "; " << (int) _weights->size() << "      \n";


	return (double)total_ncorrect/sentence_n;
	cout << "\n";
}


void PIdFTrainer::writeWeights(int epoch) {
	UTF8OutputStream out;
	if (epoch != -1) {
		char file[600];
		sprintf(file, "%s-epoch-%d", _model_file, epoch);
		out.open(file);
	} else out.open(_model_file);

	if (out.fail()) {
		throw UnexpectedInputException("PIdFTrainer::writeWeights()",
			"Could not open model file for writing");
	}

	for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
		 iter != _weights->end(); ++iter)
	{
		DTFeature *feature = (*iter).first;

		out << L"((" << feature->getFeatureType()->getName().to_string()
			<< L" ";
		feature->write(out);
		out << L") " << (*iter).second.getSum() << L")\n";
	}

	out.close();
}
void PIdFTrainer::writeTransitions(int epoch) {
	char file[600];
	if (epoch != -1) {
		sprintf(file, "%s-epoch-%d-transitions", _model_file, epoch);

	}
	else{
		sprintf(file, "%s-transitions", _model_file);

	}
	_tagSet->writeTransitions(file);
}


void PIdFTrainer::seekToFirstSentence() {
	_curSentBlock = _firstSentBlock;
	_cur_sent_no = 0;
}

bool PIdFTrainer::moreSentences() {
	return _curSentBlock != 0 &&
		   _cur_sent_no < _curSentBlock->n_sentences;
}

PIdFSentence *PIdFTrainer::getNextSentence() {
	if (moreSentences()) {
		PIdFSentence *result = &_curSentBlock->sentences[_cur_sent_no];

		// move to next sentence for next call
		_cur_sent_no++;
		if (_cur_sent_no == SentenceBlock::BLOCK_SIZE) {
			_curSentBlock = _curSentBlock->next;
			_cur_sent_no = 0;
		}

		return result;
	}
	else {
		return 0;
	}
}
void PIdFTrainer::addTrainingSentencesFromTrainingFileList(const char *file) {
	boost::scoped_ptr<UTF8InputStream> filelist_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& filelist(*filelist_scoped_ptr);
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	if (file == 0)
		filelist.open(_training_file);
	else
		filelist.open(file);

	if (filelist.fail()) {
		throw UnexpectedInputException(
			"PIdFTrainer::addSentencesFromFileList()",
			"Unable to open traing file list.");
	}
	int nfiles;
	filelist >> nfiles;
	UTF8Token token;
	cout << "Reading traing sentences from disk...\n";
	int sentence_n = 0;

	for(int i=0; i< nfiles; i++){
		if (filelist.eof())
			throw UnexpectedInputException("PIdFTrainer::addSentencesFromTrainingFileList",
				"fewer training files than specified in file");
		filelist >> token;
		cout<< "reading "<<token.symValue().to_debug_string()<< "\n";
		in.open(token.symValue().to_string());
		if (in.fail()) {
			throw UnexpectedInputException(
				"PIdFSimActiveLearningTrainer::addActiveLarningSentencesFromFile()",
				"Unable to open file from active learning file list.");
		}


		PIdFSentence idfSentence(_tagSet, MAX_SENTENCE_TOKENS);

		while (idfSentence.readTrainingSentence(in)) {
			addTrainingSentence(idfSentence);
			sentence_n++;
			if (sentence_n % 1000 == 0) {
				cout << sentence_n << "\n";
				cout.flush();
			}
			else if (_output_mode == VERBOSE_OUTPUT) {
				cout << sentence_n << "              \r";
			}
		}

		cout << "\n";
		in.close();
	}
	filelist.close();


}

