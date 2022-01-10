// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_NODE_INFO_H
#define XX_NODE_INFO_H

#include "Generic/theories/NodeInfo.h"

class GenericNodeInfo {
    // Note: this class is intentionally not a subclass of
    // NodeInfo.  See NodeInfo.h for explanation.
public:
	// we can't possibly know anything in UNSPEC_LANGUAGE mode, so always return false
	static bool isReferenceCandidate(const SynNode *node) { return false; }
	static bool isOfNPKind(const SynNode *node) { return false; }
	static bool isOfHobbsNPKind(const SynNode *node) { return false; }
	static bool isOfHobbsSKind(const SynNode *node) { return false; }
	static bool canBeNPHeadPreterm(const SynNode *node) { return false; }
	static bool isNominalPremod(const SynNode *node) { return false; }
};


#endif
