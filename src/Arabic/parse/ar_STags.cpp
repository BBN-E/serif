// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Arabic/parse/ar_STags.h"

 Symbol ArabicSTags::PUNC = Symbol(L"PUNC");
 Symbol ArabicSTags::UNKNOWN = Symbol(L"-UNKNOWN-");
 Symbol ArabicSTags::BSLASH = Symbol(L"-BSLASH-");
 Symbol ArabicSTags::LRB = Symbol(L"-LRB-");
 Symbol ArabicSTags::RRB = Symbol(L"-RRB-");
 Symbol ArabicSTags::SEMICOLON = Symbol(L";");
 Symbol ArabicSTags::COLON = Symbol(L":");
 Symbol ArabicSTags::FSLASH = Symbol(L"-FSLASH-");
 Symbol ArabicSTags::PLUS = Symbol(L"-PLUS-");
 Symbol ArabicSTags::QUESTION = Symbol(L"?");
 Symbol ArabicSTags::DOT = Symbol(L".");
 Symbol ArabicSTags::HYPHEN = Symbol(L"-");
 Symbol ArabicSTags::COMMA = Symbol(L",");
 Symbol ArabicSTags::NUMERIC_COMMA = Symbol(L"NUMERIC_COMMA");
 Symbol ArabicSTags::DQUOTE = Symbol(L"''");
 Symbol ArabicSTags::SQUOTE = Symbol(L"'");
 Symbol ArabicSTags::EX_POINT = Symbol(L"!");
 Symbol ArabicSTags::PERCENT = Symbol(L"%");

 Symbol ArabicSTags::NNS = Symbol(L"NNS");
 Symbol ArabicSTags::DET_NNS = Symbol(L"DET:NNS");
 Symbol ArabicSTags::NN = Symbol(L"NN");
 Symbol ArabicSTags::DET_NN = Symbol(L"DET:NN");
 Symbol ArabicSTags::NNPS = Symbol(L"NNPS");
 Symbol ArabicSTags::DET_NNPS = Symbol(L"DET:NNPS");
 Symbol ArabicSTags::NNP = Symbol(L"NNP");
 Symbol ArabicSTags::DET_NNP = Symbol(L"DET:NNP");
 Symbol ArabicSTags::CD = Symbol(L"CD");
 
 Symbol ArabicSTags::VBN = Symbol(L"VBN");
 Symbol ArabicSTags::VBP = Symbol(L"VBP");
 Symbol ArabicSTags::VBD = Symbol(L"VBD");
 Symbol ArabicSTags::VB = Symbol(L"VB");

 Symbol ArabicSTags::DATE = Symbol(L"DATE");
 
 // part of parser experiments that are not included in our current parser
 // was added/changed in ATB_V3.2
/* Symbol ArabicSTags::DV = Symbol(L"DV"); */
 // was added/changed in ATB_V3.3
/*
 Symbol ArabicSTags::NQ = Symbol(L"NQ");
 Symbol ArabicSTags::DET_NQ = Symbol(L"DET:NQ");
 Symbol ArabicSTags::JJ_NUM = Symbol(L"JJ_NUM");
 Symbol ArabicSTags::DET_JJ_NUM = Symbol(L"DET:JJ_NUM");
 Symbol ArabicSTags::JJ_COMP = Symbol(L"JJ_COMP");
 Symbol ArabicSTags::DET_JJ_COMP = Symbol(L"DET:JJ_COMP");
*/

 Symbol ArabicSTags::WP = Symbol(L"WP");
 Symbol ArabicSTags::PRP = Symbol(L"PRP");
 Symbol ArabicSTags::PRP_POS = Symbol(L"PRP$");

 Symbol ArabicSTags::JJ = Symbol(L"JJ");
 Symbol ArabicSTags::DET_JJ = Symbol(L"DET:JJ");



 Symbol ArabicSTags::RB = Symbol(L"RB");
 Symbol ArabicSTags::DT = Symbol(L"DT");
 Symbol ArabicSTags::RP = Symbol(L"RP");
 Symbol ArabicSTags::CC = Symbol(L"CC");
 Symbol ArabicSTags::IN = Symbol(L"IN");
 Symbol ArabicSTags::UH = Symbol(L"UH");
 Symbol ArabicSTags::VPN = Symbol(L"VPN");

 Symbol ArabicSTags::SBAR = Symbol(L"SBAR");
 Symbol ArabicSTags::PRT = Symbol(L"PRT");
 Symbol ArabicSTags::PRN = Symbol(L"PRN");
 Symbol ArabicSTags::ADJP = Symbol(L"ADJP");
 Symbol ArabicSTags::WHNP = Symbol(L"WHNP");
 Symbol ArabicSTags::LST = Symbol(L"LST");
 Symbol ArabicSTags::QP = Symbol(L"QP");
 Symbol ArabicSTags::S = Symbol(L"S");
 Symbol ArabicSTags::NP = Symbol(L"NP");
 Symbol ArabicSTags::NX = Symbol(L"NX");
 Symbol ArabicSTags::VP = Symbol(L"VP");
 Symbol ArabicSTags::ADVP = Symbol(L"ADVP");
 Symbol ArabicSTags::CONJP = Symbol(L"CONJP");
 Symbol ArabicSTags::PP = Symbol(L"PP");
 Symbol ArabicSTags::NPA = Symbol(L"NPA");
 Symbol ArabicSTags::NPP = Symbol(L"NPP");
 Symbol ArabicSTags::S_ADV = Symbol(L"S-ADV");
 Symbol ArabicSTags::INTJ = Symbol(L"INTJ");
 Symbol ArabicSTags::FRAG = Symbol(L"FRAG");
 Symbol ArabicSTags::FRAGMENTS = Symbol(L"FRAGMENTS");

 Symbol ArabicSTags::DEF_NP = Symbol(L"DEF_NP");
 Symbol ArabicSTags::DEF_NPA = Symbol(L"DEF_NPA");

 Symbol ArabicSTags::SINV = Symbol(L"SINV");
 Symbol ArabicSTags::SQ = Symbol(L"SQ");
 Symbol ArabicSTags::WHADJP = Symbol(L"WHADJP");
 Symbol ArabicSTags::WHADVP = Symbol(L"WHADVP");
 Symbol ArabicSTags::WHPP = Symbol(L"WHPP");
 Symbol ArabicSTags::WDT = Symbol(L"WDT");
 Symbol ArabicSTags::WP$ = Symbol(L"WP$");
 Symbol ArabicSTags::DOLLAR = Symbol(L"$");
 Symbol ArabicSTags::NAC = Symbol(L"NAC");
 Symbol ArabicSTags::RRC = Symbol(L"RRC");
 Symbol ArabicSTags::SBARQ = Symbol(L"SBARQ");
 Symbol ArabicSTags::UCP = Symbol(L"UCP");

 Symbol ArabicSTags::X = Symbol(L"X");
 Symbol ArabicSTags::NON_ALPHABETIC = Symbol(L"NON_ALPHABETIC");
 
void ArabicSTags::initializeTagList(std::vector<Symbol> tags) {
	tags.push_back(PUNC);
	tags.push_back(UNKNOWN);
	tags.push_back(BSLASH);
	tags.push_back(LRB);
	tags.push_back(RRB);
	tags.push_back(SEMICOLON);
	tags.push_back(COLON);
	tags.push_back(FSLASH);
	tags.push_back(PLUS);
	tags.push_back(QUESTION);
	tags.push_back(DOT);
	tags.push_back(HYPHEN);
	tags.push_back(COMMA);
	tags.push_back(NUMERIC_COMMA);
	tags.push_back(DQUOTE);
	tags.push_back(SQUOTE);
	tags.push_back(EX_POINT);
	tags.push_back(PERCENT);

	tags.push_back(NNS);
	tags.push_back(DET_NNS);
	tags.push_back(NN);
	tags.push_back(DET_NN);
	tags.push_back(NNPS);
	tags.push_back(DET_NNPS);
	tags.push_back(NNP);
	tags.push_back(DET_NNP);
	tags.push_back(CD);

	tags.push_back(VBN);
	tags.push_back(VBP);
	tags.push_back(VBD);
	tags.push_back(VB);

	tags.push_back(WP);
	tags.push_back(PRP);
	tags.push_back(PRP_POS);

	tags.push_back(JJ);
	tags.push_back(DET_JJ);

	tags.push_back(RB);
	tags.push_back(DT);
	tags.push_back(RP);
	tags.push_back(CC);
	tags.push_back(IN);
	tags.push_back(UH);
	tags.push_back(VPN);

	tags.push_back(SBAR);
	tags.push_back(PRT);
	tags.push_back(PRN);
	tags.push_back(ADJP);
	tags.push_back(WHNP);
	tags.push_back(LST);
	tags.push_back(QP);
	tags.push_back(S);
	tags.push_back(NP);
	tags.push_back(NX);
	tags.push_back(VP);
	tags.push_back(ADVP);
	tags.push_back(CONJP);
	tags.push_back(PP);
	tags.push_back(NPA);
	tags.push_back(NPP);
	tags.push_back(S_ADV);
	tags.push_back(INTJ);
	tags.push_back(FRAG);
	tags.push_back(FRAGMENTS);

	tags.push_back(DEF_NP);
	tags.push_back(DEF_NPA);

	tags.push_back(SINV);
	tags.push_back(SQ);
	tags.push_back(WHADJP);
	tags.push_back(WHADVP);
	tags.push_back(WHPP);
	tags.push_back(WDT);
	tags.push_back(WP$);
	tags.push_back(DOLLAR);
	tags.push_back(NAC);
	tags.push_back(RRC);
	tags.push_back(SBARQ);
	tags.push_back(UCP);

	tags.push_back(X);
	tags.push_back(NON_ALPHABETIC);
}

 /*
 //rule based conversion from BW produced part of speech that look like
 //VERB_PERFECT+PVSUFF_SUBJ:3FS and DET+NOUN_PROP+NSUFF_FEM_SG+CASE_DEF_GEN
 //to the POS in STAGS 
 //This mirrors the script 
 //\\traid04\u0\annotation\arabic\ArabicTreebank\scripts\convertPOS.pl
 //which roughly follows the LDC suggested converesion
*/
Symbol ArabicSTags::convertDictPOSToParserPOS(Symbol dict_pos){
	 const wchar_t* dictstr = dict_pos.to_string();
	 const wchar_t* substr = 0;
	 wchar_t tag_buffer[500];
	 wcsncpy(tag_buffer, L"", 500);
	 //look for a noun
	 substr = wcsstr(dictstr, L"NOUN");
	 if(substr != 0){
		 substr = wcsstr(dictstr, L"DET");
		 if(substr != 0){
			 wcsncat(tag_buffer, L"DET:", 500);
		 }
		 substr = wcsstr(dictstr, L"NOUN_PROP");
		 if(substr != 0){
			 wcsncat(tag_buffer, L"NNP", 500);
		 }
		 else{
			 wcsncat(tag_buffer, L"NN", 500);
		 }
		 substr = wcsstr(dictstr, L"_PL");
		 if(substr != 0){
			 wcsncat(tag_buffer, L"S", 500);
		 }
		 substr = wcsstr(dictstr, L"_DU");
		 if(substr != 0){
			 wcsncat(tag_buffer, L"S", 500);
		 }
		 return Symbol(tag_buffer);
	 }
	 substr = wcsstr(dictstr, L"DET+ABBREV");
	 if(substr != 0)  return DET_NN;
	 substr = wcsstr(dictstr, L"ABBREV");
	 if(substr != 0)  return NN;
	 substr = wcsstr(dictstr, L"LATIN");
	 if(substr != 0)  return NN;

	 //Verbs
	 substr = wcsstr(dictstr, L"PASSIVE");
	 if(substr != 0)  return VBN;
	 substr = wcsstr(dictstr, L"VERB_IMPERFECT");
	 if(substr != 0)  return VBP;
	 substr = wcsstr(dictstr, L"VERB_PERFECT");
	 if(substr != 0)  return VBD;
	 substr = wcsstr(dictstr, L"VERB_IMPERATIVE");
	 if(substr != 0)  return VB;
	 
	 substr = wcsstr(dictstr, L"NUMERIC");
	 if(substr != 0)  return CD;
	 //pronouns
	 substr = wcsstr(dictstr, L"POSS_PRON");
	 if(substr != 0)  return PRP_POS;
	 substr = wcsstr(dictstr, L"REL_PRON");
	 if(substr != 0)  return WP;
	 substr = wcsstr(dictstr, L"PRON");
	 if(substr != 0)  return PRP;
	 substr = wcsstr(dictstr, L"VSUFF_SUBJ");
	 if(substr != 0)  return PRP;		 
	 substr = wcsstr(dictstr, L"VSUFF_DO");
	 if(substr != 0)  return PRP;
	 //adjectives
	 substr = wcsstr(dictstr, L"ADJ");
	 if(substr != 0){
		 substr = wcsstr(dictstr, L"DET");
		 if(substr != 0){
			 return DET_JJ;
		 }
		 return JJ;
	 }
	  substr = wcsstr(dictstr, L"DET+INTERJ+CASE_DEF_NOM");
	  if(substr != 0) return JJ;
	
	  //others
	 substr = wcsstr(dictstr, L"INTERJ");
	 if(substr != 0)  return UH;
	 substr = wcsstr(dictstr, L"ADV");
	 if(substr != 0)  return RB;
	 substr = wcsstr(dictstr, L"CONJ");
	 if(substr != 0)  return CC;
	 substr = wcsstr(dictstr, L"PREP");
	 if(substr != 0)  return IN;
	 substr = wcsstr(dictstr, L"DEM)");
	 if(substr != 0)  return DT;
	 substr = wcsstr(dictstr, L"DEM)");
	 if(substr != 0)  return DT;	 
	 if(wcscmp(dictstr, L"DET") == 0) return DT;
	 if(wcscmp(dictstr, L"FUNC_WORD") == 0) return IN;
	 return dict_pos;

 }


 

 











	
