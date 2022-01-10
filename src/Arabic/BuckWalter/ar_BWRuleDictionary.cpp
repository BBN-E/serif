// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Arabic/BuckWalter/ar_BWRuleDictionary.h"
#include <boost/scoped_ptr.hpp>

BWRuleDictionary * BWRuleDictionary::instance = 0;

BWRuleDictionary * BWRuleDictionary::getInstance() {
	if (instance == 0) {
		instance = _new BWRuleDictionary();
		instance->initialize();
	}
	return instance;
}

void BWRuleDictionary::destroy() {
	delete instance;
	instance = 0;
}

BWRuleDictionary::BWRuleDictionary(){
	_n_rules = 0;

	_A_map = _new SingleHashMap(MAX_CATEGORIES, singleHasher, singleEqTester);
	_B_map = _new SingleHashMap(MAX_CATEGORIES, singleHasher, singleEqTester);
	_C_map = _new SingleHashMap(MAX_CATEGORIES, singleHasher, singleEqTester);

	_BC_map = _new DoubleHashMap(MAX_RULES, doubleHasher, doubleEqTester);
	_AB_map = _new DoubleHashMap(MAX_RULES, doubleHasher, doubleEqTester);
	_AC_map = _new DoubleHashMap(MAX_RULES, doubleHasher, doubleEqTester);

}

BWRuleDictionary::~BWRuleDictionary(){
	delete _A_map;
	delete _B_map;
	delete _C_map;
	delete _BC_map;
	delete _AB_map;
	delete _AC_map;
}

void BWRuleDictionary::initialize() {
	std::string buffer = ParamReader::getRequiredParam("BWRuleDict");

	//read rule dictionary
	readBWRuleDictionaryFile(buffer.c_str());

	SessionLogger::logger->reportInfoMessage()<<"Read BW Rule Dictionary: "<<buffer<<
		" "<<(int)getNRules()<<" rules\n";
}

void BWRuleDictionary::readBWRuleDictionaryFile(const char *rule_dict_file) {
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(rule_dict_file));
	UTF8InputStream& stream(*stream_scoped_ptr);

	size_t numEntries;
	UTF8Token position1;
	UTF8Token position2;	
	UTF8Token category1;
	UTF8Token category2;

	stream >> numEntries;

	for (size_t line = 0; line < numEntries; line++) {

		stream >> position1;
		stream >> position2;
		stream >> category1;
		stream >> category2;
		addRule(position1.symValue(), category1.symValue(), 
			    position2.symValue(), category2.symValue());

	}	
	stream.close();
}

void BWRuleDictionary::addRule(const Symbol& position1, const Symbol& category1, const Symbol& position2, const Symbol& category2) {

	DoubleHashMap* dhm = getAppropriateDoubleMap(position1, position2);
	SingleHashMap* shm1 = getAppropriateSingleMap(position1);
	//SingleHashMap* shm2 = getAppropriateSingleMap(position1);  //is this incorrect? marjorie
	SingleHashMap* shm2 = getAppropriateSingleMap(position2);

	std::pair<Symbol, Symbol> combinedRule(category1, category2);
	if(dhm->get(combinedRule) != NULL){
		(*dhm)[combinedRule].n_exists++;	
	}else{
		if(_n_rules >= MAX_RULES){
			sprintf(_message, "%s%d", "Attempted to add more rules than allowed", MAX_RULES);  
			throw UnrecoverableException("BWRuleDictionary::addRule", _message);
		}
		_n_rules++;
		(*dhm)[combinedRule].n_exists = 1;
		(*dhm)[combinedRule].first_category = category1;
		(*dhm)[combinedRule].second_category = category2;
	}

	//update the following list
	if(shm1->get(category1) != NULL){
	//	std::cout <<"shm1: "<<category1.to_debug_string()
	//		<<" nfollowing "<< (*shm1)[category1].n_following
	//		<<" npreceding "<< (*shm1)[category1].n_preceding<<std::endl;
		if ((*shm1)[category1].n_following < MAX_RULES_PER_CATEGORY){
			(*shm1)[category1].following[(*shm1)[category1].n_following] = category2;
			(*shm1)[category1].n_following++;
		}else{
			sprintf(_message, "%s%d%s%s", "Tried to add too many left hand sides, ", (*shm1)[category1].n_following ," to category ", category1.to_debug_string());  
			throw UnrecoverableException("BWRuleDictionary::addRule", _message);
		}
	}else{
		(*shm1)[category1].n_following = 0;		
		(*shm1)[category1].n_preceding = 0;	
	//	std::cout <<"shm1: "<<category1.to_debug_string()
	//		<<" nfollowing "<< (*shm1)[category1].n_following
	//		<<" npreceding "<< (*shm1)[category1].n_preceding <<std::endl;;
		(*shm1)[category1].following[(*shm1)[category1].n_following] = category2;
		(*shm1)[category1].n_following++;
	}

	//update the preceding list
	if(shm2->get(category2) != NULL){
	//	std::cout <<"shm2: "<<category2.to_debug_string()
	//		<<" nfollowing "<< (*shm2)[category2].n_following
	//		<<" npreceding "<< (*shm2)[category2].n_preceding<<std::endl;
		if ((*shm2)[category2].n_preceding < MAX_RULES_PER_CATEGORY){
			//(*shm2)[category2].preceding[(*shm1)[category1].n_preceding] = category1;
			(*shm2)[category2].preceding[(*shm2)[category2].n_preceding] = category1;
			(*shm2)[category2].n_preceding++;
		}else{

			sprintf(_message, "%s%d%s%s", "Tried to add too many right hand sides, ", (*shm1)[category1].n_following ," to category ", category1.to_debug_string());  
			throw UnrecoverableException("BWRuleDictionary::addRule", _message);
		}
	}else{
		(*shm2)[category2].n_preceding = 0;	
		(*shm2)[category2].n_following = 0;		
		//std::cout <<"shm2: "<<category2.to_debug_string()
		//	<<" nfollowing "<< (*shm2)[category2].n_following
		//	<<" npreceding "<< (*shm2)[category2].n_preceding<<std::endl;
		//(*shm2)[category2].preceding[(*shm1)[category2].n_preceding] = category 1;
		(*shm2)[category2].preceding[(*shm2)[category2].n_preceding] = category1;
		(*shm2)[category2].n_preceding++;
	}
}


int BWRuleDictionary::getFollowingPossibilities(const Symbol& position, const Symbol& category, Symbol *results){
	if (getAppropriateSingleMap(position)->get(category) != NULL){
		int n_following = (*getAppropriateSingleMap(position))[category].n_following;
		for(int n =0; n < n_following; n++){
			results[n] = (*getAppropriateSingleMap(position))[category].following[n];
		}
		return n_following;
	}else{
		return 0;
	}
}
/*
size_t BWRuleDictionary::getPrecedingPossibilities(const Symbol& position, const Symbol& category, Symbol *results){
	if (getAppropriateSingleMap(position)->get(category) != NULL){
		size_t n_following = (*getAppropriateSingleMap(position))[category].n_preceding;
		for(size_t n =0; n < n_following; n++){
			results[n] = (*getAppropriateSingleMap(position))[category].preceding[n];
		}
	}else{
		return 0;
	}
}
*/	
bool BWRuleDictionary::isABPermitted(const Symbol& A,  const Symbol& B){
	return (_AB_map->get(std::make_pair(A,B)) != NULL);
}
bool BWRuleDictionary::isBCPermitted(const Symbol& B,  const Symbol& C){
	return (_BC_map->get(std::make_pair(B,C)) != NULL);
}
bool BWRuleDictionary::isACPermitted(const Symbol& A,  const Symbol& C){
	return (_AC_map->get(std::make_pair(A,C)) != NULL);
}

void BWRuleDictionary::dump(UTF8OutputStream &uos){
	DoubleHashMap::iterator curr;
	
	for(curr =_AB_map->begin(); curr !=_AB_map->end(); ++curr){
		DoubleHashEntry entry = (*curr).second;
		uos <<  L"A B " << entry.first_category.to_string() << L" " << entry.second_category.to_string() << L"\n";
	}
	for(curr =_BC_map->begin(); curr !=_BC_map->end(); ++curr){
		DoubleHashEntry entry = (*curr).second;
		uos <<  L"B C "  << entry.first_category.to_string() << L" " << entry.second_category.to_string() << L"\n";
	}
	for(curr =_AC_map->begin(); curr !=_AC_map->end(); ++curr){
		DoubleHashEntry entry = (*curr).second;
		uos <<  L"A C "<< entry.first_category.to_string() << L" " << entry.second_category.to_string() << L"\n";
	}
}

BWRuleDictionary::SingleHashMap* BWRuleDictionary::getAppropriateSingleMap(const Symbol& position) {
	if (position == Symbol(L"A")){
		return _A_map;
	}else if (position == Symbol(L"B")){
		return _B_map;
	}else if (position == Symbol(L"C")){
		return _C_map;
	}
	else{
		sprintf(_message, "%s%s", "InvalidPosition ", position.to_debug_string());  
		throw UnrecoverableException("BWRuleDictionary::getAppropriateSingleMap", _message);
	}
}
BWRuleDictionary::DoubleHashMap* BWRuleDictionary::getAppropriateDoubleMap(const Symbol& position1, const Symbol& position2) {
	if ((position1 == Symbol(L"A")) && (position2 == Symbol(L"B"))){
		return _AB_map;
	}else if ((position1 == Symbol(L"B")) && (position2 == Symbol(L"C"))){
		return _BC_map;
	}else if ((position1 == Symbol(L"A")) && (position2 == Symbol(L"C"))){
		return _AC_map;
	}else{
		sprintf(_message, "%s%s%s", "Impossible rule postion combination ", position1.to_debug_string(), position2.to_debug_string());  
		throw UnrecoverableException("BWRuleDictionary::getAppropriateDoubleMap", _message);
	}
}
