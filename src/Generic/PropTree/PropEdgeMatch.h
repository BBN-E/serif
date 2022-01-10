// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROPEDGEMATCH_H
#define PROPEDGEMATCH_H

#include "Generic/PropTree/Predicate.h"
#include "Generic/PropTree/PropNode.h"
#include "Generic/PropTree/PropMatch.h"
#include <boost/shared_ptr.hpp>
#include <vector>


class PropEdgeMatch : public PropMatch {
public:
	
	typedef boost::shared_ptr<PropEdgeMatch> ptr_t;
	
	PropEdgeMatch(const PropNodes &,
				  const std::vector<float> & weights = std::vector<float>());	
	~PropEdgeMatch();

	void print(std::wostream& s) const;
	void debugPrintValidityAndSourceEdges() const;
	void debugPrintValidityAndCoverage() const;
protected:
	void computeCoverage(const PropNodes &);


protected:
		
	struct LocatedEdge {
		LocatedEdge(const Predicate & p,
					const Predicate & c,
					const Symbol & r,
					float w, size_t s) : 
			parent(p), child(c), role(r),
			prior_prob(w), source_ind(s)
		{}
		
		Predicate parent;
		Predicate child;
		Symbol    role;
		size_t    source_ind;
		float     prior_prob;
	};

	struct LocatedRoot {
		LocatedRoot(const Predicate & p,
					float w, size_t s) :
			pred(p), prior_prob(w), source_ind(s)
		{}
		
		Predicate pred;
		float     prior_prob;
		size_t    source_ind;
	};


	// Used to collect predicates within a graph for purposes of fast matching
	typedef std::vector< LocatedEdge > LocatedEdges;	
	LocatedEdges _loc_edges;
	typedef std::vector<LocatedRoot> LocatedRoots;
	LocatedRoots _loc_roots;

	friend struct locatededge_sym_cmp;
	friend struct locatedroot_sym_cmp;

/*	std::vector< Edge >  _source_edges;
	std::vector< float > _edge_weights;*/

	
	// defines P( target pred type | source pred type )
	//float predicateTypeConfusionProb( const Predicate & trg_pred, const Predicate & src_pred );
	
	// defines P( target role | source role )
	//float roleConfusionProb( const Symbol & trg_role, const Symbol & src_role );

};

#endif //#ifndef PROPEDGEMATCH_H
