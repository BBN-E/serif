// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/DocPropForest.h"

DocPropForest::DocPropForest (const DocTheory* dt) : _docTheory(dt),
	_nodesBySentence(dt->getNSentences())
{}

void DocPropForest::clearExpansions() {
	using namespace boost;
	BOOST_FOREACH(PropNodes_ptr nodes, _nodesBySentence) {
		BOOST_FOREACH(PropNode_ptr nodePtr, *nodes) {
			nodePtr->clearExpansions();
		}
	}
}
