// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_WORD_FEATURES_H
#define es_WORD_FEATURES_H

#include <iostream>
#include "Generic/parse/WordFeatures.h"

class SpanishWordFeatures : public WordFeatures{
 private:
	friend class SpanishWordFeaturesFactory;
 
 public:
	Symbol features(Symbol wordSym, bool first_word) const;
	Symbol reducedFeatures(Symbol wordSym, bool first_word) const;

 private:
	SpanishWordFeatures() {}
};

class SpanishWordFeaturesFactory: public WordFeatures::Factory {
	virtual WordFeatures *build() { return _new SpanishWordFeatures(); }
};

#endif
