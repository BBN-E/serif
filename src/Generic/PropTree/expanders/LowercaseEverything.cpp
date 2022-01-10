// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/expanders/LowercaseEverything.h"
#include <algorithm>
#include <boost/foreach.hpp>


void LowercaseEverything::expand(const PropNodes& pnodes) const {
	using namespace std;

	BOOST_FOREACH(PropNode_ptr node, pnodes) {
		BOOST_FOREACH(const PropNode::WeightedPredicate& wpred, node->getPredicates()) {
			wstring word = wpred.first.pred().to_string();
			std::transform(word.begin(), word.end(), word.begin(), towlower);

			node->addPredicate(Predicate(wpred.first.type(), word, wpred.first.negative()), wpred.second);
		}	
	}
}


