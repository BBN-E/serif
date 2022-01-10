// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SEM_BRANCH_H
#define SEM_BRANCH_H


#include "Generic/propositions/sem_tree/SemNode.h"

class SynNode;
class Proposition;
class PropositionSet;


class SemBranch : public SemNode {
private:
	// "regularized", non-tangential children:
	SemReference *_ref; // could be branch or 
	SemOPP *_opp;


public:
	SemBranch(SemNode *children, const SynNode *synNode)
		: SemNode(children, synNode), _ref(0), _opp(0) {}


	virtual Type getSemNodeType() const { return BRANCH_TYPE; }
	virtual SemBranch &asBranch() { return *this; }


	SemReference *getReference() const
		{ return _ref; }
	SemOPP *getOPP() const
		{ return _opp; }


	virtual void simplify();
	virtual void fixLinks();
	virtual void regularize();
	virtual bool createTraces(SemReference *awaiting_ref);

	/** Get proposition represented by branch (or 0 if none) */
	Proposition *getAssociatedProposition();


	virtual void dump(std::ostream &out, int indent = 0) const;
};

#endif
