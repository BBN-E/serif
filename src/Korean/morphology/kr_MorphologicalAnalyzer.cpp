// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "Korean/morphAnalysis/kr_MorphologicalAnalyzer.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/LexicalEntry.h"
#include "Generic/theories/Lexicon.h"
#include "Generic/morphSelection/Retokenizer.h"
#include "Generic/morphAnalysis/SessionLexicon.h"
#include "Korean/morphology/kr_Klex.h"


KoreanMorphologicalAnalyzer::KoreanMorphologicalAnalyzer() : _lex(0) {
	_lex = SessionLexicon::getInstance().getLexicon();
}

KoreanMorphologicalAnalyzer::~KoreanMorphologicalAnalyzer() {
	SessionLexicon::destroy();
}

int KoreanMorphologicalAnalyzer::getMorphTheories(TokenSequence *origTS) {
	
	Klex::analyzeSentence(origTS, _lex);

	/*//debugging
	for (int i = 0; i < origTS->getNTokens(); i++) {
		const Token* t = origTS->getToken(i);
		for(int k = 0; k < t->getNLexicalEntries(); k++) {
			t->getLexicalEntry(k)->dump(std::cout);
		}
	}
	*/
	return origTS->getNTokens();
}


int KoreanMorphologicalAnalyzer::getMorphTheoriesDBG(TokenSequence *origTS) {
	const int max_results = MAXIMUM_MORPH_ANALYSES;
	LexicalEntry* result[max_results];	
	

	std::cout << "getMorphTheoriesDBG() Analyze " << origTS->getNTokens() << " Tokens" << std::endl;
	for (int i = 0; i < origTS->getNTokens(); i++) {
		const Token* t = origTS->getToken(i);
		
		int n = _lex->getEntriesByKey(t->getSymbol(), result, max_results);
		std::cout << "\tAnalyze token " << i << ": " << t->getSymbol().to_debug_string() << std::endl;
		std::cout << ".";

		if (n == 0) {
			n = Klex::analyzeWord(t->getSymbol().to_string(), _lex, result, max_results);
			if (n > 0) 
				std::cout<< "Found Analysis: " << t->getSymbol().to_debug_string() << "-- " << n << std::endl;
			else 
				std::cout << "NO ANALYSIS FOUND " << t->getSymbol().to_debug_string() << std::endl;
		}
		else {
			std::cout << "Analysis already in Lexicon: " << t->getSymbol().to_debug_string() << " -- " << n << std::endl;
		}
		std::cout << "\t\tMake New Token" << std::endl;
		_newToks[i] = _new Token(t->getStartOffset(), t->getEndOffset(), i, t->getSymbol(), (int)n, result);

	}
	//retokenize the entire sequence
	std::cout << "\tRetokenize" << std::endl;
	origTS->retokenize(origTS->getNTokens(), _newToks);
	/*//debugging
	for (int i = 0; i < origTS->getNTokens(); i++) {
		const Token* t = origTS->getToken(i);
		for (int k = 0; k < t->getNLexicalEntries(); k++) 
			t->getLexicalEntry(k)->dump(std::cout);
		}
	}
	*/
	std::cout << "\tFinished" << std::endl;
	return origTS->getNTokens();
}
