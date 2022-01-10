// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROB_MODEL_H
#define PROB_MODEL_H

#include "Generic/common/Symbol.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnexpectedInputException.h"

class ProbModel {

private:
	NgramScoreTable* _table;
	NgramScoreTable* _lambdaTable;
	bool _doSmoothing;
	// for debug
	NgramScoreTable* _incorrectLambdaTable;
	static const int _LOG_OF_ZERO;
	
public:
	// return a log probability of the history1, history2, ..., future vector
	// if smoothing was specified in construction, smooth over the vocabulary size,
	// fixed at 10,000
	double getProbability (Symbol* ngram);
	// return transition weight for history1, history2...
	// this Symbol* will accept one fewer member than the prob one, so if you send
	// the whole event, it's still good.
	// note this is NOT a log transition, cause that would be silly!
	double getLambda (Symbol* ngram) {
		if (_lambdaTable)
			return _lambdaTable->lookup(ngram);
		return 0;
	}

	// while getLambda returns H/(H+kU), getIncorrectLambda returns T/(T+kU), where H is a history
	// vector, T is a transition (history+future_ vector, k is the kappa value, and U is the number of
	// unique transitions with the given history. This incorrect calculation was used in the old system
	double getIncorrectLambda (Symbol* ngram) {
		if (_incorrectLambdaTable)
			return _incorrectLambdaTable->lookup(ngram);
		return 0;
	}

	// if applySmoothing, probabilities have standard witten-bell
	// assuming a vocabulary of 10,000.


	// Probabilities are stored as LOG values, where the log of zero is predefined in this class
	// to be -10000
	ProbModel(
				int N, 
				UTF8InputStream& stream, 
				bool applySmoothing = true, 
				bool trackLambdas = true, 
				float kappa = 1.0) throw(UnexpectedInputException);

	~ProbModel();
};

#endif
