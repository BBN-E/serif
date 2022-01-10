// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef OLD_MAX_ENT_EVENT_H
#define OLD_MAX_ENT_EVENT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolSet.h"
#include "Generic/common/InternalInconsistencyException.h"

class OldMaxEntEvent {
public:
	
	OldMaxEntEvent(int max_predicates = 100) : _contextPredicates(0) {
		if (max_predicates > 0) 
			_contextPredicates = _new SymbolSet[max_predicates];
		_outcome = Symbol();
		_max_predicates = max_predicates;
		_count = 1;
		_n_predicates = 0;

		// Automatically add "empty" context predicate so that 
		// all maxent events have at least one active feature
		// (requirement for GIS training) 
		addContextPredicate(SymbolSet());
	}

	OldMaxEntEvent(const OldMaxEntEvent &other) : _contextPredicates(0), _max_predicates(0) {
		(*this) = other;
	}

	~OldMaxEntEvent() { delete [] _contextPredicates; }

	const OldMaxEntEvent &operator=(const OldMaxEntEvent &other) {
		if (_max_predicates != other._max_predicates) {
			delete [] _contextPredicates;
			_max_predicates = other._max_predicates;
			_contextPredicates = _new SymbolSet[_max_predicates];
		}
		for (int i = 0; i < other._n_predicates; i++) {
			_contextPredicates[i] = other._contextPredicates[i];
		}
		_n_predicates = other._n_predicates;
		_outcome = other._outcome;
		_count = other._count;
		
		return (*this);
	}

	void reset() {
		_n_predicates = 1;		// only empty predicate remains
		//_n_predicates = 0;
		_outcome = Symbol();
		_count = 1;
	}

	int getNContextPredicates() {
		return _n_predicates;
	}

	void setOutcome(Symbol outcome) {
		_outcome = outcome;
	}

	Symbol getOutcome() {
		return _outcome;
	}

	void setCount(int count) {
		_count = count;
	}

	int getCount() {
		return _count;
	}

	void addContextPredicate(SymbolSet predicate) {
		bool found = false;
		for (int i = 0; i < _n_predicates; i++) {
			if (_contextPredicates[i] == predicate) {
				found = true;
				break;
			}
		}
		if (!found) {
			if (_n_predicates < _max_predicates)
				_contextPredicates[_n_predicates++] = predicate;
			else
				throw InternalInconsistencyException("OldMaxEntEvent::addContextPredicate()", 
												 "Number of predicates exceeds max_predicates");
		}
	}


	void addComplexContextPredicate(int n_atoms, Symbol *atoms) {
		SymbolSet predicate = SymbolSet(n_atoms, atoms);
		bool found = false;
		for (int i = 0; i < _n_predicates; i++) {
			if (_contextPredicates[i] == predicate) {
				found = true;
				break;
			}
		}
		if (!found) {
			if (_n_predicates < _max_predicates)
				_contextPredicates[_n_predicates++] = predicate;
			else
				throw InternalInconsistencyException("OldMaxEntEvent::addComplexContextPredicate()", 
												 "Number of predicates exceeds max_predicates");
		}
	}

	void addAtomicContextPredicate(Symbol value) {
		SymbolSet predicate = SymbolSet(1, &value);
		bool found = false;
		for (int i = 0; i < _n_predicates; i++) {
			if (_contextPredicates[i] == predicate) {
				found = true;
				break;
			}
		}
		if (!found) {
			if (_n_predicates < _max_predicates) 
				_contextPredicates[_n_predicates++] = predicate;
			else
				throw InternalInconsistencyException("OldMaxEntEvent::addAtomicContextPredicate()", 
												 "Number of predicates exceeds max_predicates");
		}
	}

	SymbolSet getContextPredicate(int index) {
		if (index >= 0 && index < _n_predicates)
			return _contextPredicates[index];
		else
			throw InternalInconsistencyException("OldMaxEntEvent::getContextPredicate()", 
												 "Array index out of bounds");
	}

	bool find(SymbolSet predicate) {
		for (int i = 0; i < _n_predicates; i++) {
			if (_contextPredicates[i] == predicate) {
				return true;
			}
		}
		return false;
	}

	bool find(int n_atoms, Symbol *atoms) {
		SymbolSet predicate = SymbolSet(n_atoms, atoms);
		for (int i = 0; i < _n_predicates; i++) {
			if (_contextPredicates[i] == predicate) {
				return true;
			}
		}
		return false;
	}

	bool find(Symbol sym) {
		SymbolSet predicate(1, &sym);
		for (int i = 0; i < _n_predicates; i++) {
			if (_contextPredicates[i] == predicate) {
				return true;
			}
		}
		return false;
	}


private:
	Symbol _outcome;
	SymbolSet *_contextPredicates;

	int _count;

	int _n_predicates;
	int _max_predicates;
};

#endif
