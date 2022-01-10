// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P_IDF_ACTIVE_LEARNING_H
#define P_IDF_ACTIVE_LEARNING_H

#include "Generic/tokens/Tokenizer.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"
#include "Generic/morphSelection/MorphSelector.h"
#include "Generic/names/discmodel/PIdFActiveLearningSentence.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/discTagger/DTFeature.h"
#include "Generic/names/discmodel/PIdFSentence.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/SessionLogger.h"
#include "time.h"


//use STL implementations, maybe I should implement these myself.....
#include <iostream>
#include <algorithm>
#include <functional>
#include <vector>



class DTFeatureTypeSet;
class DTTagSet;
class TokenObservation;
class IdFWordFeatures;
class PDecoder;
/*Methods required by QuickLearn
	X	Initialize(string parameter_file) 
	Close()
	StopService()
	Save()
	X	ChangeCorpus(string new_corpus_path)
	X	Train(XML annotated_sentences, int epochs, bool incremental)
	X	Decode(XML sentences)
	X	SelectTraining(int training_pool_size, int num_to_select, 
			int context_size, int min_positive_results)

*/

using namespace std;

class PIdFActiveLearning {
private:
	class ActiveLearningSentAndScore{
		public:
			double margin;
			PIdFActiveLearningSentence* sent;
			int id;
			bool hasName;
			ActiveLearningSentAndScore(double m, PIdFActiveLearningSentence* s, int id, bool named): 
			margin(m), sent(s), id(0), hasName(named){};

			ActiveLearningSentAndScore():margin(0), sent(0), id(0), hasName(false){};
			
			

			bool operator<(const ActiveLearningSentAndScore& other) const
			{
				return margin < other.margin;
			}
	};
	class NameOffset{
	public:
			Symbol type;
			EDTOffset startoffset;
			EDTOffset endoffset;
	};
	class SentenceBlock {
	public:
		const static int BLOCK_SIZE = 10000;

		PIdFActiveLearningSentence sentences[BLOCK_SIZE];
		int n_sentences;
		SentenceBlock *next;

		SentenceBlock() : n_sentences(0), next(0) {}
	};
	
	SymbolHash _inTraining;
	SymbolHash _inTesting;

	typedef Symbol::HashMap<PIdFActiveLearningSentence*> SentenceIdMap;

	typedef Symbol::HashMap<TokenSequence*> SentIdToTokensMap;

	
	SentenceIdMap _idToSentence;
	SentIdToTokensMap _idToTokens;
	SentIdToTokensMap _trainingIdToTokens;
	SentIdToTokensMap _testingIdToTokens;
	
	
public:
	/** The default constructor gets all the information it needs from
	  * ParamReader. */

	PIdFActiveLearning();
	~PIdFActiveLearning();
	wstring Initialize(char* param_file);
	wstring ReadCorpus(char* corpus_file = 0);
	wstring ChangeCorpus(char* new_corpus_file);
	wstring Train(const wchar_t* ann_sentences, size_t str_length, const wchar_t* token_sents, size_t token_sents_length, int epochs, bool incremental);
	wstring AddToTestSet(const wchar_t* ann_sentences, size_t str_length, const wchar_t* token_sents, size_t token_sents_length);
	wstring SelectSentences(int training_pool_size, int num_to_select,
		int context_size, int min_positive_results);
	wstring GetNextSentences(int num_to_select, int context_size);
	wstring DecodeTraining(const wchar_t* ann_sentences, size_t length);
	wstring DecodeTestSet(const wchar_t* ann_sentences, size_t length);
	wstring DecodeFromCorpus(const wchar_t* ann_sentences, size_t length);
	wstring DecodeFile(const wchar_t * input_file);
	wstring Save();
	wstring SaveSentences(const wchar_t* ann_sentences, size_t str_length, const wchar_t* token_sents, size_t token_sents_length, const wchar_t * tokens_file);
	wstring Close();
	wstring GetCorpusPointer();
	//wstring StartService()	use new
	//wstring StopService();	use delete

	void writePIdFTrainingFile();

	static const int TRAIN = 1;
	static const int TEST = 2;

private:
	bool sentHasName(PIdFSentence* sent);
	/*
	*   We parse XML like data (display string can have &, >, and < so it is not actually xml)
	*	with out using an XML parser.  
	*	
	*
	*/
	// Read a single <SENTENCE>....<SENTENCE>training sentences from saved file
	//	  (training file or corpus file)
	//    Use this when loading a saved project
	int readSavedSentence(UTF8InputStream &in, LocatedString*& sentString, Symbol& sentId, NameOffset nameInfo[]);
	// Read all sentences from GUI's strings
	int readGuiInput(const wchar_t* ann_sentences, size_t length);
	// Read all sentences from GUI's strings
	int readGuiInput(const wchar_t* ann_sentences, size_t length, SentIdToTokensMap &idToTokensTable);
	// reads a corpus-like file for decoding
	wstring readDecodeFile(const wchar_t* decode_file, SentIdToTokensMap &idToTokensTable);
	// Read token sentences from GUI's input
	int readGuiTokensInput(const wchar_t* token_sents, size_t length, SentIdToTokensMap &idToTokensTable);
	// find the offset of the first  instance of c after start, return -1 if not found
	static int findNextMatchingChar(const wchar_t* txt, int start, wchar_t c);
	// find the offset of the first  instance of c after start, throw exception if not found
	static int findNextMatchingCharRequired(const wchar_t* txt, int start, wchar_t c);
	// parse <ANNOTATION .... /> into type, start_offset, end_offset.  
	// Return false if string does not start with <ANNOTATION
	bool parseAnnotationLine(const wchar_t* line, Symbol& type, EDTOffset& start_offset, EDTOffset& end_offset);
	//put substr of str from start to end in substr
	void wcsubstr(const wchar_t* str, int start, int end, wchar_t* substr);
	// Read all preprocessed sentences and create a hashtable with tokenSequences
	int readTokensCorpus(UTF8InputStream &in, SentIdToTokensMap &idToTokensTable);
	// Returned decoded sentences
	wstring decodeSentences();

	

	//delete members of every AL sentence in Corpus blocks, reset pointer to 0
	void clearCorpus();
	//return the the token number that contains offset
	int getTokenFromOffset(const TokenSequence* tok, EDTOffset offset);
	// split origTok into 1,2, or 3 tokens as specified by namestart and nameend
	// put the new tokens in newTokens, return the number of new tokens
	int splitNameTokens(const Token* origTok, EDTOffset namestart, EDTOffset nameend, const LocatedString *sentenceString, Token** newTokens);
	int splitNameTokens(const Token* origTok, int inName[], const NameOffset nameInfo[], const LocatedString *sentenceString, Token** newTokens);
	//adjust the token theory so that name boundaries are one token boundaries, return a new token sequence
	TokenSequence* adjustTokenizationToNames(const TokenSequence* toks, const NameOffset nameInfo[], int nann, const LocatedString* sentenceString);

	PIdFActiveLearningSentence makeAdjustedPIdFActiveLearningSentence(Symbol id,
											LocatedString* sentenceString, TokenSequence* tokens, 
											const NameOffset* nameInfo, int nname, SentIdToTokensMap &idToTokensTable);

	PIdFActiveLearningSentence makePIdFActiveLearningSentence(Symbol id,
											LocatedString* sentenceString, 
											const NameOffset* nameInfo, int nname);

	PIdFActiveLearningSentence makePIdFActiveLearningSentence(Symbol id,
											LocatedString* sentenceString, TokenSequence* tokens, 
											const NameOffset* nameInfo, int nname);

	PIdFSentence* makePIdFSentence(const TokenSequence* toks, const NameOffset nameInfo[], int nann);
	/** Add given sentence to list of active learning sentences to choose from.
	  * You do not need to leave your PIdFSentence objects in memory -- 
	  * PIdFSimActiveLearningTrainer makes its own copy. In fact, you should probably keep
	  * a single PIdFSentence object for yourself and recycle it (using
	  * its clear() method). */
	PIdFActiveLearningSentence* addActiveLearningSentence(const PIdFActiveLearningSentence& sent);
	PIdFActiveLearningSentence* addTrainingSentence(const PIdFActiveLearningSentence& sent);
	PIdFActiveLearningSentence* addTestingSentence(const PIdFActiveLearningSentence& sent);
	
	void getContext(PIdFActiveLearningSentence* context[], int ncontext, PIdFActiveLearningSentence* sent);

	
	wstring getRetVal(bool ok, const wchar_t* txt =L"");
	wstring getRetVal(bool ok, const char* txt );

	void printPIdFSentence(PIdFSentence* sentence){
		std::ostringstream ostr;
		for(int i=0; i<sentence->getLength(); i++){
			ostr<<"( "<<sentence->getWord(i).to_debug_string()<<" "<<
				_tagSet->getTagSymbol(sentence->getTag(i)).to_debug_string()
				<<") ";
		}
		SessionLogger::info("SERIF") << ostr.str();
	}

	// Define a template class vector of int
	typedef std::vector<ActiveLearningSentAndScore> ScoredSentenceVector;
    //Define an scoredSentenceVector for template class vector of strings
	typedef ScoredSentenceVector::iterator ScoredSentenceVectorIt;

	ScoredSentenceVector _scoredSentences;
	Tokenizer* _tokenizer;
	class MorphologicalAnalyzer *_morphAnalyzer;
	class MorphSelector *_morphSelector;

	void addTrainingFeatures();
	double trainEpoch();
	double trainIncrementalEpoch();
	void readInitialWeights();
	void writeWeights();
	void writeTransitions();
	void writeTrainingSentences();
	void writeTestingSentences();
	void readSentencesFile(const char* training_file, const char* tokens_training_file, int mode);

	std::string _model_file;
	std::string _corpus_file;
	std::string _tokens_corpus_file;
	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;
	IdFWordFeatures *_wordFeatures;

	PDecoder *_decoder;
	DTFeature::FeatureWeightMap *_weights;

	DTFeature::FeatureWeightMap *_currentWeights;

	bool _seed_features;
	bool _add_hyp_features;
	bool _allowSentenceRepeats;
	bool _interleave_tags;
	bool _learn_transitions_from_training;
	int _weightsum_granularity;
	//int _epochs;

	// AL sentences are the corpus
	void seekToFirstALSentence();
	bool moreALSentences();
	PIdFActiveLearningSentence *getNextALSentence();

	SentenceBlock *_firstALSentBlock;
	SentenceBlock *_lastALSentBlock;
	SentenceBlock *_curALSentBlock;
	int _curAL_sent_no;

	// These are for looping through the sentences in the SentenceBlocks
	// the training sentences
	void seekToFirstSentence();
	bool moreSentences();
	PIdFActiveLearningSentence *getNextSentence();

	SentenceBlock *_firstSentBlock;
	SentenceBlock *_lastSentBlock;
	SentenceBlock *_curSentBlock;
	int _cur_sent_no;

	// these are for the test set sentences
	void seekToFirstTestingSentence();
	bool moreTestingSentences();
	PIdFActiveLearningSentence *getNextTestingSentence();

	SentenceBlock *_firstTestingSentBlock;
	SentenceBlock *_lastTestingSentBlock;
	SentenceBlock *_curTestingSentBlock;
	int _cur_testing_sent_no;

	GrowableArray<PIdFActiveLearningSentence *> _currentSentences;

	int _n_corpus_sentences;

	int morphSecs;
	int tokenSecs;

	bool fileExists(const char *file);
	void createEmptyFile(const char *file);
	static std::wstring getSentenceTextFromStream(UTF8InputStream &in);

};
#endif

