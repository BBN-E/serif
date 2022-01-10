// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROPCONSTRAINEDMATCH_H
#define PROPCONSTRAINEDMATCH_H

#include "PropFullMatch.h"

// PropConstrainedMatch is like PropFullMatch,
// but it only updates a covering if the entire 
// covering and not only the local match is
// better than the best previous covering.  Effectively,
// this prevents the discontinuous matches which can
// sometimes happen with PropFullMatch where one part
// of a pattern will match one part of a tree and another
// a completely different part. This is rare in topicality
// matching, but common when working with variables. ~ RMG

class PropConstrainedMatch : public PropFullMatch {
public:
	PropConstrainedMatch(const PropNodes& nodes, float threshold) : 
		PropFullMatch(nodes, false), _threshold(threshold), verbose(false) {};
	bool verbose;

protected:
	void computeCoverage(const PropNodes&);

private:
	float internalCompareConstrained (const size_t & n_ind,  
		const PropNode_ptr & trg,
		const Symbol & s_role, const Symbol & t_role,  std::vector<float>& covering, 
		PropNodes& covered, float pruning_score, float threshold);
	float _threshold;
};



#endif
