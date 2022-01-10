//Copyright 2011 BBN Technologies
//All rights reserved

#ifndef XX_PATTERN_EVENT_VALUE_RECOGNIZER_H
#define XX_PATTERN_EVENT_VALUE_RECOGNIZER_H

#include "Generic/values/PatternEventValueRecognizer.h"

class DefaultPatternEventValueRecognizer : public PatternEventValueRecognizer {
private:
	friend class DefaultPatternEventValueRecognizerFactory;

public:

	// This method should eventually build a document level mention set based
	// on string-fill event args and add it to the docTheory.
	// (docTheory->takeDocumentValueMentionSet(valueMentionSet))
	virtual void createEventValues(DocTheory* docTheory) {};

private:
	DefaultPatternEventValueRecognizer() {}

};


// RelationFinder factory
class DefaultPatternEventValueRecognizerFactory: public PatternEventValueRecognizer::Factory {
	virtual PatternEventValueRecognizer *build() { return _new DefaultPatternEventValueRecognizer(); } 
};
 
#endif
