// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Urdu/parse/ur_WordFeatures.h"

namespace {
	Symbol DEFAULT_FEATURE(L"x");
}


Symbol UrduWordFeatures::features(Symbol wordSym, bool first_word) const {
	return DEFAULT_FEATURE;
}
Symbol UrduWordFeatures::reducedFeatures(Symbol wordSym, bool first_word) const {
	return DEFAULT_FEATURE;
}
