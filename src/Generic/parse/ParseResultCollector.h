// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARSE_RESULT_COLLECTOR_H
#define PARSE_RESULT_COLLECTOR_H

#include "Generic/results/ResultCollector.h"
#include "Generic/common/OutputStream.h"

class DocTheory;

class ParseResultCollector : public ResultCollector {
public:
	ParseResultCollector() {}

	virtual void loadDocTheory(DocTheory* docTheory);
	
	virtual void produceOutput(const wchar_t *output_dir,
							   const wchar_t* document_filename);
	virtual void produceOutput(std::wstring *results) { ResultCollector::produceOutput(results); }

	void produceParseOutput(OutputStream &stream);
	void produceTokenOutput(OutputStream &stream);

private:
	DocTheory *_docTheory;
};

#endif
