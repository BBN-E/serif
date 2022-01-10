// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "Arabic/parse/ar_Segment.h"
#include "Arabic/parse/ar_STags.h"
#include "Arabic/common/ar_ArabicSymbol.h"
#include "common/InternalInconsistencyException.h"
#include <stdio.h>
#include <iostream>
Symbol Segment::PREP_TAGS[] = { ArabicSTags::PREP};
Symbol Segment::CONJ_TAGS[] = {ArabicSTags::CONJ};
Symbol Segment::PREP_OR_PART_TAGS[] = {ArabicSTags::PART, 
			ArabicSTags::EMPHATIC_PARTICLE, ArabicSTags::RESULT_CLAUSE_PARTICLE,
			ArabicSTags::SUBJUNC,	ArabicSTags::PREP};
Symbol Segment::PRON_TAGS[] = {ArabicSTags::IVSUFF_DO_3MS, 
		ArabicSTags::POSS_PRON_3MS, ArabicSTags::PRON_3MS, ArabicSTags::PVSUFF_DO_3MS};

Segment::Segment(Symbol seg, int start, int type){
	_seg = seg;
	_start = start;
	_end = -1;
	_pos_class = _getPOSClass(seg, type);

};
Segment::Segment(const Segment* other){
	_seg = other->_seg;
	_pos_class = other->_pos_class;
	_start = other->_start;
	_end =other->_end;
}

Segment::~Segment(){};
int Segment::getStart(){
	return _start;
}
int Segment::getEnd(){
	return _end;
}
Symbol Segment::getText(){
	return _seg;
}
void Segment::setEnd(int end){
	_end = end;
}
void Segment::setStart(int start){
	_start = start;
}

int Segment::_getPOSClass(Symbol seg, int type){
	if(type == STEM)
		return 0;
	else if(type == SUFFIX)
		return PRON;
	if((type == PREFIX)&& 
		((seg == ArabicSymbol(L"w"))||(seg == ArabicSymbol(L"f"))))
		return CONJ;
	else if((type ==PREFIX) &&
		(( seg == ArabicSymbol(L"b")) ||(seg == ArabicSymbol(L"k"))))
		return PREP;
	else if((type == PREFIX)&&(seg == ArabicSymbol(L"l")))
		return PREP_OR_PART;
	else{
		std::cerr<< "No part of speech class"<<std::endl;
		return 0;
	}
}
const Symbol* Segment::validPOSList(){
	if(_pos_class ==PRON)
		return PRON_TAGS;
	else if(_pos_class == PREP)
		return PREP_TAGS;
	else if(_pos_class ==CONJ)
		return CONJ_TAGS;
	else if(_pos_class == PREP_OR_PART)
		return PREP_OR_PART_TAGS;
	throw InternalInconsistencyException("Segment::validPOSList()","");
}
int Segment::posListSize(){
	if(_pos_class ==PRON)
		return 4;
	else if(_pos_class == PREP)
		return 1;
	else if(_pos_class ==CONJ)
		return 1;
	else if(_pos_class == PREP_OR_PART)
		return 5;
	throw InternalInconsistencyException("Segment::posListSize()","");
}


bool Segment::isValidPOS(Symbol pos){
	if(_pos_class == 0){
		return true;
	}
	else if(_pos_class == PREP)
		return pos == ArabicSTags::PREP;
	else if(_pos_class == CONJ)
		return pos == ArabicSTags::CONJ;
	else if(_pos_class == PREP_OR_PART){
		return ((pos == ArabicSTags::PREP) ||
			(pos == ArabicSTags::PART)||
			(pos == ArabicSTags::EMPHATIC)||
			(pos == ArabicSTags::EMPHATIC_PARTICLE)||
			(pos == ArabicSTags::RESULT)||
			(pos == ArabicSTags::RESULT_CLAUSE_PARTICLE)||
			(pos == ArabicSTags::SUBJUNC)||
			(pos == ArabicSTags::EXCEPT)||
			(pos == ArabicSTags::EXCEPT_PART)||
			(pos == ArabicSTags::INTERROG)||
			(pos == ArabicSTags::INTERROG_PART));
	}
	else if(_pos_class == PRON){
		return ((pos == ArabicSTags::DEM_PRON_F)||
			(pos == ArabicSTags::DEM_PRON_FD)||
			(pos == ArabicSTags::DEM_PRON_FS)||
			(pos == ArabicSTags::DEM_PRON_MD)||
			(pos == ArabicSTags::DEM_PRON_MD)||
			(pos == ArabicSTags::DEM_PRON_MP)||
			(pos == ArabicSTags::DEM_PRON_MS)||
			(pos == ArabicSTags::IVSUFF_DO_1P)||
			(pos == ArabicSTags::IVSUFF_DO_1S)||
			(pos == ArabicSTags::IVSUFF_DO_2MP)||
			(pos == ArabicSTags::IVSUFF_DO_2MS)||
			(pos == ArabicSTags::IVSUFF_DO_3D)||
			(pos == ArabicSTags::IVSUFF_DO_3FS)||
			(pos == ArabicSTags::IVSUFF_DO_3MP)||
			(pos == ArabicSTags::IVSUFF_DO_3MS)||
			(pos == ArabicSTags::POSS_PRON_1P)||
			(pos == ArabicSTags::POSS_PRON_1S)||
			(pos == ArabicSTags::POSS_PRON_2MP)||
			(pos == ArabicSTags::POSS_PRON_2MS)||
			(pos == ArabicSTags::POSS_PRON_3D)||
			(pos == ArabicSTags::POSS_PRON_3FS)||
			(pos == ArabicSTags::POSS_PRON_3MP)||
			(pos == ArabicSTags::POSS_PRON_3MS)||
			(pos == ArabicSTags::PVSUFF_DO_1P)||
			(pos == ArabicSTags::PVSUFF_DO_1S)||
			(pos == ArabicSTags::PVSUFF_DO_3D)||
			(pos == ArabicSTags::PVSUFF_DO_3FS)||
			(pos == ArabicSTags::PVSUFF_DO_3MP)||
			(pos == ArabicSTags::PVSUFF_SUBJ_1P)||
			(pos == ArabicSTags::PVSUFF_SUBJ_1S)||
			(pos == ArabicSTags::PVSUFF_SUBJ_2FS)||
			(pos == ArabicSTags::PVSUFF_SUBJ_2MP)||
			(pos == ArabicSTags::PVSUFF_SUBJ_3FD)||
			(pos == ArabicSTags::PVSUFF_SUBJ_3FP)||
			(pos == ArabicSTags::PVSUFF_SUBJ_3FS)||
			(pos == ArabicSTags::PVSUFF_SUBJ_3MD)||
			(pos == ArabicSTags::PVSUFF_SUBJ_3MP)||
			(pos == ArabicSTags::PVSUFF_SUBJ_3MS)||
			(pos == ArabicSTags::PRON_1P)||
			(pos == ArabicSTags::PRON_1S)||
			(pos == ArabicSTags::PRON_2MP)||
			(pos == ArabicSTags::PRON_3D)||
			(pos == ArabicSTags::PRON_3FS)||
			(pos == ArabicSTags::PRON_3MP)||
			(pos == ArabicSTags::PRON_3MS)||
			(pos == ArabicSTags::REL_PRON));	//REL_PRON added for mA, usually its own word but combines
										//with fiy
	}
	throw InternalInconsistencyException("Segment::isValidPOS()","");
}
