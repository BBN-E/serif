// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Urdu/common/ur_WordConstants.h"

#include <boost/regex.hpp>

// Local constants

// Union of symbol groups.
Symbol::SymbolGroup operator+(Symbol::SymbolGroup a, Symbol::SymbolGroup b) {
	Symbol::SymbolGroup result(a.begin(), a.end());
	result.insert(b.begin(), b.end());
	return result;
}

/////////////////////////////////////////////////////////////////
// Pronouns by Type
// from http://en.wikibooks.org/wiki/Urdu/Pronouns
// there is no gender specification on pronouns
// there is no singular/plural specification on the 2nd person
/////////////////////////////////////////////////////////////////

// subject pronouns
Symbol UrduWordConstants::I = Symbol(L"\u0645\u064a\u06ba");					
Symbol UrduWordConstants::WE = Symbol(L"\u06c1\u0645");
Symbol UrduWordConstants::YOU_FORMAL = Symbol(L"\u0622\u067e");
Symbol UrduWordConstants::YOU_INFORMAL = Symbol(L"\u06c1\u0645");
Symbol UrduWordConstants::YOU_VINFORMAL = Symbol(L"\u062a\u0645");
Symbol UrduWordConstants::IT = Symbol(L"\u06cc\u06c1");	
Symbol UrduWordConstants::THEY = Symbol(L"\u0648\u06c1");

// object pronouns
Symbol UrduWordConstants::ME = Symbol(L"\u0645\u062c\u06be\u06d2");
Symbol UrduWordConstants::US = Symbol(L"\u06c1\u0645\u06cc\u06ba");
// first form (formal) is bigram in accusative
Symbol UrduWordConstants::YOUACC_INFORMAL = Symbol(L"\u062a\u0645\u06c1\u06cc\u06ba");
Symbol UrduWordConstants::YOUACC_VINFORMAL = Symbol(L"\u062a\u062c\u06be\u06d2");
// third person singular is either a bigram or a suffix
Symbol UrduWordConstants::THEM_NEAR = Symbol(L"\u0627\u0650\u0646\u06c1\u06cc\u06ba");
Symbol UrduWordConstants::THEM_FAR = Symbol(L"\u0627\u064f\u0646\u06c1\u06cc\u06ba");

// possessive pronouns					
Symbol UrduWordConstants::MY = Symbol(L"\u0645\u06cc\u0631\u0627");			
Symbol UrduWordConstants::OUR = Symbol(L"\u06c1\u0645\u0627\u0631\u0627");
Symbol UrduWordConstants::YOUR_FORMAL = Symbol(L"\u0622\u067e\u06a9\u0627");			
Symbol UrduWordConstants::YOUR_INFORMAL = Symbol(L"\u062a\u0645\u06c1\u0627\u0631\u0627");		
Symbol UrduWordConstants::YOUR_VINFORMAL = Symbol(L"\u062a\u06cc\u0631\u0627");			
Symbol UrduWordConstants::ITS_NEAR = Symbol(L"\u0627\u0650\u0633\u06a9\u0627");
Symbol UrduWordConstants::ITS_FAR = Symbol(L"\u0627\u064f\u0633\u06a9\u0627");
Symbol UrduWordConstants::THEIR_NEAR = Symbol(L"\u0627\u0650\u0646\u06a9\u0627");
Symbol UrduWordConstants::THEIR_FAR = Symbol(L"\u0627\u064f\u0646\u06a9\u0627");
	


// titles
Symbol UrduWordConstants::FEM_TITLE_1 = Symbol(L"\u0645\u0633");		
Symbol UrduWordConstants::FEM_TITLE_2 = Symbol(L"\u06a9\u0645\u0627\u0631\u06cc");		
Symbol UrduWordConstants::FEM_TITLE_3 = Symbol(L"\u0622\u0646\u0633\u06c1");		
Symbol UrduWordConstants::MASC_TITLE_1 = Symbol(L"\u062c\u0646\u0627\u0628");

// interrogatives
Symbol UrduWordConstants::WHO = Symbol(L"\u06a9\u0648\u0646");
Symbol UrduWordConstants::WHAT = Symbol(L"\u06a9\u064a\u0627");
Symbol UrduWordConstants::WHEN = Symbol(L"\u06a9\u0628");
Symbol UrduWordConstants::WHERE = Symbol(L"\u06a9\u06c1\u0627\u06ba");
Symbol UrduWordConstants::WHY = Symbol(L"\u06a9\u064a\u0648\u06ba");
Symbol UrduWordConstants::HOW = Symbol(L"\u06a9\u064a\u0633\u06d2");

// punctuation
Symbol UrduWordConstants::UR_PERIOD = Symbol(L"\u06d4");
Symbol UrduWordConstants::UR_COMMA = Symbol(L"\u060c");
Symbol UrduWordConstants::UR_QUESTION = Symbol(L"\u061f");
Symbol UrduWordConstants::UR_SEMICOLON = Symbol(L"\u061b");
Symbol UrduWordConstants::UR_ELLIPSIS = Symbol(L"\u2026");
Symbol UrduWordConstants::LATIN_PERIOD = Symbol(L".");
Symbol UrduWordConstants::LATIN_COMMA = Symbol(L",");
Symbol UrduWordConstants::LATIN_QUESTION = Symbol(L"?");
Symbol UrduWordConstants::LATIN_SEMICOLON = Symbol(L";");

// Numbers
const Symbol::SymbolGroup CARDINALS = Symbol::makeSymbolGroup
	(L"\u0635\u0641\u0631 \u0627\u064a\u06a9 \u062f\u0648 \u062a\u064a\u0646"
	L"\u0686\u0627\u0631 \u067e\u0627\u0646\u0686 \u0686\u0647 \u0633\u0627\u062a"
	L"\u0622\u06bb\u0647 \u0646\u0648 \u062f\u0633 \u06af\u064a\u0627\u0631\u06c1"
	L"\u0628\u0627\u0631\u06c1 \u062a\u064a\u0631\u06c1 \u0686\u0648\u062f\u06c1"
	L"\u067e\u0646\u062f\u0631\u06c1 \u0633\u0648\u0644\u06c1 \u0633\u062a\u0631\u06c1"
	L"\u0627\u06bb\u0647\u0627\u0631\u06c1 \u0627\u0646\u064a\u0633 \u0628\u064a\u0633"
	L"\u062a\u064a\u0633 \u0686\u0627\u0644\u064a\u0633 \u067e\u0686\u0627\u0633"
	L"\u0633\u0627\u06bb\u0647 \u0633\u062a\u0631 \u0627\u0633\u0649 \u0646\u0648\u06d2 \u0633\u0648");
const Symbol::SymbolGroup ORDINALS = Symbol::makeSymbolGroup
	(L"\u067e\u06c1\u0644\u0627 \u062f\u0648\u0633\u0631\u0627 \u062a\u064a\u0633\u0631\u0627");



/////////////////////////////////////////////////////////////////
// Other Symbols
/////////////////////////////////////////////////////////////////
Symbol DOUBLE_LEFT_BRACKET(L"-ldb-");
Symbol DOUBLE_RIGHT_BRACKET(L"-rdb-");
Symbol DOUBLE_LEFT_BRACKET_UC(L"-LDB-");
Symbol DOUBLE_RIGHT_BRACKET_UC(L"-RDB-");
Symbol LEFT_BRACKET(L"-lrb-");
Symbol RIGHT_BRACKET(L"-rrb-");
Symbol LEFT_BRACKET_UC(L"-LRB-");
Symbol RIGHT_BRACKET_UC(L"-RRB-");


/////////////////////////////////////////////////////////////////
// Regular Expressions
/////////////////////////////////////////////////////////////////
const boost::wregex NUMERIC_RE(L"\\d+", boost::wregex::perl);
const boost::wregex ALPHABETIC_RE(L"\\w+", boost::wregex::perl);



bool UrduWordConstants::isSubjectPronoun(Symbol word){
	return (word == I ||
			word == WE ||
			word == YOU_FORMAL ||
			word == YOU_INFORMAL ||
			word == YOU_VINFORMAL ||
			word == IT ||
			word == THEY);
	}

bool UrduWordConstants::isObjectPronoun(Symbol word) {
		return (word == ME ||
			word == US ||
			word == YOUACC_INFORMAL ||
			word == YOUACC_VINFORMAL ||
			word == THEM_NEAR ||
			word == THEM_FAR);
	}

bool UrduWordConstants::isPossessivePronoun(Symbol word) {
		return (word == MY ||
			word == OUR ||
			word == YOUR_FORMAL ||
			word == YOUR_INFORMAL ||
			word == YOUR_VINFORMAL ||
			word == ITS_NEAR ||
			word == ITS_FAR ||
			word == THEIR_NEAR ||
			word == THEIR_FAR);
	}

bool UrduWordConstants::isPronoun(Symbol word) {
		return (isSubjectPronoun(word) ||
			isObjectPronoun(word) ||
			isPossessivePronoun(word));
	}

bool UrduWordConstants::is1pPronoun(Symbol word) {
		return (word == I || 
			word == WE ||
			word == ME || 
			word == US ||	
			word == MY ||
			word == OUR);
	}

bool UrduWordConstants::is2pPronoun(Symbol word) {
		return (word == YOU_FORMAL || 
			word == YOU_INFORMAL ||
			word == YOU_VINFORMAL ||
			word == YOUACC_INFORMAL ||
			word == YOUACC_VINFORMAL ||
			word == YOUR_FORMAL ||
			word == YOUR_INFORMAL ||
			word == YOUR_VINFORMAL);
	}

bool UrduWordConstants::is3pPronoun(Symbol word) {
		return (word == IT||
			word == THEY ||
			word == THEM_NEAR ||
			word == THEM_FAR ||
			word == ITS_NEAR ||
			word == ITS_FAR ||
			word == THEIR_NEAR ||
			word == THEIR_FAR);
	}


bool UrduWordConstants::isSingular1pPronoun(Symbol word) {
	return (word == MY ||
		word == ME ||
		word == I);
}

// 2person may be singular or plural
bool UrduWordConstants::isSingularPronoun(Symbol word) { 
	return (word == I ||
		word == ME ||
		word == MY ||
		word == IT ||
		word == ITS_NEAR ||
		word == ITS_FAR);
}

// 2person may be singular or polural
bool UrduWordConstants::isPluralPronoun(Symbol word) {
	return (word == WE ||
		word == US ||
		word == OUR ||
		word == THEY ||
		word == THEM_NEAR ||
		word == THEM_FAR ||
		word == THEIR_NEAR ||
		word == THEIR_FAR);
}

bool UrduWordConstants::isWHQPronoun(Symbol word) {
	return (word == WHO ||
		word == WHAT ||
		word == WHEN ||
		word == WHERE ||
		word == WHY || 
		word == HOW);
}


bool UrduWordConstants::isLinkingPronoun(Symbol word) { 
	return (is3pPronoun(word) || isWHQPronoun(word)); 
}


bool UrduWordConstants::isFeminineTitle(Symbol word) {
	return (word == FEM_TITLE_1 ||
		word == FEM_TITLE_2 ||
		word == FEM_TITLE_3);
}

bool UrduWordConstants::isMasculineTitle(Symbol word){
	return (word == MASC_TITLE_1);
}

bool UrduWordConstants::isTitle(Symbol word) {
	return (isFeminineTitle(word) || 
		isMasculineTitle(word));
}

bool UrduWordConstants::isPunctuation(Symbol word) {
	if (word == UR_PERIOD ||
		word == UR_COMMA ||
		word == UR_QUESTION ||
		word == UR_SEMICOLON ||
		word == UR_ELLIPSIS ||
		word == LATIN_PERIOD ||
		word == LATIN_COMMA ||
		word == LATIN_QUESTION ||
		word == LATIN_SEMICOLON) return true;
	std::wstring str(word.to_string());
	size_t length = str.length();
	for (size_t i = 0; i < length; ++i) {
	if (!iswpunct(str[i]))
		return false;
	}
	return true;
}
	

bool UrduWordConstants::isLowNumberWord(Symbol s){
	return s.isInSymbolGroup(CARDINALS);
}
bool UrduWordConstants::isOrdinal(Symbol s) {
	return s.isInSymbolGroup(ORDINALS);
}

bool UrduWordConstants::isNumeric(Symbol s) { 
	return boost::regex_match(s.to_string(), NUMERIC_RE);
}

bool UrduWordConstants::isAlphabetic(Symbol s) { 
	return boost::regex_match(s.to_string(), ALPHABETIC_RE);
}

bool UrduWordConstants::isSingleCharacter(Symbol s) {
	return (std::wstring(s.to_string()).length() == 1);
}

bool UrduWordConstants::startsWithDash(Symbol word) {
	std::wstring str = word.to_string();
	return (str.length() > 0 && str[0] == L'-');
}

bool UrduWordConstants::isOpenBracket(Symbol word) {
	return word == LEFT_BRACKET || word == LEFT_BRACKET_UC;
}

bool UrduWordConstants::isClosedBracket(Symbol word) {
	return word == RIGHT_BRACKET || word == RIGHT_BRACKET_UC;
}

bool UrduWordConstants::isOpenDoubleBracket(Symbol word) {
	return word == DOUBLE_LEFT_BRACKET || word == DOUBLE_LEFT_BRACKET_UC;
}

bool UrduWordConstants::isClosedDoubleBracket(Symbol word) {
	return word == DOUBLE_RIGHT_BRACKET || word == DOUBLE_RIGHT_BRACKET_UC;
}


