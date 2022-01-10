// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef kr_POS_RECOGNIZER_H
#define kr_POS_RECOGNIZER_H

#include "theories/PartOfSpeechSequence.h"
#include "partOfSpeech/PartOfSpeechRecognizer.h"

class KoreanPartOfSpeechRecognizer: public PartOfSpeechRecognizer {
private:
	friend class KoreanPartOfSpeechRecognizerFactory;

public:
	virtual void resetForNewSentence() {};
	virtual int getPartOfSpeechTheories(PartOfSpeechSequence **results, int max_theories,
								TokenSequence *tokenSequence);
private:
	KoreanPartOfSpeechRecognizer() {};
	void getPOSFromLexicalEntry(wchar_t* pos_buffer, LexicalEntry* le, int max_len);
};

class KoreanPartOfSpeechRecognizerFactory: public PartOfSpeechRecognizer::Factory {
	virtual PartOfSpeechRecognizer *build() { return _new KoreanPartOfSpeechRecognizer(); }
};

#endif
