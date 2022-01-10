// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_NAMELINKTRAINER_H
#define AR_NAMELINKTRAINER_H
#include "Generic/nltrain/UTF8XMLToken.h"
#include "Generic/common/NgramScoreTable.h"
#define INITIAL_TABLE_SIZE 10000

class NameLinkTrainer {
private:
	NgramScoreTable * _name_counts;
	NgramScoreTable * _type_counts;
	NgramScoreTable * _uniq_counts;
	Symbol NameLinkTrainer::_tags[6];
	Symbol NameLinkTrainer::_labels[6];
	Symbol NameLinkTrainer::_end_tag;
	typedef enum {
				PER,
				ORG,
				LOC,
				FAC,
				GPE,
				 NONE,
			} 
	_ent_type;

public:
	NameLinkTrainer();
	int LearnDoc(UTF8XMLToken* tokens, int numTok);
	int LearnDoc(UTF8XMLToken* tokens, int numTok, int startLab);
	void PrintTable(char* filename);



};
#endif
