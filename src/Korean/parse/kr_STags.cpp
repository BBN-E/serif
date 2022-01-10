// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "Generic/parse/STags.h"
#include "Korean/parse/kr_STags.h"

Symbol STags::TOP = Symbol(L"TOP");
Symbol STags::TOPTAG = Symbol(L"TOPTAG");
Symbol STags::FRAG = Symbol(L"FRAG");
Symbol STags::NPA = Symbol(L"NPA");
Symbol STags::NPP = Symbol(L"NPP");
Symbol STags::DATE = Symbol(L"DATE");

Symbol STags::ADCP = Symbol(L"ADCP");
Symbol STags::ADJP = Symbol(L"ADJP");
Symbol STags::ADVP = Symbol(L"ADVP");
Symbol STags::DANP = Symbol(L"DANP");
Symbol STags::INTJ = Symbol(L"INTJ");
Symbol STags::LST = Symbol(L"LST");
Symbol STags::NP = Symbol(L"NP");
Symbol STags::PRN = Symbol(L"PRN");
Symbol STags::S = Symbol(L"S");
Symbol STags::VP = Symbol(L"VP");
Symbol STags::X = Symbol(L"X");

Symbol STags::ADC = Symbol(L"ADC");
Symbol STags::ADV_KR = Symbol(L"ADV");
Symbol STags::CO = Symbol(L"CO");
Symbol STags::CV = Symbol(L"CV");
Symbol STags::DAN = Symbol(L"DAN");
Symbol STags::EAU = Symbol(L"EAU");
Symbol STags::EAN = Symbol(L"EAN");
Symbol STags::EFN = Symbol(L"EFN");
Symbol STags::ENM = Symbol(L"ENM");
Symbol STags::EPF = Symbol(L"EPF");
Symbol STags::IJ = Symbol(L"IJ");
Symbol STags::LV = Symbol(L"LV");
Symbol STags::NFW = Symbol(L"NFW");
Symbol STags::NNC = Symbol(L"NNC");
Symbol STags::NNU = Symbol(L"NNU");
Symbol STags::NNX = Symbol(L"NNX");
Symbol STags::NPN = Symbol(L"NPN");
Symbol STags::NPR = Symbol(L"NPR");
Symbol STags::PAD = Symbol(L"PAD");
Symbol STags::PAN = Symbol(L"PAN");
Symbol STags::PAU = Symbol(L"PAU");
Symbol STags::PCA = Symbol(L"PCA");
Symbol STags::PCJ = Symbol(L"PCJ");
Symbol STags::SCM = Symbol(L"SCM");
Symbol STags::SFN = Symbol(L"SFN");
Symbol STags::SLQ = Symbol(L"SLQ");
Symbol STags::SSY = Symbol(L"SSY");
Symbol STags::SRQ = Symbol(L"SRQ");
Symbol STags::VJ = Symbol(L"VJ");
Symbol STags::VV = Symbol(L"VV");
Symbol STags::VX = Symbol(L"VX");
Symbol STags::XPF = Symbol(L"XPF");
Symbol STags::XSF = Symbol(L"XSF");
Symbol STags::XSJ = Symbol(L"XSJ");
Symbol STags::XSV = Symbol(L"XSV");

Symbol STags::COMMA = Symbol(L",");
Symbol STags::COLON = Symbol(L":");
Symbol STags::DOT = Symbol(L".");
Symbol STags::QMARK = Symbol(L"?");
Symbol STags::EX_POINT = Symbol(L"!");
Symbol STags::DASH = Symbol(L"--");
Symbol STags::HYPHEN = Symbol(L"-");
Symbol STags::SEMICOLON = Symbol(L";");
Symbol STags::FULLWIDTH_COMMA = Symbol(L"\xff0c");
Symbol STags::FULLWIDTH_COLON = Symbol(L"\xff1a");
Symbol STags::FULLWIDTH_DOT = Symbol(L"\xff0e");
Symbol STags::FULLWIDTH_QMARK = Symbol(L"xff1f");
Symbol STags::FULLWIDTH_EX_POINT = Symbol(L"\xff01");
Symbol STags::FULLWIDTH_HYPHEN = Symbol(L"\xff0d");
Symbol STags::FULLWIDTH_SEMICOLON = Symbol(L"\xff1b");

Symbol STags::GEN_OPEN_DQUOTE = Symbol(L"\x201c");
Symbol STags::GEN_CLOSE_DQUOTE = Symbol(L"\x201d");
Symbol STags::HORIZONTAL_BAR = Symbol(L"\x2015");
Symbol STags::FULLWIDTH_OPEN_PAREN = Symbol(L"\xff08");
Symbol STags::FULLWIDTH_CLOSE_PAREN = Symbol(L"\xff09");

void KoreanSTags::initializeTagList(std::vector<Symbol> tags) {
	tags.push_back(TOP);
	tags.push_back(TOPTAG);
	tags.push_back(FRAG);
	tags.push_back(NPA);
	tags.push_back(NPP);
	tags.push_back(DATE);

	tags.push_back(ADCP);
	tags.push_back(ADJP);
	tags.push_back(ADVP);
	tags.push_back(DANP);
	tags.push_back(INTJ);
	tags.push_back(LST);
	tags.push_back(NP);
	tags.push_back(PRN);
	tags.push_back(S);
	tags.push_back(VP);
	tags.push_back(X);

	tags.push_back(ADC);
	tags.push_back(ADV_KR);
	tags.push_back(CO);
	tags.push_back(CV);
	tags.push_back(DAN);
	tags.push_back(EAU);
	tags.push_back(EAN);
	tags.push_back(EFN);
	tags.push_back(ENM);
	tags.push_back(EPF);
	tags.push_back(IJ);
	tags.push_back(LV);
	tags.push_back(NFW);
	tags.push_back(NNC);
	tags.push_back(NNU);
	tags.push_back(NNX);
	tags.push_back(NPN);
	tags.push_back(NPR);
	tags.push_back(PAD);
	tags.push_back(PAN);
	tags.push_back(PAU);
	tags.push_back(PCA);
	tags.push_back(PCJ);
	tags.push_back(SCM);
	tags.push_back(SFN);
	tags.push_back(SLQ);
	tags.push_back(SSY);
	tags.push_back(SRQ);
	tags.push_back(VJ);
	tags.push_back(VV);
	tags.push_back(VX);
	tags.push_back(XPF);
	tags.push_back(XSF);
	tags.push_back(XSJ);
	tags.push_back(XSV);

	tags.push_back(COMMA);
	tags.push_back(COLON);
	tags.push_back(DOT);
	tags.push_back(QMARK);
	tags.push_back(EX_POINT);
	tags.push_back(DASH);
	tags.push_back(HYPHEN);
	tags.push_back(SEMICOLON);
	tags.push_back(FULLWIDTH_COMMA);
	tags.push_back(FULLWIDTH_COLON);
	tags.push_back(FULLWIDTH_DOT);
	tags.push_back(FULLWIDTH_QMARK);
	tags.push_back(FULLWIDTH_EX_POINT);
	tags.push_back(FULLWIDTH_HYPHEN);
	tags.push_back(FULLWIDTH_SEMICOLON );

	tags.push_back(GEN_OPEN_DQUOTE);
	tags.push_back(GEN_CLOSE_DQUOTE);
	tags.push_back(HORIZONTAL_BAR);
	tags.push_back(FULLWIDTH_OPEN_PAREN);
	tags.push_back(FULLWIDTH_CLOSE_PAREN);
}
