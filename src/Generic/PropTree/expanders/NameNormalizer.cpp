// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/expanders/NameNormalizer.h"
#include "Generic/common/UnicodeUtil.h"
#include <boost/foreach.hpp>

void NameNormalizer::expand(const PropNodes& pnodes) const {
	using namespace std;

	BOOST_FOREACH(PropNode_ptr node, pnodes) {
		BOOST_FOREACH(const PropNode::WeightedPredicate& wpred, node->getPredicates()) {
			if( wpred.first.type() != Predicate::NAME_TYPE ) continue;

			wstring name = wpred.first.pred().to_string();
			wstring normalized_name = UnicodeUtil::normalizeNameString(UnicodeUtil::normalizeTextString(name));

			node->addPredicate(Predicate(wpred.first.type(), normalized_name, wpred.first.negative()), wpred.second);
		}	
	}
}

