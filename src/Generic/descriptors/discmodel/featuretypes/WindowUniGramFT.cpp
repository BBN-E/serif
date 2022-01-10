// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/descriptors/discmodel/featuretypes/WindowUniGramFT.h"

bool WindowUniGramFT::_initialized = false;
SymbolHash* WindowUniGramFT::_stopWords = 0;
