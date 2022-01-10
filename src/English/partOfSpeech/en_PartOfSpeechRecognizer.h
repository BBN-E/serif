// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_POS_RECOGNIZER_H
#define en_POS_RECOGNIZER_H

#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/partOfSpeech/PartOfSpeechRecognizer.h"
#include "English/parse/en_WordFeatures.h"
#include <string>
class PartOfSpeechTable;

class EnglishPartOfSpeechRecognizer: public PartOfSpeechRecognizer {
private:
	friend class EnglishPartOfSpeechRecognizerFactory;

public:
	virtual ~EnglishPartOfSpeechRecognizer();
	virtual void resetForNewSentence() {};
	virtual int getPartOfSpeechTheories(PartOfSpeechSequence **results, int max_theories,
		TokenSequence *tokenSequence);
	
private:
	EnglishPartOfSpeechRecognizer();
	PartOfSpeechRecognizer *_partOfSpeechRecognizer;
	
	std::string _modelType;
	PartOfSpeechTable *_posTable;
	WordFeatures* _wordFeatures;
	
	
};

class EnglishPartOfSpeechRecognizerFactory: public PartOfSpeechRecognizer::Factory {
	virtual PartOfSpeechRecognizer *build() { return _new EnglishPartOfSpeechRecognizer(); }
};


#endif
