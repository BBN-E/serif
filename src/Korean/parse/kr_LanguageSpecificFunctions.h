// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KR_LANGUAGE_SPECIFIC_FUNCTIONS_H
#define KR_LANGUAGE_SPECIFIC_FUNCTIONS_H

#include "common/Symbol.h"
#include "common/WordConstants.h"
#include "common/SessionLogger.h"
#include "Korean/parse/kr_STags.h"
#include "parse/ParserTags.h"
#include "theories/SynNode.h"
#include "theories/EntityType.h"
#include "Generic/parse/xx_LanguageSpecificFunctions.h"
#include <string>
#include <boost/algorithm/string.hpp> 

class KoreanLanguageSpecificFunctions : public GenericLanguageSpecificFunctions {
public:

	static bool isHyphen(Symbol sym) { return false; }

	static bool isBasicPunctuationOrConjunction(Symbol sym) {
		if (sym == KoreanSTags::SCM ||
			sym == KoreanSTags::SFN ||
			sym == KoreanSTags::SLQ ||
			sym == KoreanSTags::SSY ||
			sym == KoreanSTags::SRQ)
			return true;
		else return false;
	}

	static bool isNoCrossPunctuation(Symbol sym) {
		if (sym == KoreanSTags::COMMA ||
			sym == KoreanSTags::DASH ||
			sym == KoreanSTags::HYPHEN ||
			sym == KoreanSTags::SEMICOLON ||
			sym == KoreanSTags::DOT ||
			sym == KoreanSTags::FULLWIDTH_COMMA ||
			sym == KoreanSTags::FULLWIDTH_HYPHEN ||
			sym == KoreanSTags::FULLWIDTH_SEMICOLON ||
			sym == KoreanSTags::FULLWIDTH_DOT)
			return true;
		else return false;
	}

	static bool isSentenceEndingPunctuation(Symbol sym) {
		if (sym == KoreanSTags::QMARK ||
			sym == KoreanSTags::EX_POINT ||
			sym == KoreanSTags::DOT ||
			sym == KoreanSTags::FULLWIDTH_QMARK ||
			sym == KoreanSTags::FULLWIDTH_EX_POINT ||
			sym == KoreanSTags::FULLWIDTH_DOT)
			return true;
		else return false;
	}

	static bool isNPtypeLabel(Symbol sym) {
		if (sym == KoreanSTags::NP ||
			sym == KoreanSTags::NPA)
			return true;
		else return false;
	}

	static bool isPPLabel(Symbol sym) {
		return false;
	}

	static bool isVerbPOSLabel(Symbol sym) { 
		if (sym == KoreanSTags::VJ ||
			sym == KoreanSTags::VV ||
			sym == KoreanSTags::VX)
			return true;
		else return false;
	}

	static bool isPrepPOSLabel(Symbol sym) { 
		return false;
	}

	static Symbol getNPlabel() {
		return KoreanSTags::NP;
	}

	static Symbol getCoreNPlabel() {
		return KoreanSTags::NPA;
	}

	static Symbol getProperNounLabel() {
		return KoreanSTags::NPR;
	}

	static Symbol getNameLabel() {
		return KoreanSTags::NPP;
	}
	
	static bool isNoun(Symbol tag){
		return isNPtypePOStag(tag);
	}
	
	static bool isAdjective(Symbol tag) {
		return false;
	}

	static bool isCoreNPLabel(Symbol sym) {
		if (sym == KoreanSTags::NPA ||
			sym == getNameLabel())
			return true;
		else return false;
	}

	static bool isPrimaryNamePOStag(Symbol sym, EntityType type) {
		if (type.getPrimaryTag() != Symbol()) 
			return (sym == type.getPrimaryTag());
		else if (type.isIdfDesc())
			return true;
		else if (sym == KoreanSTags::NPR || sym == KoreanSTags::NNC || sym == KoreanSTags::NFW)
			return true;
		else return false;
	}
	
	static bool isSecondaryNamePOStag(Symbol sym, EntityType type) {
		if (type.getSecondaryTag() != Symbol())
			return (sym == type.getSecondaryTag());
		else if (type.isIdfDesc())
			return true;
		else if (sym == KoreanSTags::NNC || sym == KoreanSTags::NFW || sym == KoreanSTags::NNU || sym == KoreanSTags::NNX)
			return true;
		else return false;
	}

	static Symbol getDefaultNamePOStag(EntityType type) {
		if (type.getDefaultTag() != Symbol())
			return type.getDefaultTag();
		else
			return KoreanSTags::NPR;
	}

	static Symbol getDefaultNameWord(ChartDecoder::decoder_type_t decoder_type, EntityType type) {
		if (type.getDefaultWord() != Symbol())
			return type.getDefaultWord();
		else
			return Symbol(L""); 
	}

	static bool isNPtypePOStag(Symbol sym) {
		if (sym == KoreanSTags::NFW ||
			sym == KoreanSTags::NNC ||
			sym == KoreanSTags::NNU || 
			sym == KoreanSTags::NNX ||
			sym == KoreanSTags::NPN ||
			sym == KoreanSTags::NPR)
			return true;
		else return false;
	}	
	
	static Symbol getDateLabel() {
		return KoreanSTags::DATE;
	}	

	static Symbol getSymbolForParseNodeLeaf(Symbol sym) {
		std::wstring str = sym.to_string();
		boost::to_lower(str);
		boost::replace_all(str, L"(", L"-LRB-");
		boost::replace_all(str, L")", L"-RRB-");
		return Symbol(str.c_str());
	}

	/**
	 * Given a ParseNode, perform operations on it that alter the structure
	 * in a specified manner. In English, this ensures a single NP premod under
	 * NPPOS, and changes an NP with list-ish structure into a set of NPs
	 */
	static void modifyParse(ParseNode* node) { 
		return; 
	}

	/**
	 * Given a SynNode and its children, perform operations on it that alter the 
	 * structure in a specified manner. In English, this ensures a single NP premod 
	 * under NPPOS, and changes an NP with list-ish structure into a set of NPs
	 *
	 * Note: This version is used in DescriptorClassifierTrainer
	 */
	static void modifyParse(SynNode* node, CorefItem *coref, SynNode* children[], int n_children, int head_index) {
		return; 
	}
};

#endif
