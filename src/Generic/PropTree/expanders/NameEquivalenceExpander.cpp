// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include <iostream>
#include <map>
#include <string>
#include <boost/foreach.hpp>

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/PropTree/expanders/NameEquivalenceExpander.h"

NameEquivalenceExpander::NameEquivalenceExpander(const NameDictionary& equivNames, double threshold)
: PropTreeExpander(), _equivNames(equivNames), _threshold(threshold)  {
	if (SessionLogger::dbg_or_msg_enabled("NameEquivalenceExpander")) {
		SessionLogger::dbg("NameEquivalenceExpander") << "NameEquivalenceExpander::NameEquivalenceExpander()";
		for (NameDictionary::const_iterator outer = _equivNames.begin(); outer != _equivNames.end(); outer++) {
			NameSynonyms synonyms = outer->second;
			for (NameSynonyms::const_iterator inner = synonyms.begin(); inner != synonyms.end(); inner++) {
				SessionLogger::dbg("NameEquivalenceExpander") << outer->first << ": (" << inner->first << ", " << inner->second << ")\n";
			}
		}
	}
}


void NameEquivalenceExpander::expand(const PropNodes & pnodes) const {
	SessionLogger::dbg("NameEquivalenceExpander") << "NameEquivalenceExpander::expand()";
	using namespace boost;
	
	BOOST_FOREACH(PropNode_ptr nodep, pnodes) {
		PropNode & pnode( *nodep );

		PropNode::WeightedPredicates orig_preds(pnode.getPredicates().begin(), pnode.getPredicates().end());

		for( PropNode::WeightedPredicates::iterator it = orig_preds.begin(); it != orig_preds.end(); it++ ){
			if( it->first.type() != Predicate::NAME_TYPE ) continue;

			// try to find a name cluster
			NameDictionary::const_iterator en_it = _equivNames.find( it->first.pred().to_string() );
			if( en_it == _equivNames.end() ) continue;

			for( NameSynonyms::const_iterator ens_it = en_it->second.begin(); ens_it != en_it->second.end(); ens_it++ ){
				if( ens_it->second < _threshold ) continue;
				SessionLogger::dbg("NameEquivalenceExpander") << "NameEquivalenceExpander::expand(): " << it->first.pred().to_string() << " --> " << ens_it->first.c_str() << "\n";
				pnode.addPredicate(Predicate(Predicate::NAME_TYPE, 
					ens_it->first.c_str(), it->first.negative() ), it->second * (float)ens_it->second );
			}
		}
	}

	return;
}

