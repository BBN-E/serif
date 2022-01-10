// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/parse/ch_STags.h"

Symbol ChineseSTags::TOP = Symbol(L"TOP");
Symbol ChineseSTags::TOPTAG = Symbol(L"TOPTAG");
Symbol ChineseSTags::FRAG = Symbol(L"FRAG");
Symbol ChineseSTags::FRAGMENTS = Symbol(L"FRAGMENTS");
Symbol ChineseSTags::NPA = Symbol(L"NPA");
Symbol ChineseSTags::NPP = Symbol(L"NPP");
Symbol ChineseSTags::DATE = Symbol(L"DATE");

Symbol ChineseSTags::ADJP = Symbol(L"ADJP");
Symbol ChineseSTags::ADVP = Symbol(L"ADVP");
Symbol ChineseSTags::CLP = Symbol(L"CLP");
Symbol ChineseSTags::CP = Symbol(L"CP");
Symbol ChineseSTags::DNP = Symbol(L"DNP");
Symbol ChineseSTags::DP = Symbol(L"DP");
Symbol ChineseSTags::DVP = Symbol(L"DVP");
Symbol ChineseSTags::IP = Symbol(L"IP");
Symbol ChineseSTags::LCP = Symbol(L"LCP");
Symbol ChineseSTags::LST = Symbol(L"LST");
Symbol ChineseSTags::NP = Symbol(L"NP");
Symbol ChineseSTags::PP = Symbol(L"PP");
Symbol ChineseSTags::PRN = Symbol(L"PRN");
Symbol ChineseSTags::QP = Symbol(L"QP");
Symbol ChineseSTags::UCP = Symbol(L"UCP");
Symbol ChineseSTags::VCD = Symbol(L"VCD");
Symbol ChineseSTags::VCP = Symbol(L"VCP");
Symbol ChineseSTags::VNV = Symbol(L"VNV");
Symbol ChineseSTags::VP = Symbol(L"VP");
Symbol ChineseSTags::VPT = Symbol(L"VPT");
Symbol ChineseSTags::VRD = Symbol(L"VRD");
Symbol ChineseSTags::VSB = Symbol(L"VSB");

Symbol ChineseSTags::AD = Symbol(L"AD");
Symbol ChineseSTags::AS = Symbol(L"AS");
Symbol ChineseSTags::BA = Symbol(L"BA"); 
Symbol ChineseSTags::CC = Symbol(L"CC");
Symbol ChineseSTags::CD = Symbol(L"CD");
Symbol ChineseSTags::CS = Symbol(L"CS");
Symbol ChineseSTags::DEC = Symbol(L"DEC");
Symbol ChineseSTags::DEG = Symbol(L"DEG");
Symbol ChineseSTags::DER = Symbol(L"DER");
Symbol ChineseSTags::DEV = Symbol(L"DEV");
Symbol ChineseSTags::DT = Symbol(L"DT");
Symbol ChineseSTags::ETC = Symbol(L"ETC");
Symbol ChineseSTags::FW = Symbol(L"FW");
Symbol ChineseSTags::IJ = Symbol(L"IJ");
Symbol ChineseSTags::INTJ = Symbol(L"INTJ");
Symbol ChineseSTags::JJ = Symbol(L"JJ");
Symbol ChineseSTags::LB = Symbol(L"LB");
Symbol ChineseSTags::LC = Symbol(L"LC");
Symbol ChineseSTags::M = Symbol(L"M");
Symbol ChineseSTags::MSP = Symbol(L"MSP");
Symbol ChineseSTags::NN = Symbol(L"NN");
Symbol ChineseSTags::NR = Symbol(L"NR");
Symbol ChineseSTags::NT = Symbol(L"NT");
Symbol ChineseSTags::OD = Symbol(L"OD");
Symbol ChineseSTags::ON = Symbol(L"ON");
Symbol ChineseSTags::P = Symbol(L"P");
Symbol ChineseSTags::PN = Symbol(L"PN");
Symbol ChineseSTags::PU = Symbol(L"PU");
Symbol ChineseSTags::SB = Symbol(L"SB");
Symbol ChineseSTags::SP = Symbol(L"SP");
Symbol ChineseSTags::VA = Symbol(L"VA");
Symbol ChineseSTags::VC = Symbol(L"VC");
Symbol ChineseSTags::VE = Symbol(L"VE");
Symbol ChineseSTags::VV = Symbol(L"VV");

Symbol ChineseSTags::COMMA = Symbol(L",");
Symbol ChineseSTags::COLON = Symbol(L":");
Symbol ChineseSTags::DOT = Symbol(L".");
Symbol ChineseSTags::QMARK = Symbol(L"?");
Symbol ChineseSTags::EX_POINT = Symbol(L"!");
Symbol ChineseSTags::DASH = Symbol(L"--");
Symbol ChineseSTags::HYPHEN = Symbol(L"-");
Symbol ChineseSTags::SEMICOLON = Symbol(L";");
Symbol ChineseSTags::CH_COMMA = Symbol(L"\x3001");
Symbol ChineseSTags::CH_DOT = Symbol(L"\x3002");
Symbol ChineseSTags::FULLWIDTH_COMMA = Symbol(L"\xff0c");
Symbol ChineseSTags::FULLWIDTH_COLON = Symbol(L"\xff1a");
Symbol ChineseSTags::FULLWIDTH_DOT = Symbol(L"\xff0e");
Symbol ChineseSTags::FULLWIDTH_QMARK = Symbol(L"xff1f");
Symbol ChineseSTags::FULLWIDTH_EX_POINT = Symbol(L"\xff01");
Symbol ChineseSTags::FULLWIDTH_HYPHEN = Symbol(L"\xff0d");
Symbol ChineseSTags::FULLWIDTH_SEMICOLON = Symbol(L"\xff1b");

Symbol ChineseSTags::CH_OPEN_SBRACKET = Symbol(L"\x3008");
Symbol ChineseSTags::CH_CLOSE_SBRACKET = Symbol(L"\x3009");
Symbol ChineseSTags::CH_OPEN_DBRACKET = Symbol(L"\x300a");
Symbol ChineseSTags::CH_CLOSE_DBRACKET = Symbol(L"\x300b");
Symbol ChineseSTags::CH_OPEN_CBRACKET = Symbol(L"\x300e");
Symbol ChineseSTags::CH_CLOSE_CBRACKET = Symbol(L"\x300f");
Symbol ChineseSTags::GEN_OPEN_DQUOTE = Symbol(L"\x201c");
Symbol ChineseSTags::GEN_CLOSE_DQUOTE = Symbol(L"\x201d");
Symbol ChineseSTags::HORIZONTAL_BAR = Symbol(L"\x2015");
Symbol ChineseSTags::FULLWIDTH_OPEN_PAREN = Symbol(L"\xff08");
Symbol ChineseSTags::FULLWIDTH_CLOSE_PAREN = Symbol(L"\xff09");

/** Unused STag constiutent constants. */
//Symbol ChineseSTags::S(L"*UNDEFINED-STAG*"); 
//Symbol ChineseSTags::SBAR(L"*UNDEFINED-STAG*");
//Symbol ChineseSTags::IN(L"*UNDEFINED-STAG*");

void ChineseSTags::initializeTagList(std::vector<Symbol> tags) {
	tags.push_back(TOP);
	tags.push_back(TOPTAG);
	tags.push_back(FRAG);
	tags.push_back(FRAGMENTS);
	tags.push_back(NPA);
	tags.push_back(NPP);
	tags.push_back(DATE);

	tags.push_back(ADJP);
	tags.push_back(ADVP);
	tags.push_back(CLP);
	tags.push_back(CP);
	tags.push_back(DNP);
	tags.push_back(DP);
	tags.push_back(DVP);
	tags.push_back(IP);
	tags.push_back(LCP);
	tags.push_back(LST);
	tags.push_back(NP);
	tags.push_back(PP);
	tags.push_back(PRN);
	tags.push_back(QP);
	tags.push_back(UCP);
	tags.push_back(VCD);
	tags.push_back(VCP);
	tags.push_back(VNV);
	tags.push_back(VP);
	tags.push_back(VPT);
	tags.push_back(VRD);
	tags.push_back(VSB);

	tags.push_back(AD);
	tags.push_back(AS);
	tags.push_back(BA);
	tags.push_back(CC);
	tags.push_back(CD);
	tags.push_back(CS);
	tags.push_back(DEC);
	tags.push_back(DEG);
	tags.push_back(DER);
	tags.push_back(DEV);
	tags.push_back(DT);
	tags.push_back(ETC);
	tags.push_back(FW);
	tags.push_back(IJ);
	tags.push_back(INTJ);
	tags.push_back(JJ);
	tags.push_back(LB);
	tags.push_back(LC);
	tags.push_back(M);
	tags.push_back(MSP);
	tags.push_back(NN);
	tags.push_back(NR);
	tags.push_back(NT);
	tags.push_back(OD);
	tags.push_back(ON);
	tags.push_back(P);
	tags.push_back(PN);
	tags.push_back(PU);
	tags.push_back(SB);
	tags.push_back(SP);
	tags.push_back(VA);
	tags.push_back(VC);
	tags.push_back(VE);
	tags.push_back(VV);

	tags.push_back(COMMA);
	tags.push_back(COLON);
	tags.push_back(DOT);
	tags.push_back(QMARK);
	tags.push_back(EX_POINT);
	tags.push_back(DASH);
	tags.push_back(HYPHEN);
	tags.push_back(SEMICOLON);
	tags.push_back(CH_COMMA);
	tags.push_back(CH_DOT);
	tags.push_back(FULLWIDTH_COMMA);
	tags.push_back(FULLWIDTH_COLON);
	tags.push_back(FULLWIDTH_DOT);
	tags.push_back(FULLWIDTH_QMARK);
	tags.push_back(FULLWIDTH_EX_POINT);
	tags.push_back(FULLWIDTH_HYPHEN);
	tags.push_back(FULLWIDTH_SEMICOLON);

	tags.push_back(CH_OPEN_SBRACKET);
	tags.push_back(CH_CLOSE_SBRACKET);
	tags.push_back(CH_OPEN_DBRACKET);
	tags.push_back(CH_CLOSE_DBRACKET);
	tags.push_back(CH_OPEN_CBRACKET);
	tags.push_back(CH_CLOSE_CBRACKET);
	tags.push_back(GEN_OPEN_DQUOTE);
	tags.push_back(GEN_CLOSE_DQUOTE);
	tags.push_back(HORIZONTAL_BAR);
	tags.push_back(FULLWIDTH_OPEN_PAREN);
	tags.push_back(FULLWIDTH_CLOSE_PAREN);
}
