// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SEM_OPP_H
#define SEM_OPP_H

#include "Generic/common/Symbol.h"
#include "Generic/propositions/sem_tree/SemNode.h"

#include "Generic/theories/Proposition.h"

class SynNode;
class PropositionSet;

#define MAX_OPP_LINKS 5


class SemOPP : public SemNode {
private:
	Proposition::PredType _predType;
	Symbol _headSym;
	const SynNode *_head;
	const SynNode *_particle;
	const SynNode *_adverb;
	const SynNode *_negative;
	const SynNode *_modal;
	Proposition *_proposition;

	// "regularized", non-tangential children:
	SemNode *_arg1;
	SemNode *_arg2;
	int _n_links;
	SemLink *_links[MAX_OPP_LINKS];

	static Symbol BETWEEN_SYMBOL;

public:
	SemOPP(SemNode *children, const SynNode *synNode,
		   Proposition::PredType predType, const SynNode *head);

	SemOPP(SemNode *children, const SynNode *synNode,
		   Proposition::PredType predType, const SynNode *head,
		   Symbol headSym);

/*	SemOPP(SemNode children[], ParseNode parse_node, ParseNode head, 
		Symbol head_symbol, int predicate_type)
		*/

	virtual ~SemOPP();

	virtual Type getSemNodeType() const { return OPP_TYPE; }
	virtual SemOPP &asOPP() { return *this; }


	Proposition::PredType getPredicateType() const { return _predType; }
	void setPredicateType(Proposition::PredType type) { _predType = type; }

	SemNode *getArg1() const { return _arg1; }
	SemNode *getArg2() const { return _arg2; }

	const SynNode *getHead() const { return _head; }
	Symbol getHeadSymbol() const { return _headSym; }
	Proposition *getProposition() const { return _proposition; }

	void addParticle(const SynNode *particle);
	void addAdverb(const SynNode *adverb);
	void addNegative(const SynNode *negative);
	void addModal(const SynNode *modal);

	SemNode *findSyntacticSubject() const;

	virtual void simplify();
	virtual void regularize();
	virtual bool createTracesEarnestly(SemReference *awaiting_ref);
	virtual void createPropositions(IDGenerator &propIDGenerator);
	virtual void listPropositions(PropositionSet &result);


	virtual void dump(std::ostream &out, int indent = 0) const;

protected:
	static bool isValidArgument(SemNode *node);
};

#endif
