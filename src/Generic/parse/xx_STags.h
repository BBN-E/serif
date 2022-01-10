// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_STAGS_H
#define XX_STAGS_H

#include "Generic/common/Symbol.h"
#include "Generic/parse/STags.h"

class WordNet;

class GenericSTags {
	// Note: this class is intentionally not a subclass of
	// STags.  See STags.h for explanation.
 public:
    static Symbol COMMA;
	static Symbol DATE;
	static Symbol NP;
	static Symbol NPA;
	static Symbol PP;
	static void initializeTagList(std::vector<Symbol> tags);
};


namespace enAsGen {
/**
 * ENGLISH_AS_GENERIC
 * This class was copied from English into generic to support WordNet;
 * All members are declared as private, with WordNet as a friend class.
 **/
class EnglishSTagsForWordnet {
	friend class ::WordNet;
private:
	static Symbol TOP;
	static Symbol TOPTAG;
	static Symbol COMMA;
	static Symbol COLON;
	static Symbol DOT;
	static Symbol QMARK;
	static Symbol EX_POINT;
	static Symbol DASH;
	static Symbol HYPHEN;
	static Symbol SEMICOLON;
	static Symbol DOLLAR;
	static Symbol DUMMY;
	static Symbol ADJP;
	static Symbol ADVP;
	static Symbol CC;
	static Symbol CD;
	static Symbol CONJP;
	static Symbol DT;
	static Symbol FRAG;
	static Symbol EX;
	static Symbol FW;
	static Symbol IN;
	static Symbol INTJ;
	static Symbol JJ;
	static Symbol JJR;
	static Symbol JJS;
	static Symbol LS;
	static Symbol LST;
	static Symbol MD; 
	static Symbol NAC;
	static Symbol NCD;
	static Symbol NN; 
	static Symbol NNP;
	static Symbol NNPS;
	static Symbol NNS; 
	static Symbol NP; 
	static Symbol NPA;
	static Symbol NPP;
	static Symbol NPPOS;
	static Symbol NPPRO;
	static Symbol NX;
	static Symbol POS; 
	static Symbol PP;
	static Symbol PRN;
	static Symbol PRP;
	static Symbol PRPS; // PRP$ -- possessive pronoun
	static Symbol PRT;
	static Symbol QP; 
	static Symbol RB; 
	static Symbol RP; 
	static Symbol RBR;
	static Symbol RBS;
	static Symbol RRC;
	static Symbol S; 
	static Symbol SBAR;
	static Symbol SBARQ;
	static Symbol SINV; 
	static Symbol SQ; 
	static Symbol TO; 
	static Symbol UCP;
	static Symbol VB; 
	static Symbol VBD;
	static Symbol VBG;
	static Symbol VBN;
	static Symbol VBP;
	static Symbol VBZ;
	static Symbol VP; 
	static Symbol WDT;
	static Symbol WHADJP;
	static Symbol WHADVP;
	static Symbol WHNP;
	static Symbol WHPP;
	static Symbol WP;
	static Symbol WPDOLLAR;
	static Symbol WRB;
	static Symbol X;
	static Symbol DATE;
	
	// These labels are used by the trainer mrf
	static Symbol LOCATION_NNP;
	static Symbol PERSON_NNP;
	static Symbol ORGANIZATION_NNP;
	static Symbol PERCENT_NNP;
	static Symbol TIME_NNP;
	static Symbol DATE_NNP;
	static Symbol MONEY_NNP;
	static Symbol LOCATION_NNPS;
	static Symbol PERSON_NNPS;
	static Symbol ORGANIZATION_NNPS;
	static Symbol PERCENT_NNPS;
	static Symbol TIME_NNPS;
	static Symbol DATE_NNPS;
	static Symbol MONEY_NNPS;
	static Symbol NPP_NNP;
	static Symbol NPP_NNPS;
	static Symbol PERSON;
	static Symbol ORGANIZATION;
	static Symbol LOCATION;
	static Symbol PDT;
	static Symbol UH;
};
}

#endif
