// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P_POS_RECOGNIZER_H
#define P_POS_RECOGNIZER_H

#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/partOfSpeech/PartOfSpeechRecognizer.h"
#include "Generic/partOfSpeech/discmodel/PPartOfSpeechSentence.h"
#include "Generic/partOfSpeech/discmodel/PPartOfSpeechModel.h"

//class PPartOfSpeechModel;
class DTTagSet;

class PPartOfSpeechRecognizer: public PartOfSpeechRecognizer {
public:
	PPartOfSpeechRecognizer();
	~PPartOfSpeechRecognizer();
	virtual void resetForNewSentence();
	virtual int getPartOfSpeechTheories(PartOfSpeechSequence **results, int max_theories,
								TokenSequence *tokenSequence);
private:
	PartOfSpeechSequence *makePartOfSpeechSequence(const TokenSequence& tokenSequence, const PPartOfSpeechSentence& sentence);

	DTTagSet *_tagSet;
	PPartOfSpeechModel *_partOfSpeechModel;
};

#endif
