// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ur_WORD_FEATURES_H
#define ur_WORD_FEATURES_H

#include <iostream>
#include "Generic/parse/WordFeatures.h"

class UrduWordFeatures : public WordFeatures{
 private:
	friend class UrduWordFeaturesFactory;
 
 public:
	Symbol features(Symbol wordSym, bool first_word) const;
	Symbol reducedFeatures(Symbol wordSym, bool first_word) const;

 private:
	UrduWordFeatures() {}
};

class UrduWordFeaturesFactory: public WordFeatures::Factory {
	virtual WordFeatures *build() { return _new UrduWordFeatures(); }
};

#endif
