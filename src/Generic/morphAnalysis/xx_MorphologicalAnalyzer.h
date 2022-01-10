// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_MORPHOLOGICALANALYZER_H
#define XX_MORPHOLOGICALANALYZER_H

#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Lexicon.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"

class GenericMorphologicalAnalyzer: MorphologicalAnalyzer {
private:
	friend class GenericMorphologicalAnalyzerFactory;

public:
	~GenericMorphologicalAnalyzer(){}
	void resetForNewSentence(){};//you could set the dictionaries here
	int getMorphTheories(TokenSequence *origTS){
		return origTS->getNTokens();
	};
	void readNewVocab(const char* dict_file){};
	void resetDictionary(){};
	int getMorphTheoriesDBG(TokenSequence *origTS) { return 0; }


private:
	GenericMorphologicalAnalyzer(){}
};

class GenericMorphologicalAnalyzerFactory: public MorphologicalAnalyzer::Factory {
	virtual MorphologicalAnalyzer *build() { return _new GenericMorphologicalAnalyzer(); }
};

#endif
