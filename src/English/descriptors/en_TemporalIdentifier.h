// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_TEMPORAL_IDENTIFIER
#define EN_TEMPORAL_IDENTIFIER

#include "Generic/common/Symbol.h"

class SynNode;


class TemporalIdentifier {
public:
	static void loadTemporalHeadwordList();
	static bool looksLikeTemporal(const SynNode *node, const MentionSet *mentionSet=0, bool no_embedded=true);
	static void freeTemporalHeadwordList();

private:
	static Symbol::HashSet *_temporalWords;
};

#endif
