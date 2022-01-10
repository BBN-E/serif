// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_IDF_NAME_RECOGNIZER_H
#define en_IDF_NAME_RECOGNIZER_H

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/theories/EntityType.h"

class Entity;
class NameTheory;
class IdFSentenceTokens;
class IdFSentenceTheory;
class IdFDecoder;
class DocTheory;
class NameClassTags;
class Symbol;
class NameSpan;
class SymbolHash;
class IdFListSet;


class EnglishIdFNameRecognizer {
public:
	const static int MAX_NAMES = 100;
	const static int MAX_DOC_NAMES = 2000;
	const static int MAX_SENTENCE_LENGTH = 2000;

	EnglishIdFNameRecognizer();
	void resetForNewSentence();

	void cleanUpAfterDocument();
	void resetForNewDocument(DocTheory *docTheory = 0);

	// This puts an array of pointers to NameTheorys
	// where specified by results arg, and returns its size. It returns
	// 0 if something goes wrong. The client is responsible for deleting
	// the array and the NameTheorys.
	int getNameTheories(NameTheory **results, int max_theories,
						        TokenSequence *tokenSequence);
	
private:
	UTF8OutputStream _debugStream;
	bool DEBUG;
	IdFDecoder *_decoder;
	IdFDecoder *_defaultDecoder;
	IdFDecoder *_lowerCaseDecoder;
	IdFDecoder *_upperCaseDecoder;
	IdFSentenceTokens *_sentenceTokens;
	IdFSentenceTheory **_IdFTheories;
	NameClassTags *_nameClassTags;

	// change the type of the name, if it's been seen in the document before
	// under another type
	void fixName(NameSpan* span, IdFSentenceTokens* sent);

	// names, but pre-coagulation
	struct NameSet {
		int num_words;
		Symbol* words;
	};

	// for storing every name seen so far in the document
	Symbol _names[MAX_DOC_NAMES];
	EntityType _types[MAX_DOC_NAMES];
	NameSet _nameWords[MAX_DOC_NAMES];
	int _num_names;

	// utility methods that build a symbol from a bunch of symbols
	Symbol getNameSymbol(int start, int end, IdFSentenceTokens* sent);
	Symbol getNameSymbol(int start, int end, NameSet set);

	bool SPLIT_ORGS;
	void splitOrgGPENames(IdFSentenceTheory *theory);
	int _ORG_ST_index;
	int _GPE_ST_index;
	SymbolHash *_nationsTable;
	Symbol lowercaseSymbol(Symbol sym);
	bool isSplittableOrgName(int start, int end);
	IdFListSet *_splittableOrgsTable;
	int _splittableOrgStarts[MAX_DOC_NAMES];
	int _splittableOrgEnds[MAX_DOC_NAMES];
	int _num_splittable_orgs;
	
};

#endif
