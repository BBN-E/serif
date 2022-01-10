// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/expanders/HyphenPartExpander.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/case_conv.hpp>

void HyphenPartExpander::expand(const PropNodes& pnodes) const {
	using namespace std;
	using namespace boost;

	typedef split_iterator<wstring::const_iterator> wsplit_iterator;

	BOOST_FOREACH(PropNode_ptr node, pnodes) {
		BOOST_FOREACH(PropNode::WeightedPredicate wpred, node->getBasePredicates()) {
			const Predicate pred=wpred.first;
			if (!pred.pred().is_null()) {
				const wstring& str=pred.pred().to_string();

				wsplit_iterator splitIt=make_split_iterator(str, first_finder("-", is_iequal()));
				wsplit_iterator endIt=wsplit_iterator();

				if (str.find(L'-')!=wstring::npos) {
					for (; splitIt!=endIt; ++splitIt) {
						Symbol p_pred(copy_range<wstring>(*splitIt));
						if(Predicate::validPredicate(p_pred) )
							node->addPredicate(Predicate(pred.type(), p_pred, pred.negative()), 
												wpred.second);
					}
				}
			}
		}
	}
}

