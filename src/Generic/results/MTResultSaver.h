// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MT_RESULT_COLLECTOR_H
#define MT_RESULT_COLLECTOR_H


#include "Generic/common/ParamReader.h"
#include "Generic/common/Segment.h"

class SentenceTheoryBeam;
class DocTheory;

/*
This class is similar to MTResultCollector, but is redesigned to be called from the SentenceDriver, so as to have access to n-best parses.
*/

class MTResultSaver {
public:
	void resetForNewDocument(DocTheory* docTheory, const wchar_t *output_dir);
	void cleanUpAfterDocument();
	void produceSentenceOutput(int cur_sent_no, SentenceTheoryBeam* sent_theory_beam);

private:
	DocTheory* _docTheory;	
	UTF8OutputStream* _dout;
	std::vector< WSegment > _segments;
	std::wstring _ref_source;
};

#endif
