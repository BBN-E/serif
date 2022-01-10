// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KR_MORPHOLOGICAL_ANALYZER_H
#define KR_MORPHOLOGICAL_ANALYZER_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Lexicon.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"

class TokenSequence;
class Token;


class KoreanMorphologicalAnalyzer : public MorphologicalAnalyzer {
private:
	friend class KoreanMorphologicalAnalyzerFactory;

public:

	~KoreanMorphologicalAnalyzer();

	int getMorphTheories(TokenSequence *origTS);
	int getMorphTheoriesDBG(TokenSequence *origTS);

private:
	Lexicon* _lex;
	Token* _newToks[MAX_SENTENCE_TOKENS];

	KoreanMorphologicalAnalyzer();

	public:
	void resetForNewSentence() {}; 
	void readNewVocab(const char* dict_file) {};
	void resetDictionary() {};

	bool dictionaryFull() {
		return _lex->lexiconFull();
	}

};

class KoreanMorphologicalAnalyzerFactory: public MorphologicalAnalyzer::Factory {
	virtual MorphologicalAnalyzer *build() { return _new KoreanMorphologicalAnalyzer(); }
};

#endif
