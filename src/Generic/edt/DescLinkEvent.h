// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DESC_LINK_EVENT_H
#define DESC_LINK_EVENT_H

#include "Generic/common/Symbol.h"

class DescLinkEvent {
public:
	
	DescLinkEvent(int max_predicates) : _contextPredicates(0) {
		if (max_predicates > 0) 
			_contextPredicates = _new Symbol[max_predicates];
		_outcome = Symbol();
		_max_predicates = max_predicates;
		_n_predicates = 0;
	}

	~DescLinkEvent() { delete [] _contextPredicates; }
	

	int getNContextPredicates() {
		return _n_predicates;
	}

	void setOutcome(Symbol outcome) {
		_outcome = outcome;
	}

	Symbol getOutcome() {
		return _outcome;
	}

	void addContextPredicate(Symbol value) {
		if (_n_predicates < _max_predicates)
			_contextPredicates[_n_predicates++] = value;
		else
			; //Warning!!
	}

	Symbol getContextPredicate(int index) {
		if (index >= 0 && index < _n_predicates)
			return _contextPredicates[index];
		else
			return Symbol();
	}

	// TEMPORARY variables for debugging purposes only
	int mention_id;
	int entity_id;

private:
	Symbol _outcome;
	Symbol *_contextPredicates;

	int _n_predicates;
	int _max_predicates;
};

#endif
