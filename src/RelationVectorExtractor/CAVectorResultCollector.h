// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CA_VECTOR_RESULT_COLLECTOR_H
#define CA_VECTOR_RESULT_COLLECTOR_H

#include "CASerif_generic/results/CAResultCollector.h"
#include "relations/PotentialRelationCollector.h"
#include "theories/RelMention.h"
#include "common/UTF8OutputStream.h"
#include "common/NGramScoreTable.h"


class CAVectorResultCollector : public CAResultCollector {
public:
	CAVectorResultCollector();

	~CAVectorResultCollector() {}

	virtual void loadDocTheory(DocTheory *docTheory, CorrectDocument *cd);
	virtual void produceOutput(const char *output_dir, const char *document_filename);

	void printVectors();

private:

	void alignAndPrintVectors(UTF8OutputStream &vectorStream);
	void alignAndCollectVectors(); 

	bool exactMentionMatch(RelMention *relMent, int one, int two, MentionSet *mentionSet); 
	void adjustHeadWords(int left, int right, PotentialRelationInstance *inst, TokenSequence *tokenSequence);

	DocTheory *_docTheory;
	CorrectDocument *_correctDocument;

	PotentialRelationCollector _relationCollector;
	NgramScoreTable _vectors;

};

#endif
