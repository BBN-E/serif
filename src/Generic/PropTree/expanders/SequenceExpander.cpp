// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"
#include "Generic/PropTree/expanders/SequenceExpander.h"
#include <boost/foreach.hpp>
#include <iostream>

void SequenceExpander::expand(const PropNodes& pnodes) const {
	using namespace boost;
	
	BOOST_FOREACH(PropTreeExpander::ptr pte, _expanders) {
		pte->expand(pnodes);

		/*std::wcout << L"EXPANSION\n";
		BOOST_FOREACH(PropNode_ptr node, pnodes) {
			node->compactPrint(std::wcout, true, true, true, 0);
			std::wcout << L"\n";
		}
		std::wcout << L"\n";*/
	}
}

