// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_MT_NORMALIZER_H
#define AR_MT_NORMALIZER_H

#include "Generic/normalizer/MTNormalizer.h"
#include "Generic/common/limits.h"
#include "Generic/theories/Token.h"

class TokenSequence;

class ArabicMTNormalizer : public MTNormalizer {
private:
	friend class ArabicMTNormalizerFactory;

public:
	void normalize(TokenSequence* ts);

private:
	Token* _tokens[MAX_SENTENCE_TOKENS];

	ArabicMTNormalizer() {}
	void normalizeLocatedString(LocatedString *locatedString, bool token_is_originally_end_of_word);
};

class ArabicMTNormalizerFactory: public MTNormalizer::Factory {
	virtual MTNormalizer *build() { return _new ArabicMTNormalizer(); }
};

#endif
