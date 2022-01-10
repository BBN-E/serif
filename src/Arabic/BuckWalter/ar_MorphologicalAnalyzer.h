// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_MORPHOLOGICALANALYZER_H
#define AR_MORPHOLOGICALANALYZER_H

#include "Generic/common/limits.h"
#include "Generic/theories/Lexicon.h"
#include "Generic/common/Symbol.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"

class BWRuleDictionary;
class TokenSequence;
class Token;

class ArabicMorphologicalAnalyzer : public MorphologicalAnalyzer {
private:
	friend class ArabicMorphologicalAnalyzerFactory;

public:

	~ArabicMorphologicalAnalyzer();

	void resetForNewSentence(){}; //you could set the dictionaries here

	
	int getMorphTheories(TokenSequence *origTS);
	int getMorphTheoriesDBG(TokenSequence *origTS);
	Lexicon* getLexicon(){return lex;};
	bool dictionaryFull(){
		return lex->lexiconFull();
	}
	BWRuleDictionary* getRules(){return rules;};
	void readNewVocab(const char* dict_file);
	void resetDictionary();

private:
	Lexicon* lex;
	BWRuleDictionary* rules;
	bool _resetDictionary;

	Token* newToks[MAX_SENTENCE_TOKENS];

	ArabicMorphologicalAnalyzer();
};

class ArabicMorphologicalAnalyzerFactory: public MorphologicalAnalyzer::Factory {
	virtual MorphologicalAnalyzer *build() { return _new ArabicMorphologicalAnalyzer(); }
};

#endif
