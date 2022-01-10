// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SEM_LINK_H
#define SEM_LINK_H

#include "Generic/common/Symbol.h"
#include "Generic/propositions/sem_tree/SemNode.h"

#include <iostream>

class SynNode;


class SemLink : public SemNode {
private:
	Symbol _symbol;
	const SynNode *_head;
	bool _quote; // basically just means that this is a TEXT_ARGUMENT

	// "regularized", non-tangential children:
	SemNode *_object;


public:
	SemLink(SemNode *children, const SynNode *synNode, Symbol symbol,
			const SynNode *head, bool quote = false)
		: SemNode(children, synNode), _symbol(symbol), _head(head),
		  _quote(quote), _object(0) {}


	virtual Type getSemNodeType() const { return LINK_TYPE; }
	virtual SemLink &asLink() { return *this; }

	
	// accessors
	Symbol getSymbol() const { return _symbol; }
	const SynNode *getHead() const { return _head; }
	bool isQuote() const { return _quote; }
	SemNode *getObject() const { return _object; }

	// you shouldn't ordinarily need to use this:
	void setSymbol(Symbol symbol) { _symbol = symbol; }


	virtual void simplify();
	virtual void fixLinks();
	virtual void regularize();

	virtual bool createTraces(SemReference *awaiting_ref);


	virtual void dump(std::ostream &out, int indent) const;
};

#endif
