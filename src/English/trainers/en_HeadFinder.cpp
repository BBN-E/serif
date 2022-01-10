// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "English/trainers/en_HeadFinder.h"
#include "Generic/common/Symbol.h"
#include "Generic/trainers/Production.h"
#include "English/parse/en_STags.h"


// called only after left, right, and number_of_right_side_elements have been 
//    set by Headify_to_string()
// 
// left = constituent category
// right[] = constituent's children
// number_of_right_side_elements = size of right[]
//
int EnglishHeadFinder::get_head_index() {

	if (production.left == EnglishSTags::ADJP)
		return findHeadADJP();
	else if (production.left == EnglishSTags::ADVP)
		return findHeadADVP();
	else if (production.left == EnglishSTags::CONJP)
		return findHeadCONJP();
	else if (production.left == EnglishSTags::FRAG)
		return findHeadFRAG();
	else if (production.left == EnglishSTags::INTJ)
		return findHeadINTJ();
	else if (production.left == EnglishSTags::LST)
		return findHeadLST();
	else if (production.left == EnglishSTags::NAC)
		return findHeadNAC();
	else if (production.left == EnglishSTags::NP)
		return findHeadNP();
	else if (production.left == EnglishSTags::NPA)
		return findHeadNP();
	else if (production.left == EnglishSTags::NPP)
		return findHeadNP();
	else if (production.left == EnglishSTags::NPPOS)
		return findHeadNP();
	else if (production.left == EnglishSTags::PP)
		return findHeadPP();
	else if (production.left == EnglishSTags::PRN)
		return findHeadPRN();
	else if (production.left == EnglishSTags::PRT)
		return findHeadPRT();
	else if (production.left == EnglishSTags::QP)
		return findHeadQP();
	else if (production.left == EnglishSTags::RRC)
		return findHeadRRC();
	else if (production.left == EnglishSTags::S)
		return findHeadS();
	else if (production.left == EnglishSTags::SBAR)
		return findHeadSBAR();
	else if (production.left == EnglishSTags::SBARQ)
		return findHeadSBARQ();
	else if (production.left == EnglishSTags::SINV)
		return findHeadSINV();
	else if (production.left == EnglishSTags::SQ)
		return findHeadSQ();
	else if (production.left == EnglishSTags::UCP)
		return findHeadUCP();
	else if (production.left == EnglishSTags::VP)
		return findHeadVP();
	else if (production.left == EnglishSTags::WHADJP)
		return findHeadWHADJP();
	else if (production.left == EnglishSTags::WHADVP)
		return findHeadWHADVP();
	else if (production.left == EnglishSTags::WHNP)
		return findHeadWHNP();
	else if (production.left == EnglishSTags::WHPP)
		return findHeadWHPP();
		

	else return 0;

}

// priority_scan searches go through the tags in "set" one by one to see if
//		they match the elements in right[]
// scan searches go through the elements of right[] one by one to see if
//		they match any of the tags in "set"
//
// i.e. in priority_scan, the order of the tags in the set matters,
//   and in scan it doesn't
//
// left_to_right / right_to_left obviously refers to the order in which it
//   searches through the elements of right[], a.k.a. the children 
//   of the node whose head we're trying to find


int EnglishHeadFinder::findHeadADJP()
{
	Symbol set[22] = {EnglishSTags::NNS, EnglishSTags::QP, EnglishSTags::NN, EnglishSTags::DOLLAR, EnglishSTags::ADVP, 
		    EnglishSTags::JJ, EnglishSTags::VBN, EnglishSTags::VBG, EnglishSTags::ADJP, EnglishSTags::JJR, 
			EnglishSTags::NP, EnglishSTags::NPA, EnglishSTags::DATE, EnglishSTags::NPP, EnglishSTags::NPPOS, 
			EnglishSTags::JJS, EnglishSTags::DT, EnglishSTags::FW, EnglishSTags::RBR, EnglishSTags::RBS, 
			EnglishSTags::SBAR, EnglishSTags::RB};
	return priority_scan_from_left_to_right(set, 22);
}

int EnglishHeadFinder::findHeadADVP()
{
	Symbol set[17] = {EnglishSTags::RB, EnglishSTags::RBR, EnglishSTags::RBS, EnglishSTags::FW, EnglishSTags::ADVP, 
		    EnglishSTags::TO, EnglishSTags::CD, EnglishSTags::JJR, EnglishSTags::JJ, EnglishSTags::IN, 
			EnglishSTags::NP, EnglishSTags::NPA, EnglishSTags::DATE, EnglishSTags::NPP, EnglishSTags::NPPOS, 
			EnglishSTags::JJS, EnglishSTags::NN};
	return priority_scan_from_right_to_left (set, 17);
}

int EnglishHeadFinder::findHeadCONJP()
{
	Symbol set[3] = {EnglishSTags::CC, EnglishSTags::RB, EnglishSTags::IN};
	return priority_scan_from_right_to_left (set, 3);
}

int EnglishHeadFinder::findHeadFRAG() 
{
	return (production.number_of_right_side_elements - 1);
}

int EnglishHeadFinder::findHeadINTJ()
{
	return 0;
}

int EnglishHeadFinder::findHeadLST()
{
	Symbol set[2] = {EnglishSTags::LS, EnglishSTags::COLON};
	return priority_scan_from_right_to_left (set, 2);
}

int EnglishHeadFinder::findHeadNAC()
{
	Symbol set[22] = {EnglishSTags::NN, EnglishSTags::NNS, EnglishSTags::NNP, EnglishSTags::NPP, EnglishSTags::NNPS, 
		    EnglishSTags::NP, EnglishSTags::NPA, EnglishSTags::DATE, EnglishSTags::NPP, EnglishSTags::NPPOS, 
			EnglishSTags::NAC, EnglishSTags::EX, EnglishSTags::DOLLAR, EnglishSTags::CD, EnglishSTags::QP, 
			EnglishSTags::PRP,	EnglishSTags::VBG, EnglishSTags::JJ, EnglishSTags::JJS, EnglishSTags::JJR,
			EnglishSTags::ADJP, EnglishSTags::FW};
	return priority_scan_from_left_to_right (set, 22);
}

int EnglishHeadFinder::findHeadPP()
{
	Symbol set[6] = {EnglishSTags::IN, EnglishSTags::TO, EnglishSTags::VBG, EnglishSTags::VBN, EnglishSTags::RP,
		EnglishSTags::FW};
	return priority_scan_from_right_to_left (set, 6);
}

int EnglishHeadFinder::findHeadPRN()
{
	return 0;
}

int EnglishHeadFinder::findHeadPRT()
{
	Symbol set[1] = {EnglishSTags::RP};
	return priority_scan_from_right_to_left (set, 1);
}

int EnglishHeadFinder::findHeadQP()
{
	Symbol set[14] = {EnglishSTags::DOLLAR, EnglishSTags::IN, EnglishSTags::NNS, EnglishSTags::NN, EnglishSTags::NPP, 
		EnglishSTags::DATE, EnglishSTags::JJ, EnglishSTags::RB, EnglishSTags::DT, EnglishSTags::CD,
		EnglishSTags::NCD,	EnglishSTags::QP, EnglishSTags::JJR, EnglishSTags::JJS};
	return priority_scan_from_left_to_right (set, 14);
}

int EnglishHeadFinder::findHeadRRC()
{
	Symbol set[9] = {EnglishSTags::VP, EnglishSTags::NP, EnglishSTags::NPA, EnglishSTags::DATE, EnglishSTags::NPP,
		EnglishSTags::NPPOS, EnglishSTags::ADVP, EnglishSTags::ADJP, EnglishSTags::PP};
	return priority_scan_from_right_to_left (set, 9);
}

int EnglishHeadFinder::findHeadS()
{
	Symbol set[12] = {EnglishSTags::TO, EnglishSTags::IN, EnglishSTags::VP, EnglishSTags::S, EnglishSTags::SBAR, 
		    EnglishSTags::ADJP, EnglishSTags::UCP, EnglishSTags::NP, EnglishSTags::NPA, EnglishSTags::NPP,
			EnglishSTags::DATE, EnglishSTags::NPPOS};
	return priority_scan_from_left_to_right (set, 12);
}

int EnglishHeadFinder::findHeadSBAR()
{
	Symbol set[11] = {EnglishSTags::WHNP, EnglishSTags::WHPP, EnglishSTags::WHADVP, EnglishSTags::WHADJP, EnglishSTags::IN,
		EnglishSTags::DT, EnglishSTags::S, EnglishSTags::SQ, EnglishSTags::SINV, EnglishSTags::SBAR, 
		EnglishSTags::FRAG};
	return priority_scan_from_left_to_right (set, 11);
}

int EnglishHeadFinder::findHeadSBARQ()
{
	Symbol set[5] = {EnglishSTags::SQ, EnglishSTags::S, EnglishSTags::SINV, EnglishSTags::SBARQ, EnglishSTags::FRAG};
	return priority_scan_from_left_to_right (set, 5);
}

int EnglishHeadFinder::findHeadSINV()
{
	Symbol set[14] = {EnglishSTags::VBZ, EnglishSTags::VBD, EnglishSTags::VBP, EnglishSTags::VB, EnglishSTags::MD, 
		    EnglishSTags::VP, EnglishSTags::S, EnglishSTags::SINV, EnglishSTags::ADJP, EnglishSTags::NP, 
			EnglishSTags::NPA, EnglishSTags::DATE, EnglishSTags::NPP, EnglishSTags::NPPOS};
	return priority_scan_from_left_to_right (set, 14);
}

int EnglishHeadFinder::findHeadSQ()
{
	Symbol set[7] = {EnglishSTags::VBZ, EnglishSTags::VBD, EnglishSTags::VBP, EnglishSTags::VB, EnglishSTags::MD, 
		    EnglishSTags::VP, EnglishSTags::SQ};
	return priority_scan_from_left_to_right (set, 7);
}

int EnglishHeadFinder::findHeadUCP() 
{
	return (production.number_of_right_side_elements - 1);
}

int EnglishHeadFinder::findHeadVP()
{
	Symbol set[17] = {EnglishSTags::TO, EnglishSTags::VBD, EnglishSTags::VBN, EnglishSTags::MD, EnglishSTags::VBZ, 
		    EnglishSTags::VB, EnglishSTags::VBG, EnglishSTags::VBP, EnglishSTags::VP, EnglishSTags::ADJP,
			EnglishSTags::NN, EnglishSTags::NNS, EnglishSTags::NP, EnglishSTags::NPA, EnglishSTags::NPP,
			EnglishSTags::DATE, EnglishSTags::NPPOS};
	return priority_scan_from_left_to_right (set, 17);
}

int EnglishHeadFinder::findHeadWHADJP()
{
	Symbol set[4] = {EnglishSTags::CC, EnglishSTags::WRB, EnglishSTags::JJ, EnglishSTags::ADJP};
	return priority_scan_from_left_to_right (set, 4);
}

int EnglishHeadFinder::findHeadWHADVP()
{
	Symbol set[2] = {EnglishSTags::CC, EnglishSTags::WRB};
	return priority_scan_from_right_to_left (set, 2);
}

int EnglishHeadFinder::findHeadWHNP()
{
	Symbol set[6] = {EnglishSTags::WDT, EnglishSTags::WP, EnglishSTags::WPDOLLAR, EnglishSTags::WHADJP, EnglishSTags::WHPP,
		EnglishSTags::WHNP};
	return priority_scan_from_left_to_right (set, 6);
}

int EnglishHeadFinder::findHeadWHPP()
{
	Symbol set[3] = {EnglishSTags::IN, EnglishSTags::TO, EnglishSTags::FW};
	return priority_scan_from_right_to_left (set, 3);
}


int EnglishHeadFinder::priority_scan_from_left_to_right (Symbol* set, int size)
{
	for (int j = 0; j < size; j++)
	{
		for (int k = 0; k < production.number_of_right_side_elements; k++)
		{
			if (production.right[k] == set[j])
				return k;
		}
	}
	return 0;
}

int EnglishHeadFinder::priority_scan_from_right_to_left (Symbol* set, int size)
{
	for (int j = 0; j < size; j++)
	{
		for (int k = production.number_of_right_side_elements - 1; k >= 0; k--)
		{
			if (production.right[k] == set[j])
				return k;
		}
	}
	return production.number_of_right_side_elements - 1;
}

int EnglishHeadFinder::findHeadNP()
{
	int result;

	if (production.right[production.number_of_right_side_elements - 1] == EnglishSTags::POS)
		return (production.number_of_right_side_elements - 1);

	Symbol set1[10] = {EnglishSTags::NN, EnglishSTags::NNP, EnglishSTags::NPP, EnglishSTags::NNPS, EnglishSTags::NPP,
			EnglishSTags::DATE, EnglishSTags::NNS, EnglishSTags::NX, EnglishSTags::POS, EnglishSTags::JJR};
	result = scan_from_right_to_left (set1, 10);
	if (result >= 0)
		return result;

	Symbol set2[3] = {EnglishSTags::NP, EnglishSTags::NPA, EnglishSTags::NPPOS};
	result = scan_from_left_to_right (set2, 3);
	if (result >= 0)
		return result;


	Symbol set3[3] = {EnglishSTags::DOLLAR, EnglishSTags::ADJP, EnglishSTags::PRN};
	result = scan_from_right_to_left (set3, 3);
	if (result >= 0)
		return result;

	Symbol set4[1]= {EnglishSTags::CD};
	result = scan_from_right_to_left (set4, 1);
	if (result >= 0)
		return result;

	Symbol set5[4]= {EnglishSTags::JJ, EnglishSTags::JJS, EnglishSTags::RB, EnglishSTags::QP};
	result = scan_from_right_to_left (set5, 4);
	if (result >= 0)
		return result;

	return (production.number_of_right_side_elements - 1);

}


int EnglishHeadFinder::scan_from_left_to_right (Symbol* set, int size)
{
	for (int j = 0; j < production.number_of_right_side_elements; j++) {
		for (int k = 0; k < size; k++) {
			if (production.right[j] == set[k])
				return j;
		}
	}
	return -1;
}

int EnglishHeadFinder::scan_from_right_to_left (Symbol* set, int size)
{
	for (int j = production.number_of_right_side_elements - 1; j >= 0; j--) {
		for (int k = 0; k < size; k++) {
			if (production.right[j] == set[k])
				return j;
		}
	}
	return -1;
}
