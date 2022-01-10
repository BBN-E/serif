// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

/**
 * ENGLISH_AS_GENERIC
 * This class was copied from English into generic to support WordNet;
 * All members are declared as private, with WordNet as a friend class.
 **/

#include "Generic/common/leak_detection.h"

#include "Generic/parse/xx_STags.h"

Symbol GenericSTags::COMMA(L",");
Symbol GenericSTags::DATE(L"DATE");
Symbol GenericSTags::NP(L"NP");
Symbol GenericSTags::NPA(L"NPA");
Symbol GenericSTags::PP(L"PP");
void GenericSTags::initializeTagList(std::vector<Symbol> tags) {
	tags.push_back(NP);
	tags.push_back(NPA);
	tags.push_back(PP);
}

namespace enAsGen {

Symbol EnglishSTagsForWordnet::TOP = Symbol(L"TOP");
Symbol EnglishSTagsForWordnet::TOPTAG = Symbol(L"TOPTAG");
Symbol EnglishSTagsForWordnet::COMMA = Symbol(L",");
Symbol EnglishSTagsForWordnet::COLON = Symbol(L":");
Symbol EnglishSTagsForWordnet::DOT = Symbol(L".");
Symbol EnglishSTagsForWordnet::QMARK = Symbol(L"?");
Symbol EnglishSTagsForWordnet::EX_POINT = Symbol(L"!");
Symbol EnglishSTagsForWordnet::DASH = Symbol(L"--");
Symbol EnglishSTagsForWordnet::HYPHEN = Symbol(L"-");
Symbol EnglishSTagsForWordnet::SEMICOLON = Symbol(L";");
Symbol EnglishSTagsForWordnet::DOLLAR = Symbol(L"$");
Symbol EnglishSTagsForWordnet::ADJP = Symbol(L"ADJP");
Symbol EnglishSTagsForWordnet::ADVP = Symbol(L"ADVP");
Symbol EnglishSTagsForWordnet::CC = Symbol(L"CC");
Symbol EnglishSTagsForWordnet::CD = Symbol(L"CD");
Symbol EnglishSTagsForWordnet::CONJP = Symbol(L"CONJP");
Symbol EnglishSTagsForWordnet::DT = Symbol(L"DT");
Symbol EnglishSTagsForWordnet::DUMMY = Symbol(L"DUMMY");
Symbol EnglishSTagsForWordnet::FRAG = Symbol(L"FRAG");
Symbol EnglishSTagsForWordnet::EX = Symbol(L"EX");
Symbol EnglishSTagsForWordnet::FW = Symbol(L"FW");
Symbol EnglishSTagsForWordnet::IN = Symbol(L"IN");
Symbol EnglishSTagsForWordnet::INTJ = Symbol(L"INTJ");
Symbol EnglishSTagsForWordnet::JJ = Symbol(L"JJ");
Symbol EnglishSTagsForWordnet::JJR = Symbol(L"JJR");
Symbol EnglishSTagsForWordnet::JJS = Symbol(L"JJS");
Symbol EnglishSTagsForWordnet::LS = Symbol(L"LS");
Symbol EnglishSTagsForWordnet::LST = Symbol(L"LST");
Symbol EnglishSTagsForWordnet::MD = Symbol(L"MD");
Symbol EnglishSTagsForWordnet::NAC = Symbol(L"NAC");
Symbol EnglishSTagsForWordnet::NCD = Symbol(L"NCD");
Symbol EnglishSTagsForWordnet::NN = Symbol(L"NN");
Symbol EnglishSTagsForWordnet::NNP = Symbol(L"NNP");
Symbol EnglishSTagsForWordnet::NNPS = Symbol(L"NNPS");
Symbol EnglishSTagsForWordnet::NNS = Symbol(L"NNS");
Symbol EnglishSTagsForWordnet::NP = Symbol(L"NP");
Symbol EnglishSTagsForWordnet::NPA = Symbol(L"NPA");
Symbol EnglishSTagsForWordnet::NPP = Symbol(L"NPP");
Symbol EnglishSTagsForWordnet::NPPOS = Symbol(L"NPPOS");
Symbol EnglishSTagsForWordnet::NPPRO = Symbol(L"NPPRO");
Symbol EnglishSTagsForWordnet::NX = Symbol(L"NX");
Symbol EnglishSTagsForWordnet::POS = Symbol(L"POS");
Symbol EnglishSTagsForWordnet::PP = Symbol(L"PP");
Symbol EnglishSTagsForWordnet::PRN = Symbol(L"PRN");
Symbol EnglishSTagsForWordnet::PRP = Symbol(L"PRP");
Symbol EnglishSTagsForWordnet::PRPS = Symbol(L"PRP$");
Symbol EnglishSTagsForWordnet::PRT = Symbol(L"PRT");
Symbol EnglishSTagsForWordnet::QP = Symbol(L"QP");
Symbol EnglishSTagsForWordnet::RB = Symbol(L"RB");
Symbol EnglishSTagsForWordnet::RP = Symbol(L"RP");
Symbol EnglishSTagsForWordnet::RBR = Symbol(L"RBR");
Symbol EnglishSTagsForWordnet::RBS = Symbol(L"RBS");
Symbol EnglishSTagsForWordnet::RRC = Symbol(L"RRC");
Symbol EnglishSTagsForWordnet::S = Symbol(L"S");
Symbol EnglishSTagsForWordnet::SBAR = Symbol(L"SBAR");
Symbol EnglishSTagsForWordnet::SBARQ = Symbol(L"SBARQ");
Symbol EnglishSTagsForWordnet::SINV = Symbol(L"SINV");
Symbol EnglishSTagsForWordnet::SQ = Symbol(L"SQ");
Symbol EnglishSTagsForWordnet::TO = Symbol(L"TO");
Symbol EnglishSTagsForWordnet::UCP = Symbol(L"UCP");
Symbol EnglishSTagsForWordnet::VB = Symbol(L"VB");
Symbol EnglishSTagsForWordnet::VBD = Symbol(L"VBD");
Symbol EnglishSTagsForWordnet::VBG = Symbol(L"VBG");
Symbol EnglishSTagsForWordnet::VBN = Symbol(L"VBN");
Symbol EnglishSTagsForWordnet::VBP = Symbol(L"VBP");
Symbol EnglishSTagsForWordnet::VBZ = Symbol(L"VBZ");
Symbol EnglishSTagsForWordnet::VP = Symbol(L"VP");
Symbol EnglishSTagsForWordnet::WDT = Symbol(L"WDT");
Symbol EnglishSTagsForWordnet::WHADJP = Symbol(L"WHADJP");
Symbol EnglishSTagsForWordnet::WHADVP = Symbol(L"WHADVP");
Symbol EnglishSTagsForWordnet::WHNP = Symbol(L"WHNP");
Symbol EnglishSTagsForWordnet::WHPP = Symbol(L"WHPP");
Symbol EnglishSTagsForWordnet::WP = Symbol(L"WP");
Symbol EnglishSTagsForWordnet::WPDOLLAR = Symbol(L"WP$");
Symbol EnglishSTagsForWordnet::WRB = Symbol(L"WRB");
Symbol EnglishSTagsForWordnet::X = Symbol(L"X");
Symbol EnglishSTagsForWordnet::DATE = Symbol(L"DATE");

//These Labels are only used by the trainer mrf
Symbol EnglishSTagsForWordnet::LOCATION_NNP = Symbol(L"LOCATION-NNP");
Symbol EnglishSTagsForWordnet::PERSON_NNP= Symbol(L"PERSON-NNP");
Symbol EnglishSTagsForWordnet::ORGANIZATION_NNP= Symbol(L"ORGANIZATION-NNP");
Symbol EnglishSTagsForWordnet::PERCENT_NNP= Symbol(L"PERCENT-NNP");
Symbol EnglishSTagsForWordnet::TIME_NNP= Symbol(L"TIME-NNP");
Symbol EnglishSTagsForWordnet::DATE_NNP= Symbol(L"DATE-NNP");
Symbol EnglishSTagsForWordnet::MONEY_NNP = Symbol(L"MONEY-NNP");
Symbol EnglishSTagsForWordnet::LOCATION_NNPS = Symbol(L"LOCATION-NNPS");
Symbol EnglishSTagsForWordnet::PERSON_NNPS = Symbol(L"PERSON-NNPS");
Symbol EnglishSTagsForWordnet::ORGANIZATION_NNPS = Symbol(L"ORGANIZATION-NNPS");
Symbol EnglishSTagsForWordnet::PERCENT_NNPS = Symbol(L"PERCENT-NNPS");
Symbol EnglishSTagsForWordnet::TIME_NNPS = Symbol(L"TIME-NNPS");
Symbol EnglishSTagsForWordnet::DATE_NNPS = Symbol(L"DATE-NNPS");
Symbol EnglishSTagsForWordnet::MONEY_NNPS = Symbol(L"MONEY-NNPS");
Symbol EnglishSTagsForWordnet::NPP_NNP = Symbol(L"NPP-NNP");
Symbol EnglishSTagsForWordnet::NPP_NNPS= Symbol(L"NPP-NNPS");
Symbol EnglishSTagsForWordnet::PERSON = Symbol(L"PERSON");
Symbol EnglishSTagsForWordnet::ORGANIZATION = Symbol(L"ORGANIZATION");
Symbol EnglishSTagsForWordnet::LOCATION = Symbol(L"LOCATION");
Symbol EnglishSTagsForWordnet::PDT = Symbol(L"PDT");
Symbol EnglishSTagsForWordnet::UH = Symbol(L"UH");

}
