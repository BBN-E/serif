// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
#ifndef CH_NODE_INFO_H
#define CH_NODE_INFO_H

#include "Generic/theories/NodeInfo.h"

class ChineseNodeInfo {
    // Note: this class is intentionally not a subclass of
    // NodeInfo.  See NodeInfo.h for explanation.
public:
	static bool isReferenceCandidate(const SynNode *node);
	static bool isOfNPKind(const SynNode *node);
	static bool isOfHobbsNPKind(const SynNode *node);
	static bool isOfHobbsSKind(const SynNode *node);

	// Not implemented (yet) for this language:
	static bool canBeNPHeadPreterm(const SynNode *node) { return false; }
	static bool isNominalPremod(const SynNode *node) { return false; }
};


#endif
