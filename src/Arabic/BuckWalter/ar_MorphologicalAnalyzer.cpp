// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/BuckWalter/ar_BuckWalterizer.h"
#include "Arabic/BuckWalter/ar_BWRuleDictionary.h"
#include "Arabic/BuckWalter/ar_BWDictionaryReader.h"
#include "Arabic/BuckWalter/ar_MorphologicalAnalyzer.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/LexicalToken.h"
#include "Generic/morphAnalysis/SessionLexicon.h"
#include "Generic/morphSelection/Retokenizer.h"


ArabicMorphologicalAnalyzer::ArabicMorphologicalAnalyzer() {	
	lex = SessionLexicon::getInstance().getLexicon();
	rules = BWRuleDictionary::getInstance();

	_resetDictionary = ParamReader::isParamTrue("reset_dictionary");
}

ArabicMorphologicalAnalyzer::~ArabicMorphologicalAnalyzer() {
	SessionLexicon::destroy();
	Retokenizer::destroy();
	BWRuleDictionary::destroy();
}

int ArabicMorphologicalAnalyzer::getMorphTheories(TokenSequence *origTS) {
    if (lex == NULL) {
        std::cerr << "ArabicMorphologicalAnalyzer::getMorphTheories() called when lex hasn't been initialized.  Do you have ignore-lexical-dictionary set to true?\n";
        exit(-1);
    }
    if (rules == NULL) {
        std::cerr << "ArabicMorphologicalAnalyzer::getMorphTheories() called when rules hasn't been initialized.  Do you have ignore-lexical-dictionary set to true?\n";
        exit(-1);
    }
	for (int i = 0; i < origTS->getNTokens(); i++) {
		const Token* t = origTS->getToken(i);
		const int max_results = MAXIMUM_MORPH_ANALYSES;
		LexicalEntry* result[max_results];
		wchar_t tokstr[100];
		const wchar_t* conststr = t->getSymbol().to_string();
		for(int j = 0; j < 100; j++) {
			tokstr[j] = conststr[j];
			if (conststr[j] == L'\0') {
				break;
			}
		}
		int n = BuckWalterizer::analyze(tokstr, result, max_results, lex, rules);
		newToks[i] = _new LexicalToken(*t, i, n, result);

	}
	//retokenize the entire sequence
	origTS->retokenize(origTS->getNTokens(), newToks);
	
	return origTS->getNTokens();
}


int ArabicMorphologicalAnalyzer::getMorphTheoriesDBG(TokenSequence *origTS) {
	std::cout<<"getMorphTheoriesDBG() Analyze "<<origTS->getNTokens()<<" Tokens"<<std::endl;
	for (int i = 0; i < origTS->getNTokens(); i++) {
		const Token* t = origTS->getToken(i);
		const int max_results = MAXIMUM_MORPH_ANALYSES;
		LexicalEntry* result[max_results];
		wchar_t tokstr[100];
		const wchar_t* conststr = t->getSymbol().to_string();
		for (int j = 0; j < 100; j++) {
			tokstr[j] = conststr[j];
			if (conststr[j]==L'\0') {
				break;
			}
		}
		std::cout<<"\tAnalyze token "<<i<<": "<<t->getSymbol().to_debug_string()<<std::endl;
		std::cout<<"\tCurrently there are "<<static_cast<int>(lex->getNEntries())<<" entries in lex"<<std::endl;
		size_t n;
		n = BuckWalterizer::analyze(tokstr, result, max_results, lex, rules);
		if (n > 0) {
			std::cout<<"\t\tFound Analysis: "<<t->getSymbol().to_debug_string()<<"-- "<<static_cast<int>(n)<<std::endl;

		}
		else {
			std::cout<<"\t\tNO ANALYSIS FOUND "<<t->getSymbol().to_debug_string()<<std::endl;
		}
		std::cout<<"\t\tMake New Token"<<std::endl;
		newToks[i] = _new LexicalToken(*t, i, static_cast<int>(n), result);

	}
	//retokenize the entire sequence
	std::cout<<"\tRetokenize"<<std::endl;
	origTS->retokenize(origTS->getNTokens(), newToks);
	/*//debugging
	for(int i =0; i<origTS->getNTokens(); i++){
		const Token* t = origTS->getToken(i);
		for(int k =0; k< t->getNLexicalEntries(); k++){
			t->getLexicalEntry(k)->dump(std::cout);
		}
	}
	*/
	std::cout<<"\tFinished"<<std::endl;
	return origTS->getNTokens();
}

void ArabicMorphologicalAnalyzer::readNewVocab(const char* dict_file) {
	if (lex != 0) {
		BWDictionaryReader::addToBWDictionary(lex, dict_file);
	}
}

void ArabicMorphologicalAnalyzer::resetDictionary() {
	if (_resetDictionary) {
		if (lex != 0) {
			lex->clearDynamicEntries();
		}
		Retokenizer::getInstance().reset();
	}
}

