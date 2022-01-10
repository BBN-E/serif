// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_SPECIFIC_RELATION_TREE_MODELS_H
#define EN_SPECIFIC_RELATION_TREE_MODELS_H

// unifier
#include "Generic/relations/RelationStats.h"

// filters
#include "English/relations/en_AttachmentFilter.h"
#include "English/relations/en_NodeFilter.h"
#include "English/relations/en_PredicateFilter.h"
#include "English/relations/en_ConstructionFilter.h"
#include "English/relations/en_PriorFilter.h"

// probability models
#include "Generic/relations/BackoffProbModel.h"
#include "Generic/relations/BackoffToPriorProbModel.h"

// This file exists as a central repository for typedef'd
// models created out of Stats by the combination of a Filter
// and a probability model

typedef RelationStatsTemplate < BackoffProbModel < 2 >, AttachmentFilter > type_attachment_model_t;
typedef RelationStatsTemplate < BackoffProbModel < 3 >, NodeFilter > type_node_model_t;
typedef RelationStatsTemplate < BackoffToPriorProbModel < 1 >, PredicateFilter > type_predicate_model_t;
typedef RelationStatsTemplate < BackoffProbModel < 1 >, ConstructionFilter > type_construction_model_t;
typedef RelationStatsTemplate < BackoffToPriorProbModel < 0 >, PriorFilter > type_prior_model_t;

#endif
