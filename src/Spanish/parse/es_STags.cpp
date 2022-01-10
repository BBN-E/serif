// Copyright 2013y BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Spanish/parse/es_STags.h"

// ======== Constituent Tags ==========
Symbol SpanishSTags::CONJ(L"CONJ");                              // conjunction phrase
Symbol SpanishSTags::CONJ_SUBORD(L"CONJ.subord");                // subordinating conjunction phrase
Symbol SpanishSTags::GERUNDI(L"GERUNDI");                        // gerund
Symbol SpanishSTags::GRUP_A(L"GRUP.A");                          // adjective group
Symbol SpanishSTags::GRUP_ADV(L"GRUP.ADV");                      // adverbial group
Symbol SpanishSTags::GRUP_NOM(L"GRUP.NOM");                      // nominal group
Symbol SpanishSTags::GRUP_VERB(L"GRUP.VERB");                    // verbal group
Symbol SpanishSTags::INC(L"INC");                                // inserted element
Symbol SpanishSTags::INFINITIU(L"INFINITIU");                    // infinitive
Symbol SpanishSTags::INTERJECCIO(L"INTERJECCIO");                // interjection
Symbol SpanishSTags::MORFEMA_PRONOMINAL(L"MORFEMA.PRONOMINAL");  // pronominal morpheme
Symbol SpanishSTags::MORFEMA_VERBAL(L"MORFEMA.VERBAL");          // verbal morpheme
Symbol SpanishSTags::NEG(L"NEG");                                // negation
Symbol SpanishSTags::PARTICIPI(L"PARTICIPI");                    // participle
Symbol SpanishSTags::PREP(L"PREP");                              // preposition (perhaps multi word)
Symbol SpanishSTags::RELATIU(L"RELATIU");                        // relative
Symbol SpanishSTags::SENTENCE(L"SENTENCE");                      // top-level sentence
Symbol SpanishSTags::S(L"S");                                    // sentence
Symbol SpanishSTags::S_A(L"S.A");                                // adjectival phrase as noun complement
Symbol SpanishSTags::S_F_A(L"S.F.A");                            // adverbial comparative clause
Symbol SpanishSTags::S_F_C(L"S.F.C");                            // completive clause
Symbol SpanishSTags::S_F_R(L"S.F.R");                            // relative clause
Symbol SpanishSTags::S_NF_P(L"S.NF.P");                          // participle
Symbol SpanishSTags::SA(L"SA");                                  // argumental adjectival phrase
Symbol SpanishSTags::SADV(L"SADV");                              // adverbial phrase
Symbol SpanishSTags::SN(L"SN");                                  // nominal phrase
Symbol SpanishSTags::SP(L"SP");                                  // prepositional phrase
Symbol SpanishSTags::SPEC(L"SPEC");                              // specifier
// ======== Part of Speech Tags ==========
Symbol SpanishSTags::POS_AO(L"ao");                              // adjective (ordinal)
Symbol SpanishSTags::POS_AQ(L"aq");                              // adjective (qualificative)
const Symbol::SymbolGroup SpanishSTags::POS_A_GROUP = Symbol::makeSymbolGroup
	(L"ao aq");

Symbol SpanishSTags::POS_CC(L"cc");                              // conjunction (coordinating)
Symbol SpanishSTags::POS_CS(L"cs");                              // conjunction (subordinating)
const Symbol::SymbolGroup SpanishSTags::POS_C_GROUP = Symbol::makeSymbolGroup
	(L"cs cc");

Symbol SpanishSTags::POS_DA(L"da");                              // determiner (article)
Symbol SpanishSTags::POS_DD(L"dd");                              // determiner (demonstrative)
Symbol SpanishSTags::POS_DE(L"de");                              // determiner (exclamative)
Symbol SpanishSTags::POS_DI(L"di");                              // determiner (indefinite)
Symbol SpanishSTags::POS_DN(L"dn");                              // determiner (numeral)
Symbol SpanishSTags::POS_DP(L"dp");                              // determiner (possessive)
Symbol SpanishSTags::POS_DT(L"dt");                              // determiner (interrogative)
const Symbol::SymbolGroup SpanishSTags::POS_D_GROUP = Symbol::makeSymbolGroup
	(L"da dd de di dn dp dt");

Symbol SpanishSTags::POS_FAA(L"faa");                            // punctuation (open exclamationmark)
Symbol SpanishSTags::POS_FAT(L"fat");                            // punctuation (close exclamationmark)
Symbol SpanishSTags::POS_FC(L"fc");                              // punctuation (comma)
Symbol SpanishSTags::POS_FD(L"fd");                              // punctuation (colon)
Symbol SpanishSTags::POS_FE(L"fe");                              // punctuation (quotation)
Symbol SpanishSTags::POS_FG(L"fg");                              // punctuation (hyphen)
Symbol SpanishSTags::POS_FH(L"fh");                              // punctuation (slash)
Symbol SpanishSTags::POS_FIA(L"fia");                            // punctuation (open questionmark)
Symbol SpanishSTags::POS_FIT(L"fit");                            // punctuation (close questionmark)
Symbol SpanishSTags::POS_FP(L"fp");                              // punctuation (period)
Symbol SpanishSTags::POS_FPA(L"fpa");                            // punctuation (open bracket)
Symbol SpanishSTags::POS_FPT(L"fpt");                            // punctuation (close bracket)
Symbol SpanishSTags::POS_FS(L"fs");                              // punctuation (etc)
Symbol SpanishSTags::POS_FX(L"fx");                              // punctuation (semicolon)
Symbol SpanishSTags::POS_FZ(L"fz");                              // punctuation (mathsign)
const Symbol::SymbolGroup SpanishSTags::POS_F_GROUP = Symbol::makeSymbolGroup
	(L"faa fat fc fd fe fg fh fia fit fp fpa fpt fs fx fz");

Symbol SpanishSTags::POS_I(L"i");                                // interjection

Symbol SpanishSTags::POS_NC(L"nc");                              // noun (common)
Symbol SpanishSTags::POS_NP(L"np");                              // noun (proper)
const Symbol::SymbolGroup SpanishSTags::POS_N_GROUP = Symbol::makeSymbolGroup
	(L"nc np");

Symbol SpanishSTags::POS_PD(L"pd");                              // pronoun (demonstrative)
Symbol SpanishSTags::POS_PE(L"pe");                              // pronoun (exclamative)
Symbol SpanishSTags::POS_PI(L"pi");                              // pronoun (indefinite)
Symbol SpanishSTags::POS_PN(L"pn");                              // pronoun (numeral)
Symbol SpanishSTags::POS_PP(L"pp");                              // pronoun (personal)
Symbol SpanishSTags::POS_PR(L"pr");                              // pronoun (relative)
Symbol SpanishSTags::POS_PT(L"pt");                              // pronoun (interrogative)
Symbol SpanishSTags::POS_PX(L"px");                              // pronoun (possessive)
const Symbol::SymbolGroup SpanishSTags::POS_P_GROUP = Symbol::makeSymbolGroup
	(L"pd pe pi pn pp pr pt px");

Symbol SpanishSTags::POS_RG(L"rg");                              // adverb (general)
Symbol SpanishSTags::POS_RN(L"rn");                              // adverb (negative)
const Symbol::SymbolGroup SpanishSTags::POS_R_GROUP = Symbol::makeSymbolGroup
	(L"rg rn");

Symbol SpanishSTags::POS_S(L"s");                                // preposition word

Symbol SpanishSTags::POS_VAG(L"vag");                            // verb (auxilliary gerund)
Symbol SpanishSTags::POS_VAI(L"vai");                            // verb (auxilliary infinitive)
Symbol SpanishSTags::POS_VAM(L"vam");                            // verb (auxilliary imperative)
Symbol SpanishSTags::POS_VAN(L"van");                            // verb (auxilliary indicative)
Symbol SpanishSTags::POS_VAP(L"vap");                            // verb (auxilliary past participle)
Symbol SpanishSTags::POS_VAS(L"vas");                            // verb (auxilliary subjunctive)
Symbol SpanishSTags::POS_VMG(L"vmg");                            // verb (main gerund)
Symbol SpanishSTags::POS_VMI(L"vmi");                            // verb (main infinitive)
Symbol SpanishSTags::POS_VMM(L"vmm");                            // verb (main imperative)
Symbol SpanishSTags::POS_VMN(L"vmn");                            // verb (main indicative)
Symbol SpanishSTags::POS_VMP(L"vmp");                            // verb (main past participle)
Symbol SpanishSTags::POS_VMS(L"vms");                            // verb (main subjunctive)
Symbol SpanishSTags::POS_VSG(L"vsg");                            // verb (semiauxilliary gerund)
Symbol SpanishSTags::POS_VSI(L"vsi");                            // verb (semiauxilliary infinitive)
Symbol SpanishSTags::POS_VSM(L"vsm");                            // verb (semiauxilliary imperative)
Symbol SpanishSTags::POS_VSN(L"vsn");                            // verb (semiauxilliary indicative)
Symbol SpanishSTags::POS_VSP(L"vsp");                            // verb (semiauxilliary past participle)
Symbol SpanishSTags::POS_VSS(L"vss");                            // verb (semiauxilliary subjunctive)
const Symbol::SymbolGroup SpanishSTags::POS_V_GROUP = Symbol::makeSymbolGroup
	(L"vag vai vam van vap vas vmg vmi vmm vmn vmp vms vsg vsi vsm vsn vsp vss PARTICIPI");

Symbol SpanishSTags::POS_W(L"w");                                // date
Symbol SpanishSTags::POS_Z(L"z");                                // number
Symbol SpanishSTags::POS_ZC(L"zc");                              // number (currency)
const Symbol::SymbolGroup SpanishSTags::POS_Z_GROUP = Symbol::makeSymbolGroup
	(L"z zc");

// Special tags not found in the treebank
Symbol SpanishSTags::SNP(L"SNP"); // proper noun phrase
Symbol SpanishSTags::S_STAR(L"S*"); // verb-less sentence (usually coordinated to complete S)
Symbol SpanishSTags::S_F_C_STAR(L"S.F.C*"); // verb-less sentece complement phrase

// Aliases
Symbol SpanishSTags::COMMA(SpanishSTags::POS_FC); //L","
Symbol SpanishSTags::DATE(SpanishSTags::POS_W);   //L"w"
Symbol SpanishSTags::NP(SpanishSTags::SN);
Symbol SpanishSTags::PP(SpanishSTags::SP);
Symbol SpanishSTags::NPA(SpanishSTags::GRUP_NOM);

// Clauses with dropped pronoun subjects
Symbol SpanishSTags::S_DP_SBJ(L"S.dp-sbj");
Symbol SpanishSTags::S_F_A_DP_SBJ(L"S.F.A.dp-sbj");
Symbol SpanishSTags::S_F_A_J_DP_SBJ(L"S.F.A.j.dp-sbj");
Symbol SpanishSTags::S_F_C_P_DP_SBJ(L"S.F.C.p.dp-sbj");
Symbol SpanishSTags::S_F_C_DP_SBJ(L"S.F.C.dp-sbj");
Symbol SpanishSTags::S_F_R_DP_SBJ(L"S.F.R.dp-sbj");
Symbol SpanishSTags::S_J_DP_SBJ(L"S.j.dp-sbj");
Symbol SpanishSTags::SENTENCE_DP_SBJ(L"SENTENCE.dp-sbj");
 
void SpanishSTags::initializeTagList(std::vector<Symbol> tags) {
	tags.push_back(CONJ);
	tags.push_back(GERUNDI);
	tags.push_back(GRUP_A);
	tags.push_back(GRUP_ADV);
	tags.push_back(GRUP_NOM);
	tags.push_back(GRUP_VERB);
	tags.push_back(INC);
	tags.push_back(INFINITIU);
	tags.push_back(INTERJECCIO);
	tags.push_back(MORFEMA_PRONOMINAL);
	tags.push_back(MORFEMA_VERBAL);
	tags.push_back(NEG);
	tags.push_back(PARTICIPI);
	tags.push_back(PREP);
	tags.push_back(RELATIU);
	tags.push_back(S);
	tags.push_back(S_A);
	tags.push_back(S_F_A);
	tags.push_back(S_F_C);
	tags.push_back(S_F_R);
	tags.push_back(S_NF_P);
	tags.push_back(SA);
	tags.push_back(SADV);
	tags.push_back(SN);
	tags.push_back(SP);
	tags.push_back(SPEC);
	tags.push_back(POS_AO);
	tags.push_back(POS_AQ);
	tags.push_back(POS_CC);
	tags.push_back(POS_CS);
	tags.push_back(POS_DA);
	tags.push_back(POS_DD);
	tags.push_back(POS_DE);
	tags.push_back(POS_DI);
	tags.push_back(POS_DN);
	tags.push_back(POS_DP);
	tags.push_back(POS_DT);
	tags.push_back(POS_FAA);
	tags.push_back(POS_FAT);
	tags.push_back(POS_FC);
	tags.push_back(POS_FD);
	tags.push_back(POS_FE);
	tags.push_back(POS_FG);
	tags.push_back(POS_FH);
	tags.push_back(POS_FIA);
	tags.push_back(POS_FIT);
	tags.push_back(POS_FP);
	tags.push_back(POS_FPA);
	tags.push_back(POS_FPT);
	tags.push_back(POS_FS);
	tags.push_back(POS_FX);
	tags.push_back(POS_FZ);
	tags.push_back(POS_I);
	tags.push_back(POS_NC);
	tags.push_back(POS_NP);
	tags.push_back(POS_PD);
	tags.push_back(POS_PE);
	tags.push_back(POS_PI);
	tags.push_back(POS_PN);
	tags.push_back(POS_PP);
	tags.push_back(POS_PR);
	tags.push_back(POS_PT);
	tags.push_back(POS_PX);
	tags.push_back(POS_RG);
	tags.push_back(POS_RN);
	tags.push_back(POS_S);
	tags.push_back(POS_VAG);
	tags.push_back(POS_VAI);
	tags.push_back(POS_VAM);
	tags.push_back(POS_VAN);
	tags.push_back(POS_VAP);
	tags.push_back(POS_VAS);
	tags.push_back(POS_VMG);
	tags.push_back(POS_VMI);
	tags.push_back(POS_VMM);
	tags.push_back(POS_VMN);
	tags.push_back(POS_VMP);
	tags.push_back(POS_VMS);
	tags.push_back(POS_VSG);
	tags.push_back(POS_VSI);
	tags.push_back(POS_VSM);
	tags.push_back(POS_VSN);
	tags.push_back(POS_VSP);
	tags.push_back(POS_VSS);
	tags.push_back(POS_W);
	tags.push_back(POS_Z);
	tags.push_back(POS_ZC);

	// Special tags not found in the treebank
	tags.push_back(SNP);
	tags.push_back(S_STAR);
	tags.push_back(S_F_C_STAR);

	tags.push_back(S_DP_SBJ);
	tags.push_back(S_F_A_DP_SBJ);
	tags.push_back(S_F_A_J_DP_SBJ);
	tags.push_back(S_F_C_P_DP_SBJ);
	tags.push_back(S_F_C_DP_SBJ);
	tags.push_back(S_F_R_DP_SBJ);
	tags.push_back(S_J_DP_SBJ);
	tags.push_back(SENTENCE_DP_SBJ);

}
