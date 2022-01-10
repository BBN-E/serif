// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SEM_REFERENCE_H
#define SEM_REFERENCE_H

#include "Generic/propositions/sem_tree/SemNode.h"

class SynNode;
class Mention;


class SemReference : public SemNode {
public:
	SemReference(SemNode *children, const SynNode *synNode)
		: SemNode(children, synNode) {}

	virtual Type getSemNodeType() const { return REFERENCE_TYPE; }
	virtual bool isReference() const { return true; }
	virtual SemReference &asReference() { return *this; }

	virtual const Mention *getMention() const = 0;

	/** Overrides SemNode::containsReferences() */
	virtual bool containsReferences() const { return true; }
};

#endif
