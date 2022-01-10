// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/common/Symbol.h"

Symbol P1RelationFeatureType::modeltype = Symbol(L"P1Relation");

Symbol P1RelationFeatureType::AT_LEAST_ONE = Symbol(L">=1");
Symbol P1RelationFeatureType::AT_LEAST_TEN = Symbol(L">=10");
Symbol P1RelationFeatureType::AT_LEAST_ONE_HUNDRED = Symbol(L">=100");
Symbol P1RelationFeatureType::AT_LEAST_ONE_THOUSAND = Symbol(L">=1000");

Symbol P1RelationFeatureType::AT_LEAST_25_PERCENT = Symbol(L">=25%");
Symbol P1RelationFeatureType::AT_LEAST_50_PERCENT = Symbol(L">=50%");
Symbol P1RelationFeatureType::AT_LEAST_75_PERCENT = Symbol(L">=75%");

Symbol P1RelationFeatureType::MOST_FREQ_TYPE_MATCH = (Symbol(L"MOST_FREQ_MTCH"));
