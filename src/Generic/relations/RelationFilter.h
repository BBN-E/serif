// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_FILTER_H
#define RELATION_FILTER_H

#include "Generic/common/Symbol.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include <iostream>


class RelationFilter {
public:
	RelationFilter()
		: _vectorLength(0), _probEvent(0), _uniqueMultipliers(0)
	{}
	virtual ~RelationFilter() {
		delete[] _probEvent;
		delete[] _uniqueMultipliers;
	}

	float* getUniqueMultiplierArray() { return _uniqueMultipliers; }
	virtual Symbol* getSymbolVector( PotentialRelationInstance *instance , Symbol indicator) = 0; 
	int getVectorLength() { return _vectorLength; }
	void printSymbolVector( PotentialRelationInstance *instance, Symbol indicator, ostream& out ) {
		Symbol *symVec = getSymbolVector(instance, indicator);
		out << "[ ";
		for (int i = 0; i < getVectorLength(); i++) {
			out << symVec[i].to_string() << " ";
		}
		out << "]\n";
	}

protected:
	RelationFilter(int vectorLength) : _vectorLength( vectorLength ) { 
		_probEvent = _new Symbol[vectorLength];
		_uniqueMultipliers = _new float[vectorLength-1];
		for (int i = 0; i < vectorLength-1; i++)
			_uniqueMultipliers[i] = 1;
	}

	Symbol* _probEvent;
	float * _uniqueMultipliers;

private:
	int _vectorLength; 
};

#endif

