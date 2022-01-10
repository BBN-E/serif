// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/expanders/Americanizer.h"
#include "Generic/PropTree/Predicate.h"
#include <string>
#include <boost/foreach.hpp>


std::wstring Americanizer::americanizeWord(const std::wstring& word) {
	using namespace std;
	static const wstring our=L"our";
	static const wstring or_=L"or";
	static const wstring ise=L"ise";
	static const wstring ize=L"ize";
	static const wstring re=L"re";
	static const wstring er=L"er";

	int len=static_cast<int>(word.length());

	if (len>4) {
		wstring ret(word);
		wstring last3=ret.substr(len-3,3);

		if (our==last3) {
			ret.replace(len-3, 3, or_);
		} else if (ise==last3) {
			ret.replace(len-3, 3, ize);
		} // this case needs to be more constrained
		// before being used
		/*else if (re==word.substr(len-2,2)) {
		ret.replace(len-3, 2, er);
		}*/ 

		return ret;
	} else {
		return word;
	}
}


void Americanizer::expand(const PropNodes& pnodes) const {
	using namespace boost; using namespace std;

	BOOST_FOREACH(PropNode_ptr node, pnodes) {
		BOOST_FOREACH(const PropNode::WeightedPredicate& wpred, node->getPredicates()) {
			const wstring word=wpred.first.pred().to_string();
			const wstring americanWord=americanizeWord(word);

			if (word!=americanWord) {
				node->addPredicate(Predicate(wpred.first.type(), americanWord, wpred.first.negative()), wpred.second);				
			}
		}	
	}
}


