// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/ProbModel.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include <math.h>

const int ProbModel::_LOG_OF_ZERO = -10000;

ProbModel::	ProbModel(int N, 
					  UTF8InputStream& stream, 
					  bool applySmoothing,
					  bool trackLambdas,
					  float kappa) throw(UnexpectedInputException)
	: _doSmoothing(applySmoothing), 
	  _table(0), _lambdaTable(0), _incorrectLambdaTable(0)
{
	int numEntries = -10; // just in case it's randomly set to something positive
	UTF8Token token;
	stream >> numEntries;
	if (numEntries < 0)
		throw UnexpectedInputException("ProbModel::()", "ERROR: inappropriate number of entries token in stream");
	_table = _new NgramScoreTable(N, numEntries);
	if (trackLambdas) {
		_lambdaTable = _new NgramScoreTable(N-1, numEntries);
		_incorrectLambdaTable = _new NgramScoreTable(N, numEntries);
	}
	else {
		_lambdaTable = 0;
		_incorrectLambdaTable = 0;
	}

	for (int i = 0; i < numEntries; i++) {

		Symbol* ngram = _new Symbol[N];

		stream >> token;
		if (token.symValue() != SymbolConstants::leftParen)
			throw UnexpectedInputException("ProbModel::()", "ERROR: ill-formed prob-model record");

		stream >> token;
		if (token.symValue() != SymbolConstants::leftParen)
			throw UnexpectedInputException("ProbModel::()", "ERROR: ill-formed prob-model record");

		for (int j = 0; j < N; j++) {
			stream >> token;
			ngram[j] = token.symValue();
		}

		stream >> token;
		if (token.symValue() != SymbolConstants::rightParen)
			throw UnexpectedInputException("ProbModel::()", "ERROR: ill-formed prob-model record");

		float trans;
		float hist;
		float unique;
		stream >> trans;
		stream >> hist;
		stream >> unique;
		float prob = trans / hist;			
		// store as straight prob. log will be later

		stream >> token;
		if (token.symValue() != SymbolConstants::rightParen)
			throw UnexpectedInputException("ProbModel::()", "ERROR: ill-formed prob-model record");

		_table->add(ngram, prob);
		if (trackLambdas) {
			if (_lambdaTable->lookup(ngram) == 0) {
				float lambda = 0.0;
				if (hist != 0)
					lambda = (hist)/(hist + kappa*unique);
				_lambdaTable->add(ngram, lambda);
			}
			if (_incorrectLambdaTable->lookup(ngram) == 0) {
				float lambda = 0.0;
				if (trans != 0)
					lambda = (trans)/(trans + kappa*unique);
				_incorrectLambdaTable->add(ngram, lambda);
			}		
		}
		
		delete[] ngram;
	}
}

ProbModel::~ProbModel() {
	delete _table;
	delete _lambdaTable;
	delete _incorrectLambdaTable;
}

double ProbModel::getProbability (Symbol* ngram)
{
	float score = _table->lookup(ngram);
	// convert to log
	score = score == 0 ? _LOG_OF_ZERO : logf(score);

	if (_doSmoothing) {
		float lambda = _lambdaTable->lookup(ngram);
		score = log(lambda*exp(score) + ((1.0f-lambda)/10000));
	}
	return score;
}
