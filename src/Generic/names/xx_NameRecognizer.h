// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_NAMERECOGNIZER_H
#define xx_NAMERECOGNIZER_H

#include "Generic/names/DefaultNameRecognizer.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/TokenSequence.h"

class GenericNameRecognizer: public NameRecognizer {
private:
	friend class GenericNameRecognizerFactory;

public:
	//delegate all functions to the DefaultNameRecognizer
	virtual ~GenericNameRecognizer() {}
	
	virtual void resetForNewSentence(const Sentence *sentence)
	{
		return _nameRecognizer.resetForNewSentence(sentence);
	}

	virtual void cleanUpAfterDocument() 
	{
		return _nameRecognizer.cleanUpAfterDocument();
	}

	virtual void resetForNewDocument(class DocTheory *docTheory)
	{
		return _nameRecognizer.resetForNewDocument(docTheory);
	}

	virtual int getNameTheories(NameTheory **results, int max_theories, 
								TokenSequence *tokenSequence)
	{
		return _nameRecognizer.getNameTheories(results, max_theories, tokenSequence);
	}

protected:
	GenericNameRecognizer() : _nameRecognizer() {}

private:
	DefaultNameRecognizer _nameRecognizer;
	
};

class GenericNameRecognizerFactory: public NameRecognizer::Factory {
	virtual NameRecognizer *build() { return _new GenericNameRecognizer(); }
};

#endif
