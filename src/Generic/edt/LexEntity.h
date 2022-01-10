// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LEXENTITY_H
#define LEXENTITY_H

#include "Generic/common/limits.h"
#include "Generic/edt/CountsTable.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityType.h" 
#include "Generic/common/DebugStream.h"
#include "Generic/common/Symbol.h"

class LexEntity {
private:
	CountsTable _wordCounts;
	int _totalCount;
	EntityType _type;
	static DebugStream &_debugOut;

public:
	static double unseen_weights[MAX_ENTITY_TYPES];
	static double LEX_ENT_LOG_OF_ZERO;
	
	LexEntity(const LexEntity &other);
	LexEntity(EntityType type);
	double estimate(Symbol words[], int nWords);
	void learn(Symbol words[], int nWords);

private:
	bool _use_edit_distance;
	int _edit_threshold;
	int editDistance(Symbol sym1, Symbol sym2) const;
};

#endif
