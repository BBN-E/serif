// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

 
#ifndef PROPNODEMATCH_H
#define PROPNODEMATCH_H

#include "Generic/PropTree/PropMatch.h"
#include "PropNode.h"
#include "Predicate.h"
#include <boost/shared_ptr.hpp>
#include <vector>

class PropNodeMatch : public PropMatch {
public:
	typedef boost::shared_ptr<PropNodeMatch > ptr_t;

	PropNodeMatch(const PropNodes &,
		const std::vector<float> & weights = std::vector<float>());

	void print(std::wostream& s) const;

	//// Must be a full enumeration (all children present); Bag-of-Nodes style matching.
	//float compareToTarget( const PropNodes & nodes );
	//
	//// node_similarity is a square matrix, accessed as node_similarity[ _source_pnodes.size() * trg_ind + src_ind ]
	////  get/setSourcePNodes() defines the source node enumeration sequence; 'nodes' here defines the target node
	////  enumeration. Must be a full enumeration (all children explicitly present)
	//float compareToTarget( const PropNodes & nodes, const std::vector<float> & node_similarity );
	//
	//// Must be a full enumeration (all children present); Bag-of-Nodes style matching. Tracks which target node indicies
	//// matched the source tree, and the largest match probability in the source tree for each of those indicies.
	//float compareToTarget( const PropNodes & nodes, std::vector<size_t> & match_ind, std::vector<float> & match_prob );
	//
	//const PropNodes & getSourcePNodes() const { return _source_pnodes; }
	//const std::vector< float > & getSourcePNodeWeights() const { return _source_weights; }
	//
	//void setSourcePNodeWeights( const std::vector<float> & w ) { _source_weights = w; }

protected:
	void computeCoverage(const PropNodes & nodes);

private:
	struct LocatedPred {
		LocatedPred(const Predicate & p,
					const Symbol & r,
					float w, size_t s) :
			pred(p), role(r),
			prior_prob(w), source_ind(s)
		{}
		
		Predicate pred;
		Symbol role;
		
		float prior_prob;
		size_t source_ind;
	};

	// Used to collect predicates within a graph for purposes of fast matching
	typedef std::vector< LocatedPred > LocatedPreds;
	LocatedPreds _loc_preds;

	// Just for storage, not for matching
	const PropNodes _propNodes;

	friend struct locatedpred_sym_cmp;

	//// defines P( target pred type | source pred type )
	//float predicateTypeConfusionProb( const Predicate & trg_pred, const Predicate & src_pred );
	//
	//// defines P( target role | source role )
	//float roleConfusionProb( const Symbol & trg_role, const Symbol & src_role );
	
	//PropNodes    _source_pnodes;
	//std::vector< float > _source_weights;
public:
	
	Symbol getNthPredicateSymbol(size_t n) {
		PropNode_ptr pnode = _propNodes.at(n);
		if (pnode && pnode->getRepresentativePredicate()) {
			Symbol pred = pnode->getRepresentativePredicate()->pred();
			if (!pred.is_null())
				return pred;
		}
		// return this because we might want to print it
		return Symbol(L"NULL");
	}
	
};

// Not a total-ordering; allows for quickly finding ranges of matching predicates
struct locatedpred_sym_cmp {
	inline bool operator()(	const PropNodeMatch::LocatedPreds::value_type & lhs,
							const PropNodeMatch::LocatedPreds::value_type & rhs ) const
	{
		return predicate_pred_cmp<Predicate>()( lhs.pred, rhs.pred );
	}
	inline bool operator()(	const PropNodeMatch::LocatedPreds::value_type & lhs,
		const Predicate & rhs ) const
	{
		return predicate_pred_cmp<Predicate>()( lhs.pred, rhs );
	}
	inline bool operator()(	const Predicate & lhs,
							const PropNodeMatch::LocatedPreds::value_type & rhs ) const
	{
		return predicate_pred_cmp<Predicate>()( lhs, rhs.pred );
	}
};


#endif //#ifndef PROPNODEMATCH_H


