// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STAT_SENT_BREAKER_F_VEC_MODEL_H
#define STAT_SENT_BREAKER_F_VEC_MODEL_H

// unifier
#include "Generic/sentences/StatSentBreakerModel.h"

#include "Generic/sentences/StatSentModelFVecFilter.h"

// probability models
#include "Generic/sentences/SentBreakerBackoffProbModel.h"

typedef StatSentBreakerModel<SentBreakerBackoffProbModel<3>, StatSentModelFVecFilter>
	StatSentBreakerFVecModel;

#endif
