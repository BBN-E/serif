// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_POS_RECOGNIZER_H
#define ar_POS_RECOGNIZER_H

#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/partOfSpeech/PartOfSpeechRecognizer.h"

class ArabicPartOfSpeechRecognizer: public PartOfSpeechRecognizer {
private:
	friend class ArabicPartOfSpeechRecognizerFactory;

public:
	virtual void resetForNewSentence();
	virtual int getPartOfSpeechTheories(PartOfSpeechSequence **results, int max_theories,
								TokenSequence *tokenSequence);
private:
	ArabicPartOfSpeechRecognizer();
	void getPOSFromLE(wchar_t* pos_buffer, LexicalEntry* le, int max_len);




};

class ArabicPartOfSpeechRecognizerFactory: public PartOfSpeechRecognizer::Factory {
	virtual PartOfSpeechRecognizer *build() { return _new ArabicPartOfSpeechRecognizer(); }
};

#endif
