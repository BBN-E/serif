// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_NODE_INFO_H
#define EN_NODE_INFO_H

#include "Generic/common/Symbol.h"
#include "Generic/theories/NodeInfo.h"

class EnglishNodeInfo {
    // Note: this class is intentionally not a subclass of
    // NodeInfo.  See NodeInfo.h for explanation.
public:
	static bool isReferenceCandidate(const SynNode *node);
	static bool isOfNPKind(const SynNode *node);
	static bool isOfHobbsNPKind(const SynNode *node);
	static bool isOfHobbsSKind(const SynNode *node);
	static bool canBeNPHeadPreterm(const SynNode *node);
	static bool isNominalPremod(const SynNode *node);

	// The following methods are defined only on EnglishNodeInfo, and
	// not on NodeInfo; they should be accessed directly from 
	// EnglishNodeInfo.
	static bool isWHQNode(const SynNode *node);
	static bool useNominalPremods();
	static bool useWHQMentions();
	static bool isValidAdjectiveNomPremod(Symbol headword);
};


#endif
