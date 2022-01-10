// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_NAME_RECOGNIZER_H
#define ar_NAME_RECOGNIZER_H

#include "Generic/names/NameRecognizer.h"


class NameTheory;
class DocTheory;
class ArabicIdFNameRecognizer;
class PIdFModel;
class SymbolHash;


class ArabicNameRecognizer : public NameRecognizer {
private:
	friend class ArabicNameRecognizerFactory;

public:
	virtual void resetForNewSentence(const Sentence *sentence);

	virtual void cleanUpAfterDocument();
	virtual void resetForNewDocument(DocTheory *docTheory = 0);

	// This puts an array of pointers to NameTheorys
	// where specified by results arg, and returns its size. It returns
	// 0 if something goes wrong. The client is responsible for deleting
	// the array and the NameTheorys.
	virtual int getNameTheories(NameTheory **results, int max_theories,
						        TokenSequence *tokenSequence);
	int getNameTheoriesForSentenceBreaking(NameTheory **results, int max_theories, 
									TokenSequence *tokenSequence);



private:
	ArabicNameRecognizer();

	// which implementation to use
	enum { IDF_NAME_FINDER, PIDF_NAME_FINDER } _name_finder;

	ArabicIdFNameRecognizer *_idfNameRecognizer;
	PIdFModel *_pidfDecoder;
	DocTheory *_docTheory;
	int _sent_no;
	SymbolHash* _knownNames;
	bool PRINT_SENTENCE_SELECTION_INFO;
	/*
	//This function has 2 parts, 
	//Pt1 removes clitics and punctuation that have been mistakenly
	//attached to a name.  This prevents the name finder from finding w washington as name, it 
	//does however over remove certain words from names- for instance, Af b (AFP), loses the b.  
	//
	//Pt2 is an ACE2005 specific hack.  Certain "descriptor" words are included in the ACE
	//definition of names (eg. city of Bagdad is a name).  In cases where one of these words is
	//adjacent to a name of the correct type, I add the word to the name.  Pt 2 also fixes the
	//Af b problem mentioned above (only Af p, not all cases of this problem
	*/
	void cleanNameTheories(NameTheory **results, int ntheories, 
									TokenSequence *tokenSequence);
};

class ArabicNameRecognizerFactory: public NameRecognizer::Factory {
	virtual NameRecognizer *build() { return _new ArabicNameRecognizer(); }
};


#endif
