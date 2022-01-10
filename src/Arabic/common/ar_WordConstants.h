// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_WORD_CONSTANTS_H
#define ar_WORD_CONSTANTS_H

#include "Generic/common/Symbol.h"
#include "Generic/common/WordConstants.h"

// place static symbols here that may be used by other classes 
// in various Arabic specific areas.

class ArabicWordConstants {
	// Note: this class is intentionally not a subclass of
	// WordConstants.  See WordConstants.h for explanation.
private:
	//M= masculine, F= Feminine, S = singular, P=plural, D=Dual,
	static Symbol I_1;
	static Symbol I_2;
	static Symbol YOU_S_1;
	static Symbol YOU_S_2;
	static Symbol YOU_D_1;
	static Symbol YOU_D_2;
	static Symbol YOU_MP_1;
	static Symbol YOU_FP_1;
	static Symbol YOU_MP_2;
	static Symbol YOU_FP_2;
	static Symbol HE;
	static Symbol SHE;
	static Symbol THEY_M;
	static Symbol THEY_D;
	static Symbol THEY_F;
	//new clitic symbols
	static Symbol MY;
	static Symbol OUR;
	static Symbol YOUR;
	static Symbol YOUR_P1;
	static Symbol YOUR_P2;
	static Symbol YOUR_D;
	static Symbol HIS;
	static Symbol HER;
	static Symbol THEIR;
	//prefix clitics
	static Symbol L;
	static Symbol B;
	static Symbol W;
	static Symbol F;
	static Symbol K;

	//The ACE2005 annotation disagrees with the BBN Annotation about the annotation of things such as
	//	- city of Boston and company Microsoft Corp.
	//in both cases BBN marks and name and desc, but the LDC marks the entire phrase as a name
	//I am fixing this with a list of words (city, country, company,  that attach to the name that
	//follows them when that name is of the correct type. 
	//the same word need to be removed from names in the name linker :(
	static Symbol _gpe_words[13];
	static Symbol _fac_words[29];
	static Symbol _org_words[27];
	static Symbol _loc_words[1];


public:
	static Symbol MASC;
	static Symbol FEM;
	static Symbol UNKNOWN;
	static Symbol SINGULAR;
	static Symbol PLURAL;
	static Symbol DUAL;

	////////////////////////////////////////////////////////////////////
	// These are all Arabic-specific (not defined in WordConstants.h):
	////////////////////////////////////////////////////////////////////
	static Symbol guessGender(Symbol word);
	static Symbol guessNumber(Symbol word);
	static Symbol removeAl(Symbol word);
	static int getFirstLetterAlefVariants(Symbol word, Symbol* variants, int max);
	static int getLastLetterAlefVariants(Symbol word, Symbol* variants, int max);
	static Symbol getNationalityStemVariant(Symbol word);
	static int getPossibleStems(Symbol word, Symbol* variants, int max);
	static bool matchGPEWord(Symbol hw);
	static bool matchFACWord(Symbol hw);
	static bool matchORGWord(Symbol hw);
	static bool matchLOCWord(Symbol hw);
	static bool isDiacritic(wchar_t c);
	static bool isPrefixClitic(Symbol word);
	static bool isNonKeyCharacter(wchar_t c);
	static bool isEquivalentChar(wchar_t a, wchar_t b);
	
	////////////////////////////////////////////////////////////////////
	// These methods correspond to methods in WordConstants.h:
	////////////////////////////////////////////////////////////////////

	//the following function delimits the class of pronouns that we care about
	static bool isPronoun(Symbol word) {
		return is1pPronoun(word) ||
			   is2pPronoun(word) ||
			   is3pPronoun(word);
	}
	static bool is1pPronoun(Symbol word);
	static bool is2pPronoun(Symbol word);
	static bool is3pPronoun(Symbol word);
	static bool isSingular1pPronoun(Symbol word);
	static bool isLinkingPronoun(Symbol word) { return is3pPronoun(word); }

	// The following methods have not been defined for Arabic yet, so
	// we just return the default value (false).
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
	static bool isTensedCopulaTypeVerb(Symbol){ return false;}
	static bool isNameLinkStopWord(Symbol) { return false; }
	static bool isAcronymStopWord(Symbol) { return false; }
	static bool isReflexivePronoun(Symbol) { return false; }
	static bool isRelativePronoun(Symbol) { return false; }
	static bool isOtherPronoun(Symbol) { return false; }
	static bool isWHQPronoun(Symbol) { return false; }
	static bool isPERTypePronoun(Symbol) { return false; }
	static bool isLOCTypePronoun(Symbol) { return false; }
	static bool isSingularPronoun(Symbol) { return false; }
	static bool isPluralPronoun(Symbol) { return false; }
	static bool isPossessivePronoun(Symbol) { return false; }
	static bool isNonPersonPronoun(Symbol) { return false; }
	static bool isLocativePreposition(Symbol) { return false; }
	static bool isForReasonPreposition(Symbol) { return false; }
	static bool isOfActionPreposition(Symbol) {return false; }
	static bool isUnknownRelationReportedPreposition(Symbol) { return false; }
	static bool isPunctuation(Symbol) { return false; }
	static bool isOpenDoubleBracket(Symbol) { return false; }
	static bool isClosedDoubleBracket(Symbol) { return false; }
	static bool isPartitiveWord(Symbol) { return false; }
	static bool isDeterminer(Symbol) { return false; }
    static bool isDefiniteArticle(Symbol word) { return false; }
    static bool isIndefiniteArticle(Symbol word) { return false; }
private:
	static bool matchWordToArray(Symbol word, Symbol* array, int max);
	static Symbol _nationalityEndings[12];
	static const int _nEndings = 12;
	static bool endsWith(const wchar_t* word, Symbol end, size_t index);

};

#endif
