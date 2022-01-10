// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/FeatureModule.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/tokens/Tokenizer.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"
#include "Generic/morphSelection/MorphSelector.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/names/discmodel/PIdFModel.h"
#include "Generic/names/discmodel/PIdFFeatureTypes.h"

#include "Generic/names/IdFWordFeatures.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/PDecoder.h"
#include "Generic/common/GrowableArray.h"

#include "Generic/names/discmodel/PIdFActiveLearning.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/common/UTF8InputStream.h"
#include <boost/scoped_ptr.hpp>

#if defined(_WIN32)
#define snprintf _snprintf
#endif

#define MAX_NAMES_PER_SENTENCE 100
#define MAX_NAMES_PER_SPACE_SEP_TOK 5
#define MAX_CONTEXT_SIZE 16

//PIdFActiveLearning::SentenceIdMap PIdFActiveLearning::_idToSentence;
//PIdFActiveLearning::SentIdToTokensMap PIdFActiveLearning::_idToTokens;
//PIdFActiveLearning::SentIdToTokensMap PIdFActiveLearning::_trainingIdToTokens;
PIdFActiveLearning::PIdFActiveLearning(): _featureTypes(0), _tagSet(0), _wordFeatures(0),
	  _decoder(0), _weights(0), _firstSentBlock(0), _lastSentBlock(0),
	  _curSentBlock(0), _firstTestingSentBlock(0), _lastTestingSentBlock(0),
	  _curTestingSentBlock(0), _firstALSentBlock(0), _lastALSentBlock(0),
	  _curALSentBlock(0), _cur_sent_no(0), _currentWeights(0), _tokenizer(0),
	  _morphAnalyzer(0), _morphSelector(0), _inTraining(1000), _inTesting(1000)
{
		//ParamReader::readParamFile(param_file);

		//intialize tokenization components
		//_tokenizer = Tokenizer::build();

		//_morphAnalyzer = MorphologicalAnalyzer::build();
		//_morphSelector = _new MorphSelector(_morphAnalyzer);
		PIdFFeatureTypes::ensureFeatureTypesInstantiated();
}

PIdFActiveLearning::~PIdFActiveLearning(){
	//delete _tokenizer;
	//delete _morphAnalyzer;
	//delete _morphSelector;
	/*
	SentIdToTokensMap::iterator iter;
	for (iter = _idToTokens.begin(); iter != _idToTokens.end(); ++iter)
		delete (*iter).second;

	for (iter = _trainingIdToTokens.begin(); iter != _trainingIdToTokens.end(); ++iter)
		delete (*iter).second;
		*/

}
wstring PIdFActiveLearning::Initialize(char* param_file){
	try{
		//does this work???
		ParamReader::finalize();
		ParamReader::readParamFile(param_file);
		FeatureModule::load();
		/*
		//epochs are an argument of Train()		
		_epochs = ParamReader::getRequiredIntParam("quicklearn_epochs");
		*/
		_corpus_file = ParamReader::getRequiredParam("quicklearn_corpus_file");
		_seed_features = ParamReader::getRequiredTrueFalseParam(L"quicklearn_trainer_seed_features");
		_add_hyp_features = ParamReader::getRequiredTrueFalseParam(L"quicklearn_trainer_add_hyp_features");
		_allowSentenceRepeats = ParamReader::isParamTrue("quicklearn_active_learning_allow_repeats");

		_weightsum_granularity = ParamReader::getRequiredIntParam("quicklearn_trainer_weightsum_granularity");
		std::string features_file = ParamReader::getRequiredParam("pidf_features_file");
		_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), PIdFFeatureType::modeltype);

		std::string tag_set_file = ParamReader::getRequiredParam("pidf_tag_set_file");
		_interleave_tags = ParamReader::getRequiredTrueFalseParam(L"pidf_interleave_tags");;
		_tagSet = _new DTTagSet(tag_set_file.c_str(), true, true, _interleave_tags);


		_wordFeatures = IdFWordFeatures::build();

		WordClusterTable::ensureInitializedFromParamFile();
		_learn_transitions_from_training = ParamReader::getRequiredTrueFalseParam(L"pidf_learn_transitions");

		_model_file = ParamReader::getRequiredParam("pidf_model_file");

		_weights = _new DTFeature::FeatureWeightMap(500009);
	}
	catch(UnrecoverableException &e) {
		SessionLogger::err("SERIF") << "error: " << e.getSource() << " " <<e.getMessage() << std::endl;
		return getRetVal(false, e.getMessage());
	}
	//not ideal to have this be exception driven, but basically we are loading a modelif it finds
	try{
		boost::scoped_ptr<UTF8InputStream> tempuis_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& tempuis(*tempuis_scoped_ptr);

		// quick check of file existing without throwing exception
		fstream fin;
		fin.open(_model_file.c_str(), ios::in);
		bool file_exists = fin.is_open();
		fin.close();

		if (file_exists) 
			tempuis.open(_model_file.c_str());
		if(file_exists && !tempuis.fail()){
			SessionLogger::info("SERIF")<<"Load existing model"<<std::endl;

			tempuis.close();
			//read weights
			DTFeature::readWeights(*_weights, _model_file.c_str(), PIdFFeatureType::modeltype);
			//read transitions
			if(_learn_transitions_from_training){
				std::string transition_file = _model_file + "-transitions";
				_tagSet->readTransitions(transition_file.c_str());
			}
			//read sentences
			std::string training_file = _model_file + "-training-sentences";
			std::string tokens_training_file = training_file + ".tokens";

			std::string test_file = _model_file + "-test-sentences";
			std::string tokens_test_file = test_file + ".tokens";
			
			if (!fileExists(test_file.c_str()))
				createEmptyFile(test_file.c_str());
			if (!fileExists(tokens_test_file.c_str()))
				createEmptyFile(tokens_test_file.c_str());

			readSentencesFile(training_file.c_str(), tokens_training_file.c_str(), TRAIN);
			readSentencesFile(test_file.c_str(), tokens_test_file.c_str(), TEST);
		} else {
			SessionLogger::info("SERIF") << "Create new model" << std::endl;

			if(_learn_transitions_from_training){
				_tagSet->resetPredecessorTags();
				_tagSet->resetSuccessorTags();
			}
		}
	}
	catch(UnrecoverableException& e){
		SessionLogger::info("SERIF") << "Create new model" << std::endl;
		SessionLogger::info("SERIF") << e.getSource() << " " << e.getMessage() << std::endl;

		if(_learn_transitions_from_training){
			_tagSet->resetPredecessorTags();
			_tagSet->resetSuccessorTags();
		}

	}
	try {
		_decoder = _new PDecoder(_tagSet, _featureTypes, _weights, _add_hyp_features);
		_currentSentences.setLength(1000);
		_currentSentences.setLength(0);

		return getRetVal(true);
	}
	catch(UnrecoverableException &e) {
		SessionLogger::err("SERIF") << "error: " << e.getSource() << " "<< e.getMessage() << std::endl;
		return getRetVal(false, e.getMessage());
	}
}

// It appears this function crashes when there are sentences with duplicate ids.
// QL now bipasses this function by just shutting down the server which contains 
// a PIdFActiveLearning object, instead of calling Close().
wstring PIdFActiveLearning::Close(){
	try{
		delete _featureTypes;
		delete _wordFeatures;
		delete _tagSet;
		delete _decoder;
		delete _weights;

		/*SentenceIdMap::iterator sent_iter;
		for (sent_iter = _idToSentence.begin(); sent_iter != _idToSentence.end(); ++sent_iter)
			delete (*sent_iter).second;
		SentIdToTokensMap::iterator iter;
		for (iter = _idToTokens.begin(); iter != _idToTokens.end(); ++iter)
			delete (*iter).second;
		for (iter = _trainingIdToTokens.begin(); iter != _trainingIdToTokens.end(); ++iter)
			delete (*iter).second;
		*/
		_idToSentence.clear();
		_idToTokens.clear();
		_trainingIdToTokens.clear();
		_testingIdToTokens.clear();

		SentenceBlock* first = _firstSentBlock;
		while(first != 0) {
			SentenceBlock* next = first->next;
			for (int i = 0; i < first->n_sentences; i++) {
				if (first->sentences[i].GetSentenceNumber() != -1)
					first->sentences[i].deleteMembers();
			}
			delete first;
			first = 0;
			first = next;
		}
		_firstSentBlock = 0;
		_lastSentBlock = 0;
		_curSentBlock = 0;

		first = _firstTestingSentBlock;
		while(first != 0) {
			SentenceBlock* next = first->next;
			for (int i = 0; i < first->n_sentences; i++) {
				if (first->sentences[i].GetSentenceNumber() != -1)
					first->sentences[i].deleteMembers();
			}
			delete first;
			first = 0;
			first = next;
		}
		_firstTestingSentBlock = 0;
		_lastTestingSentBlock = 0;
		_curTestingSentBlock = 0;

		first = _firstALSentBlock;
		while(first != 0){
			SentenceBlock* next = first->next;
			for (int i = 0; i < first->n_sentences; i++) {
				if (first->sentences[i].GetSentenceNumber() != -1)
					first->sentences[i].deleteMembers();
			}
			delete first;
			first = 0;
			first = next;
		}
		_firstALSentBlock = 0;
		_lastALSentBlock = 0;
		_curALSentBlock = 0;

		return getRetVal(true);
	}
	catch(UnrecoverableException &e) {
		return getRetVal(false, e.getMessage());
	}
}

wstring PIdFActiveLearning::Save(){
	try{
		writeWeights();
		if(_learn_transitions_from_training){
			writeTransitions();
		}
		writeTrainingSentences();
		return getRetVal(true);
	}
	catch(UnrecoverableException &e) {
		return getRetVal(false, e.getMessage());
	}
}

wstring PIdFActiveLearning::readDecodeFile(const wchar_t* decode_file, SentIdToTokensMap &decodeTokens) {
	char errmsg[600];
	try{
		_currentSentences.setLength(0);
		// Read the tokens file and populate tokens hashtable
		wchar_t tokens_file[500];
		wcscpy(tokens_file, decode_file);
		wcsncat(tokens_file, L".tokens", 500);
		boost::scoped_ptr<UTF8InputStream> tokens_in_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& tokens_in(*tokens_in_scoped_ptr);
		tokens_in.open(tokens_file);
		readTokensCorpus(tokens_in, decodeTokens);
		tokens_in.close();

		boost::scoped_ptr<UTF8InputStream> decode_stream_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& decode_stream(*decode_stream_scoped_ptr);
		decode_stream.open(decode_file);
		if(decode_stream.fail()){
			snprintf(errmsg, 599, "PIdFActiveLearning::readDecodeFile(), Can't read decode file: %s", Symbol(decode_file).to_debug_string());
			return getRetVal(false, errmsg);
		}
		//get the root
		UTF8Token tok;
		NameOffset nameInfo[MAX_NAMES_PER_SENTENCE];
		LocatedString* sentenceString;

//		int n_cache_docs = 0;
		while(!decode_stream.eof()){
			Symbol sentId;
			decode_stream >> tok;	//<doc
			int nsent =0;
			if(wcsncmp(tok.chars(), L"<DOC", 4) ==0){
				decode_stream >> tok; // docid="ID"
				Document* doc = _new Document(tok.symValue());
				int nname = readSavedSentence(decode_stream, sentenceString, sentId, nameInfo);
				//PIdFActiveLearningSentence* prev = 0;

				while(nname != -1){
					//tokenizer has to be reset fore every new sentence
					TokenSequence * tokSequence;
					SentIdToTokensMap::iterator it = decodeTokens.find(sentId);
					if (it == decodeTokens.end()) {
						//throw an exception
						break;
					} else {
						tokSequence = (*it).second;
					}
					PIdFActiveLearningSentence thisSent = makePIdFActiveLearningSentence(sentId, sentenceString, tokSequence, nameInfo, nname);
					PIdFActiveLearningSentence* alsentpt = _new PIdFActiveLearningSentence();
					alsentpt->populate(thisSent);

					_currentSentences.add(alsentpt);
					//decodeTokens.remove(sentId);
					nsent++;
					nname = readSavedSentence(decode_stream, sentenceString, sentId, nameInfo);
				}
				delete doc;
			}
		}
		return getRetVal(true);
	}
	catch(UnrecoverableException &e) {
		return getRetVal(false, e.getMessage());
	}
}

wstring PIdFActiveLearning::ReadCorpus(char* corpus_file){
	try{
		std::string corpus_file_str;
		if (corpus_file == 0) {
			corpus_file_str = _corpus_file;
		} else {
			corpus_file_str = std::string(corpus_file);
		}
		std::string tokens_corpus_file = corpus_file_str + ".tokens";

		boost::scoped_ptr<UTF8InputStream> an_corpusstream_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& an_corpusstream(*an_corpusstream_scoped_ptr);
		an_corpusstream.open(tokens_corpus_file.c_str());
		if(an_corpusstream.fail()){
			std::string error = "PIdFActiveLearning::ReadCorpus(), Can't read tokens corpus file: " + tokens_corpus_file;
			return getRetVal(false, error.c_str());
		}
		readTokensCorpus(an_corpusstream, _idToTokens);
		an_corpusstream.close();

		boost::scoped_ptr<UTF8InputStream> corpusstream_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& corpusstream(*corpusstream_scoped_ptr);
		corpusstream.open(corpus_file_str.c_str());
		if(corpusstream.fail()){
			std::string error = "PIdFActiveLearning::ReadCorpus(), Can't read corpus file: " + corpus_file_str;
			return getRetVal(false, error.c_str());
		}
		//get the root
		UTF8Token tok;
		NameOffset nameInfo[MAX_NAMES_PER_SENTENCE];
		LocatedString* sentenceString;

		int n_cache_docs = 0;
		int total_sentences = 0;
		while(!corpusstream.eof()){
			Symbol sentId;
			corpusstream >> tok;	//<doc
			int nsent =0;
			if(wcsncmp(tok.chars(), L"<DOC", 4) ==0){
					corpusstream >> tok; // docid="ID"
					Document* doc = _new Document(tok.symValue());
					int nname = readSavedSentence(corpusstream, sentenceString, sentId, nameInfo);
					PIdFActiveLearningSentence* prev = 0;

					while(nname != -1){
						//tokenizer has to be reset fore every new sentence
						//_tokenizer->resetForNewSentence(doc, nsent);
						TokenSequence * tokSequence;
						SentIdToTokensMap::iterator it = _idToTokens.find(sentId);
						if (it == _idToTokens.end()) {
							//throw an exception
							break;
						} else {
							tokSequence = (*it).second;
						}
						//TokenSequence * tokSequence = _idToTokens[sentId];
						PIdFActiveLearningSentence thisSent = makePIdFActiveLearningSentence(sentId, sentenceString, tokSequence, nameInfo, nname);
						//PIdFActiveLearningSentence thisSent = makePIdFActiveLearningSentence(sentId, sentenceString, nameInfo, nname);
						prev = addActiveLearningSentence(thisSent);
						(_idToSentence)[prev->GetId()] = prev;
						if(nsent == 0){
							prev->_isFirstSent = true;
						}
						nsent++;
						total_sentences++;
						nname = readSavedSentence(corpusstream, sentenceString, sentId, nameInfo);
					}
					delete doc;
					/*n_cache_docs++;
					if (n_cache_docs > 50) {
						_morphAnalyzer->getLexicon()->clearDynamicEntries();
						n_cache_docs = 1;
					}
					*/
					prev->_isLastSent = true;
			}
		}

		Symbol startid = ParamReader::getParam(L"quicklearn_corpuspointer");
		seekToFirstALSentence();//should seek to corpus pointer

		if(!startid.is_null()){
			while(getNextALSentence()->GetId() != startid){
				if(!moreALSentences()){
					seekToFirstALSentence();
					break;
					//return getRetVal(false, "corpus does not contain quicklearn-corpuspointer");
				}
			}
		}
		_n_corpus_sentences = total_sentences;
		//printf("Read %d corpus sentences\n", _n_corpus_sentences);
		return getRetVal(true);

	}
	catch(UnrecoverableException &e) {
		return getRetVal(false, e.getMessage());
	}


}

void PIdFActiveLearning::readSentencesFile(const char *sent_file, const char *tokens_sent_file, int mode) 
{
	SentIdToTokensMap *idToTokensMap;
	SymbolHash *sentenceIdCache;
	char errmsg[600];

	if (mode == TRAIN) {
		idToTokensMap = &_trainingIdToTokens;
		sentenceIdCache = &_inTraining;
	} else if (mode == TEST) {
		idToTokensMap = &_testingIdToTokens;
		sentenceIdCache = &_inTesting;
	} else {
		snprintf(errmsg, 599, "PIdFActiveLearning::readSentencesFile(), Bad mode");
		return;
	}

	
	boost::scoped_ptr<UTF8InputStream> an_stream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& an_stream(*an_stream_scoped_ptr);
	an_stream.open(tokens_sent_file);
	if(an_stream.fail()){
		snprintf(errmsg, 599, 
			"PIdFActiveLearning::readSentencesFile(), Can't read tokens file: %s", 
			tokens_sent_file);
		return;
	}

	readTokensCorpus(an_stream, *idToTokensMap);
	an_stream.close();

	boost::scoped_ptr<UTF8InputStream> sentstream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& sentstream(*sentstream_scoped_ptr);
	sentstream.open(sent_file);

	if(sentstream.fail()){
		char errmsg[600];
		snprintf(errmsg, 599, "Can't read sentence file: %s", sent_file);
		throw UnexpectedInputException("PIdFActiveLearning::ReadTrainingFile()", errmsg);

	}
	//get the root
	UTF8Token tok;
	NameOffset nameInfo[MAX_NAMES_PER_SENTENCE];
	LocatedString* sentenceString;
	//int startoff;
	//int endoff;
	int nsent = 0;
	Document* doc = _new Document(Symbol(L"FAKE_DOCUMENT"));

	while (!sentstream.eof()) {
		Symbol sentId;
		int nname = readSavedSentence(sentstream, sentenceString, sentId, nameInfo);
		if(nname == -1){
			throw UnexpectedInputException("PIdFActiveLearning::ReadTrainingFile()",
				"error reading sentence file");
		}
		else if(nname == -2){
			continue;
		}
		else{
			//tokenize
			//tokenizer has to be reset fore every new sentence
			//just fake it here,
			//_tokenizer->resetForNewSentence(doc, nsent);
			TokenSequence * tokSequence;
			SentIdToTokensMap::iterator it = idToTokensMap->find(sentId);
			if (it == idToTokensMap->end()) {
				//throw an exception
				continue;
			} else {
				tokSequence = (*it).second;
			}

			PIdFActiveLearningSentence thisSent = makePIdFActiveLearningSentence(sentId, sentenceString, tokSequence, nameInfo, nname);
			//PIdFActiveLearningSentence thisSent = makePIdFActiveLearningSentence(sentId, sentenceString, nameInfo, nname);
			//_morphAnalyzer->getLexicon()->clearDynamicEntries();
			
			if (mode == TRAIN) 
				addTrainingSentence(thisSent);
			else if (mode == TEST)
				addTestingSentence(thisSent);

			sentenceIdCache->add(sentId);
			nsent++;
		}
	}
	delete doc;
}


PIdFActiveLearningSentence PIdFActiveLearning::makePIdFActiveLearningSentence(Symbol id, LocatedString* sentenceString, TokenSequence* tokens, const NameOffset* nameInfo, int nname) {
	PIdFSentence* pidfSent = makePIdFSentence(tokens, nameInfo, nname);
	PIdFActiveLearningSentence alSent;
	alSent.populate(id, sentenceString, tokens, pidfSent);
	return alSent;
}


PIdFActiveLearningSentence PIdFActiveLearning::makeAdjustedPIdFActiveLearningSentence(Symbol id,
											LocatedString* sentenceString, TokenSequence* tokens,
											const NameOffset* nameInfo, int nname, SentIdToTokensMap &idToTokensTable){

	TokenSequence* fixedTokenSequence = adjustTokenizationToNames(tokens, nameInfo, nname, sentenceString);
	idToTokensTable.erase(id);
	idToTokensTable[id] = fixedTokenSequence;
	return makePIdFActiveLearningSentence(id, sentenceString, fixedTokenSequence, nameInfo, nname);
}


PIdFActiveLearningSentence PIdFActiveLearning::makePIdFActiveLearningSentence(Symbol id,
											LocatedString* sentenceString,
											const NameOffset* nameInfo, int nname){
// THIS FUNCTION IS NOT BEING USED
	PIdFSentence* pidfSent;
	TokenSequence* fixedTokenSequence;

	TokenSequence* tempTheory;

	_tokenizer->getTokenTheories(&tempTheory, 1, sentenceString);
	_morphAnalyzer->getMorphTheories(tempTheory);
	_morphSelector->selectTokenization(sentenceString, tempTheory);

	fixedTokenSequence = adjustTokenizationToNames(tempTheory, nameInfo, nname, sentenceString);
/*
	UTF8OutputStream stream;
	stream.open(L"C:\\Projects\\QuickLearn\\Ar_Corpus_processed.txt", true);
	stream << L"<SENTENCE ID=\"" << id.to_string() << L"\">\n";
	for (int i = 0; i < fixedTokenSequence->getNTokens(); i++) {
		const Token * token = fixedTokenSequence->getToken(i);
		stream << L"<TOKEN START=\"" << token->getStartEDTOffset() << L"\" END=\"" << token->getEndEDTOffset() << L"\">";
		stream << token->getSymbol().to_string();
		stream << L"</TOKEN>\n";
	}
	stream << L"</SENTENCE>\n";
	stream.close();
*/
	delete tempTheory;
	pidfSent = makePIdFSentence(fixedTokenSequence, nameInfo, nname);
	PIdFActiveLearningSentence alSent;
	alSent.populate(id, sentenceString, fixedTokenSequence, pidfSent);
	return alSent;
}

wstring PIdFActiveLearning::ChangeCorpus(char* corpus_file){
	clearCorpus();
	return ReadCorpus(corpus_file);

}

wstring PIdFActiveLearning::SaveSentences(const wchar_t* ann_sentences, size_t str_length, const wchar_t* token_sents, size_t token_sents_length, const wchar_t * tokens_file) {
try {
		// Read the tokens file and populate tokens hashtable
		readGuiTokensInput(token_sents, token_sents_length, _idToTokens);

		_currentSentences.setLength(0);
		int nsent = readGuiInput(ann_sentences, str_length);

		UTF8OutputStream out;
		out.open(tokens_file);
		for(int i=0; i<_currentSentences.length(); i++){
			Symbol sent_id = (*_currentSentences[i]).GetId();
			TokenSequence * tokSequence;
			SentIdToTokensMap::iterator it = _idToTokens.find(sent_id);
			if (it == _idToTokens.end()) {
				//throw an exception
				continue;
			} else {
				tokSequence = (*it).second;
			}
			(*_currentSentences[i]).WriteTokenSentence(out, sent_id, tokSequence);
		}
		out.close();
		return getRetVal(true);

	} catch(UnrecoverableException &e) {
		return getRetVal(false, e.getMessage());
	}
}

wstring PIdFActiveLearning::Train(const wchar_t* ann_sentences, size_t str_length, const wchar_t* token_sents, size_t token_sents_length, int epochs, bool incremental){
	//std::cout << "PIdFActiveLearning::Train called\n";
	try {
		// Read sentences tokens info
		readGuiTokensInput(token_sents, token_sents_length, _idToTokens);
		_currentSentences.setLength(0);
		int nsent = readGuiInput(ann_sentences, str_length);

		for(int i=0; i<_currentSentences.length(); i++){
			addTrainingSentence(*_currentSentences[i]);
		}

		if(!incremental){

			delete _decoder;
			delete _weights;
			if(_learn_transitions_from_training){

				_tagSet->resetSuccessorTags();
				_tagSet->resetPredecessorTags();
				int sentence_n = 0;
				int n_correct = 0;
				seekToFirstSentence();
				while (moreSentences()) {
					PIdFSentence *sentence = getNextSentence()->GetPIdFSentence();
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
			for(int i=0; i<epochs; i++){
				SessionLogger::info("SERIF")<<"epoch "<<i<<std::endl;
				trainEpoch();
			}
			//writeWeights();
		}
		else{

			if(_learn_transitions_from_training){
				for(int i=0; i<_currentSentences.length(); i++){
					PIdFSentence *sentence = _currentSentences[i]->GetPIdFSentence();
					Symbol prevTag = _tagSet->getStartTag();
					for(int i =0; i< sentence->getLength(); i++){
						Symbol nextTag = _tagSet->getTagSymbol(sentence->getTag(i));
						_tagSet->addTransition(prevTag, nextTag);
						prevTag = nextTag;
					}
					_tagSet->addTransition(prevTag, _tagSet->getEndTag());


				}
			}
			for(int i=0; i<epochs; i++){
				trainIncrementalEpoch();
			}
		}
		for(int j=0; j<_currentSentences.length(); j++){
			delete _currentSentences[j];
		}
		_currentSentences.setLength(0);

		_decoder->finalizeWeights();
		return getRetVal(true);
	}
	catch(UnrecoverableException &e) {
		return getRetVal(false, e.getMessage());
	}
}

wstring PIdFActiveLearning::AddToTestSet(const wchar_t* ann_sentences, size_t str_length, 
										 const wchar_t* token_sents, size_t token_sents_length) {
	try {
		// Read sentences tokens info
		readGuiTokensInput(token_sents, token_sents_length, _idToTokens);

		_currentSentences.setLength(0);
		int nsent = readGuiInput(ann_sentences, str_length);

		for(int i=0; i<_currentSentences.length(); i++)
			addTestingSentence(*_currentSentences[i]);

		writeTestingSentences();
		
		return getRetVal(true);
	}
	catch(UnrecoverableException &e) {
		return getRetVal(false, e.getMessage());
	}
}

wstring PIdFActiveLearning::DecodeFile(const wchar_t * input_file) {
	try {
		SentIdToTokensMap decodeTokens;
		readDecodeFile(input_file, decodeTokens);
		wstring result = decodeSentences();

		SentIdToTokensMap::iterator iter;
		for (iter = decodeTokens.begin(); iter != decodeTokens.end(); ++iter)
			delete (*iter).second;
		return result;
/*
		// Read the tokens file and populate tokens hashtable
		SentIdToTokensMap decodeTokens;
		wchar_t tokens_file[500];
		wcscpy(tokens_file, input_file);
		wcsncat(tokens_file, L".tokens", 500);
		boost::scoped_ptr<UTF8InputStream> tokens_in_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& tokens_in(*tokens_in_scoped_ptr);
		tokens_in.open(tokens_file);
		readTokensCorpus(tokens_in, decodeTokens);
		tokens_in.close();

		// read the sentences to decode file
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& in(*in_scoped_ptr);
		in.open(input_file);
		wstring sents = L"";
		wstring line_str;
		while (!in.eof()) {
			in.getLine(line_str);
			sents.append(line_str);
		}
		in.close();


 		const wchar_t* sents_str = sents.c_str();
		//wchar_t sentences[500001];
		//sents.copy(sentences);
		//wcsncpy(sentences, sents_str, 500000);
		//int sent_len = static_cast<int>(wcslen(sentences));
		//readGuiInput(sentences, sent_len, decodeTokens);
		int sent_len = static_cast<int>(sents.length());
		readGuiInput(sents_str, sent_len, decodeTokens);
		*/


	} catch(UnrecoverableException &e) {
		return getRetVal(false, e.getMessage());
	}
}

wstring PIdFActiveLearning::DecodeTraining(const wchar_t* ann_sentences, size_t length) {
	try {
		if(!_decoder){
			throw UnexpectedInputException("PIdFActiveLearning::Decode()",
				"Trying to decode with initializing decoder");
		}
		_currentSentences.setLength(0);
		readGuiInput(ann_sentences, length, _trainingIdToTokens);
		return decodeSentences();
	}
	catch(UnrecoverableException &e) {
		return getRetVal(false, e.getMessage());
	}
}

wstring PIdFActiveLearning::DecodeTestSet(const wchar_t* ann_sentences, size_t length) {
	try {
		if(!_decoder){
			throw UnexpectedInputException("PIdFActiveLearning::Decode()",
				"Trying to decode with initializing decoder");
		}
		_currentSentences.setLength(0);
		readGuiInput(ann_sentences, length, _testingIdToTokens);
		return decodeSentences();
	}
	catch(UnrecoverableException &e) {
		return getRetVal(false, e.getMessage());
	}
}

wstring PIdFActiveLearning::DecodeFromCorpus(const wchar_t* ann_sentences, size_t length) {
	try {
		if(!_decoder){
			throw UnexpectedInputException("PIdFActiveLearning::Decode()",
				"Trying to decode with initializing decoder");
		}
		_currentSentences.setLength(0);
		readGuiInput(ann_sentences, length, _idToTokens);
		return decodeSentences();
	}
	catch(UnrecoverableException &e) {
		return getRetVal(false, e.getMessage());
	}
}

wstring PIdFActiveLearning::decodeSentences() {

	std::vector<DTObservation *> observations;
	int tags[MAX_SENTENCE_TOKENS+2];

	Token blankToken(Symbol(L"NULL"));
	Symbol blankLCSymbol = Symbol(L"NULL");
	Symbol blankWordFeatures = Symbol(L"NULL");
	WordClusterClass blankWordClass = WordClusterClass::nullCluster();
	std::wstring str = L"";
	for(int i =0; i<_currentSentences.length(); i++){
		PIdFSentence* sentence = _currentSentences[i]->GetPIdFSentence();

		int n_observations = sentence->getLength() + 2;

		static_cast<TokenObservation*>(observations[0])->populate(
			blankToken, blankLCSymbol, blankWordFeatures, blankWordClass, 0, 0);
		for (int j = 0; j < sentence->getLength(); j++) {
			PIdFModel::populateObservation(
				static_cast<TokenObservation*>(observations[j+1]),
				_wordFeatures, sentence->getWord(j), j == 0, 0, false, false, 0);
		}
		static_cast<TokenObservation*>(observations[n_observations - 1])
			->populate(blankToken, blankLCSymbol, blankWordFeatures,
						blankWordClass, 0, 0);

		_decoder->decode(observations, tags);

		for (int k = 0; k < sentence->getLength(); k++){
			sentence->setTag(k, tags[k+1]);
		}

		_currentSentences[i]->AppendSentence(str, _tagSet, true);
	}
	for(int j=0; j<_currentSentences.length(); j++){
		//_currentSentences[i]->deleteMembers();
		delete _currentSentences[j];
	}
	_currentSentences.setLength(0);
	return getRetVal(true, str.c_str());
}

//Warning: this writes over the tags in the Corpus Sentences,
//there for corpus tags will have no meaning until added to the training set!
wstring PIdFActiveLearning::SelectSentences(int training_pool_size, int num_to_select,
											int context_size, int min_positive_results)
{
	try {

		if (training_pool_size > _n_corpus_sentences) {
			printf 
				("Adjusting training_pool_size to be equal to number of sentences in corpus: %d\n", 
				_n_corpus_sentences);

			training_pool_size = _n_corpus_sentences;
		}


		std::vector<DTObservation *> observations;
		int tags[MAX_SENTENCE_TOKENS+2];

		Token blankToken(Symbol(L"NULL"));
		Symbol blankLCSymbol = Symbol(L"NULL");
		Symbol blankWordFeatures = Symbol(L"NULL");
		WordClusterClass blankWordClass = WordClusterClass::nullCluster();
		PIdFActiveLearningSentence* context[MAX_CONTEXT_SIZE];
		double score = 0;
		int n_def_included = 0;
		//
		PIdFActiveLearningSentence* alSent;
		PIdFSentence* sentence;
		bool corpus_reset = false;
		for(int i =0; i < training_pool_size; i++){
			if(!moreALSentences()){
				if (!corpus_reset) {
					seekToFirstALSentence();
					corpus_reset = true;
				} else
					break;
				//out of sentences....
			}
			else if(n_def_included >= num_to_select){
				break;
				//we've found all of the sentences we need so skip further decode...
			}

			alSent = getNextALSentence();
			sentence = alSent->GetPIdFSentence();

			if (_allowSentenceRepeats ||
				(!_inTraining.lookup(alSent->GetId()) && !_inTesting.lookup(alSent->GetId())))
			{
				int n_observations = sentence->getLength() + 2;
				//populate an observation

				static_cast<TokenObservation*>(observations[0])->populate(
					blankToken, blankLCSymbol, blankWordFeatures, blankWordClass, 0, 0);
				for (int j = 0; j < sentence->getLength(); j++) {
					PIdFModel::populateObservation(
						static_cast<TokenObservation*>(observations[j+1]),
						_wordFeatures, sentence->getWord(j), j == 0, 0, false, false, 0);
				}
				static_cast<TokenObservation*>(observations[n_observations - 1])
					->populate(blankToken, blankLCSymbol, blankWordFeatures,
							blankWordClass, 0, 0);
				score = _decoder->decodeAllTags(observations, 0 , tags);
				for(int k=1; k < (n_observations-1); k++){
					sentence->setTag(k-1,tags[k]);
				}
				bool hasname = sentHasName(sentence);
				ActiveLearningSentAndScore thisSent(score, alSent, i, hasname);
				_scoredSentences.push_back(thisSent);
				if((score == 0) && hasname ){
					n_def_included++;
				}
			}
		}
		sort(_scoredSentences.begin(),_scoredSentences.end());
		int n_no_name = 0;
		std::wstring str = L"";
		int max_num_noname = num_to_select - min_positive_results;
		int nadded = 0;
		int currsent =0;
		while((nadded < num_to_select)){
			if(currsent >= (static_cast<int>(_scoredSentences.size()))){
				break;
			//throw UnexpectedInputException("PIdFActiveLearning::SelectSentences()", "Not enough Qualified Sentences");
			}
			ActiveLearningSentAndScore thisSent = _scoredSentences[currsent];
			currsent++;
			if(thisSent.hasName){
				nadded++;
				getContext(context, context_size, thisSent.sent);
				thisSent.sent->AppendSentenceWithContext(str, _tagSet, context, context_size, true);
				thisSent.sent->GetPIdFSentence()->addToTraining();
			}
			else if(n_no_name < max_num_noname){
				nadded++;
				n_no_name++;
				getContext(context, context_size, thisSent.sent);
				thisSent.sent->AppendSentenceWithContext(str, _tagSet, context, context_size, true);
				thisSent.sent->GetPIdFSentence()->addToTraining();
				_inTraining.add(thisSent.sent->GetId());
			}
		}
		_scoredSentences.clear();
		return getRetVal(true, str.c_str());
	}
	catch(UnrecoverableException &e) {
		return getRetVal(false, e.getMessage());
	}
}

wstring PIdFActiveLearning::GetNextSentences(int num_to_select, int context_size)
{
	try {
		PIdFActiveLearningSentence* alSent;
		bool corpus_reset = false;
		std::wstring str = L"";
		int nadded = 0;
		PIdFActiveLearningSentence* context[MAX_CONTEXT_SIZE];
		PIdFSentence* sentence;

		std::vector<DTObservation *> observations;
		int tags[MAX_SENTENCE_TOKENS+2];

		Token blankToken(Symbol(L"NULL"));
		Symbol blankLCSymbol = Symbol(L"NULL");
		Symbol blankWordFeatures = Symbol(L"NULL");
		WordClusterClass blankWordClass = WordClusterClass::nullCluster();

		while (nadded < num_to_select) {
			if (!moreALSentences()) {
				if (!corpus_reset) {
					seekToFirstALSentence();
					corpus_reset = true;
				} else
					break;
				//out of sentences....
			}

			alSent = getNextALSentence();
			sentence = alSent->GetPIdFSentence();

			if (_allowSentenceRepeats ||
				(!_inTraining.lookup(alSent->GetId()) && !_inTesting.lookup(alSent->GetId())))
			{
				int n_observations = sentence->getLength() + 2;
				//populate an observation

				observations[0] = _new TokenObservation();
				static_cast<TokenObservation*>(observations[0])->populate(blankToken, blankLCSymbol, blankWordFeatures, blankWordClass, 0, 0);
				for (int j = 0; j < sentence->getLength(); j++) {
					PIdFModel::populateObservation(
						static_cast<TokenObservation*>(observations[j+1]),
						_wordFeatures, sentence->getWord(j), j == 0, 0, false, false, 0);
				}
				static_cast<TokenObservation*>(observations[n_observations - 1])
					->populate(blankToken, blankLCSymbol, blankWordFeatures,
					blankWordClass, 0, 0);
				double score = _decoder->decodeAllTags(observations, 0 , tags);
				for(int k=1; k < (n_observations-1); k++){
					sentence->setTag(k-1,tags[k]);
				}
				
				ActiveLearningSentAndScore thisSent(score, alSent, 0, true);
				getContext(context, context_size, thisSent.sent);
				thisSent.sent->AppendSentenceWithContext(str, _tagSet, context, context_size, true);
				nadded++;
			}
		}
		//wprintf (L"%s", str.c_str());
		return getRetVal(true, str.c_str());
	}
	catch(UnrecoverableException &e) {
		return getRetVal(false, e.getMessage());
	}
}


void PIdFActiveLearning::clearCorpus(){
	seekToFirstALSentence();
	while(moreALSentences()){
		getNextALSentence()->deleteMembers();
	}
	seekToFirstALSentence();
	while(_curALSentBlock !=0){
		_curALSentBlock->n_sentences = 0;
		_curALSentBlock = _curALSentBlock->next;
	}
}

bool PIdFActiveLearning::moreALSentences() {
	return _curALSentBlock != 0 &&
		   _curAL_sent_no < _curALSentBlock->n_sentences;
}

bool PIdFActiveLearning::moreSentences(){
		return _curSentBlock != 0 &&
		   _cur_sent_no < _curSentBlock->n_sentences;

}

bool PIdFActiveLearning::moreTestingSentences(){
		return _curTestingSentBlock != 0 &&
		   _cur_testing_sent_no < _curTestingSentBlock->n_sentences;
}

PIdFActiveLearningSentence *PIdFActiveLearning::getNextALSentence() {
	if (moreALSentences()) {
		PIdFActiveLearningSentence *result = &_curALSentBlock->sentences[_curAL_sent_no];

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
PIdFActiveLearningSentence *PIdFActiveLearning::getNextSentence() {
	if (moreSentences()) {
		PIdFActiveLearningSentence *result = &_curSentBlock->sentences[_cur_sent_no];

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

PIdFActiveLearningSentence *PIdFActiveLearning::getNextTestingSentence() {
	if (moreTestingSentences()) {
		PIdFActiveLearningSentence *result = &_curTestingSentBlock->sentences[_cur_testing_sent_no];

		// move to next sentence for next call
		_cur_testing_sent_no++;
		if (_cur_testing_sent_no == SentenceBlock::BLOCK_SIZE) {
			_curTestingSentBlock = _curTestingSentBlock->next;
			_cur_testing_sent_no = 0;
		}

		return result;
	}
	else {
		return 0;
	}
}

void PIdFActiveLearning::seekToFirstALSentence() {
	_curALSentBlock = _firstALSentBlock;
	_curAL_sent_no = 0;
}
void PIdFActiveLearning::seekToFirstSentence() {
	_curSentBlock = _firstSentBlock;
	_cur_sent_no = 0;
}
void PIdFActiveLearning::seekToFirstTestingSentence() {
	_curTestingSentBlock = _firstTestingSentBlock;
	_cur_testing_sent_no = 0;
}

PIdFSentence* PIdFActiveLearning::makePIdFSentence(const TokenSequence* toks,
												   const NameOffset nameInfo[], int nann)
{
	PIdFSentence* sent  = _new PIdFSentence(_tagSet, *toks);
	wchar_t tagbuff[105];
	for(int i=0; i< nann; i++){
		int starttok = getTokenFromOffset(toks, nameInfo[i].startoffset);
		int endtok = getTokenFromOffset(toks, nameInfo[i].endoffset);
		Symbol type = nameInfo[i].type;
		wcsncpy(tagbuff, type.to_string(), 100);
		wcscat(tagbuff, L"-ST");
		Symbol sttag = Symbol(tagbuff);
		wcsncpy(tagbuff, type.to_string(), 100);
		wcscat(tagbuff, L"-CO");
		Symbol cotag = Symbol(tagbuff);
		if(sent->getTag(starttok) == -1){
			sent->setTag(starttok, _tagSet->getTagIndex(sttag));
		}
		else{
			throw UnexpectedInputException("PIdFActiveLearning::makePIdFSentence()",
				"More than one annotation per Token");
		}
		for(int j =starttok+1; j<=endtok; j++){
			if(sent->getTag(j) == -1){
				sent->setTag(j, _tagSet->getTagIndex(cotag));
			}
			else{
				throw UnexpectedInputException("PIdFActiveLearning::makePIdFSentence()",
				"More than one annotation per Token");
			}
		}
	}
	//add none tags
	int nonest = _tagSet->getTagIndex(_tagSet->getNoneSTTag());
	int noneco = _tagSet->getTagIndex(_tagSet->getNoneCOTag());
	if(sent->getTag(0) == -1){
		sent->setTag(0, nonest);
	}
	for(int j=1; j< sent->getLength(); j++){
		if(sent->getTag(j) == -1){
			int prevtag = sent->getTag(j-1);
			if( (prevtag== nonest) || (prevtag == noneco)){
				sent->setTag(j, noneco);
			}
			else{
				sent->setTag(j, nonest);

			}
		}
	}
	return sent;
}

TokenSequence* PIdFActiveLearning::adjustTokenizationToNames(const TokenSequence* toks,
															 const NameOffset nameInfo[], int nann,
															 const LocatedString* sentenceString)
{
	int starttok;
	int endtok;

	int inName[MAX_SENTENCE_TOKENS][MAX_NAMES_PER_SPACE_SEP_TOK+1];
	Token* newTokens[MAX_SENTENCE_TOKENS];

	int i;
	for(i=0; i<toks->getNTokens(); i++){
		inName[i][0] =0;
	}
	for(i=0; i<nann; i++){
		starttok= getTokenFromOffset(toks, nameInfo[i].startoffset);
		endtok = getTokenFromOffset(toks, nameInfo[i].endoffset);
		inName[starttok][inName[starttok][0]+1] = i;
		inName[starttok][0]++;

		if(starttok != endtok){
			inName[endtok][inName[endtok][0]+1] = i;
			inName[endtok][0]++;

		}
	}
	int currTok =0;
	for(i=0; i< toks->getNTokens(); i++){
		//this token isnt in a name, just add it to the newTokens
		if(inName[i][0] == 0){
			newTokens[currTok++] = _new Token(*toks->getToken(i));
		}
		//simple case only one name assigned to this token
		else if(inName[i][0] == 1){
			Token* nameParts[3];
			int nnewtoks = splitNameTokens(toks->getToken(i),
				nameInfo[inName[i][1]].startoffset, nameInfo[(inName[i][1])].endoffset,
				sentenceString, nameParts);
			for(int j=0; j<nnewtoks; j++){
				newTokens[currTok++] = nameParts[j];
			}
		}
		//hardest case multiple names assigned to a single token
		else{
			Token* nameParts[2*MAX_NAMES_PER_SPACE_SEP_TOK + 1];
			int nnewtoks = splitNameTokens(toks->getToken(i), inName[i], nameInfo, sentenceString, nameParts);
			for(int j=0; j<nnewtoks; j++){
				newTokens[currTok++] = nameParts[j];
			}
/*			std::cout<<"problem name: starttoken"<<i
				<<" nnames: "<<inName[i][0]<<std::endl;


			throw UnexpectedInputException("PIdFActiveLearning::AdjustTokenizationToNames()",
				"More than one annotation per Token");
*/
		}
	}
	return _new TokenSequence(0, currTok, newTokens);
}

int PIdFActiveLearning::splitNameTokens(const Token* origTok, int inName[], const NameOffset nameInfo[], const LocatedString *sentenceString, Token** newTokens) {
	int nnames = inName[0];
	int sorted_names[MAX_NAMES_PER_SPACE_SEP_TOK + 1];
	sorted_names[0] = nnames;
	EDTOffset min_offset(9999);
	EDTOffset prev_offset;
	int token_number = -1;
	// sort the names by their start offsets
	for (int j = 1; j <= nnames; j++) {
		for (int i = 1; i <= nnames; i++) {
			EDTOffset offset = nameInfo[inName[i]].startoffset;
			if ((offset < min_offset) && (offset > prev_offset)) {
				min_offset = offset;
				token_number = i;
			}
		}
		sorted_names[j] = inName[token_number];
		prev_offset = min_offset;
		min_offset = EDTOffset(9999);
	}

	Token * token_to_split = _new Token(*origTok);
	int currTok = 0;
	for (int i = 1; i <= nnames; i++) {
		Token* nameParts[3];
		int nnewtoks = splitNameTokens(token_to_split,
			nameInfo[sorted_names[i]].startoffset, nameInfo[(sorted_names[i])].endoffset,
			sentenceString, nameParts);
		for(int j = 0; j < nnewtoks-1; j++){
			newTokens[currTok] = nameParts[j];
			currTok++;
		}
		token_to_split = nameParts[nnewtoks - 1];
	}
	newTokens[currTok] = token_to_split;

	return currTok+1;
}

int PIdFActiveLearning::splitNameTokens(const Token* origTok, EDTOffset namestart,
										EDTOffset nameend, const LocatedString *sentenceString, Token** newTokens)
{
	if((namestart <= origTok->getStartEDTOffset()) && (nameend >= origTok->getEndEDTOffset())){
		newTokens[0] = _new Token(*origTok);
		return 1;
	}
	const wchar_t* toktxt = origTok->getSymbol().to_string();

	if((namestart <= origTok->getStartEDTOffset()) && (nameend <= origTok->getEndEDTOffset())){
		int start_pos = sentenceString->positionOfStartOffset(origTok->getStartEDTOffset());
		int split_pos = sentenceString->positionOfEndOffset(nameend);
		int end_pos = sentenceString->positionOfEndOffset(origTok->getEndEDTOffset());
		newTokens[0] = _new Token(sentenceString, start_pos, split_pos+1);
		newTokens[1] = _new Token(sentenceString, split_pos+1, end_pos+1);
		return 2;
	}
	if((namestart >= origTok->getStartEDTOffset()) && (nameend >= origTok->getEndEDTOffset())){
		int start_pos = sentenceString->positionOfStartOffset(origTok->getStartEDTOffset());
		int split_pos = sentenceString->positionOfEndOffset(namestart)-1;
		int end_pos = sentenceString->positionOfEndOffset(origTok->getEndEDTOffset());
		newTokens[0] = _new Token(sentenceString, start_pos, split_pos+1);
		newTokens[1] = _new Token(sentenceString, split_pos+1, end_pos+1);
		return 2;
	}
	if((namestart >= origTok->getStartEDTOffset()) && (nameend <= origTok->getEndEDTOffset())){
		int start_pos = sentenceString->positionOfStartOffset(origTok->getStartEDTOffset());
		int split1_pos = sentenceString->positionOfEndOffset(namestart)-1;
		int split2_pos = sentenceString->positionOfEndOffset(nameend);
		int end_pos = sentenceString->positionOfEndOffset(origTok->getEndEDTOffset());
		newTokens[0] = _new Token(sentenceString, start_pos, split1_pos+1);
		newTokens[1] = _new Token(sentenceString, split1_pos+1, split2_pos+1);
		newTokens[2] = _new Token(sentenceString, split2_pos+1, end_pos+1);
		return 3;
	}
	return 0;
}
void PIdFActiveLearning::wcsubstr(const wchar_t* str, int start, int end, wchar_t* substr){
	int j =0;
	for(int i=start ; i< end; i++, j++){
		substr[j] = str[i];
	}
	substr[j] = L'\0';
}


// Note the start/end offsets are used to set *both* the character *and* the EDT offset values.
int PIdFActiveLearning::readTokensCorpus(UTF8InputStream &in, SentIdToTokensMap &idToTokensTable) {
/*
	<SENTENCE ID=sentence_id>
		<TOKEN START="5" END="8">token1</TOKEN>
		<TOKEN START="10" END="15">token2</TOKEN>
		...
	</SENTENCE>
	<SENTENCE>
	...
*/
	UTF8Token tok;
	std::wstring line;
	Symbol sentId;
	GrowableArray<Token *> tokens;
	Token * token_array[500];
	int sent_no = -1;
	while (!in.eof()) {
		int start, end, curr_char = 0;
		wchar_t str_buffer[500];
		in.getLine(line);
		if (line.length() < 1)
			continue;
		const wchar_t* line_str = line.c_str();
		wcsubstr(line_str, 1, 6, str_buffer);
		if(wcsncmp(str_buffer, L"SENTE", 5) == 0) {
			//ID="..."
			sent_no++;
			curr_char = findNextMatchingCharRequired(line_str, curr_char, L'\"');
			curr_char++;
			start = curr_char;
			curr_char = findNextMatchingCharRequired(line_str, curr_char, L'\"');
			end = curr_char;
			curr_char++;
			wcsubstr(line_str, start, end, str_buffer);
			sentId = Symbol(str_buffer);
		} else
			if (wcsncmp(str_buffer, L"TOKEN", 5) == 0) {
				//START="..."
				curr_char = findNextMatchingCharRequired(line_str, curr_char, L'\"');
				curr_char++;
				start = curr_char;
				curr_char = findNextMatchingCharRequired(line_str, curr_char, L'\"');
				end = curr_char;
				curr_char++;
				wcsubstr(line_str, start, end, str_buffer);
				int start_offset = _wtoi(str_buffer);
				OffsetGroup start_offset_group((ByteOffset()), CharOffset(start_offset), EDTOffset(start_offset), ASRTime());


				//END=".."
				curr_char = findNextMatchingCharRequired(line_str, curr_char, L'\"');
				curr_char++;
				start = curr_char;
				curr_char = findNextMatchingCharRequired(line_str, curr_char, L'\"');
				end = curr_char;
				curr_char++;
				wcsubstr(line_str, start, end, str_buffer);
				int end_offset = _wtoi(str_buffer);
				OffsetGroup end_offset_group((ByteOffset()), CharOffset(end_offset), EDTOffset(end_offset), ASRTime());

				//Token string
				start = findNextMatchingCharRequired(line_str, curr_char, L'>');
				start++;
				end = findNextMatchingCharRequired(line_str, start, L'<');

				wcsubstr(line_str, start, end, str_buffer);
				tokens.add(_new Token(start_offset_group, end_offset_group, Symbol(str_buffer)));
			} else {
				if (wcsncmp(str_buffer, L"/SENT", 5) == 0) {
					int ntokens = tokens.length();
					for (int i = 0; i < ntokens; i++) {
						token_array[i] = tokens[i];
					}
					for (int j = 0; j < ntokens; j++) {
						tokens.removeLast();
					}
					idToTokensTable[sentId] = _new TokenSequence(sent_no, ntokens, token_array);
				}
			}
	}
	return static_cast<int>(idToTokensTable.size());
}

int PIdFActiveLearning::readSavedSentence(UTF8InputStream &in,
											 LocatedString*& sentString, Symbol& sentId,
											 NameOffset nameInfo[]){
	UTF8Token tok;
/*
  <SENTENCE ID=sentence_id>
    012345678901234
	<DISPLAY_TEXT>This is the sentence text.</DISPLAY_TEXT>
    <ANNOTATION TYPE=PER START_OFFSET=5 END_OFFSET=8 SOURCE=Example1/>
    <ANNOTATION TYPE=GPE START_OFFSET=21 END_OFFSET=27 SOURCE=Example1/>
  </SENTENCE>
*/
	std::wstring line;
	in >> tok;	//<SENTENCE, ignore this
	if(wcscmp(tok.chars(), L"<SENTENCE") !=0){
		if((wcslen(tok.chars()) < 1) || iswspace(tok.chars()[0])){
			return -2;
		}
		return -1;
	}
	in >> tok;	//id="ID">
	wchar_t idstring[500];
	int len = static_cast<int>( wcslen(tok.chars()));
	wcsubstr(tok.chars(), 4, len-2, idstring);

	sentId = Symbol(idstring);

	// getting the sentence text
	std::wstring sent_txt = getSentenceTextFromStream(in);
	sentString = _new LocatedString(sent_txt.c_str());

	in.getLine(line);
	Symbol type;
	int nann = 0;
	while(wcsncmp(line.c_str(), L"</SENTENCE>", 11) != 0){
		if(nann >= MAX_NAMES_PER_SENTENCE){
			return -1;
		}
		if(!parseAnnotationLine(line.c_str(), nameInfo[nann].type, nameInfo[nann].startoffset, nameInfo[nann].endoffset)){
			return -1;
		}
		nann++;
		in.getLine(line);
	}
	return nann;

}

bool PIdFActiveLearning::parseAnnotationLine(const wchar_t* line, Symbol& type, EDTOffset& start_offset, EDTOffset& end_offset){
//    <ANNOTATION TYPE=PER START_OFFSET=5 END_OFFSET=8 SOURCE=Example1/>
	try{
		int start, end, curr_char = 0;
		wchar_t str_buffer[500];
		int len = static_cast<int>(wcslen(line));
		while((curr_char < len) && iswspace(line[curr_char])){
			curr_char++;
		}
		wcsubstr(line, curr_char+1, curr_char+11, str_buffer);
		if(wcsncmp(str_buffer, L"ANNOTATION",10) !=0){
			return false;
		}
		//TYPE="..."
		curr_char = findNextMatchingCharRequired(line, curr_char, L'\"');
		curr_char++;
		start = curr_char;
		curr_char = findNextMatchingCharRequired(line, curr_char, L'\"');
		end = curr_char;
		curr_char++;
		wcsubstr(line, start, end, str_buffer);
		type = Symbol(str_buffer);

		//START_OFFSET=..
		curr_char = findNextMatchingCharRequired(line, curr_char, L'\"');
		curr_char++;
		start = curr_char;
		curr_char = findNextMatchingCharRequired(line, curr_char, L'\"');
		end = curr_char;
		curr_char++;
		wcsubstr(line, start, end, str_buffer);
		start_offset = EDTOffset(_wtoi(str_buffer));

		//END_OFFSET=..
		curr_char = findNextMatchingCharRequired(line, curr_char, L'\"');
		curr_char++;
		start = curr_char;
		curr_char = findNextMatchingCharRequired(line, curr_char, L'\"');
		end = curr_char;
		curr_char++;
		wcsubstr(line, start, end, str_buffer);
		end_offset = EDTOffset(_wtoi(str_buffer));
		return true;

	}

	catch(UnexpectedInputException& e){
		e.prependToMessage("PIdFActiveLearning::parseAnnotationLine() ");
		throw;
	}
}



wstring PIdFActiveLearning::getRetVal(bool ok, const wchar_t* txt){
	wstring retval(L"<RETURN>\n");
	if(ok){
		retval +=L"\t<RETURN_CODE>OK</RETURN_CODE>\n";
	}
	else{
		retval += L"\t<RETURN_CODE>ERROR</RETURN_CODE>\n";
	}
	retval +=L"\t<RETURN_VALUE>";
	retval += txt;
	retval += L"</RETURN_VALUE>\n";
	retval += L"</RETURN>";

	return retval;
}
wstring PIdFActiveLearning::getRetVal(bool ok, const char* txt){
	wstring retval(L"<RETURN>\n");
	if(strlen(txt) >999){
		char errbuffer[1000];
		strcpy(errbuffer, "Invalid Conversion, String too long\nFirst 900 characters: ");
		strncat(errbuffer, txt, 900);
		return getRetVal(false, errbuffer);
	}
	wchar_t conversionbuffer[1000];
	mbstowcs(conversionbuffer, txt, 1000);

	if(ok){
		retval +=L"\t<RETURN_CODE>OK</RETURN_CODE>\n";
	}
	else{
		retval += L"\t<RETURN_CODE>ERROR</RETURN_CODE>\n";
	}
	retval +=L"\t<RETURN_VALUE>";

	retval += conversionbuffer;
	retval += L"</RETURN_VALUE>\n";
	retval += L"</RETURN>";

	return retval;
}



int PIdFActiveLearning::getTokenFromOffset(const TokenSequence* tok, EDTOffset offset){
	int midtoken = tok->getNTokens()/2;
	EDTOffset midoffset = tok->getToken(midtoken)->getStartEDTOffset();
	if(offset == midoffset){
		return midtoken;
	}
	if(offset < midoffset){
		for(int i=0; i< midtoken; i++){
			if((tok->getToken(i)->getStartEDTOffset()<= offset) &&
				(tok->getToken(i)->getEndEDTOffset()>= offset)){
					return i;
				}
		}
	}
	if(offset > midoffset){
		for(int i = midtoken; i< tok->getNTokens(); i++){
			if((tok->getToken(i)->getStartEDTOffset()<= offset) &&
				(tok->getToken(i)->getEndEDTOffset()>= offset)){
					return i;
				}
		}
	}
	return -1;
}


PIdFActiveLearningSentence* PIdFActiveLearning::addActiveLearningSentence(const PIdFActiveLearningSentence& sent){
	if (_lastALSentBlock == 0) {
		_firstALSentBlock = _new SentenceBlock();
		_lastALSentBlock = _firstALSentBlock;
	}
	else if (_lastALSentBlock->n_sentences == SentenceBlock::BLOCK_SIZE) {
		_lastALSentBlock->next = _new SentenceBlock();
		_lastALSentBlock = _lastALSentBlock->next;
	}
	_lastALSentBlock->sentences[_lastALSentBlock->n_sentences++].populate(sent);
	return &_lastALSentBlock->sentences[_lastALSentBlock->n_sentences -1];

}

PIdFActiveLearningSentence* PIdFActiveLearning::addTrainingSentence(const PIdFActiveLearningSentence& sent){
	if (_lastSentBlock == 0) {
		_firstSentBlock = _new SentenceBlock();
		_lastSentBlock = _firstSentBlock;
	}
	else if (_lastSentBlock->n_sentences == SentenceBlock::BLOCK_SIZE) {
		_lastSentBlock->next = _new SentenceBlock();
		_lastSentBlock = _lastSentBlock->next;
	}
	_lastSentBlock->sentences[_lastSentBlock->n_sentences++].populate(sent);
	return &_lastSentBlock->sentences[_lastSentBlock->n_sentences -1];
}

PIdFActiveLearningSentence* PIdFActiveLearning::addTestingSentence(const PIdFActiveLearningSentence& sent){
	if (_lastTestingSentBlock == 0) {
		_firstTestingSentBlock = _new SentenceBlock();
		_lastTestingSentBlock = _firstTestingSentBlock;
	}
	else if (_lastTestingSentBlock->n_sentences == SentenceBlock::BLOCK_SIZE) {
		_lastTestingSentBlock->next = _new SentenceBlock();
		_lastTestingSentBlock = _lastTestingSentBlock->next;
	}
	_lastTestingSentBlock->sentences[_lastTestingSentBlock->n_sentences++].populate(sent);
	return &_lastTestingSentBlock->sentences[_lastTestingSentBlock->n_sentences -1];

}

double PIdFActiveLearning::trainEpoch() {
	std::vector<DTObservation *> observations;
	int tags[MAX_SENTENCE_TOKENS+2];

	Token blankToken(Symbol(L"NULL"));
	Symbol blankLCSymbol = Symbol(L"NULL");
	Symbol blankWordFeatures = Symbol(L"NULL");
	WordClusterClass blankWordClass = WordClusterClass::nullCluster();

	seekToFirstSentence();
	int sentence_n = 0;
	int n_correct = 0;
	int total_ncorrect = 0;
	while (moreSentences()) {
		PIdFSentence *sentence = getNextSentence()->GetPIdFSentence();
		int n_observations = sentence->getLength() + 2;

		static_cast<TokenObservation*>(observations[0])->populate(
			blankToken, blankLCSymbol, blankWordFeatures, blankWordClass, 0, 0);
		for (int i = 0; i < sentence->getLength(); i++) {
			PIdFModel::populateObservation(
				static_cast<TokenObservation*>(observations[i+1]),
				_wordFeatures, sentence->getWord(i), i == 0, 0, false, false, 0);
		}
		static_cast<TokenObservation*>(observations[n_observations - 1])
			->populate(blankToken, blankLCSymbol, blankWordFeatures,
					   blankWordClass, 0, 0);

		tags[0] = _tagSet->getStartTagIndex();
		for (int j = 0; j < sentence->getLength(); j++)
			tags[j+1] = sentence->getTag(j);
		tags[n_observations-1] = _tagSet->getEndTagIndex();

		bool correct = _decoder->train(observations, tags, 1);
		if (correct){
			n_correct++;
			total_ncorrect++;
		}
		// every so-often, add weights to _weightSums
		if (sentence_n % _weightsum_granularity == 0) {
			for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
				 iter != _weights->end(); ++iter)
			{
				(*iter).second.addToSum();
			}
		}
		sentence_n++;


	}

	return (double)total_ncorrect/sentence_n;
}

double PIdFActiveLearning::trainIncrementalEpoch() {
	std::vector<DTObservation *> observations;
	int tags[MAX_SENTENCE_TOKENS+2];

	Token blankToken(Symbol(L"NULL"));
	Symbol blankLCSymbol = Symbol(L"NULL");
	Symbol blankWordFeatures = Symbol(L"NULL");
	WordClusterClass blankWordClass = WordClusterClass::nullCluster();

	int sentence_n = 0;
	int n_correct = 0;
	int total_ncorrect = 0;
	for(int i=0; i<_currentSentences.length(); i++) {
		PIdFSentence *sentence = _currentSentences[i]->GetPIdFSentence();

		int n_observations = sentence->getLength() + 2;

		static_cast<TokenObservation*>(observations[0])->populate(
			blankToken, blankLCSymbol, blankWordFeatures, blankWordClass, 0, 0);
		for (int i = 0; i < sentence->getLength(); i++) {
			PIdFModel::populateObservation(
				static_cast<TokenObservation*>(observations[i+1]),
				_wordFeatures, sentence->getWord(i), i == 0, 0, false, false, 0);
		}
		static_cast<TokenObservation*>(observations[n_observations - 1])
			->populate(blankToken, blankLCSymbol, blankWordFeatures,
					   blankWordClass, 0, 0);

		tags[0] = _tagSet->getStartTagIndex();
		for (int j = 0; j < sentence->getLength(); j++)
			tags[j+1] = sentence->getTag(j);
		tags[n_observations-1] = _tagSet->getEndTagIndex();

		bool correct = _decoder->train(observations, tags, 1);
		if (correct){
			n_correct++;
			total_ncorrect++;
		}

		sentence_n++;


	}
	return (double)total_ncorrect/sentence_n;
}


void PIdFActiveLearning::writeTrainingSentences() {
	UTF8OutputStream out;
	std::string file = _model_file + "-training-sentences";
	out.open(file.c_str());
	if (out.fail()) {
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::writeTrainingSentences()",
			"Could not open model training sentence file for writing");
	}

	UTF8OutputStream tokens_out;
	std::string tokens_file = file + ".tokens";
	tokens_out.open(tokens_file.c_str());
	if (tokens_out.fail()) {
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::writeTrainingSentences()",
			"Could not open model tokens training sentence file for writing");
	}

	seekToFirstSentence();
	PIdFActiveLearningSentence* sent;
	while(moreSentences()){
		sent = getNextSentence();
		Symbol id = sent->GetId();
		TokenSequence * tokens;
		SentIdToTokensMap::iterator it = _trainingIdToTokens.find(id);
		if (it == _trainingIdToTokens.end()) {
			SentIdToTokensMap::iterator it_new = _idToTokens.find(id);
			if (it_new == _idToTokens.end()) {
				continue;
			} else {
				_trainingIdToTokens[id] = (*it_new).second;
				tokens = (*it_new).second;
			}
		} else {
			tokens = (*it).second;
		}
		sent->WriteTokenSentence(tokens_out, id, tokens);
		sent->WriteSentence(out, _tagSet);
	}
	tokens_out.close();
	out.close();
}

void PIdFActiveLearning::writeTestingSentences() {
	UTF8OutputStream out;
	std::string file = _model_file + "-testing-sentences";
	out.open(file.c_str());
	if (out.fail()) {
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::writeTestingSentences()",
			"Could not open test set sentence file for writing");
	}

	UTF8OutputStream tokens_out;
	std::string tokens_file = file + ".tokens";
	tokens_out.open(tokens_file.c_str());
	if (tokens_out.fail()) {
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::writeTrainingSentences()",
			"Could not open test set tokens sentence file for writing");
	}

	seekToFirstTestingSentence();
	PIdFActiveLearningSentence* sent;
	while(moreTestingSentences()){
		sent = getNextTestingSentence();
		Symbol id = sent->GetId();
		TokenSequence * tokens;
		SentIdToTokensMap::iterator it = _testingIdToTokens.find(id);
		if (it == _testingIdToTokens.end()) {
			SentIdToTokensMap::iterator it_new = _idToTokens.find(id);
			if (it_new == _idToTokens.end()) {
				continue;
			} else {
				_testingIdToTokens[id] = (*it_new).second;
				tokens = (*it_new).second;
			}
		} else {
			tokens = (*it).second;
		}
		sent->WriteTokenSentence(tokens_out, id, tokens);
		sent->WriteSentence(out, _tagSet);
	}
	tokens_out.close();
	out.close();
}


void PIdFActiveLearning::writeTransitions() {
	std::string file = _model_file + "-transitions";
	_tagSet->writeTransitions(file.c_str());
}

void PIdFActiveLearning::writeWeights() {
	UTF8OutputStream out;
	out.open(_model_file.c_str());

	if (out.fail()) {
		SessionLogger::err("SERIF") << "Write failure!\n";
		throw UnexpectedInputException("PIdFSimActiveLearningTrainer::writeWeights()",
			"Could not open model file for writing");
	}

	DTFeature::writeSumWeights(*_weights, out);

	out.close();
}

bool PIdFActiveLearning::sentHasName(PIdFSentence* sent){
	int nwords = sent->getLength();
	int nonest = _tagSet->getNoneTagIndex();
	int noneco = _tagSet->getTagIndex(_tagSet->getNoneCOTag());
	int starttag = _tagSet->getStartTagIndex();
	int endtag = _tagSet->getEndTagIndex();
	for(int i=0; i< nwords; i++){
		if(!((sent->getTag(i) == noneco) ||
			(sent->getTag(i) == nonest)||
			(sent->getTag(i) == starttag) ||
			(sent->getTag(i) == endtag))){
				return true;
			}
	}
	return false;
}



void PIdFActiveLearning::getContext(PIdFActiveLearningSentence* context[], int ncontext,
									PIdFActiveLearningSentence* sent)
{
	Symbol docid = sent->GetDocId();
	int sentnum = sent->GetSentenceNumber();

	SentenceBlock* startblock  = _curALSentBlock;
	int currsent = _curAL_sent_no;
//	seekToFirstALSentence();
	int surround = ncontext/2;
	int context_start_num = sentnum - surround;
	int context_end_num = sentnum + surround;
	wchar_t idbuff[500];
	wchar_t numbuff[50];
	for(int k = 0; k< ncontext; k++){
		context[k] =0;
	}
	int cn =0;
	for(int i = context_start_num; i<sentnum; i++){
		if(i <0){
			context[cn++] =0;
		}
		else{
			wcscpy(idbuff, docid.to_string());
			wcscat(idbuff, L"-");
#ifdef _WIN32
			_itow(i, numbuff, 10);
#else
			swprintf (numbuff, sizeof(numbuff)/sizeof(numbuff[0]), L"%d", i);
#endif

			wcscat(idbuff, numbuff);
			SentenceIdMap::iterator it = _idToSentence.find(Symbol(idbuff));
			if(it == _idToSentence.end()){
				//throw an exception
			}

			PIdFActiveLearningSentence* prev = (*it).second;
			context[cn++] = prev;
		}
	}
	for(int j = sentnum+1; j<= context_end_num; j++){
		wcscpy(idbuff, docid.to_string());
		wcscat(idbuff, L"-");
#if defined(_WIN32)
		_itow(j, numbuff, 10);
#else
		swprintf (numbuff, sizeof(numbuff)/sizeof(numbuff[0]), L"%d", j);
#endif
		wcscat(idbuff, numbuff);

		SentenceIdMap::iterator it = _idToSentence.find(Symbol(idbuff));
		if(it == _idToSentence.end()){
			context[cn++] = 0;
		}
		else{
			PIdFActiveLearningSentence* prev = (*it).second;
			context[cn++] = prev;
		}
	}

}

int PIdFActiveLearning::findNextMatchingChar(const wchar_t* txt, int start, wchar_t c){
	int length = static_cast<int>(wcslen(txt));
	int curr_char = start;
	while((curr_char < length) && txt[curr_char] != c){
		curr_char++;
	}
	if(txt[curr_char] == c){
		return curr_char;
	}
	else{
		return -1;
	}
}
int PIdFActiveLearning::findNextMatchingCharRequired(const wchar_t* txt, int start, wchar_t c){
	int retval = findNextMatchingChar(txt, start, c);
	if(retval >= 0){
		return retval;
	}
	else{
		char buffer[500];
		snprintf(buffer, 500, "Couldn't find character: after char # %d ", start);
		throw UnexpectedInputException("PIdFActiveLearning:findNextMatchingCharRequired()",
					buffer);
		return -1;
	}
}

int PIdFActiveLearning::readGuiInput(const wchar_t* ann_sentences, size_t length, SentIdToTokensMap &idToTokensTable) {
/*
  <SENTENCE ID=sentence_id>
  012345678901234
			1
	<DISPLAY_TEXT>This is the sentence text.</DISPLAY_TEXT>
  </SENTENCE>
*/
	try{
		_currentSentences.setLength(0);
		int n_sentences =0;
		//int length = static_cast<int>(wcslen(ann_sentences));
		wchar_t sentence_buffer[5000];
		wchar_t idstring[500];
		NameOffset nameInfo[MAX_NAMES_PER_SENTENCE];
		int curr_char = 0;
		Document* doc = _new Document(Symbol(L"FAKE_DOCUMENT"));

		while(curr_char < (int)length){
			//static_cast<int>
			//skip <SENTENCE ..... >
			curr_char = findNextMatchingCharRequired(ann_sentences, curr_char, L'<');
			//get id
			curr_char = findNextMatchingCharRequired(ann_sentences, curr_char, L'\"');

			curr_char++;
			int idstart = curr_char;
			curr_char = findNextMatchingCharRequired(ann_sentences, curr_char, L'\"');

			int idend = curr_char;
			wcsubstr(ann_sentences, idstart, idend, idstring);
			Symbol id = Symbol(idstring);
			//skip <DISP.....>

			curr_char = findNextMatchingCharRequired(ann_sentences, curr_char, L'<');
			curr_char = findNextMatchingCharRequired(ann_sentences, curr_char, L'>');

			curr_char++;
			int start = curr_char;
			//warning, there can be <, > in this text look for </DISPLAY_TEXT>
			wcscpy(sentence_buffer, L"");
			while(wcscmp(sentence_buffer, L"</DISPLAY_TEXT>") != 0){
				curr_char = findNextMatchingCharRequired(ann_sentences, curr_char, L'<');
				int tagstart = curr_char;
				curr_char++;
				wcsubstr(ann_sentences, tagstart, tagstart+15, sentence_buffer);
				//char outstr[1000];
				//wcstombs(outstr, sentence_buffer, 1000);
				//std::cout<<"\ttxt: "<<outstr<<std::endl;

			}
			int end = curr_char-1;


			int j=0;
			if((end - start) > 4999){
				throw UnexpectedInputException("PIdFActiveLearning:readGuiInput()",
						"Sentence Too Long");

			}
			wcsubstr(ann_sentences, start, end, sentence_buffer);
			LocatedString* string = _new LocatedString(sentence_buffer);
			//std::cout<<"display string: "<<std::endl;
			//string->dumpDetails(std::cout);
			//std::cout<<std::endl;

			//look for annotation
			//    <ANNOTATION TYPE=PER START_OFFSET=5 END_OFFSET=8 SOURCE=Example1/>

			bool foundann = true;
			int nann = 0;

			while(foundann){
				curr_char = findNextMatchingCharRequired(ann_sentences, curr_char, L'<');
				start = curr_char;
				curr_char = findNextMatchingCharRequired(ann_sentences, curr_char, L'>');
				end = curr_char;
				wcsubstr(ann_sentences, start, end, sentence_buffer);
				foundann = parseAnnotationLine(sentence_buffer, nameInfo[nann].type,
					nameInfo[nann].startoffset, nameInfo[nann].endoffset );
				if(foundann){
					nann++;
				}
			}
			//could error check to get rid of </SENTENCE>
			wcsubstr(ann_sentences, start, end+1, sentence_buffer);
			if(wcsncmp(sentence_buffer, L"</SENTENCE>", 11) !=0){
				char outstr[1000];
					wcstombs(outstr, sentence_buffer, 1000);
					SessionLogger::err("SERIF")<<"error: "<<outstr<<std::endl;
				throw UnexpectedInputException("PIdFActiveLearning:readGuiInput()",
							"Invalid sentence input- sentence not ended");
			}
			curr_char++;
			//_tokenizer->resetForNewSentence(doc, 0);
			TokenSequence * tokSequence;
			SentIdToTokensMap::iterator it = idToTokensTable.find(id);
			if (it == idToTokensTable.end()) {
				SessionLogger::info("SERIF") << "couldn't find ID: " << id.to_debug_string() << " in tokens file\n";
				//throw an exception
				while((curr_char < (int)length) && iswspace(ann_sentences[curr_char])){
					curr_char++;
				}
				continue;
			} else {
				tokSequence = (*it).second;
			}
			//TokenSequence * tokSequence = _idToTokens[id];
			PIdFActiveLearningSentence alsent = makeAdjustedPIdFActiveLearningSentence(id, string, tokSequence, nameInfo, nann, idToTokensTable);
			//PIdFActiveLearningSentence alsent = makePIdFActiveLearningSentence(id, string, nameInfo, nann);
			PIdFActiveLearningSentence* alsentpt = _new PIdFActiveLearningSentence();
			alsentpt->populate(alsent);

			_currentSentences.add(alsentpt);
			n_sentences++;
			while((curr_char < (int)length) && iswspace(ann_sentences[curr_char])){
				curr_char++;
			}
		}

		delete doc;
		return n_sentences;
	}
	catch(UnexpectedInputException& e){
		e.prependToMessage("PIdFActiveLearning::readGuiInput()");
		throw;
	}
}

int PIdFActiveLearning::readGuiInput(const wchar_t* ann_sentences, size_t length){
	return readGuiInput(ann_sentences, length, _idToTokens);
}

// Note the start/end offsets are used to set *both* the character *and* the EDT offset values.
int PIdFActiveLearning::readGuiTokensInput(const wchar_t* token_sents, size_t length, SentIdToTokensMap &idToTokensTable){
/*
	<SENTENCE ID=sentence_id>
		<TOKEN START="5" END="8">token1</TOKEN>
		<TOKEN START="10" END="15">token2</TOKEN>
		...
	</SENTENCE>
	<SENTENCE>
	...
*/
	try {
		if (length == 0)
			return 0;

		GrowableArray<Token *> tokens;
		Token * token_array[500];
		int sent_no = -1;
		LocatedString * loc_str = _new LocatedString(token_sents);
		int start_sent = loc_str->indexOf(L"<SENTENCE ");
		while (start_sent >= 0) {
			int end_sent = loc_str->indexOf(L"</SENTENCE>", start_sent + 10);
			if (end_sent < 0)
				break;
			sent_no++;
			LocatedString * sentence = loc_str->substring(start_sent, end_sent);
			// Get sentence ID
			int start = sentence->indexOf(L"ID=\"");
			int end = sentence->indexOf(L"\"", start + 4);
			LocatedString * id = sentence->substring(start + 4, end);
			Symbol sent_id = id->toSymbol();
			delete id;

			// Get tokens
			int start_token = sentence->indexOf(L"<TOKEN");
			while (start_token >= 0) {
				int end_token = sentence->indexOf(L"</TOKEN>", start_token);
				LocatedString * token = sentence->substring(start_token, end_token);
				// Get START value
				start = token->indexOf(L"START=\"");
				end = token->indexOf(L"\"", start + 7);
				LocatedString * value = token->substring(start + 7, end);
				const wchar_t * value_str = value->toString();
				int start_offset = _wtoi(value_str);
				OffsetGroup start_offset_group((ByteOffset()), CharOffset(start_offset), EDTOffset(start_offset), ASRTime());
				delete value;

				// Get END value
				start = token->indexOf(L"END=\"");
				end = token->indexOf(L"\"", start + 5);
				value = token->substring(start + 5, end);
				value_str = value->toString();
				int end_offset = _wtoi(value_str);
				OffsetGroup end_offset_group((ByteOffset()), CharOffset(end_offset), EDTOffset(end_offset), ASRTime());

				delete value;

				// Get token value
				start = token->indexOf(L">");
				value = token->substring(start + 1);
				Symbol token_value = value->toSymbol();
				delete value;
				tokens.add(_new Token(start_offset_group, end_offset_group, token_value));
				delete token;
				start_token = sentence->indexOf(L"<TOKEN", start_token + 6);
			}
			int ntokens = tokens.length();
			for (int i = 0; i < ntokens; i++) {
				token_array[i] = tokens[i];
			}
			for (int j = 0; j < ntokens; j++) {
				tokens.removeLast();
			}
			idToTokensTable[sent_id] = _new TokenSequence(sent_no, ntokens, token_array);
			delete sentence;
			start_sent = loc_str->indexOf(L"<SENTENCE ", start_sent + 10);
		}
		delete loc_str;

		return sent_no + 1;
	}
	catch(UnexpectedInputException& e){
		e.prependToMessage("PIdFActiveLearning::readGuiTokensInput()");
		throw;
	}
}


std::wstring PIdFActiveLearning::GetCorpusPointer(){
	if(!moreALSentences()){
		//return getRetVal(false, "PIdFActiveLearning::GetCorpusPointer(), You have finished the corpus");
		return getRetVal(true, L"");
	}
	else{
		return getRetVal(true, _curALSentBlock->sentences[_curAL_sent_no].GetId().to_string());
	}
}

void PIdFActiveLearning::writePIdFTrainingFile() {

	//read sentences
	std::string training_file = _model_file + "-sentences";
	std::string tokens_training_file = training_file + ".tokens";
	std::string pidf_file = _model_file + "-pidf-sentences";

	readSentencesFile(training_file.c_str(), tokens_training_file.c_str(), TRAIN);

	UTF8OutputStream out(pidf_file.c_str());
	if (out.fail()) {
		throw UnexpectedInputException("PIdFActiveLearning::WritePIdFTrainingFile()",
									"Could not open model training sentence file for writing");
	}
	
	seekToFirstSentence();
	while (moreSentences()) {
		getNextSentence()->GetPIdFSentence()->writeSexp(out);
	}
	
	out.close();
}

bool PIdFActiveLearning::fileExists(const char *file) 
{
	ifstream inp;
	inp.open(file, ifstream::in);
	if (inp.fail()) {
		inp.close();
		return false;
	}	
	inp.close();
	return true;
}

void PIdFActiveLearning::createEmptyFile(const char *file) 
{
	UTF8OutputStream out;
	out.open(file);
	out.close();
}


std::wstring PIdFActiveLearning::getSentenceTextFromStream(UTF8InputStream &in) {
	std::wstring sent_txt, line;
	bool add_new_line = false;

	in.getLine(line);
	const wchar_t* linestr = line.c_str();
	int start = findNextMatchingCharRequired(linestr, 0, L'>');
	start++;
	int end = findNextMatchingChar(line.c_str(),start, L'<');
	if(end!=-1) { // in case of <DISPLAY_TEXT>text...</DISPLAY_TEXT>
		sent_txt += line.substr(start, end-start);
	}else { // in case of <DISPLAY_TEXT>\n text...\n </DISPLAY_TEXT>
		while(end==-1) {
			in.getLine(line);
			end = findNextMatchingChar(line.c_str(),0, L'<');
			if(end==-1) {
				if(add_new_line)
					sent_txt += L'\n';
				sent_txt += line;
				add_new_line = true;
			}
		}
	}
	return sent_txt;
}


