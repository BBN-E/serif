// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STAT_SENT_BREAKER_MODEL_H
#define STAT_SENT_BREAKER_MODEL_H


#include "Generic/common/Symbol.h"
#include "Generic/sentences/StatSentModelInstance.h"
#include "Generic/common/NgramScoreTable.h"

class UTF8InputStream;
class UTF8OutputStream;

#include <iostream>

#define INITIAL_TABLE_SIZE 1000

template<class AProbModel, class AFilter> class StatSentBreakerModel {
public:
	// for decoding - read already-trained data
	StatSentBreakerModel(UTF8InputStream &stream)
		: _probModel(AProbModel(stream)), _filter(AFilter()) 
	{}

	// for training - construct empty structures waiting to be filled in
	StatSentBreakerModel() : _probModel(AProbModel()), _filter(AFilter()) { 
		_probModel.setUniqueMultipliers(_filter.getUniqueMultiplierArray());
	}

	// query tables for probability
	double getProbability(StatSentModelInstance *instance)
	{
		Symbol *symVector = _filter.getSymbolVector(instance);
		return _probModel.getProbability(symVector);
	}

	// add training instance to tables
	void addEvent(StatSentModelInstance *instance) {
		Symbol *symVector = _filter.getSymbolVector(instance);
		_probModel.addEvent(symVector);
	}

	void deriveAndPrintModel(UTF8OutputStream &out) {
		_probModel.deriveAndPrintModel(out);
	}

	// for inspection
	void printSymbolVector(StatSentModelInstance *instance,
						   UTF8OutputStream& out)
	{
		_filter.printSymbolVector(instance, out);
	}

	// more detailed inspection - model-level
	void printBreakdown(StatSentModelInstance *instance,
						UTF8OutputStream& out)
	{
		_probModel.printProbabilityElements(
			_filter.getSymbolVector(instance, out));
	}

	void pruneVocab(int threshold) {
		_probModel.pruneVocab(threshold);
	}

private:
	AProbModel _probModel;
	AFilter _filter;
};

#endif
