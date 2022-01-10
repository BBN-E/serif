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
#include "names/discmodel/PIdFSimActiveLearningTrainer.h"
#include "common/UTF8Token.h"
#include <boost/scoped_ptr.hpp>


using namespace std;

#define PRINT_AFTER_EVERY_EPOCH false

SentAndScore::SentAndScore(): margin(0), sent(0){};
SentAndScore::SentAndScore(double m, PIdFSentence* s, int num): margin(m), sent(s), id(num) {};


PIdFSimActiveLearningTrainer::PIdFSimActiveLearningTrainer()
	: _featureTypes(0), _tagSet(0), _wordFeatures(0),
	  _decoder(0), _weights(0), _firstSentBlock(0), _lastSentBlock(0),
	  _curSentBlock(0),_firstALSentBlock(0), _lastALSentBlock(0),
	  _curALSentBlock(0), _cur_sent_no(0), _currentWeights(0), _n_ALSentences(0),
	  _min_tot(1), _min_change(0)
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
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::PIdFSimActiveLearningTrainer()",
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
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::PIdFSimActiveLearningTrainer()",
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
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::PIdFSimActiveLearningTrainer()",
			"Parameter 'pidf_trainer_add_hyp_features' not specified");
	}

	char param_epochs[10];
	if (!ParamReader::getParam("pidf_trainer_epochs",param_epochs, 10))	{
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::PIdFSimActiveLearningTrainer()",
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
	char param_sent_to_add[10];
	if (!ParamReader::getParam("pidf_active_learning_sent_to_add",param_sent_to_add, 10))	{
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::PIdFSimActiveLearningTrainer()",
			"Parameter 'pidf_active_learning_sent_to_add' not specified");
	}
	_nToAdd = atoi(param_sent_to_add);

	if (!ParamReader::getParam("pidf_active_learning_epochs",param_sent_to_add, 10))	{
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::PIdFSimActiveLearningTrainer()",
			"Parameter 'pidf_active_learning_epochs' not specified");
	}
	_nActiveLearningIter = atoi(param_sent_to_add);

	Symbol repeats = ParamReader::getParam(Symbol(L"pidf_active_learning_allow_repeats"));
	_allowSentenceRepeats = (repeats == Symbol(L"true"));

	char param_weightsum_granularity[10];
	if (!ParamReader::getParam("pidf_trainer_weightsum_granularity",param_weightsum_granularity, 10))	{
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::PIdFSimActiveLearningTrainer()",
			"Parameter 'pidf_trainer_weightsum_granularity' not specified");
	}
	_weightsum_granularity = atoi(param_weightsum_granularity);

	if (!ParamReader::getParam("pidf_training_file",_training_file,									 500))	{
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::PIdFSimActiveLearningTrainer()",
			"Parameter 'pidf_training_file' not specified");
	}
	if (!ParamReader::getParam("active_learning_file",_active_learning_file,									 500))	{
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::PIdFSimActiveLearningTrainer()",
			"Parameter 'active_learning_file' not specified");
	}

	if (!ParamReader::getParam("pidf_model_file",_model_file,									 500))	{
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::PIdFSimActiveLearningTrainer()",
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
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::PIdFSimActiveLearningTrainer()",
			"Parameter 'pidf_features_file' not specified");
	}
	_featureTypes = _new DTFeatureTypeSet(features_file, PIdFFeatureType::modeltype);

	char tag_set_file[500];
	if (!ParamReader::getParam("pidf_tag_set_file",tag_set_file,									 500))	{
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::PIdFSimActiveLearningTrainer()",
			"Parameter 'pidf_tag_set_file' not specified");
	}
	char interleave_tags[500];
	if (!ParamReader::getParam("pidf_interleave_tags",interleave_tags,									 500))	{
		throw UnexpectedInputException("PIdFDecoder::PIdFSimActiveLearningTrainer()",
			"Parameter 'pidf_interleave_tags' not specified");
	}
	if(strcmp(interleave_tags, "true") == 0){
		_interleave_tags = true;
	}
	else{
		_interleave_tags = false;
	}
	_tagSet = _new DTTagSet(tag_set_file, true, true, _interleave_tags);

	_wordFeatures = IdFWordFeatures::build();

	WordClusterTable::ensureInitializedFromParamFile();
	char learn_transitions[500];
	if (!ParamReader::getParam("pidf_learn_transitions",learn_transitions,									 500))	{
		throw UnexpectedInputException("PIdFDecoder::PIdFSimActiveLearningTrainer()",
			"Parameter 'pidf_learn_transitions' not specified");
	}
	if(strcmp(learn_transitions, "true") == 0){
		_learn_transitions_from_training = true;
	}
	else{
		_learn_transitions_from_training = false;
	}
}

/*
PIdFSimActiveLearningTrainer::PIdFSimActiveLearningTrainer(
	enum output_mode_e output_mode, bool seed_features, bool add_hyp_features,
	int weightsum_granularity, int epochs, const char *features_file,
	const char *tag_set_file, const char *word_clusters_file,
	IdFWordFeatures *wordFeatures, const char *model_file)
	: _featureTypes(0), _tagSet(0), _decoder(0), _weights(0),
	  _output_mode(output_mode), _seed_features(seed_features),
	  _add_hyp_features(add_hyp_features),
	  _weightsum_granularity(weightsum_granularity), _epochs(epochs),
	  _wordFeatures(wordFeatures), _firstSentBlock(0), _lastSentBlock(0),
	  _curSentBlock(0), _cur_sent_no(0)
{
	PIdFFeatureTypes::ensureFeatureTypesInstantiated();

	_featureTypes = _new DTFeatureTypeSet(features_file, PIdFFeatureType::modeltype);

	_tagSet = _new DTTagSet(tag_set_file, true, true);

	if (word_clusters_file != 0)
		WordClusterTable::initTable(word_clusters_file);

	strncpy(_model_file, model_file, 500);
}
*/
PIdFSimActiveLearningTrainer::~PIdFSimActiveLearningTrainer() {
	delete _featureTypes;
	delete _wordFeatures;
	delete _tagSet;
}


void PIdFSimActiveLearningTrainer::addTrainingSentence(PIdFSentence &sentence) {
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

void PIdFSimActiveLearningTrainer::addTrainingSentencesFromTrainingFile(const char *file) {
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	if (file == 0)
		in.open(_training_file);
	else
		in.open(file);

	if (in.fail()) {
		throw UnexpectedInputException(
			"PIdFSimActiveLearningTrainer::addTrainingSentencesFromFile()",
			"Unable to open training file.");
	}

	cout << "Reading training sentences from disk...\n";

	PIdFSentence idfSentence(_tagSet, MAX_SENTENCE_TOKENS);

	int sentence_n = 0;
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
}


void PIdFSimActiveLearningTrainer::train() {
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
		cout.flush();
		prevtot = tot;
		tot = trainEpoch();
		if(tot >= _min_tot){
			break;
		}
		if((tot-prevtot) < _min_change){
			break;
		}


		if (PRINT_AFTER_EVERY_EPOCH)
			writeWeights(epoch);
	}

	writeWeights();


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

void PIdFSimActiveLearningTrainer::addTrainingFeatures() {
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
			blankToken, blankLCSymbol, blankWordFeatures, blankWordClass);
		for (int i = 0; i < sentence->getLength(); i++) {
			PIdFDecoder::populateObservation(
				static_cast<TokenObservation*>(observations[i+1]),
				_wordFeatures, sentence->getWord(i), i == 0);
		}
		static_cast<TokenObservation*>(observations[n_observations - 1])
			->populate(blankToken, blankLCSymbol, blankWordFeatures,
					   blankWordClass);

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

double PIdFSimActiveLearningTrainer::trainEpoch() {
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
			blankToken, blankLCSymbol, blankWordFeatures, blankWordClass);
		for (int i = 0; i < sentence->getLength(); i++) {
			PIdFDecoder::populateObservation(
				static_cast<TokenObservation*>(observations[i+1]),
				_wordFeatures, sentence->getWord(i), i == 0);
		}
		static_cast<TokenObservation*>(observations[n_observations - 1])
			->populate(blankToken, blankLCSymbol, blankWordFeatures,
					   blankWordClass);

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
/*				<< (int)_weights->get_path_length() << "\r";
				<< (int)_weights->get_num_lookup_eqs() << "/"
				<< (int)_weights->get_num_lookups() << "\r"; */
			_historyStream << sentence_n << ": " << n_correct
				 << " (" << 100*n_correct/1000 << "%)"
				 << "; " << (int) _weights->size() << "      \n";

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
	_historyStream <<"final: "<< sentence_n << ": " << total_ncorrect
		 << " (" << 100*((double)total_ncorrect/sentence_n)<< "%)"
		 << "; " << (int) _weights->size() << "      \n";

		cout.flush();



	cout << "\n";
	return (double)total_ncorrect/sentence_n;
}


void PIdFSimActiveLearningTrainer::writeTrainingSentences(int epoch) {
	UTF8OutputStream out;
	char file[600];

	if (epoch != -1) {
		sprintf(file, "%s-sent-%d", _model_file, epoch);
		out.open(file);
	}
	else{
		sprintf(file, "%s-sent", _model_file);
		out.open(file);
	}

	if (out.fail()) {
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::writeTrainingSentences()",
			"Could not open model training sentence file for writing");
	}
	seekToFirstSentence();
	PIdFSentence* sent;

	while(moreSentences()){
		sent = getNextSentence();
		sent->writeSexp(out);
	}
	out.close();
}
void PIdFSimActiveLearningTrainer::writeTransitions(int epoch) {
	char file[600];

	if (epoch != -1) {
		sprintf(file, "%s-epoch-%d-transitions", _model_file, epoch);

	}
	else{
		sprintf(file, "%s-transitions", _model_file);

	}
	_tagSet->writeTransitions(file);
}



void PIdFSimActiveLearningTrainer::writeWeights(int epoch) {
	UTF8OutputStream out;
	if (epoch != -1) {
		char file[600];
		sprintf(file, "%s-epoch-%d", _model_file, epoch);
		out.open(file);
	} else out.open(_model_file);

	if (out.fail()) {
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::writeWeights()",
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
void PIdFSimActiveLearningTrainer::readInitialWeights(){
	char file[600];
	sprintf(file, "%s-epoch-0", _model_file);
	DTFeature::readWeights(*_weights, file, PIdFFeatureType::modeltype);
}


void PIdFSimActiveLearningTrainer::seekToFirstSentence() {
	_curSentBlock = _firstSentBlock;
	_cur_sent_no = 0;
}
void PIdFSimActiveLearningTrainer::seekToFirstALSentence() {
	_curALSentBlock = _firstALSentBlock;
	_curAL_sent_no = 0;
}

bool PIdFSimActiveLearningTrainer::moreSentences() {
	return _curSentBlock != 0 &&
		   _cur_sent_no < _curSentBlock->n_sentences;
}

PIdFSentence *PIdFSimActiveLearningTrainer::getNextSentence() {
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



bool PIdFSimActiveLearningTrainer::moreALSentences() {
	return _curALSentBlock != 0 &&
		   _curAL_sent_no < _curALSentBlock->n_sentences;
}

PIdFSentence *PIdFSimActiveLearningTrainer::getNextALSentence() {
	if (moreALSentences()) {
		PIdFSentence *result = &_curALSentBlock->sentences[_curAL_sent_no];

		// move to next sentence for next call
		_curAL_sent_no++;
		if (_curAL_sent_no == SentenceBlock::BLOCK_SIZE) {
			_curALSentBlock = _curALSentBlock->next;
			_curAL_sent_no = 0;
		}

		return result;
	}
	else {
		return 0;
	}
}

void PIdFSimActiveLearningTrainer::addActiveLearningSentence(PIdFSentence &sentence) {

	if (_lastALSentBlock == 0) {
		_firstALSentBlock = _new SentenceBlock();
		_lastALSentBlock = _firstALSentBlock;
	}
	else if (_lastALSentBlock->n_sentences == SentenceBlock::BLOCK_SIZE) {
		_lastALSentBlock->next = _new SentenceBlock();
		_lastALSentBlock = _lastALSentBlock->next;
	}
	_lastALSentBlock->sentences[_lastALSentBlock->n_sentences++].populate(
																sentence);
}

void PIdFSimActiveLearningTrainer::addActiveLearningSentencesFromTrainingFileList(const char *file) {
	_n_ALSentences = 0;
	boost::scoped_ptr<UTF8InputStream> filelist_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& filelist(*filelist_scoped_ptr);
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	if (file == 0)
		filelist.open(_active_learning_file);
	else
		filelist.open(file);

	if (filelist.fail()) {
		throw UnexpectedInputException(
			"PIdFSimActiveLearningTrainer::addActiveLarningSentencesFromFile()",
			"Unable to open active learning file list.");
	}
	int nfiles;
	filelist >> nfiles;
	UTF8Token token;
	cout << "Reading active learning sentences from disk...\n";
	for(int i=0; i< nfiles; i++){
		if (filelist.eof())
			throw UnexpectedInputException("PIdFTrainer::addActiveLearningSentencesFromTrainingFileList",
				"fewer training files than specified in file");
		filelist >> token;
		in.open(token.symValue().to_string());
		if (in.fail()) {
			throw UnexpectedInputException(
				"PIdFSimActiveLearningTrainer::addActiveLarningSentencesFromFile()",
				"Unable to open file from active learning file list.");
		}


		PIdFSentence idfSentence(_tagSet, MAX_SENTENCE_TOKENS);

		int sentence_n = 0;
		while (idfSentence.readTrainingSentence(in)) {
			idfSentence.removeFromTraining();
			addActiveLearningSentence(idfSentence);


			sentence_n++;
			_n_ALSentences++;

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


void PIdFSimActiveLearningTrainer::trainAndSelect(bool print_models, int count, int nsent){
	// first train a model
	_weights = _new DTFeature::FeatureWeightMap(500009);

	if(_learn_transitions_from_training){
		_tagSet->resetSuccessorTags();
		_tagSet->resetPredecessorTags();
		seekToFirstSentence();
		int sentence_n = 0;
		int n_correct = 0;

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




	_decoder = _new PDecoder(_tagSet, _featureTypes, _weights,
							 _add_hyp_features);
	SessionLogger::info("SERIF")<<"Weights size: "<<(int)_weights->size()<<std::endl;
	if(count ==-1){
		readInitialWeights();
	}else{
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
				break;
			}
			if((tot-prevtot) < _min_change){
				break;
			}
		}
		if(print_models){
			writeWeights(count);
			writeTrainingSentences(count);
			if(_learn_transitions_from_training){
				writeTransitions(count);
			}
		}
	}

	//_scoredSentences.reserve(_n_ALSentences+1);

	//get intialize observations
	DTObservation *observations[MAX_SENTENCE_TOKENS+2];
	TokenObservation obsArray[MAX_SENTENCE_TOKENS+2];
	for (int i = 0; i < MAX_SENTENCE_TOKENS+2; i++)
		observations[i] = &obsArray[i];
	int tags[MAX_SENTENCE_TOKENS+2];
	Symbol decodedAnswer[MAX_SENTENCE_TOKENS+2];

	Token blankToken(0, 0, Symbol(L"NULL"));
	Symbol blankLCSymbol = Symbol(L"NULL");
	Symbol blankWordFeatures = Symbol(L"NULL");
	WordClusterClass blankWordClass = WordClusterClass::nullCluster();
	double score = 0;

	seekToFirstALSentence();
	int ndecoded = 0;
	int ntotal = 0;
	//since 0 is the minimum value for a margin, once we've found _nToAdd sentences
	// with a margin of 0. there's no reason to continue to decode....
	int n_margin_is_0 = 0;
	if(nsent == -1){
		nsent = _nToAdd;
	}
	while ((moreALSentences()) && n_margin_is_0 < nsent) {
		PIdFSentence *sentence = getNextALSentence();
		if(_allowSentenceRepeats || !sentence->inTraining() ){
			int n_observations = sentence->getLength() + 2;
			//populate an observation

			static_cast<TokenObservation*>(observations[0])->populate(
				blankToken, blankLCSymbol, blankWordFeatures, blankWordClass);
			for (int i = 0; i < sentence->getLength(); i++) {
				PIdFDecoder::populateObservation(
					static_cast<TokenObservation*>(observations[i+1]),
					_wordFeatures, sentence->getWord(i), i == 0);
			}
			static_cast<TokenObservation*>(observations[n_observations - 1])
				->populate(blankToken, blankLCSymbol, blankWordFeatures,
						blankWordClass);

			tags[0] = _tagSet->getStartTagIndex();
			for (int j = 0; j < sentence->getLength(); j++)
				tags[j+1] = sentence->getTag(j);
			tags[n_observations-1] = _tagSet->getEndTagIndex();

			score = _decoder->decodeAllTags(n_observations, observations);
			ndecoded++;
			SentAndScore thisSent(score, sentence, ntotal);
			_scoredSentences.push_back(thisSent);
			if(score == 0){
				n_margin_is_0++;
			}
		}
		ntotal++;
	}
	SessionLogger::info("SERIF")<<"ndecoded: "<<ndecoded<<std::endl;

	SessionLogger::info("SERIF")<<"sort:"<<std::endl;
	partial_sort(_scoredSentences.begin(), _scoredSentences.begin()+nsent,
		_scoredSentences.end());
	//add the first _nToAdd sentences to the training sentences

	ScoredSentenceVectorIt begin = _scoredSentences.begin();
	ScoredSentenceVectorIt end = _scoredSentences.end();
	ScoredSentenceVectorIt curr = begin;
	int nadded = 0;
	int totals[6];
	int totalgt5 =0;
	for(int i=0; i<6; i++){
		totals[i] = 0;
	}
	while((nadded < nsent) && (curr != end)){
		int marginint = (int)curr->margin;
		if(marginint > 5.0){
			totalgt5++;
		}
		else{
			totals[marginint]++;
		}

		addTrainingSentence(*curr->sent);
		curr->sent->addToTraining();
		curr++;
		nadded++;
	}
	SessionLogger::info("SERIF")<< "Added "<<nadded<<" sentences: "<<"\n";
	for(int i =0; i<6; i++){
		SessionLogger::info("SERIF")<<"\t "<<totals[i]<<" w/margin="<<i<<"\n";
	}
	SessionLogger::info("SERIF")<<"\t "<<totalgt5<<" w/margin >= 6\n";

	_historyStream<< "Added "<<nadded<<" sentences: "<<"\n";
	for(int i =0; i<6; i++){
		_historyStream<<"\t "<<totals[i]<<" w/margin="<<i<<"\n";
	}
	_historyStream<<"\t "<<totalgt5<<" w/margin >= 6\n";
	_historyStream.flush();

	for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
		 iter != _weights->end(); ++iter)
	{
		(*iter).first->deallocate();
	}


	//clean up
	_scoredSentences.erase(_scoredSentences.begin(), _scoredSentences.end());
	delete _decoder;
	delete _weights;
}



void PIdFSimActiveLearningTrainer::doActiveLearning(){
	for(int i =0; i< _nActiveLearningIter; i++){
		trainAndSelect(true, i, _nToAdd);
	}

	train();
	writeTrainingSentences();
	writeTransitions();
	_historyStream.close();

}


