// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ___PROPTREELINKS_H___
#define ___PROPTREELINKS_H___

#include "Generic/SPropTrees/SForestUtilities.h"
#include "Generic/relations/discmodel/TreeNodeChain.h"

//#include "Generic/theories/RelMentionSet.h"
//#include "Generic/theories/RelMention.h"

/** one object of this class is created for one sentence. It contains two members:
	1) SPropForest extracted for this sentence
	2) connection between mentions in this sentence extracted from the trees of this forest
  */
typedef std::map<std::pair<int,int>,TreeNodeChain*> AllMentionLinks;

class PropTreeLinks {
	SPropForest forest;
	TNEDictionary dictionary;
	AllMentionLinks links;
public:
	PropTreeLinks(int i, const DocTheory* dt);
	~PropTreeLinks();
	const TreeNodeChain* getLink(int i, int j) const;
};

#endif
