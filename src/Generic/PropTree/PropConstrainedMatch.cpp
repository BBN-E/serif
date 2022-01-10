// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/PropTree/PropConstrainedMatch.h"

void PropConstrainedMatch::computeCoverage(const PropNodes & trg) {
	PropNodes trg_compare_nodes;
	PropNode::enumAllNodes(trg, trg_compare_nodes);

	double bestScore=-std::numeric_limits<double>::max();

	std::vector<PropNode_ptr> tmpCovering(_covering.size());
	std::vector<float> tmpCovered(_covered.size());

	const std::vector<size_t>& roots=getRootIndices();

	for(size_t r_ind = 0; r_ind != roots.size(); ++r_ind)
	{
		for(PropNodes::const_iterator t_it = trg_compare_nodes.begin();
			t_it != trg_compare_nodes.end(); t_it++)
		{
			double score=internalCompareConstrained(roots[r_ind], *t_it, Symbol(), Symbol(),
				tmpCovered, tmpCovering, 1.0f, _threshold);

			if (score>bestScore) {
				bestScore=score;
				std::copy(tmpCovering.begin(), tmpCovering.end(), _covering.begin());
				std::copy(tmpCovered.begin(), tmpCovered.end(), _covered.begin());
			}
		}
	}
}


float PropConstrainedMatch::internalCompareConstrained (
	const size_t & n_ind,  const PropNode_ptr & trg,
	const Symbol & s_role, const Symbol & t_role, 
	std::vector<float>& covered, PropNodes& covering, float pruning_score, float threshold)
{
	if (verbose) {
		std::wostringstream ostr;
		ostr << "In internalCompareConstrained(), comparing to" << std::endl;
		trg->compactPrint(ostr, true, true, false, 0);
		ostr << std::endl << std::endl;
		SessionLogger::info("SERIF") << ostr.str();
	}

	// determine node-to-node match
	float match_prob = computeLocalMatchProb(trg, s_role, t_role, n_ind);

	// this still greedily matching children, perhaps matching
	// the same pattern node to multiple target nodes, which is not
	// optimal. Let's see if this is actually a problem before we 
	// change it, though. - RMG
	/*if(match_prob > _overed[n_ind])
	{*/
		covered[ n_ind] = match_prob;
		covering[n_ind] = trg;
	//}

	pruning_score*=match_prob;
	
	float kid_score;

	std::pair<rchildren_t::const_iterator, rchildren_t::const_iterator> bounds=getChildRange(n_ind);

	std::vector<float> probeCovered(covered.begin(), covered.end());
	std::vector<PropNode_ptr> probeCovering(covering.begin(), covering.end());

	bool first=true;

	for(rchildren_t::const_iterator sc_it = bounds.first;
		sc_it != bounds.second; ++sc_it)
	{
		float best_kid_score=0.0; // -numeric_limits<float>::max();

		if (!first) {
			std::copy(covered.begin(), covered.end(), probeCovered.begin());
			std::copy(covering.begin(), covering.end(), probeCovering.begin());
		}

		first=false;

		for(size_t tc_ind = 0; tc_ind != trg->getNChildren(); ++tc_ind)
		{
			if (pruning_score<threshold) {
				return 0.0;
			}
			kid_score=internalCompareConstrained(sc_it->second,
							trg->getChildren()[tc_ind],
							sc_it->first,
							trg->getRoles()[tc_ind], probeCovered, probeCovering, pruning_score, threshold);
		
			if (kid_score>best_kid_score) {
				best_kid_score=kid_score;
				std::copy(probeCovered.begin(),  probeCovered.end(), covered.begin());
				std::copy(probeCovering.begin(), probeCovering.end(), covering.begin());
			}
		}

		match_prob*=best_kid_score;
		pruning_score*=best_kid_score;
	}

	return match_prob;
}

