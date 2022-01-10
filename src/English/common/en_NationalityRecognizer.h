// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_NATIONALITYRECOGNIZER_H
#define EN_NATIONALITYRECOGNIZER_H

#include "Generic/common/NationalityRecognizer.h"

class SynNode;

class EnglishNationalityRecognizer : public NationalityRecognizer {
private:
	friend class EnglishNationalityRecognizerFactory;

public:
	static bool isNamePersonDescriptor(const SynNode *node);
	static bool isRegionWord(Symbol word);
};

class EnglishNationalityRecognizerFactory: public NationalityRecognizer::Factory {
	virtual bool isNamePersonDescriptor(const SynNode *node) {  return EnglishNationalityRecognizer::isNamePersonDescriptor(node); }
};


#endif
