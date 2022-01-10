// Copyright (c) 2013 by Raytheon BBN Technologies             
// All Rights Reserved.       
#ifndef UR_POS_RECOGNIZER_H
#define UR_POS_RECOGNIZER_H

#include "Generic/partOfSpeech/DefaultPartOfSpeechRecognizer.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PartOfSpeechSequence.h"

class DocTheory;
class PIdFModel;
class PIdFSentence;
class SymbolHash;
class PIdFSentence;
class DTTagSet;
class Sentence;

class UrduPartOfSpeechRecognizer: public DefaultPartOfSpeechRecognizer {

public:
	UrduPartOfSpeechRecognizer();
	~UrduPartOfSpeechRecognizer();


	void resetForNewDocument(DocTheory *docTheory = 0);
	int getPartOfSpeechTheories(PartOfSpeechSequence **results, int max_theories,
								TokenSequence *tokenSequence);
private:
	bool _debug_flag;
	int _sent_no;
	const TokenSequence* _tokenSequence;
	PIdFModel *_pidfDecoder;
	DTTagSet *_tagSet;
	DocTheory *_docTheory;

};

class UrduPartOfSpeechRecognizerFactory: public PartOfSpeechRecognizer::Factory {
	virtual PartOfSpeechRecognizer *build() { return _new UrduPartOfSpeechRecognizer(); }
};

#endif
