// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_CHART_DECODER_H
#define ar_CHART_DECODER_H

#include <cstddef>
#include "Generic/parse/KernelTable.h"
#include "Generic/parse/ExtensionTable.h"
#include "Generic/parse/PriorProbTable.h"
#include "Generic/parse/HeadProbs.h"
#include "Generic/parse/ModifierProbs.h"
#include "Generic/parse/LexicalProbs.h"
#include "Generic/parse/PartOfSpeechTable.h"
#include "Generic/parse/VocabularyTable.h"
#include "Arabic/parse/ar_WordFeatures.h"
#include "Generic/parse/ParseNode.h"
#include "Generic/parse/SequentialBigrams.h"
#include "Arabic/parse/ar_SignificantConstitOracle.h"
#include "Generic/parse/Constraint.h"
#include "Generic/common/limits.h" // SRS
#include "Generic/common/ParamReader.h"
#include "Generic/parse/ChartDecoder.h"
#include "Generic/theories/Entity.h"
#include "Arabic/parse/ar_WordSegment.h"
#include "Arabic/parse/ar_ChartEntry.h"
#include "Arabic/parse/ar_SplitTokenSequence.h"

#include <stdio.h>

#define MAX_SENTENCE_LENGTH MAX_SENTENCE_TOKENS // SRS
#define MAX_ENTRIES_PER_CELL 10
#define MAX_TAGS_PER_WORD 50

class ArabicChartDecoder : public ChartDecoder {
protected:
	/*
	const KernelTable* kernelTable;
	const ExtensionTable* extensionTable;
	const PriorProbTable* priorProbTable;
	const HeadProbs* headProbs;
	const ModifierProbs* premodProbs;
	const ModifierProbs* postmodProbs;
	const LexicalProbs* leftLexicalProbs;
	const LexicalProbs* rightLexicalProbs;
	const PartOfSpeechTable* partOfSpeechTable;
	const VocabularyTable* vocabularyTable;
	const ArabicWordFeatures wordFeatures;
	const SequentialBigrams* sequentialBigrams;
	ArabicSignificantConstitOracle* scOracle;
	double _frag_prob;
	*/
	UTF8OutputStream _debugStream;
	//UTF8OutputStream _parseStream;
	bool DEBUG1;
	bool DEBUG2;

public:
	//ArabicChartDecoder(){};
	/*
	//change to add ArabicChartEntry
	 ArabicChartDecoder(const KernelTable* kernelTableArg,
		const ExtensionTable* extensionTableArg,
		const PriorProbTable* priorProbTableArg,
		const HeadProbs* headProbsArg,
		const ModifierProbs* premodProbsArg,
		const ModifierProbs* postmodProbsArg,
		const LexicalProbs* leftLexicalProbsArg,
		const LexicalProbs* rightLexicalProbsArg,
		const PartOfSpeechTable* partOfSpeechTableArg,
		const VocabularyTable* vocabularyTableArg,
		const SequentialBigrams* bigrams,
		ArabicSignificantConstitOracle* scOracleArg);
	ArabicChartDecoder(const char* model_prefix, double frag_prob);

	*/

	ArabicChartDecoder(const char* model_prefix, double frag_prob) :
	  ChartDecoder(model_prefix, frag_prob) {
		char buffer[500];
		DEBUG1 = ParamReader::getParam("parse_debug",buffer,500);
		if (DEBUG1)
			_debugStream.open(buffer);

		DEBUG2 = ParamReader::getParam("parsenode_debug",buffer,500);
		//if(DEBUG2)
		//	_parseStream.open(buffer);
		DEBUG2 = false;

	  };

	virtual ParseNode* decode(Symbol* sentence, int length,	Constraint* _constraints,
		int _numConstraints, bool collapseNPlabels);
	ParseNode* decode(SplitTokenSequence* bw_tokens, bool collapseNPlabels);



protected:

	//ArabicChartEntry** chart[MAX_SENTENCE_LENGTH][MAX_SENTENCE_LENGTH];
	//ArabicChartEntry* theories[MAX_ENTRIES_PER_CELL];
	//use parents!
	//ChartEntry** chart[MAX_SENTENCE_LENGTH][MAX_SENTENCE_LENGTH];
	//ChartEntry* theories[MAX_ENTRIES_PER_CELL];
	//for post processing
	ParseNode* _makeFlatParse(Symbol* sentence, int length,
		Constraint* _constraints, int _numConstraints);
	Symbol _cleanWords[MAX_SENTENCE_LENGTH][MAX_SENTENCE_LENGTH];
	Symbol _nameWords[MAX_SENTENCE_LENGTH];
	int _numNameWords;
	//post processing overwrites only name words
	void postprocessParse(ParseNode* node,	bool collapseNPlabels);
	void addNameToNode(ParseNode* node, int& currentPosition);





	int initChart(Symbol* sentence, Symbol* split_sentence, int length);
	int initChart(SplitTokenSequence* words, Symbol* split_sentence);


	int initChartWordMultiple(const WordSegment* word, int chartIndex, bool firstWord);
	void addConstraintEntry(int left_word, int right_word,
									 int left_chart, int right_chart,
									  Symbol* sentence, Symbol type,
									  EntityType entityType);
	//change to not write over preterminals with spans
	virtual void transferTheoriesToChart(int start, int end);
	//replace headWord in non name preterminals with cleanWord of same span
	void postProcessChart();


	/*
	//change to add originalWord to ArabicChartEntry
	void fillWordEntry(ArabicChartEntry *entry,
						Symbol tag, Symbol word, Symbol originalWord,
						int left, int right, bool setRankingScore = true);
	void fillWordEntry(ArabicChartEntry *entry, Symbol cat,
						Symbol tag, Symbol word, Symbol originalWord,
						int left, int right, bool setRankingScore= true);
	//chanbe b/c chartEntry becomes ArabicChartEntry
	void addKernelTheories(ArabicChartEntry* leftEntry,
		ArabicChartEntry* rightEntry);
	void addExtensionTheories(ArabicChartEntry* leftEntry,
		ArabicChartEntry* rightEntry);
	*/
	void addWordNode(ParseNode* currNode, Symbol word);
	void addNPPNode(ParseNode* currNode, Constraint constraint, Symbol* sentence);

	void addNameToParse(ParseNode* node, int& currentPosition);
	void addNameToPremods(ParseNode* node, int& currentPosition);
	void addNameToPostmods(ParseNode* node, int& currentPosition);


};

#endif

