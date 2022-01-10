// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_PROB_MODEL_H
#define RELATION_PROB_MODEL_H

class Symbol;
class NGramScoreTable;
class UTF8OutputStream;

/** Abstract base class for BackoffProbModel and friends */
class RelationProbModel {
public:
	static const int LOG_OF_ZERO = -10000;
	virtual void setUniqueMultipliers (float *uMults) = 0;
	virtual double getProbability( Symbol *features ) const = 0;
	virtual double getLambdaForFullHistory(Symbol *features) = 0;
	virtual double getLambdaForHistoryMinusOne(Symbol *features) = 0;
	virtual void addEvent(Symbol *symVector) = 0;
	virtual void deriveModel( ) = 0;
	virtual void deriveModel(NgramScoreTable *rawData) = 0;
	virtual void printTables(UTF8OutputStream &out) = 0;
	virtual int getHistorySize() = 0;
	virtual void printFormula(UTF8OutputStream &out) const = 0;
	virtual void printProbabilityElements(Symbol *features, UTF8OutputStream &out) const = 0;
	virtual ~RelationProbModel() {}
};

#endif

