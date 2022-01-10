// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_NATIONALITYRECOGNIZER_H
#define XX_NATIONALITYRECOGNIZER_H

#include "Generic/common/NationalityRecognizer.h"

class SynNode;

class GenericNationalityRecognizer : public NationalityRecognizer {
private:
	friend class GenericNationalityRecognizerFactory;

public:
	static bool isNamePersonDescriptor(const SynNode *node){return false;};
};

class GenericNationalityRecognizerFactory: public NationalityRecognizer::Factory {
	virtual bool isNamePersonDescriptor(const SynNode *node) {  return GenericNationalityRecognizer::isNamePersonDescriptor(node); }
};


#endif
