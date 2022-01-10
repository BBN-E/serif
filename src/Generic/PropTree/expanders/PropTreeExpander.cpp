// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/expanders/PropTreeExpander.h"
#include "Generic/PropTree/DocPropForest.h"
#include "Generic/PropTree/PropNode.h"
#include <boost/foreach.hpp>


void PropTreeExpander::expandForest(DocPropForest& docForest) const {
	PropNodes allNodes;

	BOOST_FOREACH (PropNodes_ptr & sentenceRoots, docForest) {
		PropNodes sentenceNodes;
		PropNode::enumAllNodes(*sentenceRoots, sentenceNodes);
		allNodes.insert(allNodes.begin(), sentenceNodes.begin(), sentenceNodes.end());
	}
		
	expand(allNodes);
}

