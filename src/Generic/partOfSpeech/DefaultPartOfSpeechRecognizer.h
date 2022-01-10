// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DEFAULT_POS_RECOGNIZER_H
#define DEFAULT_POS_RECOGNIZER_H

#include "Generic/partOfSpeech/PartOfSpeechRecognizer.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PartOfSpeechSequence.h"

class DefaultPartOfSpeechRecognizer: public PartOfSpeechRecognizer {
public:
	DefaultPartOfSpeechRecognizer() {};
	virtual ~DefaultPartOfSpeechRecognizer() {};

	virtual void resetForNewSentence(){};
	virtual int getPartOfSpeechTheories(PartOfSpeechSequence **results, int max_theories,
								TokenSequence *tokenSequence)
	{
		results[0] = _new PartOfSpeechSequence(tokenSequence);
                for (int num = 0; num < results[0]->getNTokens(); ++num) {
                  results[0]->addPOS(Symbol(L"DummyPOS"), static_cast<float>(1.0), num);
                }

		return 1;
	}
};

#endif
