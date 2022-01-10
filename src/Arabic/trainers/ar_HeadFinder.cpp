// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Arabic/trainers/ar_HeadFinder.h"
#include "Generic/common/Symbol.h"
#include "Generic/trainers/Production.h"
#include "Arabic/parse/ar_STags.h"

//ArabicHeadFinder::production; //this causes an error
// called only after left, right, and number_of_right_side_elements have been 
//    set!
// 
// left = constituent category
// right[] = constituent's children
// number_of_right_side_elements = size of right[]
//9-15-05 I modified the head finder to use the new POS tags, and updated to PENN head rules

//

int ArabicHeadFinder::get_head_index()
{
    int result = -1;;
	if (production.left == ArabicSTags::ADJP)
		result = findHeadADJP();
	else if (production.left == ArabicSTags::ADVP)
		result = findHeadADVP();
	else if (production.left == ArabicSTags::NAC)
		result = findHeadNAC();
	else if (production.left == ArabicSTags::NP)
		result = findHeadNP();
	else if (production.left == ArabicSTags::NX)
		result = findHeadNX();
	else if (production.left == ArabicSTags::PP)
		result = findHeadPP();
	else if (production.left == ArabicSTags::S)
		result = findHeadS();
	else if (production.left == ArabicSTags::SBAR)
		result = findHeadSBAR();
	else if (production.left == ArabicSTags::SBARQ)
		result = findHeadSBARQ();
	else if (production.left == ArabicSTags::SQ)
		result = findHeadSQ();
	else if (production.left == ArabicSTags::VP)
		result = findHeadVP();
	else if (production.left == ArabicSTags::UCP)
		result = findHeadUCP();
	else if (production.left == ArabicSTags::RRC)
		result = findHeadRRC();
	else if (production.left == ArabicSTags::WHNP)
		result = findHeadWHNP();
	if(result == -1){
		return 0;
	}
	else{
		return result;
	}

    /*
	Symbol set3[37] = { ArabicSTags::EXCEPT_PART, ArabicSTags::NEG_PART, ArabicSTags::PART, ArabicSTags::INTERROG_PART, 
		ArabicSTags::RESULT_CLAUSE_PARTICLE, ArabicSTags::EMPHATIC_PARTICLE,
		ArabicSTags::IV2MP, ArabicSTags::IV2MS, ArabicSTags::IV2FS,  ArabicSTags::IV3MD, ArabicSTags::IV3MP, ArabicSTags::IV3MS, 
		ArabicSTags::IV3FD, ArabicSTags::IV3FP, ArabicSTags::IV3FS, ArabicSTags::IV1P, ArabicSTags::IV1S, ArabicSTags::IV2D,  
		ArabicSTags::CONJ, ArabicSTags::DET, ArabicSTags::INTERJ, ArabicSTags::FSLASH, ArabicSTags::FUT, ArabicSTags::HYPHEN, 
		ArabicSTags::LRB, ArabicSTags::RRB, ArabicSTags::DQUOTE, ArabicSTags::DOT, ArabicSTags::SEMICOLON, ArabicSTags::COLON, 
		ArabicSTags::PLUS, ArabicSTags::QUESTION, 
		// Added to handled reduced tag set
		ArabicSTags::EXCEPT, ArabicSTags::NEG, ArabicSTags::INTERROG, ArabicSTags::RESULT, ArabicSTags::EMPHATIC };
	*/
	/*
	//old head rules, do not vary by constituent
	Symbol set2[4] = {ArabicSTags::VB, ArabicSTags::VBD, ArabicSTags::VBP, ArabicSTags::VBN};
	result = scan_from_left_to_right(set2, 4);
	if (result >= 0)
		return result;
	
	Symbol set[1] = {ArabicSTags::VP};
	result = scan_from_left_to_right(set, 1);
	if (result >= 0) 
		return result;

	Symbol set3[20] = { 
		 ArabicSTags::RP, ArabicSTags::DT, ArabicSTags::CC, ArabicSTags::UH,
		 ArabicSTags::FSLASH, ArabicSTags::HYPHEN, ArabicSTags::LRB, ArabicSTags::RRB, 
		 ArabicSTags::DOT, ArabicSTags::SEMICOLON, ArabicSTags::COLON,  ArabicSTags::PLUS, 
		 ArabicSTags::QUESTION, ArabicSTags::BSLASH, ArabicSTags::SEMICOLON, ArabicSTags::COMMA, 
		 ArabicSTags::NUMERIC_COMMA, ArabicSTags::EX_POINT, ArabicSTags::PUNC,  ArabicSTags::DQUOTE };
	result = nonmatch_scan_from_left_to_right(set3, 19);
	if (result >= 0)
		return result;

	return 0;
	*/
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
int ArabicHeadFinder::scan_from_left_to_right_by_set (Symbol* set, int size)
{
	for (int j = 0; j < size; j++){
		for (int k = 0; k < production.number_of_right_side_elements; k++){
			if (production.right[k] == set[j])
				return k;
		}
	}
	return -1;
}

int ArabicHeadFinder::scan_from_right_to_left_by_set (Symbol* set, int size)
{
	for (int j = 0; j < size; j++){
		for (int k = production.number_of_right_side_elements - 1; k >= 0; k--)	{
			if (production.right[k] == set[j])
				return k;
		}
	}
	return -1;
}
int ArabicHeadFinder::scan_from_left_to_right (Symbol* set, int size)
{
	for (int j = 0; j < production.number_of_right_side_elements; j++) {
		for (int k = 0; k < size; k++) {
			if (production.right[j] == set[k])
				return j;
		}
	}
	return -1;
}

int ArabicHeadFinder::scan_from_right_to_left (Symbol* set, int size)
{
	for (int j = production.number_of_right_side_elements - 1; j >= 0; j--) {
		for (int k = 0; k < size; k++) {
			if (production.right[j] == set[k])
				return j;
		}
	}
	return -1;
}

int ArabicHeadFinder::nonmatch_scan_from_left_to_right (Symbol* set, int size)
{
	
	for (int j = 0; j < production.number_of_right_side_elements; j++) {
	    bool found = false;
		for (int k = 0; k < size; k++) {
			if (production.right[j] == set[k])
				found = true;
		}
		if (!found)
			return j;
	}
	return -1;
}

int ArabicHeadFinder::findHeadS(){
	int result = -1;
	Symbol set[1] = {ArabicSTags::VP};
	result = scan_from_right_to_left(set, 1);
	if (result >= 0)
		return result;
	return -1;
}
int ArabicHeadFinder::findHeadSQ(){
	int result = -1;
	Symbol set[1] = {ArabicSTags::VP};
	result = scan_from_right_to_left(set, 1);
	if (result >= 0)
		return result;
	return -1;
}
int ArabicHeadFinder::findHeadVP(){
	int result = -1;
	// DV was added for ATB_3.2
	Symbol set1[4] = {ArabicSTags::VBD, ArabicSTags::VBP, ArabicSTags::VBN}; 
	result = scan_from_left_to_right(set1,3);
/*	Symbol set1[4] = {ArabicSTags::VBD, ArabicSTags::VBP, ArabicSTags::VBN, ArabicSTags::DV}; 
	result = scan_from_left_to_right(set1,4); // parser experiments */
	if (result >= 0)
		return result;
	Symbol set2[1] = {ArabicSTags::VP};
	result = scan_from_right_to_left(set2, 1);
	if (result >= 0)
		return result;
	return -1;
}
int ArabicHeadFinder::findHeadNP(){
	int result = -1;
	Symbol set1[8] = {ArabicSTags::NN, ArabicSTags::NNS, ArabicSTags::NNP, ArabicSTags::NNPS,
		ArabicSTags::DET_NN, ArabicSTags::DET_NNS, ArabicSTags::DET_NNP, ArabicSTags::DET_NNPS};
	result = scan_from_left_to_right(set1, 8);
/*	Symbol set1[10] = {ArabicSTags::NN, ArabicSTags::NNS, ArabicSTags::NNP, ArabicSTags::NNPS,
		ArabicSTags::DET_NN, ArabicSTags::DET_NNS, ArabicSTags::DET_NNP, ArabicSTags::DET_NNPS,
		ArabicSTags::DET_NQ, ArabicSTags::NQ};
	result = scan_from_left_to_right(set1, 10); // parser experiments */
	if (result >= 0)
		return result;
	Symbol set2[5] = {ArabicSTags::NP, ArabicSTags::NPP, ArabicSTags::NPA, ArabicSTags::DEF_NP, ArabicSTags::DEF_NPA};
	result = scan_from_right_to_left(set2, 5);
	if (result >= 0)
		return result;
	return -1;

}
int ArabicHeadFinder::findHeadNX(){
	return findHeadNP();
}
int ArabicHeadFinder::findHeadPP(){
	int result = -1;
	Symbol set1[1] = {ArabicSTags::IN};
	result = scan_from_left_to_right(set1, 1);
	if (result >= 0)
		return result;
	Symbol set2[1] = {ArabicSTags::PP};
	result = scan_from_left_to_right(set2, 1);
	if (result >= 0)
		return result;
	return -1;

}
int ArabicHeadFinder::findHeadSBAR(){
	int result = -1;
	Symbol set[11] = {ArabicSTags::WHNP, ArabicSTags::WHPP, ArabicSTags::ADVP, 
		ArabicSTags::WHADJP, ArabicSTags::IN, ArabicSTags::DT, ArabicSTags::S, ArabicSTags::SQ, 
		ArabicSTags::SINV, ArabicSTags::SBAR, ArabicSTags::FRAG};
	result = scan_from_left_to_right_by_set(set, 11);
	if (result >= 0)
		return result;
	return -1;

}
int ArabicHeadFinder::findHeadSBARQ(){
	return findHeadSBAR();
}
int ArabicHeadFinder::findHeadADJP(){
	int result = -1;
	Symbol set1[2] = {ArabicSTags::JJ, ArabicSTags::DET_JJ};
	result = scan_from_left_to_right(set1, 2);
/*	Symbol set1[6] = {ArabicSTags::JJ, ArabicSTags::DET_JJ
		, ArabicSTags::JJ_NUM, ArabicSTags::JJ_COMP, ArabicSTags::DET_JJ_NUM, ArabicSTags::DET_JJ_COMP};
	result = scan_from_left_to_right(set1, 6); // parser experiments */
	if(result >=0){
		return result;
	}
	Symbol set2[8] = {ArabicSTags::NN, ArabicSTags::NNS, ArabicSTags::NNP, ArabicSTags::NNPS,
		ArabicSTags::DET_NN, ArabicSTags::DET_NNS, ArabicSTags::DET_NNP, ArabicSTags::DET_NNPS};
	result = scan_from_left_to_right(set2, 8);
	if(result >=0){
		return result;
	}
	return -1;
}
int ArabicHeadFinder::findHeadADVP(){
	int result = -1;
	Symbol set1[2] = {ArabicSTags::RB};
	result = scan_from_left_to_right(set1, 2);
	if(result >=0){
		return result;
	}
	Symbol set2[8] = {ArabicSTags::NN, ArabicSTags::NNS, ArabicSTags::NNP, ArabicSTags::NNPS,
		ArabicSTags::DET_NN, ArabicSTags::DET_NNS, ArabicSTags::DET_NNP, ArabicSTags::DET_NNPS};
	result = scan_from_left_to_right(set2, 8);
	if(result >=0){
		return result;
	}
	return -1;
}
int ArabicHeadFinder::findHeadWHNP(){
	int result = -1;
	Symbol set[6] = {ArabicSTags::WDT, ArabicSTags::WP, ArabicSTags::WP$, ArabicSTags::WHADJP, ArabicSTags::WHPP, ArabicSTags::WHNP};
	result = scan_from_left_to_right_by_set(set, 6);
	if(result >= 0){
		return result;
	}
	return -1;

}
int ArabicHeadFinder::findHeadNAC(){
	int result = -1;
	//this was changed slight some of the POS are not found in Arabic
	Symbol set1[20] = {ArabicSTags::NN, ArabicSTags::NNS, ArabicSTags::NNP, ArabicSTags::NNPS,
		ArabicSTags::DET_NN, ArabicSTags::DET_NNS, ArabicSTags::DET_NNP, ArabicSTags::DET_NNPS, 
		ArabicSTags::NP, ArabicSTags::NPP, ArabicSTags::NPA, ArabicSTags::DEF_NP, ArabicSTags::DEF_NPA,
		ArabicSTags::NAC, ArabicSTags::DOLLAR, ArabicSTags::CD, ArabicSTags::QP, 
		ArabicSTags::PRP,	ArabicSTags::JJ, ArabicSTags::ADJP};
	
	result = scan_from_left_to_right_by_set(set1, 20);
	if (result >= 0)
		return result;

	return -1;
}
int ArabicHeadFinder::findHeadRRC(){
	int result = -1;
	Symbol set[1] = {ArabicSTags::VP};
	result = scan_from_left_to_right_by_set(set, 1);
	if(result >=0)
		return result;
	return -1;
}
int ArabicHeadFinder::findHeadUCP(){
	return production.number_of_right_side_elements - 1;
}
