// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_STATS_H
#define RELATION_STATS_H

#include "Generic/common/Symbol.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/relations/RelationProbModel.h"
#include "Generic/relations/RelationFilter.h"
#include <iostream>

#define INITIAL_TABLE_SIZE 1000

/** Note: this class is not instantiated directly.  Instead, one of the
 * subclasses TypeFeatureVectorModel or TypeB2PFeatureVectorModel should
 * be instantiated. */
class RelationStats {
public:
	virtual ~RelationStats() {
		delete _probModel; _probModel = 0;
		delete _filter; _filter = 0;
	}

	// query tables for probability
	double getProbability ( PotentialRelationInstance *instance , Symbol indicator ) {
		Symbol *symVector = _filter->getSymbolVector(instance, indicator);
		return _probModel->getProbability(symVector);
	}

	// add training instance to tables
	void addEvent( PotentialRelationInstance *instance , Symbol indicator ) {
		Symbol *symVector = _filter->getSymbolVector(instance, indicator);
		_probModel->addEvent(symVector);
	}
	
	// query tables for lambda
	double getLambdaForFullHistory ( PotentialRelationInstance *instance, Symbol indicator) {
		Symbol *symVector = _filter->getSymbolVector(instance, indicator);
		return _probModel->getLambdaForFullHistory(symVector);
	}

	// query tables for lambda
	double getLambdaForHistoryMinusOne ( PotentialRelationInstance *instance, Symbol indicator) {
		Symbol *symVector = _filter->getSymbolVector(instance, indicator);
		return _probModel->getLambdaForHistoryMinusOne(symVector);
	}

	// the probability model will now reflect the data entered via addEvent
	void deriveModel() {
		_probModel->deriveModel();
	}

	// for inspection
	void printSymbolVector( PotentialRelationInstance *instance, Symbol indicator , std::ostream& out ) {
		_filter->printSymbolVector(instance, indicator, out);
	}

	// more detailed inspection - model-level
	void printBreakdown( PotentialRelationInstance *instance, Symbol indicator , UTF8OutputStream& out ) {
		_probModel->printProbabilityElements(_filter->getSymbolVector(instance, indicator), out);
	}

	void printFormula( UTF8OutputStream& out ) {
		_probModel->printFormula( out );
	}
	void print( UTF8OutputStream &out ) {
		_probModel->printTables(out);
	}

	/*
	// NB: these two functions are only valid if you've
	// created initializeModel and printUnderivedTables
	// for the specific type of probModel being referenced
	void initializeModel( istream &input ) {
		_probModel->initializeModel(input);
	}

	void printUnderivedTables( ostream &out ) {
		_probModel->printUnderivedTables(out);
	}
	*/



protected:
	/** Factory interface */
	struct Factory { 
		// for decoding - read already-trained data
		virtual RelationStats *build(UTF8InputStream &stream) = 0; 
		// for training - construct empty structures waiting to be filled in
		virtual RelationStats *build() = 0; 
	};
	/** Factory template -- used by subclasses */
	template <typename RelationStatsSubclass, typename ProbModel, typename Filter>
	struct FactoryFor: public Factory {
		virtual RelationStats *build(UTF8InputStream &stream) {
			return _new RelationStatsSubclass(_new ProbModel(stream), _new Filter()); }
		virtual RelationStats *build() {
			Filter *filter = _new Filter();
			ProbModel *probModel = _new ProbModel();
			probModel->setUniqueMultipliers(filter->getUniqueMultiplierArray());
			return _new RelationStatsSubclass(probModel, filter); }
	};

	// This constructor takes ownership of both the probModel and the fitler.
	RelationStats(RelationProbModel *probModel, RelationFilter *filter)
		: _probModel(probModel), _filter(filter) {}

protected:
	RelationProbModel *_probModel;
	RelationFilter *_filter;

};


template <typename ProbModel, typename Filter>
class RelationStatsTemplate: public RelationStats {
public:
	RelationStatsTemplate()
		: RelationStats(_new ProbModel(), _new Filter())
	{ _probModel->setUniqueMultipliers(_filter->getUniqueMultiplierArray()); }

	RelationStatsTemplate(UTF8InputStream &stream)
		: RelationStats(_new ProbModel(stream), _new Filter()) {}
};

#endif
