// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/edt/discmodel/featuretypes/WorldKnowledgeFT.h"


bool WorldKnowledgeFT::_use_gpe_world_knowledge_feature = false;
Symbol WorldKnowledgeFT::GPE = Symbol(L"GPE");
Symbol WorldKnowledgeFT::ORG = Symbol(L"ORG");
Symbol WorldKnowledgeFT::PER = Symbol(L"PER");

WorldKnowledgeFT::WKMap WorldKnowledgeFT::_wkMap; 
