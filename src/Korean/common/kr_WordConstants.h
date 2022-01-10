// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef kr_WORD_CONSTANTS_H
#define kr_WORD_CONSTANTS_H

#include "common/Symbol.h"

// place static symbols here that may be used by other classes 
// in various Korean specific areas.

class KoreanWordConstants {
	// Note: this class is intentionally not a subclass of
	// WordConstants.  See WordConstants.h for explanation.
public:
	/**
	  * Punctuation
	  */
	static Symbol ASCII_COMMA;
	static Symbol NEW_LINE;
	static Symbol CARRIAGE_RETURN;
	static Symbol EOS_MARK;
	static Symbol LATIN_STOP;		
	static Symbol LATIN_EXCLAMATION;  
	static Symbol LATIN_QUESTION;	
	static Symbol FULL_STOP;
	static Symbol FULL_COMMA;
	static Symbol FULL_EXCLAMATION;
	static Symbol FULL_QUESTION;
	static Symbol FULL_SEMICOLON;
	static Symbol FULL_RRB;

	static wchar_t NULL_JAMO;
	static Symbol NULL_JAMO_SYM;
	
	//the following function delimits the class of pronouns that we care about
	static bool isPronoun(Symbol word) {
		return is1pPronoun(word) ||
			   is2pPronoun(word) ||
			   is3pPronoun(word) ||
			   isOtherPronoun(word);
	}
	static bool isURLCharacter(Symbol word);
	static bool isPhoneCharacter(Symbol word);
	static bool isASCIINumericCharacter(Symbol word);

	// The following methods have not been defined for Arabic yet, so
	// we just return the default value (false).
	static bool is1pPronoun(Symbol word) { return false; }
	static bool is2pPronoun(Symbol word) { return false; }
	static bool is3pPronoun(Symbol word) { return false; }
	static bool isSingular1pPronoun(Symbol word) { return false; }
	static bool isLinkingPronoun(Symbol word) { return false; }
	static bool isNumeric(Symbol) { return false; }
	static bool isAlphabetic(Symbol) { return false; }
	static bool isSingleCharacter(Symbol) { return false; }
	static bool startsWithDash(Symbol) { return false; }
	static bool isOrdinal(Symbol) { return false; }
	static bool isURLCharacter(Symbol) { return false; }
	static bool isPhoneCharacter(Symbol) { return false; }
	static bool isASCIINumericCharacter(Symbol) { return false; }
	static Symbol getNumericPortion(Symbol) { return Symbol(); }
	static bool isTimeExpression(Symbol) { return false; }
	static bool isTimeModifier(Symbol) { return false; }
	static bool isFourDigitYear(Symbol) { return false; }
	static bool isDashedDuration(Symbol) { return false; }
	static bool isDecade(Symbol) { return false; }
	static bool isDateExpression(Symbol) { return false; }
	static bool isLowNumberWord(Symbol) { return false; }
	static bool isNameSuffix(Symbol) { return false; }
	static bool isHonorificWord(Symbol) { return false; }
	static bool isMilitaryWord(Symbol) { return false; }
	static bool isCopula(Symbol) { return false; }
	static bool isNameLinkStopWord(Symbol) { return false; }
	static bool isAcronymStopWord(Symbol) { return false; }
	static bool isReflexivePronoun(Symbol) { return false; }
	static bool isOtherPronoun(Symbol) { return false; }
	static bool isWHQPronoun(Symbol) { return false; }
	static bool isPERTypePronoun(Symbol) { return false; }
	static bool isLOCTypePronoun(Symbol) { return false; }
	static bool isSingularPronoun(Symbol) { return false; }
	static bool isPluralPronoun(Symbol) { return false; }
	static bool isPossessivePronoun(Symbol) { return false; }
	static bool isNonPersonPronoun(Symbol) { return false; }
	static bool isLocativePreposition(Symbol) { return false; }
	static bool isUnknownRelationReportedPreposition(Symbol) { return false; }
	static bool isPunctuation(Symbol) { return false; }
	static bool isOpenDoubleBracket(Symbol) { return false; }
	static bool isClosedDoubleBracket(Symbol) { return false; }
	static bool isPartitiveWord(Symbol) { return false; }
	static bool isDeterminer(Symbol) { return false; }
    static bool isDefiniteArticle(Symbol word) { return false; }
	static bool isIndefiniteArticle(Symbol word) { return false; }
};

#endif


