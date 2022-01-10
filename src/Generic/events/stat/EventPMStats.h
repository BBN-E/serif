// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_PM_STATS_H
#define EVENT_PM_STATS_H

#include "Generic/common/Symbol.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/common/NgramScoreTable.h"
#include <iostream>

#define INITIAL_TABLE_SIZE 1000
template <class AProbModel, class AFilter> class EventPMStats {
public:
	// for decoding - read already-trained data
	EventPMStats( UTF8InputStream &stream ) : _probModel(AProbModel(stream)), 
		_filter(AFilter()) 
	{ } 

	// for training - construct empty structures waiting to be filled in
	EventPMStats( ) : _probModel(AProbModel()), 
		_filter(AFilter()) 
	{ 
		_probModel.setUniqueMultipliers(_filter.getUniqueMultiplierArray());
	}

	// query tables for probability
	double getProbability ( EventTriggerObservation *observation, Symbol eventType) {
		Symbol *symVector = _filter.getSymbolVector(observation, eventType);
		return _probModel.getProbability(symVector);
	}

	// query tables for lambda
	double getLambdaForFullHistory ( EventTriggerObservation *observation, Symbol eventType) {
		Symbol *symVector = _filter.getSymbolVector(observation, eventType);
		return _probModel.getLambdaForFullHistory(symVector);
	}

	// add training instance to tables
	void addEvent( EventTriggerObservation *observation , Symbol eventType ) {
		Symbol *symVector = _filter.getSymbolVector(observation, eventType);
		_probModel.addEvent(symVector);
	}

	// the probability model will now reflect the data entered via addEvent
	void deriveModel() {
		_probModel.deriveModel();
	}

	// for inspection
	void printSymbolVector( EventTriggerObservation *observation, Symbol eventType , UTF8OutputStream& out ) {
		_filter.printSymbolVector(observation, eventType, out);
	}

	// more detailed inspection - model-level
	void printBreakdown( EventTriggerObservation *observation, Symbol eventType , UTF8OutputStream& out ) {
		_probModel.printProbabilityElements(_filter.getSymbolVector(observation, eventType), out);
	}

	void printFormula( UTF8OutputStream& out ) {
		_probModel.printFormula( out );
	}
	void print( UTF8OutputStream &out ) {
		_probModel.printTables(out);
	}

	// NB: these two functions are only valid if you've
	// created initializeModel and printUnderivedTables
	// for the specific type of probModel being referenced
	void initializeModel( istream &input ) {
		_probModel.initializeModel(input);
	}

	void printUnderivedTables( ostream &out ) {
		_probModel.printUnderivedTables(out);
	}



private:
	AProbModel _probModel;
	AFilter _filter;

};
#endif
