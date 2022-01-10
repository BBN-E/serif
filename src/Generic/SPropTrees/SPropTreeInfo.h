// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ___SPROPTREE_INFO_H_
#define ___SPROPTREE_INFO_H_

#include "Generic/SPropTrees/STreeNode.h"
#include "Generic/common/UTF8InputStream.h"

class STreeNode;
class SPropTree;

struct SPropTreeInfo {
	SPropTree* tree;
	int sentNumber;
	RelevantMentions allEntityMentions;
	int spanStart, spanEnd;
	SPropTreeInfo() {};
	bool operator<(const SPropTreeInfo& pti) const {
		return tree < pti.tree; //just compare pointers!
	}
	static bool resurect(UTF8InputStream& utf8, const DocTheory* dt, SPropTreeInfo& ptin);
};

struct lessProp {
	bool operator()(const SPropTreeInfo& p1, const SPropTreeInfo& p2) const;
};

typedef std::set<SPropTreeInfo,lessProp> SPropForest;


#endif

