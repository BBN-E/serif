// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SEM_MENTION_H
#define SEM_MENTION_H

#include "Generic/propositions/sem_tree/SemReference.h"

class Mention;


/// Upper bound on number of opp children:
#define MAX_SEM_MENTION_OPPS 3


class SemMention : public SemReference {
private:
	bool _definite;
	const Mention *_mention;

	// "regularized", non-tangential children:
	int _n_opps;
	SemOPP *_opps[MAX_SEM_MENTION_OPPS];
	SemBranch *_branch;

public:
	SemMention(SemNode *children, const SynNode *synNode, 
			   const Mention *mention, bool definite);

	virtual Type getSemNodeType() const { return MENTION_TYPE; }
	virtual SemMention &asMention() { return *this; }

	// accessors
	bool isDefinite() const { return _definite; }
	virtual const Mention *getMention() const { return _mention; }


	virtual void simplify();
	virtual void regularize();
	virtual bool createTraces(SemReference *awaiting_ref);


	virtual void dump(std::ostream &out, int indent = 0) const;
};

#endif
