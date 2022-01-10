// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STATS_COLLECTOR_H
#define STATS_COLLECTOR_H

#include <cstddef>
#include <string>
#include "Generic/common/Symbol.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/parse/ParseNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/parse/ParserTrainer/TrainerPOS.h"
#include "Generic/parse/SequentialBigrams.h"
#include "Generic/parse/WordFeatures.h"
#define INITIAL_PRIOR_TABLE_SIZE 10000
#define INITIAL_HEAD_TABLE_SIZE 10000
#define INITIAL_MODIFIER_PRE_TABLE_SIZE 10000
#define INITIAL_MODIFIER_POST_TABLE_SIZE 10000
#define INITIAL_LEXICAL_LEFT_TABLE_SIZE 10000
#define INITIAL_LEXICAL_RIGHT_TABLE_SIZE 10000
#define INITIAL_POS_TABLE_SIZE 70000


class StatsCollector {
private:
    NgramScoreTable* priorCounts;
    NgramScoreTable* headCounts;
    NgramScoreTable* premodCounts;
    NgramScoreTable* postmodCounts;
    NgramScoreTable* leftLexicalCounts;
    NgramScoreTable* rightLexicalCounts;
	TrainerPOS* POS;
	//mrf get prob counts for each word
	NgramScoreTable* wordWordFeatureCounts;
	WordFeatures* wordFeat;
	Symbol head_yes;
	Symbol head_no;
	Symbol head_yes_modifier_no;
	Symbol head_yes_modifier_yes;
	Symbol head_no_modifier_yes;
	Symbol head_no_modifier_no;
	int nodeCount;

	SequentialBigrams *sequentialBigrams;

public:
	StatsCollector();
	StatsCollector(const char* sequentialBigramFile);

	void collect (ParseNode* parse);
	void collectModifierStats(NgramScoreTable* LexicalCounts, 
			NgramScoreTable* ModCounts, Symbol P, Symbol H, Symbol previous, 
			Symbol hw, Symbol ht, bool headisfirst, ParseNode* modifier);
	void print_all(char *p, char *h, char *pre, char *post, char *ll, char *rl, char *pos);
	void print_vocab(const char* voc);
};

#endif
