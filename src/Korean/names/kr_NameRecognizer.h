// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef kr_NAME_RECOGNIZER_H
#define kr_NAME_RECOGNIZER_H

#include "Generic/names/NameRecognizer.h"

class NameTheory;
class DocTheory;
class PIdFModel;
class PIdFSentence;
class DTTagSet;

class KoreanNameRecognizer : public NameRecognizer {
private:
	friend class KoreanNameRecognizerFactory;

public:

	virtual ~KoreanNameRecognizer() = 0;
	virtual void resetForNewSentence(Sentence *sentence) {};

	virtual void cleanUpAfterDocument() {};
	virtual void resetForNewDocument(DocTheory *docTheory = 0);

	// This puts an array of pointers to NameTheorys
	// where specified by results arg, and returns its size. It returns
	// 0 if something goes wrong. The client is responsible for deleting
	// the array and the NameTheorys.
	virtual int getNameTheories(NameTheory **results, int max_theories,
						        TokenSequence *tokenSequence);
	
private:
	KoreanNameRecognizer();

	NameTheory *makeNameTheory(PIdFSentence &sentence);
	void fixName(NameSpan *span, PIdFSentence *sent);

	static Symbol _NONE_ST;
	static Symbol _NONE_CO;

	PIdFModel *_pidfDecoder;
	DTTagSet *_tagSet;
	DocTheory *_docTheory;

	int _sent_no;
};

class KoreanNameRecognizerFactory: public NameRecognizer::Factory {
	virtual NameRecognizer *build() { return _new KoreanNameRecognizer(); }
};


#endif

