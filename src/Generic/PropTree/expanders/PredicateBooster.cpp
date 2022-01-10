// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/expanders/PredicateBooster.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Argument.h"
#include <boost/foreach.hpp>


void PredicateBooster::expand(const PropNodes &pnodes) const {
	using namespace boost;
	
	PropNodes ordered_pnodes( pnodes );
	sort( ordered_pnodes.begin(), ordered_pnodes.end(), PropNode::propnode_id_cmp() );

	BOOST_FOREACH(PropNode_ptr nodep, ordered_pnodes) {
		PropNode & p( *nodep );

		for( size_t i = 0; i < p.getChildren().size(); i++ ){
			float prob_adj = 0;

			if(p.getRoles()[i] == Argument::MEMBER_ROLE ) prob_adj = BOOST_MEMBER_PROB;
			else if( p.getRoles()[i] == Argument::UNKNOWN_ROLE) prob_adj = BOOST_UNKNOWN_PROB;
			else if( p.getRoles()[i] == Argument::POSS_ROLE   ) prob_adj = BOOST_POSS_PROB;
			else if( p.getRoles()[i] == Symbol(L"of")         ) prob_adj = BOOST_OF_PROB;

			if( ! prob_adj ) continue;

			BOOST_FOREACH(PropNode::WeightedPredicate wpred, p.getChildren()[i]->getPredicates()) {
				p.addPredicate(wpred.first, prob_adj*wpred.second);
			}
		}
	}

	return;
}
