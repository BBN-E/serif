// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_TEMPORAL_NORMALIZER_H
#define XX_TEMPORAL_NORMALIZER_H

#include "Generic/values/TemporalNormalizer.h"

class GenericTemporalNormalizer : public TemporalNormalizer {
private:
	friend class GenericTemporalNormalizerFactory;

public:
	
	virtual void normalizeTimexValues(DocTheory* docTheory) {};
	virtual void normalizeDocumentDate(const DocTheory *docTheory, std::wstring &docDateTime, std::wstring &additionalDocTime) {};

private:
	GenericTemporalNormalizer() {}

};

class GenericTemporalNormalizerFactory: public TemporalNormalizer::Factory {
	virtual TemporalNormalizer *build() { return _new GenericTemporalNormalizer(); }
};


#endif
