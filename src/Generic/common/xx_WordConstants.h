// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_WORDCONSTANTS_H
#define xx_WORDCONSTANTS_H

#include "Generic/common/WordConstants.h"

class GenericWordConstants {
public:
	static bool isNumeric(Symbol) { return false; }
	static bool isAlphabetic(Symbol) { return false; }
	static bool isSingleCharacter(Symbol) { return false; }
	static bool startsWithDash(Symbol) { return false; }
	static bool isOrdinal(Symbol) { return false; }
	static bool isURLCharacter(Symbol) { return false; }
	static bool isPhoneCharacter(Symbol) { return false; }
	static bool isASCIINumericCharacter(Symbol) { return false; }
	static Symbol getNumericPortion(Symbol) { return Symbol(); }
	static bool isDailyTemporalExpression(Symbol) { return false; }
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
	static bool isTensedCopulaTypeVerb(Symbol) { return false; }
	static bool isNameLinkStopWord(Symbol) { return false; }
	static bool isAcronymStopWord(Symbol) { return false; }
	static bool isPronoun(Symbol) { return false; }
	static bool isReflexivePronoun(Symbol) { return false; }
	static bool isRelativePronoun(Symbol) { return false; }
	static bool is1pPronoun(Symbol) { return false; }
	static bool isSingular1pPronoun(Symbol) { return false; }
	static bool is2pPronoun(Symbol) { return false; }
	static bool is3pPronoun(Symbol) { return false; }
	static bool isOtherPronoun(Symbol) { return false; }
	static bool isWHQPronoun(Symbol) { return false; }
	static bool isPERTypePronoun(Symbol) { return false; }
	static bool isLOCTypePronoun(Symbol) { return false; }
	static bool isSingularPronoun(Symbol) { return false; }
	static bool isPluralPronoun(Symbol) { return false; }
	static bool isPossessivePronoun(Symbol) { return false; }
	static bool isLinkingPronoun(Symbol) { return false; }
	static bool isNonPersonPronoun(Symbol) { return false; }
	static bool isLocativePreposition(Symbol) { return false; }
	static bool isForReasonPreposition(Symbol) { return false; }
	static bool isOfActionPreposition(Symbol) { return false; }
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

