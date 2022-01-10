// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/descriptors/discmodel/featuretypes/WindowGramFT.h"

bool WindowGramFT::_initialized = false;
SymbolHash* WindowGramFT::_stopWords = 0;


