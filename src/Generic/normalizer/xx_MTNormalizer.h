// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_MT_NORMALIZER_H
#define XX_MT_NORMALIZER_H

#include "Generic/normalizer/MTNormalizer.h"

class GenericMTNormalizer : public MTNormalizer {
private:
	friend class GenericMTNormalizerFactory;

public:
	~GenericMTNormalizer() {};

	virtual void normalize(TokenSequence* ts) {}

private:
	GenericMTNormalizer() {}

};

class GenericMTNormalizerFactory: public MTNormalizer::Factory {
	virtual MTNormalizer *build() { return _new GenericMTNormalizer(); }
};



#endif
