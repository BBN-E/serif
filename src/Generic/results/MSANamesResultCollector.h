// Copyright 2016 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MSA_NAMES_RESULT_COLLECTOR_H
#define MSA_NAMES_RESULT_COLLECTOR_H

#include "Generic/results/ResultCollector.h"
#include "Generic/common/OutputStream.h"

class DocTheory;

class MSANamesResultCollector : public ResultCollector {
public:
	MSANamesResultCollector();
	virtual ~MSANamesResultCollector() {}
	virtual void finalize() {}

	virtual void loadDocTheory(DocTheory* docTheory);
	
	virtual void produceOutput(const wchar_t *output_dir,
							   const wchar_t* document_filename);
	virtual void produceOutput(std::wstring *results);

private:
	DocTheory *_docTheory;
	std::wstring produceOutputHelper();

};

#endif
