// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_LANGUAGE_SPECIFIC_FUNCTIONS_H
#define CH_LANGUAGE_SPECIFIC_FUNCTIONS_H

#include "Generic/common/Symbol.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/SessionLogger.h"
#include "Chinese/parse/ch_STags.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/EntityType.h"
#include "Generic/parse/xx_LanguageSpecificFunctions.h"
#include <string>

#include "Generic/CASerif/correctanswers/CorrectMention.h"
#include <boost/algorithm/string.hpp> 

class ChineseLanguageSpecificFunctions : public GenericLanguageSpecificFunctions {
public:

	static bool isHyphen(Symbol sym){return false;}

	static bool isBasicPunctuationOrConjunction(Symbol sym) {
		if (sym == ChineseSTags::CC ||
			sym == ChineseSTags::PU)
			return true;
		else return false;
	}
	static bool isSentenceEndingPunctuation(Symbol sym) {
		if (sym == ChineseSTags::QMARK ||
			sym == ChineseSTags::EX_POINT ||
			sym == ChineseSTags::DOT ||
			sym == ChineseSTags::CH_DOT ||
			sym == ChineseSTags::FULLWIDTH_QMARK ||
			sym == ChineseSTags::FULLWIDTH_EX_POINT ||
			sym == ChineseSTags::FULLWIDTH_DOT)
			return true;
		else return false;
	}
	static bool isNoCrossPunctuation(Symbol sym) {
		if (sym == ChineseSTags::COMMA ||
			sym == ChineseSTags::DASH ||
			sym == ChineseSTags::HYPHEN ||
			sym == ChineseSTags::SEMICOLON ||
			sym == ChineseSTags::DOT ||
			sym == ChineseSTags::FULLWIDTH_COMMA ||
			sym == ChineseSTags::FULLWIDTH_HYPHEN ||
			sym == ChineseSTags::FULLWIDTH_SEMICOLON ||
			sym == ChineseSTags::FULLWIDTH_DOT ||
			sym == ChineseSTags::CH_DOT)
			return true;
		else return false;
	}
	static bool isNPtypeLabel(Symbol sym) {
		if (sym == ChineseSTags::NP ||
			sym == ChineseSTags::NPA)
			return true;
		else return false;
	}
	static bool isCoreNPLabel(Symbol sym) {
		if (sym == ChineseSTags::NPA ||
			sym == getNameLabel())
			return true;
		else return false;
	}
	static bool isPPLabel(Symbol sym) {
		if (sym == ChineseSTags::PP)
			return true;
		else return false;
	}
	static bool isVerbPOSLabel(Symbol sym) { 
		if (sym == ChineseSTags::VA ||
			sym == ChineseSTags::VC ||
			sym == ChineseSTags::VE ||
			sym == ChineseSTags::VV)
			return true;
		else return false;
	}
	static bool isPrepPOSLabel(Symbol sym) { 
		if (sym == ChineseSTags::P)
			return true;
		else return false;
	}
	static bool isNPtypePOStag(Symbol sym) {
		if (sym == ChineseSTags::NN ||
			sym == ChineseSTags::NT ||
			sym == ChineseSTags::NR)
			return true;
		else return false;
	}	
	static Symbol getCoreNPlabel() {
		return ChineseSTags::NPA;
	}
	static Symbol getNPlabel() {
		return ChineseSTags::NP;
	}
	static Symbol getProperNounLabel() {
		return ChineseSTags::NR;
	}
	static Symbol getNameLabel() {
		return ChineseSTags::NPP;
	}
	static bool isPrimaryNamePOStag(Symbol sym, EntityType type) {
		if (!type.getPrimaryTag().is_null()) 
			return (sym == type.getPrimaryTag());
		else if (type.isIdfDesc())
			return true;
		else if (sym == ChineseSTags::NR || sym == ChineseSTags::NN)
			return true;
		else return false;
	}
	static bool isSecondaryNamePOStag(Symbol sym, EntityType type) {
		if (!type.getSecondaryTag().is_null())
			return (sym == type.getSecondaryTag());
		else if (type.isIdfDesc())
			return true;
		else if (sym == ChineseSTags::NN)
			return true;
		else return false;
	}
	static Symbol getDefaultNamePOStag(EntityType type) {
		if (!type.getDefaultTag().is_null())
			return type.getDefaultTag();
		else
			return ChineseSTags::NR;
	}
	static Symbol getDefaultNameWord(ChartDecoder::decoder_type_t decoder_type, EntityType type) {
		if (!type.getDefaultWord().is_null())
			return type.getDefaultWord();
		else
			return Symbol(L"\x4e2d");  // character for "chinese" - reduces to XXWFDO
	}
	static Symbol getDateLabel() {
		return ChineseSTags::DATE;
	}	
	static Symbol getSymbolForParseNodeLeaf(Symbol sym) {
		std::wstring str = sym.to_string();
		boost::to_lower(str);
		boost::replace_all(str, L"(", L"-LRB-");
		boost::replace_all(str, L")", L"-RRB-");
		return Symbol(str.c_str());
	}

	/**
	 * ensures a single NP premod under
	 * NPPOS, changes an NP with list-ish structure into a set of NPs,
	 * and adds a new NP above any headless QPs
	 * @param node The original parse node (modified in place)
	 */
	static void modifyParse(ParseNode* node) { 
		_fixPartitives(node);
		_fixNPList(node);
		_fixHeadlessQP(node);
		return; 
	}

	/**
	 * ensures a single NP premod under
	 * NPPOS, (eventually) will change an NP with list-ish structure into a set of NPs,
	 * and adds a new NP above any headless QPs
	 * @param node The original syntax node (modified in place)
	 * @param children The array of children belonging to the node
	 * @param n_children The number of children belonging to the node
	 * @param head_index The child array index corresponding to the head of the node
	 *
	 *	Used by AnnotatedParseReader
	 */
	static void modifyParse(SynNode* node, CorefItem *coref, SynNode* children[], int n_children, int head_index) { 
		// With the current setup, we cannot make the necessary modifications since we don't have access
		// to non-const nodes below the children (i.e. the partitive word node).
		//_fixPartitives(node, children, n_children, head_index);	
		_fixNPList(node, children, n_children, head_index);
		_fixHeadlessQP(node, children, n_children, head_index);
		return; 
	}
	
	static bool isNoun(Symbol tag){
		return isNPtypePOStag(tag);
	}

	static bool matchesHeadToken(Mention *mention, CorrectMention *correctMention) {
		const SynNode *mentionNode = mention->getNode();
		return (mentionNode->getHeadPreterm()->getEndToken() == correctMention->getHeadEndToken());
	}

	static bool matchesHeadToken(const SynNode *node, CorrectMention *correctMention) {
		return (node->getHeadPreterm()->getEndToken() == correctMention->getHeadEndToken());
	}

private:
	static void _fixPartitives(ParseNode *node);
	static void _fixNPList(ParseNode* node);
	static void _fixNPList(SynNode* node, SynNode* children[], int& n_children, int& head_index);
	static void _fixHeadlessQP(ParseNode* node);
	static void _fixHeadlessQP(SynNode* node, SynNode* children[], int& n_children, int& head_index);
	static void _fillNPFromVector(ParseNode* node, ParseNode* kids[], int size);
	static int _findNPHead(ParseNode* arr[], int numNodes);
	static int _findNPHead(SynNode* arr[], int numNodes);
	static int _scanBackward(ParseNode* nodes[], int numNodes, Symbol syms[], int numSyms);
	static int _scanBackward(SynNode* nodes[], int numNodes, Symbol syms[], int numSyms);
};

#endif
