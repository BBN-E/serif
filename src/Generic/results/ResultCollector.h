// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RESULT_COLLECTOR_H
#define RESULT_COLLECTOR_H

#include "Generic/common/ParamReader.h"
#include "dynamic_includes/common/SerifRestrictions.h"
#include "Generic/common/InternalInconsistencyException.h"

class DocTheory;


/** This is actually part of the interface to the DocumentDriver. It is an
  * abstract class which DocumentDriver expects an instance of. That object
  * is called upon by the DocumentDriver to produce output during the "output"
  * stage.
  *
  * The regular Serif executable uses the APFResultCollector subclass, to
  * generate APF output.
  */

class ResultCollector {
public:
	ResultCollector() {

		#ifdef BLOCK_FULL_SERIF_OUTPUT
		std::vector<std::string> outputFormats = ParamReader::getStringVectorParam("output_format");
		if (outputFormats.empty()) {
		  throw UnexpectedInputException("ResultCollector::ResultCollector()", "Parameter 'output_format' must be set to 'CAMEOXML'");
		}
		else if (outputFormats[0] != "CAMEOXML") {
		  throw UnexpectedInputException("ResultCollector::ResultCollector()", "Parameter 'output_format' must be set to 'CAMEOXML'");
		}
		#endif
	}

	virtual ~ResultCollector() {}

	// An subclass must be able to loadDocTheory() multiple times -- be careful
	// about memory leaks!
	virtual void loadDocTheory(DocTheory* docTheory) = 0;
	
	virtual void produceOutput(const wchar_t *output_dir,
							   const wchar_t* document_filename) = 0;

	// Free any memory assocaited with this result collector.  It must be 
	// possible to continue using the ResultCollector after finalize() has
	// been called.
	virtual void finalize() {}

	virtual void produceOutput(std::wstring *results) {
		throw InternalInconsistencyException(
			"ResultCollector::produceOutput()",
			"You tried to use the in-memory output version of a result "
			"collector that did not implement that method.");
	};
};

#endif
