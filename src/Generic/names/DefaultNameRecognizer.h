// Copyright 2011 BBN Technologies
// All Rights Reserved

#ifndef DEFAULT_NAMERECOGNIZER_H
#define DEFAULT_NAMERECOGNIZER_H

#include "Generic/names/NameRecognizer.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class NameTheory;
class DocTheory;
class IdFNameRecognizer;
class PIdFModel;
class SymbolHash;
class PIdFSentence;
class DTTagSet;
class Sentence;
class IdFListSet;

class SERIF_EXPORTED DefaultNameRecognizer : public NameRecognizer {

public:
	const static int MAX_DOC_NAMES = 2000;

	DefaultNameRecognizer();
	~DefaultNameRecognizer();

	void resetForNewSentence(const Sentence *sentence);
	void resetForNewDocument(class DocTheory *doctheory);
	void cleanUpAfterDocument();
	int getNameTheories(NameTheory **results, int max_theories, 
								TokenSequence *tokenSequence);

private:
	bool _debug_flag;
	bool _force_lowercase_sentence;
	enum { IDF_NAME_FINDER, PIDF_NAME_FINDER, NONE} _name_finder;
	int _num_names;
	int _sent_no;
	IdFNameRecognizer *_idfNameRecognizer;
	PIdFModel *_pidfDecoder;
	DTTagSet *_tagSet;
	DocTheory *_docTheory;
	const TokenSequence* _tokenSequence;
	static Symbol _NONE_ST;
	static Symbol _NONE_CO;

	
	NameTheory *makeNameTheory(PIdFSentence &sentence);
	// names, but pre-coagulation
	struct NameSet {
		int num_words;
		Symbol* words;
	};
	NameSet _nameWords[MAX_DOC_NAMES];
	bool DISALLOW_EMAILS;
	bool isEmailorURL(Symbol sym);
};

#endif
