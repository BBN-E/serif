// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_WORD_CONSTANTS_H
#define es_WORD_CONSTANTS_H

#include "Generic/common/Symbol.h"
#include "Generic/common/WordConstants.h"

#include <string>

class SpanishWordConstants {
	// Note: this class is intentionally not a subclass of
	// WordConstants.  See WordConstants.h for explanation.
public:


	static Symbol Y;

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
	static bool isNameSuffix(Symbol) { return false; }
	static bool isHonorificWord(Symbol) { return false; }
	static bool isMilitaryWord(Symbol) { return false; }
	static bool isNameLinkStopWord(Symbol) { return false; }
	static bool isUnknownRelationReportedPreposition(Symbol) { return false; }


// semi-implemented
	static bool isPartitiveWord(Symbol);

	// implemented
	static bool isDailyTemporalExpression(Symbol);

	static bool isLocativePreposition(Symbol);
	static bool isForReasonPreposition(Symbol);
	static bool isOfActionPreposition(Symbol);

	static bool isFeminineTitle(Symbol);
	static bool isMasculineTitle(Symbol);
	static bool isCopula(Symbol); 
	static bool isTensedCopulaTypeVerb(Symbol);
	static bool isCopulaForPassive(Symbol); 
	static bool isLowNumberWord(Symbol);

	////////////////////////////////////////////////////////////
	// Pronouns
	////////////////////////////////////////////////////////////
	static bool isPronoun(Symbol);
	static bool isReflexivePronoun(Symbol);
	static bool is1pPronoun(Symbol);
	static bool isSingular1pPronoun(Symbol);
	static bool is2pPronoun(Symbol);
	static bool is3pPronoun(Symbol);
	static bool isSingularPronoun(Symbol);
	static bool isPluralPronoun(Symbol);
	static bool isOtherPronoun(Symbol);
	static bool isWHQPronoun(Symbol);
	static bool isLinkingPronoun(Symbol);
	static bool isPERTypePronoun(Symbol);
	static bool isLOCTypePronoun(Symbol);
	static bool isPossessivePronoun(Symbol);
	static bool isMasculinePronoun(Symbol);
	static bool isFemininePronoun(Symbol);
	static bool isNeuterPronoun(Symbol);
	static bool isNonPersonPronoun(Symbol);
	static bool isRelativePronoun(Symbol);

	////////////////////////////////////////////////////////////
	// Articles (determiners)
	////////////////////////////////////////////////////////////
	static bool isDefiniteArticle(Symbol);
	static bool isIndefiniteArticle(Symbol);
	static bool isMasculineArticle(Symbol);
	static bool isFeminineArticle(Symbol);
	static bool isDeterminer(Symbol);

	////////////////////////////////////////////////////////////
	// Word shape:
	////////////////////////////////////////////////////////////
	static bool isNumeric(Symbol);
	static bool isAlphabetic(Symbol);
	static bool isSingleCharacter(Symbol);
	static bool startsWithDash(Symbol);
	static bool isPunctuation(Symbol);
	static bool isOpenBracket(Symbol);
	static bool isClosedBracket(Symbol);
	static bool isOpenDoubleBracket(Symbol);
	static bool isClosedDoubleBracket(Symbol);
	static bool isParenOrBracket(Symbol);
	static bool isAKA(Symbol);

	////////////////////////////////////////////////////////////
	// Stopwords
	////////////////////////////////////////////////////////////
	static bool isAcronymStopWord(Symbol);


 private:
	SpanishWordConstants(); // private constructor: this class may not be instantiated.
};

#endif


