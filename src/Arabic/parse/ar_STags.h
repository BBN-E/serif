// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_ARABIC_STAGS_H
#define AR_ARABIC_STAGS_H

#include "Generic/parse/STags.h"
#include "Generic/common/Symbol.h"

class ArabicSTags {
	// Note: this class is intentionally not a subclass of
	// STags.  See STags.h for explanation.
public:
	//new POS tag set is more similar to the 
	//UPenn conversion (it also more closely 
	//mirrors Eng POS
	//UNKNOWN occurs in treebank, 
	static Symbol UNKNOWN;
	//Punctuation
    static Symbol PUNC;
    static Symbol BSLASH;
    static Symbol LRB;
    static Symbol RRB;
	static Symbol SEMICOLON;
    static Symbol COLON;
    static Symbol FSLASH;
	static Symbol PLUS;
	static Symbol QUESTION;
	static Symbol DOT; 
    static Symbol HYPHEN; 
	static Symbol COMMA;
	static Symbol NUMERIC_COMMA;
	static Symbol DQUOTE;
	static Symbol SQUOTE;
	static Symbol EX_POINT;
	static Symbol PERCENT;

	static Symbol DATE;

	//Nouns- ignore gender, keep pl/sg distinction
	//add definite marker on POS Tag
	static Symbol NNS;
	static Symbol DET_NNS;
	static Symbol NN;
	static Symbol DET_NN;
	static Symbol NNPS;
	static Symbol DET_NNPS;
	static Symbol NNP;
	static Symbol DET_NNP;
	static Symbol CD;

	//Verbs- ignore agreement features, use english style tags
	static Symbol VBN;
	static Symbol VBP;
	static Symbol VBD;
	static Symbol VB;

	// was changed in ATB_V3.2 (instead of having nouns as heads of VP)
/*	static Symbol DV; */
	// was changed in ATB_V3.3
/*	static Symbol NQ;
	static Symbol DET_NQ;
 	static Symbol JJ_NUM;
 	static Symbol DET_JJ_NUM;
 	static Symbol JJ_COMP;
 	static Symbol DET_JJ_COMP;
*/
	//Pronouns include 
	//direct object and subject pronouns
	//that are attached to the verb
	static Symbol WP;
	static Symbol PRP;
	static Symbol PRP_POS;

	//Adjectives include definiteness
	static Symbol JJ;
	static Symbol DET_JJ;

	//more
	static Symbol RB;
	static Symbol DT;
	static Symbol RP;
	static Symbol CC;
	static Symbol IN;
	static Symbol UH;
	static Symbol VPN;

	//Phrase Level
    static Symbol SBAR;
    static Symbol PRT;
    static Symbol PRN;
    static Symbol ADJP;
    static Symbol FRAG;
	static Symbol FRAGMENTS;
    static Symbol WHNP;
    static Symbol LST;
    static Symbol QP;
    static Symbol S;
    static Symbol NP;
    static Symbol NX;
    static Symbol VP;
    static Symbol ADVP;
    static Symbol CONJP;
    static Symbol PP;
	static Symbol NPA;
	static Symbol NPP;
	static Symbol S_ADV;
	static Symbol INTJ;

	//automatically add these, will they help
	static Symbol DEF_NP;
	static Symbol DEF_NPA;

	//added to handel Bikel head rules
	static Symbol SINV;
	static Symbol SQ;
	static Symbol WHADJP;
	static Symbol WHADVP;
	static Symbol WHPP;

	static Symbol WDT;
	static Symbol WP$;
	static Symbol DOLLAR;
	static Symbol NAC;
	static Symbol RRC;
	static Symbol UCP;
	static Symbol SBARQ;

	static Symbol X;
	static Symbol NON_ALPHABETIC;



	/*
	  These following functions test for "features" of tags.
	  Please update when Arabic tag set changes
	*/
	static bool isPunctuation(Symbol tag) {
		return (
			(tag == DQUOTE) ||
			(tag == PUNC) ||
			(tag == BSLASH) ||
			(tag == LRB) ||
			(tag == SEMICOLON) ||
			(tag == COLON) ||
			(tag == NUMERIC_COMMA) ||
			(tag == RRB) ||
			(tag == FSLASH) ||
			(tag == PLUS) ||
			(tag == QUESTION) ||
			(tag == DOT) ||
			(tag == HYPHEN) ||
			(tag == COMMA) ||
			(tag == EX_POINT)
			);
	};

	static bool isPreposition(Symbol tag){
		return tag == IN;
	};
			 
    static bool isConjunction(Symbol tag){
		return ( 
			(tag == CC) ||
			(tag == CONJP)
			);
	};
	
	static bool isNoun(Symbol tag){
		return ( 
			(tag == NN) ||
			(tag == NNS) ||
			(tag == NNP) ||
			(tag == NNPS) ||
			(tag == DET_NN) ||
			(tag == DET_NNS) ||
			(tag == DET_NNP) ||
			(tag == DET_NNPS)

			);
	};

	static bool isAdjective(Symbol tag){
		return ( 
			(tag == JJ) ||
			(tag == DET_JJ)
			);
	};
	static bool isPossPronoun(Symbol tag){
		return (tag ==  PRP_POS);
				


	}
	static bool isPronoun(Symbol tag){
		return (tag == PRP || 
			tag == PRP_POS ||
			tag == WP);
	};
	static Symbol convertDictPOSToParserPOS(Symbol dict_pos);

	static void initializeTagList(std::vector<Symbol> tags);

	private:
		static Symbol isNNFromBW(Symbol dict_pos);
	//these functions won't work PENN POS tagset
	/*
	static bool isMalePronPOS(Symbol tag){
		if(!isPronoun(tag)) return false;
		const wchar_t* tagstr = tag.to_string();
		int len = static_cast<int>(wcslen(tagstr));
		if(len < 2) return false;
		return tagstr[len-2] == L'M';

	};
	static bool isFemalePronPOS(Symbol tag){
		if(!isPronoun(tag)) return false;
		const wchar_t* tagstr = tag.to_string();
		int len = static_cast<int>(wcslen(tagstr));
		if(len < 2) return false;
		return tagstr[len-2] == L'F';
	

				
	}
	static bool isPluralPronPOS(Symbol tag){
		if(!isPronoun(tag)) return false;
		const wchar_t* tagstr = tag.to_string();
		int len = static_cast<int>(wcslen(tagstr));
		if(len < 2) return false;
		return ((tagstr[len-1] == L'P')|| (tagstr[len-1] == L'D'));
	}
	static bool isSingularPronPOS(Symbol tag){
		if(!isPronoun(tag)) return false;
		const wchar_t* tagstr = tag.to_string();
		int len = static_cast<int>(wcslen(tagstr));
		if(len < 2) return false;
		return ((tagstr[len-1] == L'S'));
	}

	static int getPronPOSPerson(Symbol tag){
		if(!isPronoun(tag)) return false;
		const wchar_t* tagstr = tag.to_string();
		int len = static_cast<int>(wcslen(tagstr));
		if(len < 3) return false;
		if((tagstr[len-3] == L'3')){
			return 3;
		}
		else if((tagstr[len-3] == L'2')){
			return 2;
		}
		else if((tagstr[len-3] == L'1')){
			return 1;
		}
		return 0;
	}
	*/
};
#endif
