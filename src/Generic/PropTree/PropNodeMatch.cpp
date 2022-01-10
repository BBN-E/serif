// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/PropTree/PropNodeMatch.h"
#include <algorithm>


PropNodeMatch::PropNodeMatch(const PropNodes & nodes,
							 const std::vector<float> & weights /*= std::vector<float>()*/)
 :
	PropMatch(nodes, weights), _propNodes(nodes)
{
	for(size_t p_ind = 0; p_ind != nodes.size(); p_ind++){
		const PropNode & p(*nodes[p_ind]);
		
		// enumerate regular predicates & stems
		for(PropNode::WeightedPredicates::const_iterator pr_it = p.getPredicates().begin();
											  pr_it != p.getPredicates().end(); pr_it++)
		{
			for( std::vector<Symbol>::const_iterator rit = p.getRolesPlayed().begin(); rit != p.getRolesPlayed().end(); rit++ )
				_loc_preds.push_back( LocatedPred(pr_it->first, *rit, pr_it->second, p_ind));
			if( p.getRolesPlayed().empty() )
				_loc_preds.push_back( LocatedPred(pr_it->first, Symbol(L""), pr_it->second, p_ind));
		}
		// ... and extended predicates
		// Ryan says comment this out
		//for(PropNode::WeightedPredicates::const_iterator pr_it = p.getExtendedPredicates().begin();
		//									  pr_it != p.getExtendedPredicates().end(); pr_it++)
		//{
		//	for( vector<Symbol>::const_iterator rit = p.getRolesPlayed().begin(); rit != p.getRolesPlayed().end(); rit++ )
		//		_loc_preds.push_back( LocatedPred(pr_it->first, *rit, pr_it->second, p_ind));
		//	if( p.getRolesPlayed().empty() )
		//		_loc_preds.push_back( LocatedPred(pr_it->first, Symbol(L""), pr_it->second, p_ind));
		//}
		
		// nodes with no predicates contribute zero weight
		if (p.getPredicates().empty()) { // && p.getExtendedPredicates().empty())
			this->_weights[p_ind] = 0;
		}
	}
	
	// index for binary search
	sort(_loc_preds.begin(), _loc_preds.end(), locatedpred_sym_cmp());
	return;
}

void PropNodeMatch::computeCoverage(const PropNodes & nodes) {	
	for(size_t tn_ind = 0; tn_ind != nodes.size(); tn_ind++){
		const PropNode & p(*nodes[tn_ind]);
		
		// Loop over the nodes passed in as the argument
		for(PropNode::WeightedPredicates::const_iterator pr_it = p.getPredicates().begin(); pr_it != p.getPredicates().end(); pr_it++) {
			std::pair<LocatedPreds::iterator, LocatedPreds::iterator> match_range =
				equal_range(_loc_preds.begin(), _loc_preds.end(), pr_it->first, locatedpred_sym_cmp());
			
			while(match_range.first != match_range.second) {
				const LocatedPred & lpred(*(match_range.first++));
				
				// independently combine the predicate transition probability (eg, 'desc' -> 'verb'),
				// and the predicate prior probabilities
				float prob = predicateTypeConfusionProb(lpred.pred, pr_it->first) *
							 lpred.prior_prob * pr_it->second;
				
				if(prob > this->_covered[lpred.source_ind]){
					this->_covered[ lpred.source_ind] = prob;
					this->_covering[lpred.source_ind] = nodes[tn_ind];
				}
			}
		}
	}
	
	return;
}


void PropNodeMatch::print(std::wostream& stream) const {
	int num_groups = 0;
	if (this->_covered.size() == 0)
		SessionLogger::warn("prop_trees") << "_covered has size 0 in node match\n";
	std::map<std::wstring,int> pred_counts;
	for (LocatedPreds::const_iterator it = _loc_preds.begin(); it != _loc_preds.end(); it++) {
		std::wstring pred_string = it->pred.pred().to_string();		
		const Symbol& role = it->role;
		float prior_prob = it->prior_prob;
		size_t source_ind = it->source_ind;
		float covered = _covered[source_ind];
		const PropNode_ptr& covering = _covering[source_ind];
		if (covering) {
			stream << source_ind << L". <<" << role << L">> " << pred_string << L"-" << it->pred.type().to_string() << L": " << prior_prob << L" (" << covered << L") -- ";
			covering->compactPrint(stream, false, false, false, 0);
			stream << L"\n";	
		} else {
			stream << source_ind << L". <<" << role << L">> " << pred_string << L"-" << it->pred.type().to_string() << L": " << prior_prob << L" (" << covered << L")\n";	
		}
	}
}
