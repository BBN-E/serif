// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_NATIONALITYRECOGNIZER_H
#define AR_NATIONALITYRECOGNIZER_H

#include "Generic/common/NationalityRecognizer.h"


class SynNode;

class ArabicNationalityRecognizer : public NationalityRecognizer {
private:
	friend class ArabicNationalityRecognizerFactory;

public:
	static bool isNamePersonDescriptor(const SynNode *node);
	static bool _allowLikelyNationalities;
	static bool _ar_initialized;
	static void arInitialize();
	static bool isPossibleNationalitySymbol(Symbol hw);
};

class ArabicNationalityRecognizerFactory: public NationalityRecognizer::Factory {
	virtual bool isNamePersonDescriptor(const SynNode *node) {  return ArabicNationalityRecognizer::isNamePersonDescriptor(node); }
};


#endif
