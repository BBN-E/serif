// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_STAGS_H
#define CH_STAGS_H

#include "Generic/parse/STags.h"
#include "Generic/common/Symbol.h"

class ChineseSTags {
public:
	static void initializeTagList(std::vector<Symbol> tags);

	static Symbol TOP;
	static Symbol TOPTAG;
	static Symbol FRAG;
	static Symbol FRAGMENTS;
	static Symbol NPA;
	static Symbol NPP;
	static Symbol DATE;
    
	static Symbol ADJP;
	static Symbol ADVP;
	static Symbol CLP;
	static Symbol CP;
	static Symbol DNP;
	static Symbol DP;
	static Symbol DVP;
	static Symbol IP;
	static Symbol LCP;
	static Symbol LST;
	static Symbol NP;
	static Symbol PP;
	static Symbol PRN;
	static Symbol QP;
	static Symbol UCP;
	static Symbol VCD;
	static Symbol VCP;
	static Symbol VNV;
	static Symbol VP;
	static Symbol VPT;
	static Symbol VRD;
	static Symbol VSB;

	static Symbol AD;
	static Symbol AS;
	static Symbol BA;
	static Symbol CC;
	static Symbol CD;
	static Symbol CS;
	static Symbol DEC;
	static Symbol DEG;
	static Symbol DER;
	static Symbol DEV;
	static Symbol DT;
	static Symbol ETC;
	static Symbol FW;
	static Symbol IJ;
	static Symbol INTJ;
	static Symbol JJ;
	static Symbol LB;
	static Symbol LC;
	static Symbol M;
	static Symbol MSP;
	static Symbol NN;
	static Symbol NR;
	static Symbol NT;
	static Symbol OD;
	static Symbol ON;
	static Symbol P;
	static Symbol PN;
	static Symbol PU;
	static Symbol SB;
	static Symbol SP;
	static Symbol VA;
	static Symbol VC;
	static Symbol VE;
	static Symbol VV;
	
	static Symbol COMMA;
	static Symbol COLON;
	static Symbol DOT;
	static Symbol QMARK;
	static Symbol EX_POINT;
	static Symbol DASH;
	static Symbol HYPHEN;
	static Symbol SEMICOLON;
	static Symbol CH_COMMA;
	static Symbol CH_DOT;
	static Symbol FULLWIDTH_COMMA;
	static Symbol FULLWIDTH_COLON;
	static Symbol FULLWIDTH_DOT;
	static Symbol FULLWIDTH_QMARK;
	static Symbol FULLWIDTH_EX_POINT;
	static Symbol FULLWIDTH_HYPHEN;
	static Symbol FULLWIDTH_SEMICOLON;

	static Symbol CH_OPEN_SBRACKET;
	static Symbol CH_CLOSE_SBRACKET;
	static Symbol CH_OPEN_DBRACKET;
	static Symbol CH_CLOSE_DBRACKET;
	static Symbol CH_OPEN_CBRACKET;
	static Symbol CH_CLOSE_CBRACKET;
	static Symbol GEN_OPEN_DQUOTE;
	static Symbol GEN_CLOSE_DQUOTE;
	static Symbol HORIZONTAL_BAR;
	static Symbol FULLWIDTH_OPEN_PAREN;
	static Symbol FULLWIDTH_CLOSE_PAREN;

	// Not used in this language
	//static Symbol S; 
	//static Symbol SBAR;
	//static Symbol IN;
};

#endif
