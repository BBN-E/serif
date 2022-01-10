// Copyright 2013 Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ur_WORD_CONSTANTS_H
#define ur_WORD_CONSTANTS_H

#include "Generic/common/Symbol.h"
#include "Generic/common/WordConstants.h"

#include <string>

class UrduWordConstants {
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
	static Symbol WE;
	static Symbol YOU_FORMAL;
	static Symbol YOU_INFORMAL;
	static Symbol YOU_VINFORMAL;
	static Symbol IT;
	static Symbol THEY;

	static Symbol ME;
	static Symbol US;
	static Symbol YOUACC_INFORMAL;
	static Symbol YOUACC_VINFORMAL;
	static Symbol THEM_NEAR;
	static Symbol THEM_FAR;

	static Symbol MY;
	static Symbol OUR;
	static Symbol YOUR_FORMAL;
	static Symbol YOUR_INFORMAL;
	static Symbol YOUR_VINFORMAL;
	static Symbol ITS_NEAR;
	static Symbol ITS_FAR;
	static Symbol THEIR_NEAR;
	static Symbol THEIR_FAR;
	
	static Symbol MASC_TITLE_1;
	static Symbol FEM_TITLE_1;
	static Symbol FEM_TITLE_2;
	static Symbol FEM_TITLE_3;

	static Symbol WHO;
	static Symbol WHAT;
	static Symbol WHEN;
	static Symbol WHERE;
	static Symbol WHY;
	static Symbol HOW;

	/**
	  * Punctuation
	  */
	static Symbol UR_PERIOD;
	static Symbol UR_COMMA;
	static Symbol UR_QUESTION;
	static Symbol UR_SEMICOLON;
	static Symbol UR_ELLIPSIS;
	static Symbol LATIN_PERIOD;
	static Symbol LATIN_COMMA;
	static Symbol LATIN_QUESTION;
	static Symbol LATIN_SEMICOLON;

	//static Symbol ASCII_COMMA;
	//static Symbol NEW_LINE;
	//static Symbol CARRIAGE_RETURN;
	//static Symbol EOS_MARK;
	//static Symbol LATIN_STOP;		
	//static Symbol LATIN_EXCLAMATION;  
	//static Symbol LATIN_QUESTION;	
	//static Symbol FULL_STOP;
	//static Symbol FULL_COMMA;
	//static Symbol FULL_EXCLAMATION;
	//static Symbol FULL_QUESTION;
	//static Symbol FULL_SEMICOLON;
	//static Symbol FULL_RRB;
	
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


	// return false for undefined functions
	
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
	static bool isNameSuffix(Symbol) { return false; }
	static bool isHonorificWord(Symbol) { return false; }
	static bool isMilitaryWord(Symbol) { return false; }
	static bool isNameLinkStopWord(Symbol) { return false; }
	static bool isLocativePreposition(Symbol) { return false; }
	static bool isForReasonPreposition(Symbol) { return false; }
	static bool isOfActionPreposition(Symbol) { return false; }
	static bool isUnknownRelationReportedPreposition(Symbol) { return false; }
	static bool isPartitiveWord(Symbol) { return false; }
	static bool isCopula(Symbol) {return false; }
	static bool isTensedCopulaTypeVerb(Symbol) {return false; }
	static bool isCopulaForPassive(Symbol) {return false; }
	static bool isParenOrBracket(Symbol) {return false; }
	static bool isAKA(Symbol) {return false; }

	////////////////////////////////////////////////////////////
	// Stopwords
	////////////////////////////////////////////////////////////
	static bool isAcronymStopWord(Symbol) { return false; }
	////////////////////////////////////////////////////////////
	// Articles (determiners)
	////////////////////////////////////////////////////////////
	static bool isDefiniteArticle(Symbol) { return false; }
	static bool isIndefiniteArticle(Symbol){ return false; }
	static bool isMasculineArticle(Symbol){ return false; }
	static bool isFeminineArticle(Symbol){ return false; }
	static bool isDeterminer(Symbol){ return false; }


	////////////////////////////////////////////////////////////
	// Pronouns, undefined
	////////////////////////////////////////////////////////////
	static bool isReflexivePronoun(Symbol) {return false; }
	static bool isRelativePronoun(Symbol) {return false; }
	static bool isOtherPronoun(Symbol) { return false; }
	static bool isPERTypePronoun(Symbol) { return false; }
	static bool isLOCTypePronoun(Symbol) { return false; }
	static bool isMasculinePronoun(Symbol) { return false; }
	static bool isFemininePronoun(Symbol) { return false; }
	static bool isNeuterPronoun(Symbol) { return false; }

	////////////////////////////////////////////////////////////
	// Titles
	////////////////////////////////////////////////////////////
	static bool isTitle(Symbol);
	static bool isFeminineTitle(Symbol);
	static bool isMasculineTitle(Symbol);
	

	////////////////////////////////////////////////////////////
	// Pronouns
	////////////////////////////////////////////////////////////
	static bool isPronoun(Symbol);
	static bool is2pPronoun(Symbol);
	static bool is3pPronoun(Symbol);
	static bool is1pPronoun(Symbol);
	static bool isSingular1pPronoun(Symbol);
	static bool isSingularPronoun(Symbol);
	static bool isPluralPronoun(Symbol);
	static bool isWHQPronoun(Symbol);
	static bool isLinkingPronoun(Symbol);
	static bool isSubjectPronoun(Symbol);
	static bool isObjectPronoun(Symbol);
	static bool isPossessivePronoun(Symbol);
	

	// Not implemented yet:
	static bool isNonPersonPronoun(Symbol) { return false; }


	////////////////////////////////////////////////////////////
	// Word shape:
	////////////////////////////////////////////////////////////
	static bool isOrdinal(Symbol);
	static bool isLowNumberWord(Symbol);
	static bool isNumeric(Symbol);
	static bool isAlphabetic(Symbol);
	static bool isSingleCharacter(Symbol);
	static bool startsWithDash(Symbol);
	static bool isPunctuation(Symbol);
	static bool isOpenBracket(Symbol);
	static bool isClosedBracket(Symbol);
	static bool isOpenDoubleBracket(Symbol);
	static bool isClosedDoubleBracket(Symbol);



 private:
	UrduWordConstants(); // private constructor: this class may not be instantiated.
};

#endif


