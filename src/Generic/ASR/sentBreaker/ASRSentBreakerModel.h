// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// this whole thing is copied and modified from RelationStats.h -- SRS

#ifndef ASR_SENT_BREAKER_MODEL_H
#define ASR_SENT_BREAKER_MODEL_H


#include "Generic/common/Symbol.h"
#include "Generic/ASR/sentBreaker/ASRSentModelInstance.h"
#include "Generic/common/NgramScoreTable.h"

class UTF8InputStream;
class UTF8OutputStream;

#include <iostream>

#define INITIAL_TABLE_SIZE 1000

template<class AProbModel, class AFilter> class ASRSentBreakerModel {
public:
	// for decoding - read already-trained data
	ASRSentBreakerModel(UTF8InputStream &stream)
		: _probModel(AProbModel(stream)), _filter(AFilter()) 
	{}

	// for training - construct empty structures waiting to be filled in
	ASRSentBreakerModel() : _probModel(AProbModel()), _filter(AFilter()) { 
		_probModel.setUniqueMultipliers(_filter.getUniqueMultiplierArray());
	}

	// query tables for probability
	double getProbability(ASRSentModelInstance *instance)
	{
		Symbol *symVector = _filter.getSymbolVector(instance);
		return _probModel.getProbability(symVector);
	}

	// add training instance to tables
	void addEvent(ASRSentModelInstance *instance) {
		Symbol *symVector = _filter.getSymbolVector(instance);
		_probModel.addEvent(symVector);
	}

	// the probability model will now reflect the data entered via addEvent
	void deriveModel() {
		_probModel.deriveModel();
	}

	// for inspection
	void printSymbolVector(ASRSentModelInstance *instance,
						   UTF8OutputStream& out)
	{
		_filter.printSymbolVector(instance, out);
	}

	// more detailed inspection - model-level
	void printBreakdown(ASRSentModelInstance *instance,
						UTF8OutputStream& out)
	{
		_probModel.printProbabilityElements(
			_filter.getSymbolVector(instance, out));
	}

	void printFormula(UTF8OutputStream& out) {
		_probModel.printFormula(out);
	}
	void print(UTF8OutputStream &out) {
		_probModel.printTables(out);
	}

	// NB: these two functions are only valid if you've
	// created initializeModel and printUnderivedTables
	// for the specific type of probModel being referenced
	void initializeModel(std::istream &input) {
		_probModel.initializeModel(input);
	}

	void printUnderivedTables(UTF8OutputStream &out) {
		_probModel.printUnderivedTables(out);
	}



private:
	AProbModel _probModel;
	AFilter _filter;
};

#endif
