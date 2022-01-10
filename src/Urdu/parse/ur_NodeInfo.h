// Copyright 2013 Raytheon BBN Technologies 
// All Rights Reserved.

#ifndef UR_NODE_INFO_H
#define UR_NODE_INFO_H

#include "Generic/theories/NodeInfo.h"

// Always returns false- avoids link erros
class UrduNodeInfo {
    // Note: this class is intentionally not a subclass of
    // NodeInfo.  See NodeInfo.h for explanation.
public:
	static bool isReferenceCandidate(const SynNode *node);
	static bool isOfNPKind(const SynNode *node);
	static bool isOfHobbsNPKind(const SynNode *node);
	static bool isOfHobbsSKind(const SynNode *node);
	static bool canBeNPHeadPreterm(const SynNode *node);

	// Not implemented (yet) for this language:
	static bool isNominalPremod(const SynNode *node) { return false; }
};
#endif
