// Copyright 2011 BBN Technologies
// All rights reserved

#ifndef DEFAULT_VALUE_RECOGNIZER
#define DEFAULT_VALUE_RECOGNIZER

#include "Generic/values/ValueRecognizer.h"

class ValueMentionSet;
class DocTheory;
class PIdFModel;
class PIdFSentence;
class DTTagSet;
class PatternValueFinder;


class DefaultValueRecognizer : public ValueRecognizer{
	/*
		Implements xx_ValueRecognizer.
		Methods copied from ar_ValueRecognizer which are 
		language-independent.
	*/
public:
	DefaultValueRecognizer();
	~DefaultValueRecognizer();

	void resetForNewSentence();
	void resetForNewDocument(DocTheory* docTheory);
	int getValueTheories(ValueMentionSet **vms, int i, TokenSequence* ts);

private:
	ValueRecognizer::SpanList getPIdFValues(TokenSequence *tokenSequence);
	ValueMentionSet *makeValueMentionSet(PIdFSentence &sentence);
	ValueRecognizer::SpanList combineSpanLists(SpanList resultsPidf, SpanList resultsRule);

	bool DO_VALUES;
	
	// which implementation to use
	enum { RULE_VALUE_FINDER, PIDF_VALUE_FINDER, BOTH } _value_finder;

	PIdFModel *_pidfDecoder;
	DocTheory *_docTheory;
	PatternValueFinder *_patternValueFinder;

};

#endif
