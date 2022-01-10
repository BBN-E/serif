// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_IDF_VALUE_RECOGNIZER_H
#define en_IDF_VALUE_RECOGNIZER_H

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/theories/ValueType.h"

class ValueMentionSet;
class IdFSentenceTokens;
class IdFSentenceTheory;
class IdFDecoder;
class DocTheory;
class NameClassTags;
class Symbol;
class NameSpan;
class SymbolHash;
class IdFListSet;


class IdFValueRecognizer {
public:
	const static int MAX_VALUES = 100;
	const static int MAX_DOC_VALUES = 2000;
	const static int MAX_SENTENCE_LENGTH = 2000;

	IdFValueRecognizer(const char *value_class_file,
					   const char *model_file_prefix, 
					   const char *lc_model_file_prefix = 0);
	~IdFValueRecognizer();
	void resetForNewSentence();

	void cleanUpAfterDocument();
	void resetForNewDocument(DocTheory *docTheory = 0);


	virtual int getValueTheories(IdFSentenceTheory **results, int max_theories,
						        TokenSequence *tokenSequence);
	
	NameClassTags *getTagSet() { return _valueClassTags; }
	void setWordFeaturesMode(int mode) { _wordFeatures->setMode(mode); }

private:
	UTF8OutputStream _debugStream;
	bool DEBUG;
	IdFDecoder *_decoder;
	IdFDecoder *_defaultDecoder;
	IdFDecoder *_lowerCaseDecoder;
	IdFSentenceTokens *_sentenceTokens;
	NameClassTags *_valueClassTags;
	IdFWordFeatures *_wordFeatures;

	// names, but pre-coagulation
	struct ValueSet {
		int num_words;
		Symbol* words;
	};

	// for storing every name seen so far in the document
	Symbol _values[MAX_DOC_VALUES];
	ValueType _types[MAX_DOC_VALUES];
	ValueSet _valueWords[MAX_DOC_VALUES];
	int _num_values;
};

#endif
