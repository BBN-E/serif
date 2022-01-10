// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.


#ifndef PROPFULLMATCH_H
#define PROPFULLMATCH_H

#include <vector>
#include <iostream>
#include <boost/shared_ptr.hpp>

#include "Generic/common/SessionLogger.h"
#include "Generic/PropTree/PropMatch.h"
#include "Generic/PropTree/PropNode.h"

class PropFullMatch : public PropMatch {
public:
	
	typedef boost::shared_ptr<PropFullMatch> ptr_t;
	
	PropFullMatch(const PropNodes &,
		bool compare_roots_only=true,
     	  const std::vector<float> & weights = std::vector<float>());

	virtual float compareToTarget(const PropNodes&, bool multiplicative=false);	
	void print(std::wostream& s) const;

protected:
	typedef std::vector<PropNode::WeightedPredicate> wpreds_t;
	typedef std::pair<const wpreds_t::const_iterator, const wpreds_t::const_iterator> PatternPredicateBounds;
	typedef std::vector< std::pair<Symbol, size_t> > rchildren_t;
	
	virtual void computeCoverage(const PropNodes &) {SessionLogger::info("SERIF") << "PropFullMatch::computeCoverage is intentionally unimplemented.\n"; }
	virtual float computeLocalMatchProb(const PropNode_ptr& trg, const Symbol & s_role, const Symbol & t_role, size_t n_ind) const;

	std::pair<rchildren_t::const_iterator, rchildren_t::const_iterator> getChildRange(size_t par_idx) const {
            // putting this inline here made a compiler error go away for
            // GCC ~ RMG
            const node_state& ns=_nodes[par_idx];
            return make_pair(_rchildren.begin()+ns.rchildren_begin, _rchildren.begin()+ns.rchildren_end);
        }

	const std::vector<size_t>& getRootIndices() const {return _roots; }
	PatternPredicateBounds getWeightedPredicateRange(size_t idx) const;

	virtual float matchPredicate(size_t pat_node_id, 
		const wpreds_t::const_iterator& startIt, 
					 const wpreds_t::const_iterator& endIt, 
					 const PropNode::WeightedPredicate& docPred,
					 const Symbol& s_role, const Symbol& t_role) const;

private:
	
	void internalCompare(size_t n_ind,  const PropNode_ptr & trg,
						 const Symbol & s_role, const Symbol & t_role);
	void printNode(std::wostream& s, size_t node_state_index, int indent, const std::wstring& role) const;	

	struct node_state {
		node_state(): wpreds_begin(-1), wpreds_end(-1), rchildren_begin(-1), rchildren_end(-1),
			exact_role_match(false) {}
		size_t wpreds_begin,    wpreds_end;
		size_t rchildren_begin, rchildren_end;
		size_t id;
		bool exact_role_match;
	};
	
	
	wpreds_t    _wpreds;    // Stores WeightedPredicates
	rchildren_t _rchildren; // Stores (role, index) pairs
	
	std::vector<node_state> _nodes;
	std::vector<size_t>     _roots;

	bool _compare_roots_only;
};


#endif 

