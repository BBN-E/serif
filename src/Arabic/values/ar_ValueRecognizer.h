// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_VALUE_RECOGNIZER_H
#define ar_VALUE_RECOGNIZER_H

#include "Generic/values/ValueRecognizer.h"

class ValueMentionSet;
class DocTheory;
class PIdFModel;
class PIdFSentence;
class DTTagSet;


class ArabicValueRecognizer : public ValueRecognizer {
private:
	friend class ArabicValueRecognizerFactory;

public:
	~ArabicValueRecognizer();

	virtual void resetForNewSentence() {};
	virtual void resetForNewDocument(DocTheory *docTheory = 0);

	// This puts an array of pointers to ValueMentionSets
	// where specified by results arg, and returns its size. It returns
	// 0 if something goes wrong. The client is responsible for deleting
	// the array and the ValueMentionSets.
	virtual int getValueTheories(ValueMentionSet **results, int max_theories,
						        TokenSequence *tokenSequence);
	
private:
	ArabicValueRecognizer();

	ValueRecognizer::SpanList getPIdFValues(TokenSequence *tokenSequence);
	ValueMentionSet *makeValueMentionSet(PIdFSentence &sentence);

	bool DO_VALUES;
	
	// which implementation to use
	enum { RULE_VALUE_FINDER, PIDF_VALUE_FINDER } _value_finder;

	PIdFModel *_pidfDecoder;
	DocTheory *_docTheory;
};


class ArabicValueRecognizerFactory: public ValueRecognizer::Factory {
	virtual ValueRecognizer *build() { return _new ArabicValueRecognizer(); } 
};
 
#endif
