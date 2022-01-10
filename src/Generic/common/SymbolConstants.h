// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SYMBOL_CONSTANTS_H
#define SYMBOL_CONSTANTS_H

#include "Generic/common/Symbol.h"

// place static symbols here that may be used by other classes in the common area.
// module-specific constants should be placed in appropriate classes in the area of 
// the module.

// NOTE: It is *NOT* ok to initialize static symbol variables by copying
// these values.  Static variables that come from different translation
// units are initialized in an undefined order, and it's quite possible
// that these will not yet be initialized when you attempt to copy them.

// It would help if you noted where these symbols are to be originally used when you add them
class SymbolConstants {
public:

	// parens are for the NgramScoreTable
	static Symbol leftParen;
	static Symbol rightParen;

	// not thrilled about this location. placeholder for null history for en_DescriptorClassifier,
	// also for other null elements in en_DescriptorClassifier
	static Symbol nullSymbol;

	// symbol used in pronoun linking to denote antecedents unreachable by the hobbs searcher
	static Symbol unreachableSymbol;

};



#endif
