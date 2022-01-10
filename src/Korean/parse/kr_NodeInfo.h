#ifndef KR_NODE_INFO_H
#define KR_NODE_INFO_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/theories/NodeInfo.h"

class KoreanNodeInfo {
    // Note: this class is intentionally not a subclass of
    // NodeInfo.  See NodeInfo.h for explanation.
public:
	static bool isReferenceCandidate(const SynNode *node);
	static bool isOfNPKind(const SynNode *node) { return false; }
	static bool isOfHobbsNPKind(const SynNode *node) { return false; }
	static bool isOfHobbsSKind(const SynNode *node) { return false; }

	// Not implemented (yet) for this language:
	static bool canBeNPHeadPreterm(const SynNode *node) { return false; }
	static bool isNominalPremod(const SynNode *node) { return false; }
};


#endif
