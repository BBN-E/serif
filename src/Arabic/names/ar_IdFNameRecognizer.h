// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_IDFNAME_RECOGNIZER_H
#define ar_IDFNAME_RECOGNIZER_H
#include "Generic/names/IdFSentenceTokens.h"
#include "Generic/names/IdFDecoder.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/names/NameClassTags.h"
#include "Arabic/names/ar_IdFTokens.h"



const int MAX_NAMES = 100;
const int MAX_DOC_NAMES = 2000;

class ArabicIdFNameRecognizer  {
public:
	ArabicIdFNameRecognizer();
	virtual void resetForNewSentence();

	void cleanUpAfterDocument(){}
	void resetForNewDocument(class DocTheory *docTheory){};

	// This does the work. It puts an array of pointers to NameTheorys
	// where specified by results arg, and returns its size. It returns
	// 0 if something goes wrong. The client is responsible for deleting
	// the array and the NameTheorys.
	virtual int getNameTheories(NameTheory **results, int max_theories,
		TokenSequence *tokenSequence);
	int getNameTheoriesForSentenceBreaking(NameTheory **results, int max_theories, 
									TokenSequence *tokenSequence);

	
private:
	IdFDecoder *_decoder;
	IdFSentenceTokens *_sentenceTokens;
	IdFSentenceTheory **_IdFTheories;
	NameClassTags *_nameClassTags;
	bool DEBUG;
	UTF8OutputStream _debugStream;
	IdFTokens *_idfToks;	//object for keeping track of sub tokens that are 
							//the result of clitic spliting
	bool _separateClitics;

	bool token_smaller_than_serif_token(int i);
};
#endif
