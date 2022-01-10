// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/parse/es_WordFeatures.h"

namespace {
	Symbol DEFAULT_FEATURE(L"x");
}


Symbol SpanishWordFeatures::features(Symbol wordSym, bool first_word) const {
	return DEFAULT_FEATURE;
}
Symbol SpanishWordFeatures::reducedFeatures(Symbol wordSym, bool first_word) const {
	return DEFAULT_FEATURE;
}
