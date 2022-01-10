// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_VALUE_RECOGNIZER_H
#define en_VALUE_RECOGNIZER_H

#include "Generic/names/discmodel/PIdFSentence.h"
#include "Generic/values/ValueRecognizer.h"


class ValueMention;
class ValueMentionSet;
class IdFValueRecognizer;
class DocTheory;
class PIdFModel;
class Sexp;
class SymbolHash;
class PatternValueFinder;

class EnglishValueRecognizer : public ValueRecognizer {
private:
	friend class EnglishValueRecognizerFactory;

public:
	~EnglishValueRecognizer();
	virtual void resetForNewSentence() {}
	virtual void resetForNewDocument(DocTheory *docTheory = 0);

	// This puts an array of pointers to ValueMentionSets
	// where specified by results arg, and returns its size. It returns
	// 0 if something goes wrong. The client is responsible for deleting
	// the array and the ValueMentionSets.
	virtual int getValueTheories(ValueMentionSet **results, int max_theories,
						        TokenSequence *tokenSequence);
	
private:
	EnglishValueRecognizer();

	SpanList getIdFValues(TokenSequence *tokenSequence);
	SpanList getPIdFValues(TokenSequence *tokenSequence);

	bool DO_VALUES;
	int _sent_no;

	void correctTimexSentence(PIdFSentence &timexSentence);
	void forceTimex(PIdFSentence &timexSentence, int start, int end);
	bool isTimexNone(PIdFSentence &timexSentence, int i);

	DocTheory *_docTheory;

	// which implementation to use
	enum { IDF_VALUE_FINDER, PIDF_VALUE_FINDER } _value_finder;

	static const int N_DECODERS = 3;
	PIdFModel *_valuesDecoders[N_DECODERS];
	IdFValueRecognizer *_idfValuesDecoders[N_DECODERS];

	int TIMEX_DECODER_INDEX; 

	SymbolHash *_timexWords;
	static Symbol TIME_TAG;
	static Symbol TIME_TAG_ST;

	PatternValueFinder *_patternValueFinder;

};


class EnglishValueRecognizerFactory: public ValueRecognizer::Factory {
	virtual ValueRecognizer *build() { return _new EnglishValueRecognizer(); } 
};
 

#endif
