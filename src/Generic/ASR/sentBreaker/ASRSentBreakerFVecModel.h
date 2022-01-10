// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ASR_SENT_BREAKER_F_VEC_MODEL_H
#define ASR_SENT_BREAKER_F_VEC_MODEL_H

// unifier
#include "Generic/ASR/sentBreaker/ASRSentBreakerModel.h"

#include "Generic/ASR/sentBreaker/ASRSentModelFVecFilter.h"

// probability models
#include "Generic/relations/BackoffProbModel.h"
#include "Generic/relations/BackoffToPriorProbModel.h"

typedef ASRSentBreakerModel<BackoffProbModel<3>, ASRSentModelFVecFilter>
	ASRSentBreakerFVecModel;

#endif
