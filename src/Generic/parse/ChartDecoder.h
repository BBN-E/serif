// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CHART_DECODER_H
#define CHART_DECODER_H

#include "Generic/parse/ChartEntry.h"
#include "Generic/parse/Constraint.h"
#include "Generic/parse/ExtensionTable.h"
#include "Generic/parse/HeadProbs.h"
#include "Generic/parse/KernelTable.h"
#include "Generic/parse/LexicalProbs.h"
#include "Generic/parse/ModifierProbs.h"
#include "Generic/parse/ParseNode.h"
#include "Generic/parse/PartOfSpeechTable.h"
#include "Generic/parse/PriorProbTable.h"
#include "Generic/parse/SequentialBigrams.h"
#include "Generic/parse/SignificantConstitOracle.h"
#include "Generic/parse/TokenTagTable.h"
#include "Generic/parse/VocabularyTable.h"
#include "Generic/parse/WordFeatures.h"

#include "Generic/theories/Entity.h"
#include "Generic/common/limits.h" // SRS
#include "dynamic_includes/parse/ParserConfig.h"

#include <cstddef>
#include <time.h>
class PartOfSpeechSequence;
#define MAX_SENTENCE_LENGTH MAX_SENTENCE_TOKENS // SRS

#define MAX_TAGS_PER_WORD 50

class ChartDecoder {
//private:	//MF
private:
        static const size_t cacheN = 3;
protected:
	double _frag_prob;
protected:
	static const size_t maxSentenceLength;
	static const size_t maxTagsPerWord;
	int maxEntriesPerCell;  // set by parser_max_entries_per_cell parameter.
	float lambda;           // set by parser_lambda parameter.

	const NgramScoreTable* featTable;
	const NgramScoreTable* wordProbTable;
	const KernelTable* kernelTable;
	const ExtensionTable* extensionTable;
	const PriorProbTable* priorProbTable;
        long cache_max;
        Cache<cacheN>::simple simpleCache;
#if !defined(_WIN32) && !defined(__APPLE_CC__)
        Cache<cacheN>::lru lruCache;
#endif
        CacheType cache_type;
	HeadProbs* headProbs;
	ModifierProbs* premodProbs;
	ModifierProbs* postmodProbs;
	LexicalProbs* leftLexicalProbs;
	LexicalProbs* rightLexicalProbs;
	const PartOfSpeechTable* partOfSpeechTable;
	const PartOfSpeechTable* auxPartOfSpeechTable;
	bool lowerCaseForUnknown;
	bool constrainKnownNounsAndVerbs;
	const Symbol* restrictedPosTags;
	int restrictedPosTagsSize;
	const TokenTagTable* parserShortcuts;

	const VocabularyTable* vocabularyTable;
	const WordFeatures* wordFeatures;
	const SequentialBigrams* sequentialBigrams;
	SignificantConstitOracle* scOracle;
public:
	ChartDecoder(){};	//for derived class - mf

	/** For this constructor, the ChartDecoder takes over ownership of all 
	  * pointer arguments (kernelTableArg, extensionTableArg, etc), EXCEPT  
	  * for partOfSpeechTableArg. */
	ChartDecoder(const KernelTable* kernelTableArg,
                     const ExtensionTable* extensionTableArg,
                     const PriorProbTable* priorProbTableArg,
                     HeadProbs* headProbsArg,
                     ModifierProbs* premodProbsArg,
                     ModifierProbs* postmodProbsArg,
                     LexicalProbs* leftLexicalProbsArg,
                     LexicalProbs* rightLexicalProbsArg,
                     const PartOfSpeechTable* partOfSpeechTableArg,
                     const VocabularyTable* vocabularyTableArg,
                     const NgramScoreTable* featTableArg,
                     const SequentialBigrams* bigrams,
                     SignificantConstitOracle* scOracleArg, 
                     CacheType cacheType, 
                     long cacheMax, 
		     const PartOfSpeechTable* auxPOSTable,
		     const Symbol* restrictedPOSTags, 
		     int restrictePOSTagsSize,
		     const TokenTagTable* parserShortcutsArg,
		     bool useLowerCaseForUnknown,
			 bool constrainKnownNounsAndVerbsParam,
			 float lambda, 
			 int maxEntriesPerCell);
	ChartDecoder(const char* model_prefix, double frag_prob);
	ChartDecoder(const char* model_prefix, double frag_prob, const PartOfSpeechTable* auxPOSTable);
	virtual ~ChartDecoder();
	virtual ParseNode* decode(Symbol* sentence, int length,
		std::vector<Constraint> & constraints,
		bool collapseNPlabels = true, Symbol* pos_constraints = 0 );
	virtual ParseNode* decode(Symbol* sentence, int length,
		std::vector<Constraint> & constraints,
		bool collapseNPlabels, const PartOfSpeechSequence* pos_constraints);
	//MRF: Use these calculate the unigram probablity of a sentence
	//useful for determining parser confidence
	void readWordProbTable(const char* model_prefix);
	float getProbabilityOfWords(Symbol* sentence, int length);

	float *theory_scores; // array length = ChartDecoder::maxEntriesPerCell
	string *theory_sc_strings; // array length = ChartDecoder::maxEntriesPerCell
	int highest_scoring_final_theory;

	typedef enum { MIXED, UPPER, LOWER } decoder_type_t;
	decoder_type_t DECODER_TYPE;
	void setDecoderType(decoder_type_t dt) { DECODER_TYPE = dt; }
	void setMaxParserSeconds(int maxsecs) { MAX_CLOCKS = maxsecs * CLOCKS_PER_SEC; }

    void writeCaches();
    void readCaches();
	void cleanup();
	
	ParseNode* returnDefaultParse(Symbol* sentence, int length, std::vector<Constraint> & constraints, bool collapseNPlabels);

protected:
	ChartEntry** chart[MAX_SENTENCE_LENGTH][MAX_SENTENCE_LENGTH];
	size_t punctuationUpperBound[MAX_SENTENCE_LENGTH];
	bool nonRightClosableToken[MAX_SENTENCE_LENGTH];
	bool nonLeftClosableToken[MAX_SENTENCE_LENGTH];
	bool possiblePunctuationOrConjunction[MAX_SENTENCE_LENGTH];
	ChartEntry** theories;   // array length = ChartDecoder::maxEntriesPerCell
	int numTheories;
	void addKernelTheories(ChartEntry* leftEntry, ChartEntry* rightEntry);
	void addExtensionTheories(ChartEntry* leftEntry, ChartEntry* rightEntry);
	void addTheory(ChartEntry* chartEntry);
	virtual void transferTheoriesToChart(int start, int end);
	void initChart(Symbol* sentence, int length, std::vector<Constraint> & constraints, const PartOfSpeechSequence* pos_constraints = 0);
	//MRF:  allow a weak constraint to be placed on the POS
	//if POS == "UNCONSTRAINED_POS", there is no constraint
	//if pos lookup for word does not include POS, 
	//don't constrain the POS, since this may lead to parser death
	void initChartWord(Symbol word, int chartIndex, bool firstWord, const PartOfSpeechSequence* pos_constraints);
	// JM: setRankingScore is false for name entries that are forced to nnp
	void fillWordEntry(ChartEntry *entry,
		const Symbol &tag, const Symbol &word, const Symbol &originalWord,
		int left, int right, bool setRankingScore=true);
	void fillWordEntry(ChartEntry *entry, const Symbol &cat,
		const Symbol &tag, const Symbol &word, const Symbol &originalWord,
		int left, int right, bool setRankingScore = true);

	void addConstraintEntry(int left, int right, Symbol* sentence, const Symbol &type, EntityType);
	void addHyphenEntry(int left, int right, Symbol* sentence);

	void scoreKernel(ChartEntry* entry);
	void scoreExtension(ChartEntry* entry);
	ParseNode* getBestParse(float &score, 
		int start,
		int end,
		bool topRequired);
	ParseNode* getBestFragmentedParse(float &score, 
		int start, 
		int end);
	int getTreeDepth(ParseNode* node);
	
	ParseNode* getMultipleParses(ChartEntry **possibleTrees, 
		int numPossibleTrees, bool only_right_child);
	
	void cleanupChart(int length);
	void capLeft(ChartEntry* entry);
	void capRight(ChartEntry* entry);
	void initPunctuationUpperBound(Symbol* sentence, int length);
	bool punctuationCrossing(size_t i, size_t j, size_t length);
	bool crossingConstraintViolation(int start, int end, std::vector<Constraint> & constraints);
	bool leftClosable;
	bool rightClosable;

	bool chartHasAdverbPOS(int nindex);
	bool chartHasOnlyAdverbPOS(int nindex);
	bool chartHasVerbPOS(int nindex);
	bool chartHasPronounPOS(int nindex);
	bool chartHasParticlePOS(int nindex);
	bool chartHasGeneralPrepositionPOS(int nindex);
	bool chartHasOnlyGeneralPrepositionPOS(int nindex);

	virtual void postprocessParse(ParseNode* node, std::vector<Constraint> & constraints, bool collapseNPlabels);
	void postprocessPremods(ParseNode* premod, std::vector<Constraint> & constraints, bool collapseNPlabels);
	void postprocessPostmods(ParseNode* postmod, std::vector<Constraint> & constraints, bool collapseNPlabels);
	void replaceWords(ParseNode* node, int& currentPosition, Symbol *sentence);
	void replaceWordsInPremods(ParseNode* premod, int& currentPosition, Symbol *sentence);
	void replaceWordsInPostmods(ParseNode* postmod, int& currentPosition, Symbol *sentence);
	void insertNestedNameNodes(ParseNode* node, std::vector<Constraint> & constraints);

	void startClock();
	bool timedOut(Symbol* sentence, int length);
	ParseNode* getDefaultParse(Symbol* sentence, int length, std::vector<Constraint> & constraints);
	ParseNode* getCompletelyDefaultParse(Symbol* sentence, int length);
	ParseNode* getBestFragment(int start, int end);

	clock_t _startTime;
	clock_t MAX_CLOCKS;

        float computePartialRankingScore(ChartEntry* entry);

        const char* decoderTypeString();

};

#endif

