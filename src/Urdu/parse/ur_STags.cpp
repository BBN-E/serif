// Copyright 2013 Raytheon BBN Technologies 
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Urdu/parse/ur_STags.h"


// ======== Part of Speech Tags (HTB-v2.5) =======
Symbol UrduSTags::POS_CC(L"CC");
Symbol UrduSTags::POS_CCC(L"CCC");
Symbol UrduSTags::POS_CL(L"CL");
Symbol UrduSTags::POS_DEM(L"DEM");
Symbol UrduSTags::POS_ECH(L"ECH");
Symbol UrduSTags::POS_INTF(L"INTF");
Symbol UrduSTags::POS_INJ(L"INJ");
Symbol UrduSTags::POS_JJ(L"JJ");
Symbol UrduSTags::POS_JJC(L"JJC");
Symbol UrduSTags::POS_NEG(L"NEG");
Symbol UrduSTags::POS_NN(L"NN");
Symbol UrduSTags::POS_NNC(L"NNC");
Symbol UrduSTags::POS_NNP(L"NNP");
Symbol UrduSTags::POS_NNPC(L"NNPC");
Symbol UrduSTags::POS_NST(L"NST");
Symbol UrduSTags::POS_NSTC(L"NSTC");
Symbol UrduSTags::POS_NULL(L"NULL");
Symbol UrduSTags::POS_PRP(L"PRP");
Symbol UrduSTags::POS_PRPC(L"PRPC");
Symbol UrduSTags::POS_PSP(L"PSP");
Symbol UrduSTags::POS_PSPC(L"PSPC");
Symbol UrduSTags::POS_QC(L"QC");
Symbol UrduSTags::POS_QCC(L"QCC");
Symbol UrduSTags::POS_QF(L"QF");
Symbol UrduSTags::POS_QFC(L"QFC");
Symbol UrduSTags::POS_QO(L"QO");
Symbol UrduSTags::POS_RB(L"RB");
Symbol UrduSTags::POS_RBC(L"RBC");
Symbol UrduSTags::POS_RDP(L"RDP");
Symbol UrduSTags::POS_RP(L"RP");
Symbol UrduSTags::POS_RPC(L"RPC");
Symbol UrduSTags::POS_SYM(L"SYM");
Symbol UrduSTags::POS_SYMC(L"SYMC");
Symbol UrduSTags::POS_UNK(L"UNK");
Symbol UrduSTags::POS_UNKC(L"UNKC");
Symbol UrduSTags::POS_UT(L"UT");
Symbol UrduSTags::POS_VAUX(L"VAUX");
Symbol UrduSTags::POS_VM(L"VM");
Symbol UrduSTags::POS_VMC(L"VMC");
Symbol UrduSTags::POS_WQ(L"WQ");

// ======== Chunk Tags (HTB-v2.5) ================
Symbol UrduSTags::BLK(L"BLK");
Symbol UrduSTags::CCP(L"CCP");
Symbol UrduSTags::FRAGP(L"FRAGP");
Symbol UrduSTags::JJP(L"JJP");
Symbol UrduSTags::NEGP(L"NEGP");
Symbol UrduSTags::NP(L"NP");
Symbol UrduSTags::RBP(L"RBP");
Symbol UrduSTags::VGF(L"VGF");
Symbol UrduSTags::VGINF(L"VGINF");
Symbol UrduSTags::VGNF(L"VGNF");
Symbol UrduSTags::VGNN(L"VGNN");

// ======== Paninian Tags (HTB-v2.5) =======
Symbol UrduSTags::ADV(L"adv");
Symbol UrduSTags::CCOF(L"ccof");
Symbol UrduSTags::ENM(L"enm");
Symbol UrduSTags::FRAGOF(L"fragof");
Symbol UrduSTags::JJMOD(L"jjmod");
Symbol UrduSTags::JJMOD_INTF(L"jjmod__intf");
Symbol UrduSTags::JJMOD_RELC(L"jjmod__relc");
Symbol UrduSTags::JK1(L"jk1");
Symbol UrduSTags::K1(L"k1");
Symbol UrduSTags::K1S(L"k1s");
Symbol UrduSTags::K1U(L"k1u");
Symbol UrduSTags::K2(L"k2");
Symbol UrduSTags::K2G(L"k2g");
Symbol UrduSTags::K2P(L"k2p");
Symbol UrduSTags::K2S(L"k2s");
Symbol UrduSTags::K2U(L"k2u");
Symbol UrduSTags::K3(L"k3");
Symbol UrduSTags::K3U(L"k3u");
Symbol UrduSTags::K4(L"k4");
Symbol UrduSTags::K4A(L"k4a");
Symbol UrduSTags::K4U(L"k4u");
Symbol UrduSTags::K5(L"k5");
Symbol UrduSTags::K5PRK(L"k5prk");
Symbol UrduSTags::K5U(L"k5u");
Symbol UrduSTags::K7(L"k7");
Symbol UrduSTags::K7A(L"k7a");
Symbol UrduSTags::K7P(L"k7p");
Symbol UrduSTags::K7PU(L"k7pu");
Symbol UrduSTags::K7T(L"k7t");
Symbol UrduSTags::K7TU(L"k7tu");
Symbol UrduSTags::K7U(L"k7u");
Symbol UrduSTags::LWG(L"lwg");
Symbol UrduSTags::LWG_CONT(L"lwg__cont");
Symbol UrduSTags::LWG_NEG(L"lwg__neg");
Symbol UrduSTags::LWG_PSP(L"lwg__psp"); // also lwg_psp
Symbol UrduSTags::LWG_QF(L"lwg__qf");
Symbol UrduSTags::LWG_RP(L"lwg__rp");
Symbol UrduSTags::LWG_UNK(L"lwg__unk");
Symbol UrduSTags::LWG_VAUX(L"lwg__vaux"); // also lwg_vaux
Symbol UrduSTags::LWG_VM(L"lwg__vm");
Symbol UrduSTags::MK1(L"mk1");
Symbol UrduSTags::MOD(L"mod");
Symbol UrduSTags::MOD_CC(L"mod__cc");
Symbol UrduSTags::MOD_NULL(L"mod__null");
Symbol UrduSTags::MOD_WQ(L"mod__wq");
Symbol UrduSTags::NMOD(L"nmod");
Symbol UrduSTags::NMOD_ADJ(L"nmod__adj"); // also nmod_adj
Symbol UrduSTags::NMOD_EMPH(L"nmod__emph");
Symbol UrduSTags::NMOD_K1INV(L"nmod__k1inv");
Symbol UrduSTags::NMOD_K2INV(L"nmod__k2inv");
Symbol UrduSTags::NMOD_K3INV(L"nmod__k3inv");
Symbol UrduSTags::NMOD_K4INV(L"nmod__k4inv");
Symbol UrduSTags::NMOD_K5INV(L"nmod__k5inv");
Symbol UrduSTags::NMOD_K7INV(L"nmod__k7inv");
Symbol UrduSTags::NMOD_NEG(L"nmod__neg");
Symbol UrduSTags::NMOD_POFINV(L"nmod__pofinv");
Symbol UrduSTags::NMOD_RELC(L"nmod__relc");
Symbol UrduSTags::PK1(L"pk1");
Symbol UrduSTags::POF(L"pof");
Symbol UrduSTags::POF_CN(L"pof__cn");
Symbol UrduSTags::POF_INV(L"pof__inv");
Symbol UrduSTags::POF_JJ(L"pof__jj");
Symbol UrduSTags::POF_QC(L"pof__qc");
Symbol UrduSTags::POF_REDUP(L"pof__redup");
Symbol UrduSTags::POF_IDIOM(L"pof_idiom"); // one underscore in hindi data
Symbol UrduSTags::PSP_CL(L"psp__cl");
Symbol UrduSTags::R6(L"r6");
Symbol UrduSTags::R6_K1(L"r6-k1");
Symbol UrduSTags::R6_K2(L"r6-k2");
Symbol UrduSTags::R6_K2S(L"r6-k2s");
Symbol UrduSTags::R6_K2U(L"r6-k2u");
Symbol UrduSTags::R6V(L"r6v");
Symbol UrduSTags::RAD(L"rad");
Symbol UrduSTags::RAS_K1(L"ras-k1");
Symbol UrduSTags::RAS_K1U(L"ras-k1u");
Symbol UrduSTags::RAS_K2(L"ras-k2");
Symbol UrduSTags::RAS_K3(L"ras-k3");
Symbol UrduSTags::RAS_K4(L"ras-k4");
Symbol UrduSTags::RAS_K5(L"ras-k5");
Symbol UrduSTags::RAS_K7(L"ras-k7");
Symbol UrduSTags::RAS_K7P(L"ras-k7p");
Symbol UrduSTags::RAS_NEG(L"ras-neg");
Symbol UrduSTags::RAS_POF(L"ras-pof");
Symbol UrduSTags::RAS_R6(L"ras-r6");
Symbol UrduSTags::RAS_R6_K2(L"ras-r6-k2");
Symbol UrduSTags::RAS_RT(L"ras-rt");
Symbol UrduSTags::RBMOD(L"rbmod");
Symbol UrduSTags::RBMOD_RELC(L"rbmod__relc");
Symbol UrduSTags::RD(L"rd");
Symbol UrduSTags::RH(L"rh");
Symbol UrduSTags::RS(L"rs");
Symbol UrduSTags::RSP(L"rsp");
Symbol UrduSTags::RSYM(L"rsym");
Symbol UrduSTags::RT(L"rt");
Symbol UrduSTags::RTU(L"rtu");
Symbol UrduSTags::SENT_ADV(L"sent-adv");
Symbol UrduSTags::VMOD(L"vmod");
Symbol UrduSTags::VMOD_ADV(L"vmod__adv");

// ======== Phrase Structure Tags ==========
// from original documentation, not from htb
Symbol UrduSTags::A(L"A");
Symbol UrduSTags::AP_MOD(L"AP-Mod");
Symbol UrduSTags::AP_PRED(L"AP-Pred");
Symbol UrduSTags::C(L"C");
Symbol UrduSTags::CC(L"CC");
//Symbol UrduSTags::CCP(L"CCP");
Symbol UrduSTags::CP(L"CP");
Symbol UrduSTags::CP_2(L"CP,2");
Symbol UrduSTags::DEG(L"Deg");
Symbol UrduSTags::DEGP(L"DegP");
Symbol UrduSTags::DEM(L"Dem");
Symbol UrduSTags::FOC(L"Foc");
Symbol UrduSTags::N(L"N");
//Symbol UrduSTags::NEG(L"Neg");
//Symbol UrduSTags::NP(L"NP");
Symbol UrduSTags::NPA(L"NP"); // generic/parse/STags.h requires a 'npa' symbol
Symbol UrduSTags::NP_2(L"NP,2");
Symbol UrduSTags::NP_P(L"NP-P");
Symbol UrduSTags::NP_PRED(L"NP-Pred");
Symbol UrduSTags::NP_P_PRED(L"NP-P-Pred");
Symbol UrduSTags::NP_V(L"NP-V");
Symbol UrduSTags::NP_V_P(L"NP-V-P");
Symbol UrduSTags::NST(L"NST");
Symbol UrduSTags::NSTP(L"NSTP");
Symbol UrduSTags::NUM(L"Num");
Symbol UrduSTags::N_P(L"N-P");
Symbol UrduSTags::N_PROP(L"N-Prop");
Symbol UrduSTags::P(L"P");
Symbol UrduSTags::PP(L"P"); // generic/parse/STags.h requires a 'pp' symbol
Symbol UrduSTags::QAP(L"QAP");
Symbol UrduSTags::QP(L"QP");
Symbol UrduSTags::SC_A(L"SC-A");
Symbol UrduSTags::SC_DEG(L"SC-Deg");
Symbol UrduSTags::SC_N(L"SC-N");
Symbol UrduSTags::SC_P(L"SC-P");
Symbol UrduSTags::V_(L"V'");
Symbol UrduSTags::V(L"V");
Symbol UrduSTags::VP(L"VP");
Symbol UrduSTags::VP_PART(L"VP-Part");
Symbol UrduSTags::VP_PRED(L"VP-Pred");
Symbol UrduSTags::VP_PRED_PART(L"VP-Pred-Part");
Symbol UrduSTags::V_AUX(L"V-Aux");
Symbol UrduSTags::V_PART(L"V-Part");

Symbol UrduSTags::COMMA(L","); // generic/parse/STags.h requires a 'comma' symbol
Symbol UrduSTags::DATE(L"DATE"); // generic/parse/STags.h requires a 'date' symbol

void UrduSTags::initializeTagList(std::vector<Symbol> tags) {
	tags.push_back(POS_CC);
	tags.push_back(POS_CCC);
	tags.push_back(POS_CL);
	tags.push_back(POS_DEM);
	tags.push_back(POS_ECH);
	tags.push_back(POS_INTF);
	tags.push_back(POS_INJ);
	tags.push_back(POS_JJ);
	tags.push_back(POS_JJC);
	tags.push_back(POS_NEG);
	tags.push_back(POS_NN);
	tags.push_back(POS_NNC);
	tags.push_back(POS_NNP);
	tags.push_back(POS_NNPC);
	tags.push_back(POS_NST);
	tags.push_back(POS_NULL);
	tags.push_back(POS_PRP);
	tags.push_back(POS_PRPC);
	tags.push_back(POS_PSP);
	tags.push_back(POS_PSPC);
	tags.push_back(POS_QC);
	tags.push_back(POS_QCC);
	tags.push_back(POS_QF);
	tags.push_back(POS_QFC);
	tags.push_back(POS_QO);
	tags.push_back(POS_RB);
	tags.push_back(POS_RBC);
	tags.push_back(POS_RDP);
	tags.push_back(POS_RP);
	tags.push_back(POS_RPC);
	tags.push_back(POS_SYM);
	tags.push_back(POS_SYMC);
	tags.push_back(POS_UNK);
	tags.push_back(POS_UNKC);
	tags.push_back(POS_UT);
	tags.push_back(POS_VAUX);
	tags.push_back(POS_VM);
	tags.push_back(POS_VMC);
	tags.push_back(POS_WQ);
	tags.push_back(BLK);
	tags.push_back(CCP);
	tags.push_back(FRAGP);
	tags.push_back(JJP);
	tags.push_back(NEGP);
	tags.push_back(NP);
	tags.push_back(RBP);
	tags.push_back(VGF);
	tags.push_back(VGINF);
	tags.push_back(VGNF);
	tags.push_back(VGNN);
}

