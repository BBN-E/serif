// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_VALUE_RECOGNIZER_H
#define xx_VALUE_RECOGNIZER_H

#include "Generic/values/DefaultValueRecognizer.h"
#include "Generic/values/ValueRecognizer.h"

// Note: there's already a DefaultValueRecognizer; it probably should be 
// combined with this class

class XXValueRecognizer: public ValueRecognizer {
private:
	friend class XXValueRecognizerFactory;

private:
	
	DefaultValueRecognizer _valueRecognizer;

public:
	virtual void resetForNewSentence() 
	{
		return _valueRecognizer.resetForNewSentence();
	}
	
	virtual void resetForNewDocument(DocTheory *docTheory = 0)
	{
		return _valueRecognizer.resetForNewDocument(docTheory);
	}

	virtual int getValueTheories(ValueMentionSet **results, int max_theories,
								TokenSequence *tokenSequence)
	{
		//results[0] = _new ValueMentionSet(0); // RPB: I am unclear why the generic version of this would ever be called in the first place.
		//return 1;
		return _valueRecognizer.getValueTheories(results, max_theories, tokenSequence);
	}

private:
	XXValueRecognizer() : _valueRecognizer() {}

};


class XXValueRecognizerFactory: public ValueRecognizer::Factory {
	virtual ValueRecognizer *build() { return _new XXValueRecognizer(); } 
};


#endif
