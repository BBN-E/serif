// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "AcronymPeriodDeleter.h"

#include "../PropNode.h"
#include "../Predicate.h"

#include <string>
#include <locale>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/erase.hpp>

void AcronymPeriodDeleter::expand(const PropNodes& pnodes) const {
	BOOST_FOREACH(PropNode_ptr node, pnodes) {
		BOOST_FOREACH(const PropNode::WeightedPredicate& wpred, node->getPredicates()) {
			std::wstring word=wpred.first.pred().to_string();
			
			bool not_a_number=false;

			BOOST_FOREACH (wchar_t c, word) {
				if (iswalpha(c)) {
					not_a_number=true;
				}
			}

			if (not_a_number) {
			//if (isAllCapsAndPeriods(word)) {
				boost::erase_all(word, L".");
				node->addPredicate(Predicate(wpred.first.type(), word, wpred.first.negative()), wpred.second);
			//}
			}
		}
	}
}

// note this will return true for an empty string

bool AcronymPeriodDeleter::isAllCapsAndPeriods(const std::wstring& word) {
	using namespace std;

	BOOST_FOREACH(const wchar_t c, word) {
		if (c!='.' && !isupper(c)) {
			return false;
		}
	}

	return true;
}
