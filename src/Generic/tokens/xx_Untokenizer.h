// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_UNTOKENIZER_H
#define xx_UNTOKENIZER_H

#include "Generic/tokens/Untokenizer.h"


class GenericUntokenizer : public Untokenizer {
private:
	friend class GenericUntokenizerFactory;

public:	
	void untokenize(LocatedString& source) const { };

private:
	GenericUntokenizer() {};

};

class GenericUntokenizerFactory: public Untokenizer::Factory {
	virtual Untokenizer *build() { return _new GenericUntokenizer(); }
};


#endif
