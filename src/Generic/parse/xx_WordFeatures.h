// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_WORD_FEATURES_H
#define xx_WORD_FEATURES_H

#include <iostream>
#include "Generic/parse/WordFeatures.h"

class GenericWordFeatures : public WordFeatures{
private:
	friend class GenericWordFeaturesFactory;
 
	static void defaultMsg() {
		std::cerr<<"<<<<<<<<<WARNING: Using unimplemented parse/GenericWordFeatures!>>>>>\n";
	};

public:
	Symbol features(Symbol wordSym, bool first_word) const{
		defaultMsg();
		return Symbol();
	}
	Symbol reducedFeatures(Symbol wordSym, bool first_word) const{
		defaultMsg();
		return Symbol();
	}

private:
	GenericWordFeatures(){
		defaultMsg();
	}
};

class GenericWordFeaturesFactory: public WordFeatures::Factory {
	virtual WordFeatures *build() { return _new GenericWordFeatures(); }
};

#endif
