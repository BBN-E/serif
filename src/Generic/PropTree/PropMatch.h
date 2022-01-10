// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.


#ifndef PROPTREE_PROPMATCH_H
#define PROPTREE_PROPMATCH_H

#include "PropNode.h"
#include "Predicate.h"
#include <boost/shared_ptr.hpp>
#include <vector>

class PropMatch {
public:
	
	typedef boost::shared_ptr<PropMatch> ptr_t;

	typedef std::map<std::pair<Symbol, Symbol>, float> ConfusionProbMatrix;
	typedef std::pair<std::pair<Symbol, Symbol>, float> ConfusionProbMatrixEntry;
	
	// Input nodes must be sorted on getPropNodeID() (as returned by PropFactory)
	PropMatch(const PropNodes & nodes,
			  const std::vector<float> & weights = std::vector<float>());
	
	virtual ~PropMatch(){}

	// clears _covered & _covering, invokes computeCoverage(),
	// and computes an overall coverage score in range [0,1].
	// WARNING: compareToTarget expects a list of all the nodes in
	// the tree, not just the roots. Since a document forest contains
	// roots, you have to use PropNode::enumAllNodes to get a list
	// of nodes you can pass into compareToTarget.
	virtual float compareToTarget(const PropNodes &, bool multiplicative=false);

	// Retrieval of match statistics; they're reset each compareToTarget() invocation
	const std::vector<float>           & getMatchCovered()  { return _covered; }
	const std::vector<PropNode_ptr> & getMatchCovering() { return _covering; }

	const int getNumNodes() { return int(getMatchCovered().size()); }
	
	// defines P(target pred type | source pred type); eg, the probability that given the
	// source predicate type, the target predicate type was intended
	float predicateTypeConfusionProb(const Predicate & src_pred, 
		const Predicate & trg_pred) const;
	
	// defines P(target role | source role); eg, the probability that given the
	// source role, the target role was intended
	float roleConfusionProb(const Symbol & src_role, const Symbol & trg_role) const;
	
	virtual void print(std::wostream& s) const = 0;

protected:

	// fills the _covered/ing arrays given targets
	virtual void computeCoverage(const PropNodes &) = 0;
	float computeScoreFromCoverage(bool multiplicative) const;
	
	//weight to apply to covering this node.  Never used in multiplicative modeFor PropEdgeMatch and P 
	std::vector<float> _weights;	
	//score assigned to a covering node
	std::vector<float> _covered;	
	//pointer to the target propNode that covers
	std::vector<PropNode_ptr> _covering;	
	//Should this node be accounted for in computeScoreFromCoverage(). 
	//PropEdgeMatch uses covered/covering, but represents edges as the index of the childeren. 
	//As a result we need to ignore roots
	std::vector<bool> _valid;
	
	ConfusionProbMatrix* _roleConfusionProbs;
	ConfusionProbMatrix* _typeConfusionProbs;
	//static mutable ConfusionProbMatrix _nominalTypeConfusionProbs;

private:
	static void _populateBaseRoleConfusionProbs();
	static void _populateBaseTypeConfusionProbs();

	static ConfusionProbMatrix _baseRoleConfusionProbs;
	static ConfusionProbMatrix _baseTypeConfusionProbs;
};



#endif 
