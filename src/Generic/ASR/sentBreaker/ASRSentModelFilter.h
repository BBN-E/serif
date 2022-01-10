// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// copied from RelationFilter.h -- SRS

#ifndef ASR_SENT_MODEL_FILTER_H
#define ASR_SENT_MODEL_FILTER_H

#include "Generic/common/Symbol.h"
#include "Generic/ASR/sentBreaker/ASRSentModelInstance.h"

#include <iostream>


class ASRSentModelFilter {
public:
	ASRSentModelFilter()
		: _vectorLength(0), _probEvent(0), _uniqueMultipliers(0)
	{}

	virtual ~ASRSentModelFilter() {
		delete[] _probEvent;
		delete[] _uniqueMultipliers;
	}

	float *getUniqueMultiplierArray() { return _uniqueMultipliers; }
	virtual Symbol* getSymbolVector(ASRSentModelInstance *instance) = 0; 
	int getVectorLength() { return _vectorLength; }
	void printSymbolVector(ASRSentModelInstance *instance,
						   std::ostream& out)
	{
		Symbol *symVec = getSymbolVector(instance);
		out << "[ ";
		for (int i = 0; i < getVectorLength(); i++) {
			out << symVec[i].to_string() << " ";
		}
		out << "]\n";
	}

protected:
	ASRSentModelFilter(int vectorLength) : _vectorLength(vectorLength) { 
		_probEvent = _new Symbol[vectorLength];
		_uniqueMultipliers = _new float[vectorLength-1];
		for (int i = 0; i < vectorLength-1; i++)
			_uniqueMultipliers[i] = 1;
	}

	Symbol *_probEvent;
	float *_uniqueMultipliers;

private:
	int _vectorLength; 
};

#endif

