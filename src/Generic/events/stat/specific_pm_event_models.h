// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SPECIFIC_EVENT_MODELS_H
#define SPECIFIC_EVENT_MODELS_H

// unifier
#include "Generic/events/stat/EventPMStats.h"

#include "Generic/events/stat/ETWordVectorFilter.h"
#include "Generic/events/stat/ETObjectArgVectorFilter.h"
#include "Generic/events/stat/ETSubjectArgVectorFilter.h"
#include "Generic/events/stat/ETWordNetVectorFilter.h"
#include "Generic/events/stat/ETCluster16VectorFilter.h"
#include "Generic/events/stat/ETCluster1216VectorFilter.h"
#include "Generic/events/stat/ETCluster81216VectorFilter.h"

// probability models
#include "Generic/relations/BackoffProbModel.h"
#include "Generic/relations/BackoffToPriorProbModel.h"

// This file exists as a central repository for typedef'd
// models created out of Stats by the combination of a Filter
// and a probability model

typedef EventPMStats<BackoffProbModel<2>,ETWordVectorFilter> type_et_word_model_t;
typedef EventPMStats<BackoffToPriorProbModel<2>,ETWordVectorFilter> type_et_word_prior_model_t;
typedef EventPMStats<BackoffProbModel<2>,ETObjectArgVectorFilter> type_et_obj_model_t;
typedef EventPMStats<BackoffProbModel<2>,ETSubjectArgVectorFilter> type_et_sub_model_t;
typedef EventPMStats<BackoffProbModel<3>,ETWordNetVectorFilter> type_et_wordnet_model_t;
typedef EventPMStats<BackoffProbModel<4>,ETCluster81216VectorFilter> type_et_cluster81216_model_t;
typedef EventPMStats<BackoffProbModel<3>,ETCluster1216VectorFilter> type_et_cluster1216_model_t;
typedef EventPMStats<BackoffProbModel<2>,ETCluster16VectorFilter> type_et_cluster16_model_t;

#endif
