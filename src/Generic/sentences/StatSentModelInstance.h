// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STAT_SENT_MODEL_INSTANCE_H
#define STAT_SENT_MODEL_INSTANCE_H

#include "Generic/common/Symbol.h"

class StatSentModelInstance {
public:
	StatSentModelInstance(Symbol tag, Symbol word, Symbol word1,
						 Symbol word2, int tok_index)
		: _tag(tag), _word(word), _word1(word1), _word2(word2),
		  _tok_index(tok_index)
	{}

	Symbol getTag() const { return _tag; }
	Symbol getWord() const { return _word; }
	Symbol getWord1() const { return _word1; }
	Symbol getWord2() const { return _word2; }

private:
	Symbol _tag;
	Symbol _word;
	Symbol _word1;
	Symbol _word2;
	int _tok_index;
};

#endif
