// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KR_STAGS_H
#define KR_STAGS_H

#include "Generic/parse/STags.h"
#include "Generic/common/Symbol.h"

class KoreanSTags {
	// Note: this class is intentionally not a subclass of
	// STags.  See STags.h for explanation.
public:
	static void initializeTagList(std::vector<Symbol> tags);

	static Symbol TOP;
	static Symbol TOPTAG;
	static Symbol FRAG;
	static Symbol NPA;
	static Symbol NPP;
	static Symbol DATE;
    
	static Symbol ADCP;
	static Symbol ADJP;
	static Symbol ADVP;
	static Symbol DANP;
	static Symbol INTJ;
	static Symbol LST;
	static Symbol NP;
	static Symbol PRN;
	static Symbol S;
	static Symbol VP;
	static Symbol X;

	static Symbol ADC;
	static Symbol ADV_KR;
	static Symbol CO;
	static Symbol CV;
	static Symbol DAN;
	static Symbol EAU;
	static Symbol EAN;
	static Symbol EFN;
	static Symbol ENM;
	static Symbol EPF;
	static Symbol IJ;
	static Symbol LV;
	static Symbol NFW;
	static Symbol NNC;
	static Symbol NNU;
	static Symbol NNX;
	static Symbol NPN;
	static Symbol NPR;
	static Symbol PAD;
	static Symbol PAN;
	static Symbol PAU;
	static Symbol PCA;
	static Symbol PCJ;
	static Symbol SCM;
	static Symbol SFN;
	static Symbol SLQ;
	static Symbol SSY;
	static Symbol SRQ;
	static Symbol VJ;
	static Symbol VV;
	static Symbol VX;
	static Symbol XPF;
	static Symbol XSF;
	static Symbol XSJ;
	static Symbol XSV;
	
	static Symbol COMMA;
	static Symbol COLON;
	static Symbol DOT;
	static Symbol QMARK;
	static Symbol EX_POINT;
	static Symbol DASH;
	static Symbol HYPHEN;
	static Symbol SEMICOLON;
	static Symbol FULLWIDTH_COMMA;
	static Symbol FULLWIDTH_COLON;
	static Symbol FULLWIDTH_DOT;
	static Symbol FULLWIDTH_QMARK;
	static Symbol FULLWIDTH_EX_POINT;
	static Symbol FULLWIDTH_HYPHEN;
	static Symbol FULLWIDTH_SEMICOLON;

	static Symbol GEN_OPEN_DQUOTE;
	static Symbol GEN_CLOSE_DQUOTE;
	static Symbol HORIZONTAL_BAR;
	static Symbol FULLWIDTH_OPEN_PAREN;
	static Symbol FULLWIDTH_CLOSE_PAREN;
};

#endif
