// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_POS_RECOGNIZER_H
#define xx_POS_RECOGNIZER_H

#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/partOfSpeech/PartOfSpeechRecognizer.h"
#include "Generic/common/ParamReader.h"


class GenericPartOfSpeechRecognizer: public PartOfSpeechRecognizer {
private:
	friend class GenericPartOfSpeechRecognizerFactory;

	static void defaultMsg(){
		SessionLogger::warn("unimplemented_class")<<"<<<<<<<<<WARNING: Using unimplemented Part Of Speech Recognizer!>>>>>\n";
	}

	GenericPartOfSpeechRecognizer() {
		//if (!ParamReader::isParamTrue("suppress_warnings")) defaultMsg();
	}

public:
	virtual ~GenericPartOfSpeechRecognizer() {};

	virtual void resetForNewSentence(){}
	virtual int getPartOfSpeechTheories(PartOfSpeechSequence **results, int max_theories,
								TokenSequence *tokenSequence)
	{
		results[0] = _new PartOfSpeechSequence(tokenSequence);
		for (int i = 0; i < tokenSequence->getNTokens(); i++)
			results[0]->addPOS(Symbol(L"DummyPOS"), 1, i);
		return 1;
	}


};

class GenericPartOfSpeechRecognizerFactory: public PartOfSpeechRecognizer::Factory {
	virtual PartOfSpeechRecognizer *build() { return _new GenericPartOfSpeechRecognizer(); }
};

#endif
