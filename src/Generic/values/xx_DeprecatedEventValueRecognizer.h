// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_EVENT_VALUE_RECOGNIZER_H
#define XX_EVENT_VALUE_RECOGNIZER_H

#include "Generic/values/DeprecatedEventValueRecognizer.h"

class DefaultDeprecatedEventValueRecognizer : public DeprecatedEventValueRecognizer {

private:
	friend class DefaultDeprecatedEventValueRecognizerFactory;

	DefaultDeprecatedEventValueRecognizer() {}

public:

	// This method should eventually build a document level mention set based
	// on string-fill event args and add it to the docTheory.
	// (docTheory->takeDocumentValueMentionSet(valueMentionSet))
	virtual void createEventValues(DocTheory* docTheory) {};
};


class DefaultDeprecatedEventValueRecognizerFactory: public DeprecatedEventValueRecognizer::Factory {
	virtual DeprecatedEventValueRecognizer *build() { return _new DefaultDeprecatedEventValueRecognizer(); } 
};
 
#endif
