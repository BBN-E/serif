// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_WORD_CONSTANTS_H
#define en_WORD_CONSTANTS_H

#include "Generic/common/Symbol.h"
#include "Generic/common/WordConstants.h"

#include <string>

// place static symbols here that may be used by other classes 
// in various English specific areas.

// TODO: move other english constants 
// (e.g. stuff from compoundMentionFinder) to here!

class EnglishWordConstants {
	// Note: this class is intentionally not a subclass of
	// WordConstants.  See WordConstants.h for explanation.
public:

	// it's nice to see what these constants are originally intended to 
	// be used for, even if they end up getting used by multiple things

	/** 
	 * constants used for event and relation finding
	 */
	static Symbol COULD;
	static Symbol SHOULD;
	static Symbol MIGHT;
	static Symbol MAY;
	static Symbol WHETHER;
	static Symbol WHERE;
	static Symbol JUST;

	/**
	 * constants used for definiteness checks
	 */
	static Symbol THE;
	static Symbol THIS;
	static Symbol THAT; // also used for event and relation finding
	static Symbol THESE;
	static Symbol THOSE;

	/**
	 * constant used for determining hypotheticalness
	 */
	static Symbol IF; // also used for event and relation finding

	/**
	 * constants used for guessing type, number, and gender of SynNodes
	 */
	static Symbol HE;
	static Symbol HIM;
	static Symbol HIS;
	static Symbol SHE;
	static Symbol HER; 
	static Symbol IT;
	static Symbol ITS;
	static Symbol THEY;
	static Symbol THEM; 
	static Symbol THEIR;
	static Symbol I;
	static Symbol WE;
	static Symbol ME;
	static Symbol US;
	static Symbol YOU;
	static Symbol MY;
	static Symbol OUR;
	static Symbol YOUR;

	// reflexive pronouns
	static Symbol MYSELF;
	static Symbol YOURSELF;
	static Symbol HIMSELF;
	static Symbol HERSELF;
	static Symbol OURSELVES;
	static Symbol YOURSELVES;
	static Symbol THEMSELVES;

	// "WHQ" pronouns
	static Symbol WHO;
	static Symbol WHOM;
	static Symbol WHOSE;
	static Symbol WHICH;
	// THAT, WHERE (already a symbol here)

	// these are considered pronouns
	static Symbol HERE;
	static Symbol THERE;
	static Symbol ABROAD;
	static Symbol OVERSEAS;
	static Symbol HOME;

	static Symbol MR;
	static Symbol MRS;
	static Symbol MS;
	static Symbol MISS;

	/**
	 * if these words are in the premods of a descriptor, that descriptor won't link
	 */
	static Symbol OTHER;
	static Symbol ADDITIONAL;
	static Symbol EARLIER;
	static Symbol PREVIOUS;
	static Symbol FORMER;
	static Symbol ANOTHER;
	static Symbol MANY;
	static Symbol FEW;
	static Symbol A;
	static Symbol SEVERAL;
	static Symbol AN; // added 3/15/13
	
	//copulas: is, are, was, were, will, would, has, had, have, 's, 'd, 'll, 've, 're, wo(n't)
	static Symbol IS;
	static Symbol ARE;
	static Symbol WAS;
	static Symbol WERE;
	static Symbol WILL;
	static Symbol WOULD;
	static Symbol HAS;
	static Symbol HAD;
	static Symbol HAVE;
	static Symbol _S;
	static Symbol _D;
	static Symbol _LL;
	static Symbol _VE;
	static Symbol _re;
	static Symbol WO_;

	//"stop words" for name linking: i.e., words which are irrelevant to the NameLinker's lexical modeler
	//TODO: fill in

	//"stop words" for acronym generation
	
	//static Symbol THE;
	//static Symbol A;
	static Symbol AND;
	static Symbol OF;
	static Symbol CORP_;
	static Symbol LTD_;
	static Symbol CORP;
	static Symbol INC_;
	static Symbol INC;
	static Symbol LTD;
	static Symbol _COMMA_;
	static Symbol _HYPHEN_;
	//static Symbol _S;
	static Symbol PLC;
	static Symbol PLC_;

	// used in faking name lists
	static Symbol OR;

	// used by TemporalIdentifier
	static Symbol BEGINNING;
	static Symbol START;
	static Symbol END;
	static Symbol CLOSE;
	static Symbol MIDDLE;

	static Symbol AKA1;
	static Symbol AKA2;
	static Symbol AKA3;
	static Symbol AKA4;

	// First Name Unknown, Last Name Unknown
	static Symbol LNU;
	static Symbol FNU;

	// used by documentrelations
	static Symbol CONSIST;
	static Symbol CONTAIN;
	static Symbol MAKE;
	static Symbol COMPRISE;
	static Symbol AS;
	static Symbol FOLLOW;
	static Symbol FOLLOWING;
	static Symbol ONE;
	static Symbol PERIOD;
	static Symbol UPPER_A;
	static Symbol FOR;
	static Symbol ORGANIZE;
	static Symbol APPEAR;

	// military units hierarchy
	static Symbol ARMY;
	static Symbol CORPS;
	static Symbol DIVISION;
	static Symbol BRIGADE;
	static Symbol BATTALION;
	static Symbol COMPANY;
	static Symbol SQUAD;

	// unknown relation propositions
	static Symbol in;
	static Symbol ON;
	static Symbol AT;
	static Symbol WITH;
	static Symbol BY;
	static Symbol FROM;
	static Symbol ABOUT;
	static Symbol INTO;
	static Symbol AFTER;
	static Symbol OVER;
	static Symbol SINCE;
	static Symbol UNDER;
	static Symbol LIKE;
	static Symbol BEFORE;
	static Symbol UNTIL;
	static Symbol DURING;
	static Symbol THROUGH;
	static Symbol AGAINST;
	static Symbol BETWEEN;
	static Symbol WITHOUT;
	static Symbol BELOW;

	// time modifiiers
	static Symbol AM1;
	static Symbol AM2;
	static Symbol AM3;
	static Symbol PM1;
	static Symbol PM2;
	static Symbol PM3;
	static Symbol GMT;
	static Symbol OLD;

	static Symbol YEAR;
	static Symbol MONTH;
	static Symbol DAY;
	static Symbol HOUR;
	static Symbol MINUTE;
	static Symbol SECOND;

	static Symbol TWO;
	static Symbol THREE;
	static Symbol FOUR;
	static Symbol FIVE;
	static Symbol SIX;
	static Symbol SEVEN;
	static Symbol EIGHT;
	static Symbol NINE;
	static Symbol TEN;
	static Symbol ELEVEN;
	static Symbol TWELVE;

	// For English PER specific words - suffixes (used in edt)
	static Symbol DOT;

	static Symbol JUNIOR;
	static Symbol JR;
	static Symbol SR;
	static Symbol SNR;
	static Symbol JR_;
	static Symbol SR_;
	static Symbol SNR_;
	static Symbol FIRST;
	static Symbol FIRST_;
	static Symbol FIRST_I;
	static Symbol FIRST_I_;
	static Symbol SECOND_2ND;
	static Symbol SECOND_;
	static Symbol SECOND_II;
	static Symbol SECOND_II_;
	static Symbol THIRD;
	static Symbol THIRD_;
	static Symbol THIRD_III;
	static Symbol FORTH;
	static Symbol FORTH_;
	static Symbol FORTH_IV;
	static Symbol FIVTH;
	static Symbol TENTH;

	// For English PER specific words - honorary (used in edt)
	static Symbol DR;
	static Symbol DR_;
	static Symbol PRESIDENT;
	static Symbol POPE;
	static Symbol RABBI;
	static Symbol ST;
	static Symbol ST_;
	static Symbol MISTER;
	static Symbol MR_L;
	static Symbol MR_;
	static Symbol MS_L;
	static Symbol MISS_L;
	static Symbol MISS_L_;
	static Symbol MS_;
	static Symbol MIS_L;
	static Symbol MIS_;
	static Symbol MRS_L;
	static Symbol MRS_;
	static Symbol IMAM;
	static Symbol FATHER;
	static Symbol REV;
	static Symbol REV_;
	static Symbol SISTER;
	static Symbol AYATOLLAH;
	static Symbol SHAH;
	static Symbol EMPEROR;
	static Symbol PRIME;
	static Symbol PRIME_;
	static Symbol PRIME_MINISTER;
	static Symbol MINISTER;
	static Symbol MINISTER_;
	static Symbol PRINCE;
	static Symbol PRINCESS;
	static Symbol KING;
	static Symbol LORD;
	static Symbol MAJESTY;
	static Symbol SIR;

	static Symbol SENATOR;
	static Symbol SENATOR_;
	static Symbol GOVERNOR;
	static Symbol GOVERNOR_;
	static Symbol CONGRESSMAN;
	static Symbol CONGRESSWOMAN;

	static Symbol GENERAL;
	static Symbol COLONEL;
	static Symbol BRIGADIER;
	static Symbol FOREIGN;
	static Symbol REPUBLICAN;
	static Symbol DEMOCRAT;
	static Symbol CHIEF;
	static Symbol JUSTICE;
	static Symbol COACH;
	static Symbol VICE;
	static Symbol CHANCELLOR;
	static Symbol JUDGE;
	static Symbol AMBASSADOR;
	static Symbol REPRESENTATIVE;
	static Symbol ATTORNEY;
	static Symbol DEPUTY;
	static Symbol SPOKESMAN;
	static Symbol SPOKESWOMAN;
	static Symbol SHEIKH;
	static Symbol HEAD;
	static Symbol QUEEN;
	static Symbol PROFESSOR;
	static Symbol PROFESSOR_;
	static Symbol LATE;
	static Symbol OFFICIAL;
	static Symbol LEADER;
	static Symbol LEUTENANT;
	static Symbol LEUTENANT2;
	static Symbol LEUTENANT_;
	static Symbol SARGENT;
	static Symbol SARGENT_;
	static Symbol CMDR;
	static Symbol CMDR_;
	static Symbol ADMIRAL;
	static Symbol SECRETARY;
	static Symbol HERO;
	static Symbol REPORTER;
	static Symbol ECONOMIST;
	static Symbol M_;

	static Symbol LEFT_BRACKET;
	static Symbol RIGHT_BRACKET;
	static Symbol LEFT_CURLY_BRACKET;
	static Symbol RIGHT_CURLY_BRACKET;
	static Symbol DOUBLE_LEFT_BRACKET;
	static Symbol DOUBLE_RIGHT_BRACKET;
	static Symbol DOUBLE_LEFT_BRACKET_UC;
	static Symbol DOUBLE_RIGHT_BRACKET_UC;

	// Used by appositives ("as well as")
	static Symbol WELL;

	static bool isNumeric(Symbol word);
	static bool isAlphabetic(Symbol word);
	static bool isSingleCharacter(Symbol word);
	static bool startsWithDash(Symbol word);

	static bool isCopula(Symbol word);
	static bool isTensedCopulaTypeVerb(Symbol);

	static bool isNameLinkStopWord(Symbol word);
	static bool isAcronymStopWord(Symbol word);

	static bool isPronoun(Symbol word) {
		return (is1pPronoun(word) ||
				is2pPronoun(word) ||
				is3pPronoun(word) ||
				isReflexivePronoun(word));
	}
	static bool isReflexivePronoun(Symbol word);
	static bool is1pPronoun(Symbol word);
	static bool isSingular1pPronoun(Symbol word);
	static bool is2pPronoun(Symbol word);
	static bool is3pPronoun(Symbol word);
	static bool isWHQPronoun(Symbol word);
	static bool isLinkingPronoun(Symbol word) { 
		return (is3pPronoun(word) || isWHQPronoun(word)); 
	}
	static bool isPERTypePronoun(Symbol word);
	static bool isLOCTypePronoun(Symbol word);
	static bool isSingularPronoun(Symbol word);
	static bool isPluralPronoun(Symbol word);
	static bool isPossessivePronoun(Symbol word);
	static bool isNonPersonPronoun(Symbol word);
	static bool isRelativePronoun(Symbol word);

	static bool isPunctuation(Symbol word);

	static bool isOrdinal(Symbol word);
	static bool isMilitaryWord(Symbol word);
	static bool isUnknownRelationReportedPreposition(Symbol word);

	static bool isLocativePreposition(Symbol);
	static bool isForReasonPreposition(Symbol);
	static bool isOfActionPreposition(Symbol);

	static bool isDailyTemporalExpression(Symbol);
	
	static Symbol getNumericPortion(Symbol word);
	
	static bool isTimeExpression(Symbol word);
	static bool isTimeModifier(Symbol word);
	static bool isFourDigitYear(Symbol word);
	static bool isDashedDuration(Symbol word);
	static bool isDecade(Symbol word);
	static bool isDateExpression(Symbol word);
	static bool isLowNumberWord(Symbol word);
		
	static bool isNameSuffix(Symbol sym);
	static bool isHonorificWord(Symbol sym);

	static bool isDeterminer(Symbol word);

	static bool isOpenDoubleBracket(Symbol word);
	static bool isClosedDoubleBracket(Symbol word);

	static bool isDefiniteArticle(Symbol word);
	static bool isIndefiniteArticle(Symbol word);

	// The following methods have not been defined for English, so
	// we just return the default value (false).
	static bool isURLCharacter(Symbol) { return false; }
	static bool isPhoneCharacter(Symbol) { return false; }
	static bool isASCIINumericCharacter(Symbol) { return false; }
	static bool isOtherPronoun(Symbol) { return false; }
	static bool isPartitiveWord(Symbol) { return false; }
private:
	EnglishWordConstants(); // private constructor: this class may not be instantiated.

	// todo: replace these with boost calls.
	static std::wstring toLower(std::wstring str);
	static std::wstring stringReplace(std::wstring str, std::wstring before, std::wstring after);
};

#endif


