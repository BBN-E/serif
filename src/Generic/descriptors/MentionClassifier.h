// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTIONCLASSIFIER_H
#define MENTIONCLASSIFIER_H

#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"

class MentionClassifier {
	public:
		virtual int classifyMention (MentionSet *currSolution, Mention *currMention, MentionSet *results[], int max_results, bool isBranching) = 0;
};

#endif
