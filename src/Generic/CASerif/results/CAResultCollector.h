// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CA_RESULT_COLLECTOR_H
#define CA_RESULT_COLLECTOR_H

#include "Generic/results/ResultCollector.h"

class CorrectDocument;
class DocTheory;


/** This is actually part of the interface to the DocumentDriver. It is an
  * abstract class which DocumentDriver expects an instance of. That object
  * is called upon by the DocumentDriver to produce output during the "output"
  * stage.
  *
  * The regular Serif executable uses the APFResultCollector subclass, to
  * generate APF output.
  */

class CAResultCollector : public ResultCollector {
public:
	CAResultCollector() {}
	virtual ~CAResultCollector() {}

	// An subclass must be able to loadDocTheory() multiple times -- be careful
	// about memory leaks!
	virtual void loadDocTheory(DocTheory* docTheory,
		CorrectDocument *cd) = 0;
};

#endif
