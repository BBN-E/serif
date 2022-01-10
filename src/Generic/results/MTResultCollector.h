// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MT_RESULT_COLLECTOR_H
#define MT_RESULT_COLLECTOR_H

#include "Generic/results/ResultCollector.h"
#include "Generic/theories/DocTheory.h"

class MTResultCollector : public ResultCollector {
public:
	void loadDocTheory(DocTheory* docTheory);
	void produceOutput(const wchar_t *output_dir, const wchar_t* document_filename);
	virtual void produceOutput(std::wstring *results) { ResultCollector::produceOutput(results); }
private:
	DocTheory* _docTheory;	
};

#endif
