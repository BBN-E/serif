// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/ASR/sentBreaker/ASRSentModelInstance.h"
#include "Generic/ASR/sentBreaker/ASRSentBreakerCustomModel.h"
#include <string.h>
#include <boost/scoped_ptr.hpp>

using namespace std;


ASRSentBreakerCustomModel::ASRSentBreakerCustomModel(const char *model_prefix)
	: _wordModel(0), _prevWordModel(0), _bigramModel(0),
	  _trigramModel(0)
{
	Symbol formulaParam = ParamReader::getParam(L"score_formula");
	if (formulaParam == Symbol(L"backoff")) {
		_formula = BACKOFF;
	}
	else if (formulaParam == Symbol(L"weighted")) {
		_formula = WEIGHTED;
	}
	else {
		throw UnexpectedInputException(
			"ASRSentBreakerCustomModel::ASRSentBreakerCustomModel",
			"Parameter 'score_formula' should be set to 'backoff' "
														"or 'weighted'.");
	}

	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& stream(*stream_scoped_ptr);

	SessionLogger::info("SERIF") << "Loading word model...\n";
	std::string model_prefix_str(model_prefix);
	std::string model_file = model_prefix_str + ".word.table";
	stream.open(model_file.c_str());
	if (stream.fail()) {
		throw UnexpectedInputException(
			"ASRSentBreakerCustomModel::ASRSentBreakerCustomModel()",
			"Unable to open model file");
	}
	_wordModel = _new NgramScoreTable(2, stream);
	stream.close();

	SessionLogger::info("SERIF") << "Loading prev-word model...\n";
	model_file = model_prefix_str + ".prevword.table";
	stream.open(model_file.c_str());
	if (stream.fail()) {
		throw UnexpectedInputException(
			"ASRSentBreakerCustomModel::ASRSentBreakerCustomModel()",
			"Unable to open model file");
	}
	_prevWordModel = _new NgramScoreTable(2, stream);
	stream.close();

	SessionLogger::info("SERIF") << "Loading bigram model...\n";
	model_file = model_prefix_str + ".bigram.table";
	stream.open(model_file.c_str());
	if (stream.fail()) {
		throw UnexpectedInputException(
			"ASRSentBreakerCustomModel::ASRSentBreakerCustomModel()",
			"Unable to open model file");
	}
	_bigramModel = _new NgramScoreTable(3, stream);
	stream.close();

	SessionLogger::info("SERIF") << "Loading trigram model...\n";
	model_file = model_prefix_str + ".trigram.table";
	stream.open(model_file.c_str());
	if (stream.fail()) {
		throw UnexpectedInputException(
			"ASRSentBreakerCustomModel::ASRSentBreakerCustomModel()",
			"Unable to open model file");
	}
	_trigramModel = _new NgramScoreTable(4, stream);
	stream.close();

	SessionLogger::info("SERIF") << "All model files loaded.\n";
}

double ASRSentBreakerCustomModel::getProbability(
	ASRSentModelInstance *instance)
{
	Symbol *symVector = _filter.getSymbolVector(instance);

	double p_tri = _trigramModel->lookup(symVector);
	double p_bi = _bigramModel->lookup(symVector);
	double p_word = _wordModel->lookup(symVector);
	double p_prev = _prevWordModel->lookup(symVector);

	if (_formula == BACKOFF) {
		if (p_tri != 0)
			return p_tri;
		if (p_bi != 0)
			return p_bi;
		return (p_word + p_prev)/2;
	}
	else { // _formula == WEIGHTED
		return 0.4 * p_tri +
			   0.3 * p_bi +
			   0.15 * p_word +
			   0.15 * p_prev;
	}
}

