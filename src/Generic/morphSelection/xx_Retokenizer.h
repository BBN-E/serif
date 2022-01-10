// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_RETOKENIZER_H
#define XX_RETOKENIZER_H

#include "Generic/morphSelection/Retokenizer.h"

class GenericRetokenizer : public Retokenizer {
public:
	void Retokenize(TokenSequence* origTS, Token** newTokens, int n_new) {};
	void RetokenizeForNames(TokenSequence *origTS, Token** newTokens, int n_new) {};
	void reset() {};
protected:
	bool getInstanceIntegrityCheck() {return true;}
};

#endif
