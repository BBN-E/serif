// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/Buckwalter/ar_Retokenizer.h"
#include "Generic/morphSelection/MorphDecoder.h"
#include "Generic/morphSelection/MorphSelector.h"
#include "Arabic/morphAnalysis/ar_MorphologicalAnalyzer.h"

MorphSelector::MorphSelector(ArabicMorphologicalAnalyzer* ma){
	_morphDecoder = MorphDecoder::build(ma);
}
MorphSelector::~MorphSelector(){};

int MorphSelector::selectTokenization(TokenSequence *origTS){
	int nNewTokens = 
		_morphDecoder->getBestWordSequence(origTS, _words, _map, _startOffsets, 
		_endOffsets, MAX_SENTENCE_TOKENS);
	_updateTokenSequence(origTS, _map, _startOffsets, _endOffsets, 
		_words, nNewTokens);
	return nNewTokens;

}

int MorphSelector::selectTokenization(TokenSequence *origTS, int *constraints, int n_constraints){
	int nNewTokens = 
		_morphDecoder->getBestWordSequence(origTS, _words, _map, _startOffsets, 
		_endOffsets, constraints, n_constraints, MAX_SENTENCE_TOKENS);
	_updateTokenSequence(origTS, _map, _startOffsets, _endOffsets, 
		_words, nNewTokens);
	return nNewTokens;
}

void MorphSelector::_updateTokenSequence(TokenSequence* origTS, int* map,  
										 int* start, int* end, Symbol* words, 
										 int nwords){
	int nOrigTokens = origTS->getNTokens();
	//make the tokens- need to add or
	for(int i=0; i<nwords; i++){
		_newTokens[i] = 
			_new Token(start[i], end[i], map[i], words[i], 0, 0);
		/*const Token* origTok = origTS->getToken(_newTokens[i]->getOriginalTokenIndex());
		Token* newTok = _newTokens[i];
		std::cout<<"%%%% TSTest: "
			<<newTok->getSymbol().to_debug_string()
			<<" "<<newTok->getStartOffset()
			<<" "<<newTok->getStartOffset()
			<<" n lex choice: "<<origTok->getNLexicalEntries()<<std::endl;
		for(int m =0; m < origTok->getNLexicalEntries(); m++){
			std::cout<<"    ";
			origTok->getLexicalEntry(m)->dump(std::cout);
			std::cout<<std::endl;
		}*/

	}

	Retokenizer::Retokenize(origTS, _newTokens, nwords);
}
