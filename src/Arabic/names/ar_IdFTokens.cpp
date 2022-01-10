// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/names/ar_IdFTokens.h"
#include <string>
#include <cstdio>


IdFTokens::IdFTokens(){
	//arrays are statically allocated
	_numTokens = 0;
}
IdFTokens::~IdFTokens(){
	resetTokens();
}

int IdFTokens::getNumTokens(){
	return _numTokens;
}

const LexicalToken* IdFTokens::getToken(int tokNum){
	return _idfTokens[tokNum];
}
int IdFTokens::getSerifTokenNum(int tokNum){
	return _tokMap[tokNum];
}
void IdFTokens::tokenize(const TokenSequence* serifTokens){
	const LexicalTokenSequence *serifLexicalTokens = dynamic_cast<const LexicalTokenSequence*>(serifTokens);
	if (!serifLexicalTokens) throw InternalInconsistencyException("IdFTokens::tokenize",
		"This IdFTokens name recognizer requires a LexicalTokenSequence.");

	int numSerifTok = serifLexicalTokens->getNTokens();
	for(int i = 0; i< numSerifTok; i++){
		Symbol serifWord =  serifLexicalTokens->getToken(i)->getSymbol();
		int serifOrigTok = i;
		const std::wstring word  =serifWord.to_string();
		if((word.length() < 2) || !isIdFClitic(word.at(0))){
			_idfTokens[_numTokens] = _new LexicalToken(*serifLexicalTokens->getToken(i), serifOrigTok, 0, NULL);
			_tokMap[_numTokens] = i;
			_numTokens++;
		}
		else{
			// Note: the offset span for both the clitic and the subword are set to the span of the entire
			// source word.  We don't set clitic's span to cover the first character and the subword's 
			// span to cover the remainder of the word's characters.
			_clitic[0] = word.at(0);
			_clitic[1] =L'\0';
			_idfTokens[_numTokens] = _new LexicalToken(*serifLexicalTokens->getToken(i), Symbol(_clitic), serifOrigTok);
			_tokMap[_numTokens] = i;
			_numTokens++;
			int len = word.length() <75 ? static_cast<int>(word.length()) : 75;
			int j = 0;
			for(j = 1; j<len; j++){
				_subword[j-1] = word.at(j);
			}
			_subword[j-1] = L'\0';
			_idfTokens[_numTokens] = _new LexicalToken(*serifLexicalTokens->getToken(i), Symbol(_subword), serifOrigTok);
			_tokMap[_numTokens] = i;
			_numTokens++;
		}
	}
}

void IdFTokens::resetTokens(){
	for(int i =0; i<_numTokens; i++){
		delete _idfTokens[i];
	}
    _numTokens = 0;
}
//true if c is b, l, w
bool IdFTokens::isIdFClitic(wchar_t c){
	return ( (c==L'\x648') || (c==L'\x628') || (c==L'\x644') );
}
