// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef WORD_COLLECTOR_H
#define WORD_COLLECTOR_H


#include "Generic/parse/ParserTrainer/TrainerVocab.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/parse/WordFeatures.h"

#define INITIAL_VOCABULARY_SIZE 70000

class WordCollector {
private:
	WordFeatures* wordFeat;
	TrainerVocab* vocabularyTable;
	NgramScoreTable* wordFeatTable;

public:
	WordCollector() { 
		wordFeat = WordFeatures::build();
		vocabularyTable = _new TrainerVocab(INITIAL_VOCABULARY_SIZE); 
		wordFeatTable = _new NgramScoreTable(2, INITIAL_VOCABULARY_SIZE);
	}
	void read_from_file (UTF8InputStream& stream);
	void print_all(char *v);
};

#endif
