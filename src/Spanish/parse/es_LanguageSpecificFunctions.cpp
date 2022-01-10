// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/parse/es_LanguageSpecificFunctions.h"
#include <boost/algorithm/string.hpp> 
#include "Spanish/parse/es_STags.h"

bool SpanishLanguageSpecificFunctions::isHyphen(Symbol sym) {
	return false;
}
bool SpanishLanguageSpecificFunctions::isBasicPunctuationOrConjunction(Symbol sym){
	return false;
}
bool SpanishLanguageSpecificFunctions::isNoCrossPunctuation(Symbol sym){
	return false;
}
bool SpanishLanguageSpecificFunctions::isSentenceEndingPunctuation(Symbol sym){
	return false;
}
bool SpanishLanguageSpecificFunctions::isNPtypeLabel(Symbol sym){
	return false;
}
bool SpanishLanguageSpecificFunctions::isPPLabel(Symbol sym){
	return false;
}
Symbol SpanishLanguageSpecificFunctions::getNPlabel(){
	return SpanishSTags::SN;
}
Symbol SpanishLanguageSpecificFunctions::getNameLabel(){
	// JD 2015-08-04 - JSerif/Metro parser do not reimplement the name constraints
	// changing the Spanish name label is a partial workaround
	return SpanishSTags::SN;
}
Symbol SpanishLanguageSpecificFunctions::getCoreNPlabel(){
	return SpanishSTags::NPA;
}
Symbol SpanishLanguageSpecificFunctions::getDateLabel(){
	return SpanishSTags::POS_W;
}
Symbol SpanishLanguageSpecificFunctions::getProperNounLabel(){
	return SpanishSTags::POS_NP;
}
bool SpanishLanguageSpecificFunctions::isCoreNPLabel(Symbol sym){
	return false;
}
bool SpanishLanguageSpecificFunctions::isPrimaryNamePOStag(Symbol sym, EntityType type){
	return false;
}
bool SpanishLanguageSpecificFunctions::isSecondaryNamePOStag(Symbol sym, EntityType type){
	return false;
}
Symbol SpanishLanguageSpecificFunctions::getDefaultNamePOStag(EntityType type){
	return SpanishSTags::POS_NP;
}
bool SpanishLanguageSpecificFunctions::isNPtypePOStag(Symbol sym){
	return false;
}
Symbol SpanishLanguageSpecificFunctions::getSymbolForParseNodeLeaf(Symbol sym){
	std::wstring str = sym.to_string();
	boost::to_lower(str);
	// If any parens made it this far, then quote them now.  This can
	// happen eg if our input tokenization contains URLs with URLs in
	// them.
	boost::replace_all(str, L"(", L"-LRB-");
	boost::replace_all(str, L")", L"-RRB-");
	return Symbol(str.c_str());
}
bool SpanishLanguageSpecificFunctions::matchesHeadToken(Mention *, CorrectMention *){
	return false;
}
bool SpanishLanguageSpecificFunctions::matchesHeadToken(const SynNode *, CorrectMention *){
	return false;
}
