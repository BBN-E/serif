// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_WORD_CONSTANTS_H
#define ch_WORD_CONSTANTS_H

#include "Generic/common/Symbol.h"
#include "Generic/common/WordConstants.h"

// place static symbols here that may be used by other classes 
// in various Chinese specific areas.

class ChineseWordConstants {
	// Note: this class is intentionally not a subclass of
	// WordConstants.  See WordConstants.h for explanation.
public:

	// it's nice to see what these constants are originally intended to 
	// be used for, even if they end up getting used by multiple things

	/**
	 * constants used for guessing type, number, and gender of SynNodes, as 
	 * well as identifying pronominals
	 */
	static Symbol I;
	static Symbol MY;
	static Symbol YOU;
	static Symbol YOUR;
	static Symbol WE;
	static Symbol OUR;
	static Symbol YOU_PL;
	static Symbol YOUR_PL;
	static Symbol HE;
	static Symbol HIS;
	static Symbol SHE;
	static Symbol HER; 
	static Symbol IT;
	static Symbol ITS;
	static Symbol POSS;
	static Symbol THEY_INANIMATE;
	static Symbol THEY_FEM;
	static Symbol THEY_MASC;
	static Symbol THEIR_INANIMATE;
	static Symbol THEIR_FEM;
	static Symbol THEIR_MASC;

	static Symbol YOU_FORMAL;
	static Symbol THIRD_PERSON;
	static Symbol WHO;
	static Symbol REFLEXIVE;
	static Symbol COUNTERPART;
	static Symbol HERE;
	static Symbol THIS;
	static Symbol EACH;
	static Symbol LITERARY_THIRD_PERSON;
	static Symbol ITSELF;
	static Symbol THERE;
	static Symbol MYSELF;
	static Symbol ONESELF;
	static Symbol LATTER;
	
	static Symbol MASC_TITLE_1;
	static Symbol MASC_TITLE_2;
	static Symbol MASC_TITLE_3;
	static Symbol FEM_TITLE_1;
	static Symbol FEM_TITLE_2;
	static Symbol FEM_TITLE_3;
	static Symbol FEM_TITLE_4;
	static Symbol FEM_TITLE_5;

	/**
	  * Partitive word(s) used by ch_CompoundMentionFinder and LangaugeSpecificFunctions::modifyParse
	  */
	static Symbol ONE_OF;

	/**
	  * Generic signal word(s) used by ch_GenericsFilter
	  */
	static Symbol MANY1;
	static Symbol MANY2;				
	static Symbol QUITE_LARGE;		
	static Symbol SOME;				
	static Symbol APPROXIMATELY;		
	static Symbol MOST;			
	static Symbol NUMBER;	
	static Symbol PLURAL;	

	/**
	  * Punctuation
	  */
	static Symbol CH_SPACE;
	static Symbol CH_COMMA;
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

	static Symbol CLOSE_PUNCT_1;
	static Symbol CLOSE_PUNCT_2;
	static Symbol CLOSE_PUNCT_3;
	static Symbol CLOSE_PUNCT_4;
	static Symbol CLOSE_PUNCT_5;

	//the following function delimits the class of pronouns that we care about
	static bool isPronoun(Symbol word) {
		return is1pPronoun(word) ||
			   is2pPronoun(word) ||
			   is3pPronoun(word) ||
			   isOtherPronoun(word);
	}
	static bool is1pPronoun(Symbol word);
	static bool is2pPronoun(Symbol word);
	static bool is3pPronoun(Symbol word);
	static bool isSingular1pPronoun(Symbol word);
	static bool isOtherPronoun(Symbol word);
	static bool isLinkingPronoun(Symbol word);
	static bool isSingularPronoun(Symbol word);
	static bool isPluralPronoun(Symbol word);
	static bool isPossessivePronoun(Symbol word);

	static bool isPartitiveWord(Symbol word);

	static bool isURLCharacter(Symbol word);
	static bool isPhoneCharacter(Symbol word);
	static bool isASCIINumericCharacter(Symbol word);

	static bool isNonPersonPronoun(Symbol word) {
		return (word == IT || word == ITS); }

	// The following methods have not been defined for Chinese yet, so
	// we just return the default value (false).
	static bool isNumeric(Symbol) { return false; }
	static bool isAlphabetic(Symbol) { return false; }
	static bool isSingleCharacter(Symbol) { return false; }
	static bool startsWithDash(Symbol) { return false; }
	static bool isOrdinal(Symbol) { return false; }
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
	static bool isReflexivePronoun(Symbol) { return false; }
	static bool isRelativePronoun(Symbol) { return false; }
	static bool isWHQPronoun(Symbol) { return false; }
	static bool isPERTypePronoun(Symbol) { return false; }
	static bool isLOCTypePronoun(Symbol) { return false; }
	static bool isLocativePreposition(Symbol) { return false; }
	static bool isForReasonPreposition(Symbol) { return false; }
	static bool isOfActionPreposition(Symbol) {return false; }
	static bool isUnknownRelationReportedPreposition(Symbol) { return false; }
	static bool isPunctuation(Symbol) { return false; }
	static bool isOpenDoubleBracket(Symbol) { return false; }
	static bool isClosedDoubleBracket(Symbol) { return false; }
	static bool isDeterminer(Symbol) { return false; }
    static bool isDefiniteArticle(Symbol word) { return false; }
    static bool isIndefiniteArticle(Symbol word) { return false; }
};

#endif


