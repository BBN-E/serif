// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_NODE_INFO_H
#define AR_NODE_INFO_H

#include "Generic/theories/NodeInfo.h"

// Always returns false- avoids link erros
class ArabicNodeInfo {
    // Note: this class is intentionally not a subclass of
    // NodeInfo.  See NodeInfo.h for explanation.
public:
	static bool isReferenceCandidate(const SynNode *node);
	static bool isOfNPKind(const SynNode *node);
	static bool isNominalMod(const SynNode *node);
	static bool canBeNPHeadPreterm(const SynNode *node);

	// Not implemented (yet) for this language:
	static bool isNominalPremod(const SynNode *node) { return false; }
	static bool isOfHobbsNPKind(const SynNode *node) { return false; }
	static bool isOfHobbsSKind(const SynNode *node)  { return false; }
};
#endif
