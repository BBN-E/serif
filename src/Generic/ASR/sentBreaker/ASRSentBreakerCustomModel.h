// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ASR_SENT_BREAKER_CUSTOM_MODEL_H
#define ASR_SENT_BREAKER_CUSTOM_MODEL_H


#include "Generic/common/Symbol.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/ASR/sentBreaker/ASRSentModelFVecFilter.h"

class UTF8InputStream;
class ASRSentModelInstance;


class ASRSentBreakerCustomModel {
public:
	ASRSentBreakerCustomModel(const char *model_prefix);

	double getProbability(ASRSentModelInstance *instance);

private:
	ASRSentModelFVecFilter _filter;

	enum { BACKOFF, WEIGHTED } _formula;

	NgramScoreTable *_wordModel;
	NgramScoreTable *_prevWordModel;
	NgramScoreTable *_bigramModel;
	NgramScoreTable *_trigramModel;
};

#endif
