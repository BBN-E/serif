// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef VOCAB_PRUNER_H
#define VOCAB_PRUNER_H

#include <cstddef>
#include "Generic/common/Symbol.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/parse/VocabularyTable.h"
#include "Generic/parse/WordFeatures.h"

#define INITIAL_PRUNED_HEAD_TABLE_SIZE 1000
#define INITIAL_PRUNED_LEXICAL_TABLE_SIZE 1000
#define INITIAL_PRUNED_MODIFIER_TABLE_SIZE 1000
#define HEAD_HEADWORD_INDEX 2
#define LEXICAL_HEADWORD_INDEX 5
#define LEXICAL_MODIFIERWORD_INDEX 0
#define MODIFIER_HEADWORD_INDEX 5

#define HEAD_NGRAM_SIZE 4
#define LEXICAL_NGRAM_SIZE 7
#define PRIOR_NGRAM_SIZE 2
#define MODIFIER_NGRAM_SIZE 7


class VocabPruner {
public:
	NgramScoreTable* headCounts;
	NgramScoreTable* lexicalCounts;
	NgramScoreTable* modifierCounts;
	VocabularyTable* vocabularyTable;
	VocabularyTable* unprunedVocabularyTable;
	WordFeatures* wordFeatures;
	void prune_heads(char* outfile);
	void prune_lexical(char* outfile);
	void prune_modifiers(char* outfile);
	bool pruning_smoothing_events_with_main_vocab;
	bool keep_all_words;

	void print_vocab(char* outfile) { 
		if (keep_all_words)
			unprunedVocabularyTable->print(outfile); 
		else vocabularyTable->print(outfile); 
	}
	void print_wordprob(char* word_feat_file, char* outfile);
	VocabPruner (char* infile, int threshold, bool smooth);
	Symbol head_yes;
	Symbol head_no;
	Symbol head_yes_modifier_no;
	Symbol head_yes_modifier_yes;
	Symbol head_no_modifier_yes;
	Symbol head_no_modifier_no;

};

#endif
