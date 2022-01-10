// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/common/ar_NationalityRecognizer.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/common/ParamReader.h"

bool ArabicNationalityRecognizer::_ar_initialized = false;
bool ArabicNationalityRecognizer::_allowLikelyNationalities = false;
void ArabicNationalityRecognizer::arInitialize(){
	if(_ar_initialized)
		return;
	_allowLikelyNationalities = ParamReader::isParamTrue("do_nat_conversion");
	_ar_initialized = true;

}

bool ArabicNationalityRecognizer::isNamePersonDescriptor(const SynNode *node){
	arInitialize();
	//if word begins with 'Al' remove it!
	Symbol hw = node->getHeadWord();
	wchar_t buffer[500];
	buffer[0] = L'\x627';
	buffer[1] = L'\x644';
	buffer[2] = L'\0';
	bool startsWithAl = false;
	if(wcsncmp(hw.to_string() ,buffer,2) == 0){
		int sz = static_cast <int>(wcslen(hw.to_string()));
		sz = sz <498 ? sz : 498;
		int i = 0;
		for(i =2; i<sz; i++){
			buffer[i-2] = hw.to_string()[i];
		}
		buffer[i-2] =L'\0';
		hw = Symbol(buffer);
		startsWithAl = true;
	}
	Symbol foundhw = hw;
	// first make sure the head is a nationality word
	if (!startsWithAl && !isNationalityWord(hw))
		return false;
	else if((hw.to_string()[0] != L'\x0627') && !(
		isNationalityWord(hw) ||
		isNationalityWord(node->getHeadWord()))){ //germany is AlMn... , so still check the original hw
		return false;
	}
	else if(hw.to_string()[0]== L'\x0627'){
		//try to compensate for Al[<,>,}]->AlA
		bool foundNat = false;
		if(isNationalityWord(hw)){
			foundNat = true;
		}
		if(isNationalityWord(node->getHeadWord())){
			foundNat = true;
		}
		if(!foundNat){
			buffer[0] = L'\x0625';
			Symbol temp = Symbol(buffer);

			if(isNationalityWord(temp)){
				foundhw = temp;
				foundNat = true;
			}
		}
		if(!foundNat){
			buffer[0] = L'\x0623';
			Symbol temp = Symbol(buffer);

			if(isNationalityWord(temp)){
				foundhw = temp;
				foundNat = true;
			}
		}
		if(!foundNat){
			buffer[0] = L'\x0626';
			Symbol temp = Symbol(buffer);

			if(isNationalityWord(temp)){
				foundhw = temp;
				foundNat = true;
			}
		}
		if(!foundNat)
			return false;
	}


	if (isCertainNationalityWord(hw) ||
		isCertainNationalityWord(foundhw) ||
		isCertainNationalityWord(node->getHeadWord())){
			return true;
		}
	// now make sure that if it's in an NP, there are no other NPs
	// alongside it
	if(!_allowLikelyNationalities)
		return false;

	if (node->getParent() != 0) {
		const SynNode *parent = node->getParent();

		bool other_reference_found = false;
		for (int i = 0; i < node->getParent()->getNChildren(); i++) {
			if (parent->getChild(i)->hasMention() ||
				NodeInfo::canBeNPHeadPreterm(parent->getChild(i)))
			{
				if (parent->getChild(i) != node) {
					other_reference_found = true;
					break;
				}
			}
		}

		if (other_reference_found)
			return false;
	}

	return true;

}

bool ArabicNationalityRecognizer::isPossibleNationalitySymbol(Symbol hw){
	arInitialize();
	//std::cout<<"call to natrecognizer: "<<hw.to_debug_string()<<std::endl;
	// first make sure the head is a nationality word
	if(isNationalityWord(hw))
		return true;
	//if word begins with 'Al' remove it!
	wchar_t buffer[500];
	buffer[0] = L'\x627';
	buffer[1] = L'\x644';
	buffer[2] = L'\0';
	bool startsWithAl = false;
	if(wcsncmp(hw.to_string() ,buffer,2) == 0){
		int sz = static_cast <int>(wcslen(hw.to_string()));
		sz = sz <498 ? sz : 498;
		int i = 0;
		for(i =2; i<sz; i++){
			buffer[i-2] = hw.to_string()[i];
		}
		buffer[i-2] =L'\0';
		hw = Symbol(buffer);
		startsWithAl = true;
	}
	Symbol foundhw = hw;
	if (!startsWithAl && !isNationalityWord(hw))
		return false;
	if(isNationalityWord(hw))
		return true;
	if(hw.to_string()[0]== L'\x0627'){
		//try to compensate for Al[<,>,}]->AlA
		bool foundNat = false;
		if(isNationalityWord(hw)){
			foundNat = true;
		}
		if(!foundNat){
			buffer[0] = L'\x0625';
			Symbol temp = Symbol(buffer);

			if(isNationalityWord(temp)){
				foundhw = temp;
				foundNat = true;
			}
		}
		if(!foundNat){
			buffer[0] = L'\x0623';
			Symbol temp = Symbol(buffer);

			if(isNationalityWord(temp)){
				foundhw = temp;
				foundNat = true;
			}
		}
		if(!foundNat){
			buffer[0] = L'\x0626';
			Symbol temp = Symbol(buffer);

			if(isNationalityWord(temp)){
				foundhw = temp;
				foundNat = true;
			}
		}
		return foundNat;
	}
	return false;
}
