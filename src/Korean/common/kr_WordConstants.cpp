// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/WordConstants.h"

// Punctuation
Symbol WordConstants::ASCII_COMMA = Symbol(L",");
Symbol WordConstants::NEW_LINE = Symbol(L"\x000A");
Symbol WordConstants::CARRIAGE_RETURN = Symbol(L"\x000D");
Symbol WordConstants::EOS_MARK = Symbol(L"\x3002");					
Symbol WordConstants::LATIN_STOP = Symbol(L"\x002E");		
Symbol WordConstants::LATIN_EXCLAMATION = Symbol(L"\x0021");   
Symbol WordConstants::LATIN_QUESTION = Symbol(L"\x003F");		
Symbol WordConstants::FULL_STOP = Symbol(L"\xFF0E");				
Symbol WordConstants::FULL_COMMA = Symbol(L"\xFF0C");
Symbol WordConstants::FULL_EXCLAMATION = Symbol(L"\xFF01");		
Symbol WordConstants::FULL_QUESTION = Symbol(L"\xFF1F");		 
Symbol WordConstants::FULL_SEMICOLON = Symbol(L"\xFF1B");
Symbol WordConstants::FULL_RRB = Symbol(L"\xFF09");

wchar_t WordConstants::NULL_JAMO = 0x110B;
Symbol WordConstants::NULL_JAMO_SYM = Symbol(L"\x110B");

bool WordConstants::isURLCharacter(Symbol word) {
	const wchar_t *ch = word.to_string();
	if (wcslen(ch) != 1)
		return false;
	return (iswascii(ch[0]) != 0);
}

bool WordConstants::isPhoneCharacter(Symbol word) {
	const wchar_t *ch = word.to_string();
	size_t len = wcslen(ch);
	if (len == 1) 
		return (iswdigit(ch[0]) || ch[0] == L'-');
	else
		return (word == Symbol(L"-LRB-") ||
			    word == Symbol(L"-RRB-"));
}

bool WordConstants::isASCIINumericCharacter(Symbol word) {
	const wchar_t *ch = word.to_string();
	if (wcslen(ch) != 1)
		return false;
	return (iswdigit(ch[0]) != 0);
}
