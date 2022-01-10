// Copyright 2013 Raytheon BBN Technologies 
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Urdu/parse/ur_LanguageSpecificFunctions.h"
#include <boost/algorithm/string.hpp> 
#include "Urdu/parse/ur_STags.h"

bool UrduLanguageSpecificFunctions::isHyphen(Symbol sym) {
	return false;
}
bool UrduLanguageSpecificFunctions::isBasicPunctuationOrConjunction(Symbol sym){
	return false;
}
bool UrduLanguageSpecificFunctions::isNoCrossPunctuation(Symbol sym){
	return false;
}
bool UrduLanguageSpecificFunctions::isSentenceEndingPunctuation(Symbol sym){
	return false;
}
bool UrduLanguageSpecificFunctions::isNPtypeLabel(Symbol sym){
	return false;
}
bool UrduLanguageSpecificFunctions::isPPLabel(Symbol sym){
	return false;
}
Symbol UrduLanguageSpecificFunctions::getNPlabel(){
	return UrduSTags::NP;
}
Symbol UrduLanguageSpecificFunctions::getNameLabel(){
	return UrduSTags::N_PROP;
}
Symbol UrduLanguageSpecificFunctions::getCoreNPlabel(){
	return UrduSTags::N;
}
Symbol UrduLanguageSpecificFunctions::getDateLabel(){
	return NULL;
}
Symbol UrduLanguageSpecificFunctions::getProperNounLabel(){
	return UrduSTags::N_PROP;
}
Symbol UrduLanguageSpecificFunctions::getDefaultNamePOStag(EntityType type){
	return UrduSTags::POS_NNP;
}
bool UrduLanguageSpecificFunctions::isCoreNPLabel(Symbol sym){
	return false;
}
bool UrduLanguageSpecificFunctions::isPrimaryNamePOStag(Symbol sym, EntityType type){
	return false;
}
bool UrduLanguageSpecificFunctions::isSecondaryNamePOStag(Symbol sym, EntityType type){
	return false;
}

bool UrduLanguageSpecificFunctions::isNPtypePOStag(Symbol sym){
	return false;
}
Symbol UrduLanguageSpecificFunctions::getSymbolForParseNodeLeaf(Symbol sym){
	std::wstring str = sym.to_string();
	boost::to_lower(str);
	// If any parens made it this far, then quote them now.  This can
	// happen eg if our input tokenization contains URLs with URLs in
	// them.
	boost::replace_all(str, L"(", L"-LRB-");
	boost::replace_all(str, L")", L"-RRB-");
	return Symbol(str.c_str());
}
bool UrduLanguageSpecificFunctions::matchesHeadToken(Mention *, CorrectMention *){
	return false;
}
bool UrduLanguageSpecificFunctions::matchesHeadToken(const SynNode *, CorrectMention *){
	return false;
}
