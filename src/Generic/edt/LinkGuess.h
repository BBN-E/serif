// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LINKGUESS_H
#define LINKGUESS_H

#include "Generic/theories/SynNode.h"

struct LinkGuess {
	const SynNode *guess;
	double score;
	int sentence_num;
	static const int NO_SENTENCE = -1;
	//DEBUG
	char debug_string[1024];
	// experimental (for ace08 eval)
	float linkConfidence;
};


#endif
