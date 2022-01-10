// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_NAME_RECOGNIZER_H
#define ch_NAME_RECOGNIZER_H

#include "Generic/names/NameRecognizer.h"



class NameTheory;
class DocTheory;
class ChineseIdFNameRecognizer;
class PIdFNameRecognizer;


class ChineseNameRecognizer : public NameRecognizer {
private:
	friend class ChineseNameRecognizerFactory;

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

	typedef std::map<std::wstring, EntityType> _force_type_map_t;
	std::map<std::wstring, EntityType> _forcedTypeNames;
	void forceNameTypes(NameTheory **results, int n_results);
	const static int MAX_SENTENCE_LENGTH = 2000;
	std::wstring getNameString(int start, int end, TokenSequence* tokenSequence);
	
private:
	ChineseNameRecognizer();
	// which implementation to use
	enum { IDF_NAME_FINDER, PIDF_NAME_FINDER } _name_finder;

	ChineseIdFNameRecognizer *_idfNameRecognizer;
	PIdFNameRecognizer *_pidfNameRecognizer;

	int _sent_no;
	DocTheory *_docTheory;

	bool PRINT_SENTENCE_SELECTION_INFO;
};

class ChineseNameRecognizerFactory: public NameRecognizer::Factory {
	virtual NameRecognizer *build() { return _new ChineseNameRecognizer(); }
};


#endif
