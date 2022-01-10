// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_TRIGGER_SENTENCE_H
#define EVENT_TRIGGER_SENTENCE_H

#include "common/Symbol.h"
#include "common/limits.h"
#include "theories/TokenSequence.h"
#include "theories/Token.h"

class UTF8InputStream;
class UTF8OutputStream;
class DTTagSet;


class EventTriggerSentence {

public:
	EventTriggerSentence(const DTTagSet *tagSet);

	~EventTriggerSentence() {
		delete _tseq;
		delete[] _tags;
	}

	int getLength() { return _length; }
	TokenSequence *getTokenSequence();
	int getTag(int i);

	bool readTrainingSentence(int sentno, UTF8InputStream &in);
	bool readDecodeSentence(int sentno, UTF8InputStream &in);

private:
	static Token *_tokenBuffer[MAX_SENTENCE_TOKENS+1];
	const DTTagSet *_tagSet;
	int _length;
	TokenSequence *_tseq;
	int *_tags;
};

#endif
