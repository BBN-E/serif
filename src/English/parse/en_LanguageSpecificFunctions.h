// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_LANGUAGE_SPECIFIC_FUNCTIONS_H
#define EN_LANGUAGE_SPECIFIC_FUNCTIONS_H

#include "Generic/common/Symbol.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "English/parse/en_STags.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/parse/ParseNode.h"
#include "Generic/theories/SynNode.h"
#include "Generic/trainers/CorefItem.h"
#include "Generic/parse/xx_LanguageSpecificFunctions.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/WordConstants.h"
#include "English/common/en_WordConstants.h"
#include <string>

#include "Generic/CASerif/correctanswers/CorrectMention.h"
#include <boost/algorithm/string.hpp> 

class Constraint;


class EnglishLanguageSpecificFunctions : public GenericLanguageSpecificFunctions {
private:
	static bool _standAloneParser;

public:

	static bool isHyphen(Symbol sym) {
		if (sym == EnglishSTags::HYPHEN)
			return true;
		else return false;
	}

	static bool isBasicPunctuationOrConjunction(Symbol sym) {
		if (sym == EnglishSTags::CC ||
			sym == EnglishSTags::COLON ||
			sym == EnglishSTags::COMMA ||
			sym == EnglishSTags::DOT)
			return true;
		else return false;
	}
	static bool isSentenceEndingPunctuation(Symbol sym) {
		if (sym == EnglishSTags::QMARK ||
			sym == EnglishSTags::EX_POINT ||
			sym == EnglishSTags::DOT)
			return true;
		else return false;
	}
	static bool isNoCrossPunctuation(Symbol sym) {
		if (sym == EnglishSTags::COMMA ||
			sym == EnglishSTags::DASH ||
			sym == EnglishSTags::HYPHEN ||
			sym == EnglishSTags::SEMICOLON ||
			sym == EnglishSTags::DOT)
			return true;
		else return false;
	}
	static bool isNPtypeLabel(Symbol sym) {
		if (sym == EnglishSTags::NP ||
			sym == EnglishSTags::NPA ||
			sym == EnglishSTags::NPPOS)
			return true;
		else return false;
	}
	static bool isCoreNPLabel(Symbol sym) {
		if (sym == EnglishSTags::NPA ||
			sym == getNameLabel())
			return true;
		else return false;
	}
	static bool isPPLabel(Symbol sym) {
		if (sym == EnglishSTags::PP)
			return true;
		else return false;
	}
	static bool isNPtypePOStag(Symbol sym) {
		if (sym == EnglishSTags::NN ||
			sym == EnglishSTags::NNS ||
			sym == EnglishSTags::NNP ||
			sym == EnglishSTags::NNPS)
			return true;
		else return false;
	}	
	static bool isVerbPOSLabel(Symbol sym) { 
		return (sym == EnglishSTags::VB || sym == EnglishSTags::VBD || 
			sym == EnglishSTags::VBG || sym == EnglishSTags::VBN || 
			sym == EnglishSTags::VBP || sym == EnglishSTags::VBZ);
	}	
	static bool isPrepPOSLabel(Symbol sym) { 
		return (sym == EnglishSTags::IN);
	}
	static bool isPreplikePOSLabel(Symbol sym) { 
		return ((sym == EnglishSTags::IN) || (sym == EnglishSTags::TO));
	}
	static bool isPronounPOSLabel(Symbol sym) { 
		return (sym == EnglishSTags::PRP); 
	}
	static bool isParticlePOSLabel(Symbol sym) { 
		return (sym == EnglishSTags::RP); 
	}
	static Symbol getCoreNPlabel() {
		return EnglishSTags::NPA;
	}
	static Symbol getNPlabel() {
		return EnglishSTags::NP;
	}
	static Symbol getProperNounLabel() {
		return EnglishSTags::NNP;
	}
	static Symbol getNameLabel() {
		return EnglishSTags::NPP;
	}	
	static bool isAdjective(Symbol sym) {
		return (sym == EnglishSTags::JJ ||
				sym == EnglishSTags::JJR ||
				sym == EnglishSTags::JJS);
	}
	static Symbol getDateLabel() {
		return EnglishSTags::DATE;
	}	
	static bool isPrimaryNamePOStag(Symbol sym, EntityType type) {
		if (!type.getPrimaryTag().is_null()) 
			return (sym == type.getPrimaryTag());
		else if (type.isIdfDesc())
			return true;
		else if (sym == EnglishSTags::NNP ||
			sym == EnglishSTags::NNPS)
			return true;
		else return false;
	}
	static bool isSecondaryNamePOStag(Symbol sym, EntityType type) {
		if (!type.getSecondaryTag().is_null())
			return (sym == type.getSecondaryTag());
		else if (type.isIdfDesc())
			return true;
		else 
#if 0
		if (sym == EnglishSTags::JJ ||
			sym == EnglishSTags::NN ||
			sym == EnglishSTags::NNS)
			return true;
#else
		if (sym == EnglishSTags::JJ)
			return true;
#endif
		else return false;
	}

	static Symbol getDefaultNamePOStag(EntityType type) {
		if (!type.getDefaultTag().is_null())
			return type.getDefaultTag();
		return EnglishSTags::NNP;
	}
	static Symbol getDefaultNameWord(ChartDecoder::decoder_type_t decoder_type, EntityType type) {
		if (!type.getDefaultWord().is_null())
			return type.getDefaultWord();
		else if (decoder_type == ChartDecoder::UPPER)
			return Symbol(L"NAME");
		else if (decoder_type == ChartDecoder::LOWER)
			return Symbol(L"name");
		else return Symbol(L"Name");		
	}

	static bool isPastTenseVerbChain(wstring tags, wstring words) {
		wstring vbd = EnglishSTags::VBD.to_string();
		wstring vbn = EnglishSTags::VBN.to_string();
		wstring vbz = EnglishSTags::VBZ.to_string();
		wstring vbp = EnglishSTags::VBP.to_string();
		
		// look for any VBD in chain
		size_t i_vbd = tags.find(vbd);
		if (i_vbd != wstring::npos)
			return true;
		
		// VBZ:VBN or VBP:VBN - have seen, has been, etc.
		if (!tags.compare(vbz + L":" + vbn) || !tags.compare(vbp + L":" + vbn)) {
			size_t colon = words.find(L":");
			if (colon != wstring::npos) {
				Symbol first_word = Symbol(words.substr(0,colon));
				if (first_word == EnglishWordConstants::HAVE ||
					first_word == EnglishWordConstants::HAS ||
					first_word == EnglishWordConstants::_S ||
					first_word == EnglishWordConstants::_VE)
				{
					return true;
				}

			}
		}
		// chain is just VBN
		else if (!tags.compare(vbn)) {
			return true;
		}

		return false;
	}

	static bool isPresentTenseVerbChain(wstring tags, wstring words) {
		return false;
	}	

	static bool isFutureTenseVerbChain(wstring tags, wstring words) {
		wstring md = EnglishSTags::MD.to_string();
		// look for MD tag at beginning of chain
		if (tags.find(md) == 0) {
			size_t colon = words.find(L":");
			colon = (colon != wstring::npos) ? colon : words.length();
			Symbol first_word = Symbol(words.substr(0,colon));
			if (first_word == EnglishWordConstants::WILL ||
				first_word == EnglishWordConstants::_LL ||
				first_word == EnglishWordConstants::WO_)
			{
				return true;
			}
		}
		return false;
	}

	static Symbol getSymbolForParseNodeLeaf(Symbol sym) {
		std::wstring str = sym.to_string();
		if(!_standAloneParser) {
			boost::to_lower(str);
			boost::replace_all(str, L"(", L"-LRB-");
			boost::replace_all(str, L")", L"-RRB-");
		}
		return Symbol(str.c_str());
	}

	/**
	 * find the head word of the node (usually np) with the exception
	 * that if the head is POS, get the immediate premod instead
	 * @param node The original node
	 * @return the smart head word
	 */
	static Symbol getSmartHeadWord(const SynNode* node) {
		// preterminal ancestor of the proper head
		const SynNode* head = node->getHeadPreterm();
		if (head->getTag() == EnglishSTags::POS) {
			// find a premod
			const SynNode* parent = head->getParent();
			while (parent != 0 && parent->getHeadIndex() - 1 < 0)
				parent = parent->getParent();
			if (parent != 0)
				head = parent->getChild(parent->getHeadIndex()-1);
		}
		return head->getHeadWord();
	}

	/**
	 * ensures a single NP premod under
	 * NPPOS, and changes an NP with list-ish structure into a set of NPs
	 * @param node The original parse node (modified in place)
	 */

	static void modifyParse(ParseNode* node) {
		if(_standAloneParser) return;
		_fixNPPOS(node);
		_fixNPList(node);
		_fixNameCommaNameHeads(node);
		_fixLegalNominalAdjectives(node);
		_fixHyphen(node);
		_splitOrgNames(node);
		return;

	}

	static void modifyParse(SynNode* node, CorefItem* coref, SynNode *children[], int n_children, int head_index) {
		if(_standAloneParser) return;
		_fixNPPOS(node, coref, children, n_children, head_index);
		_fixNPList(node, children, n_children, head_index);
		_fixNameCommaNameHeads(node, children, n_children, head_index);
		return;
	}

	static int findNameListEnd(const TokenSequence *tokenSequence, const NameTheory *nameTheory,
											   int start);

	static Symbol getParseTagForWord(Symbol word);
	static bool isTrulyUnknownWord(Symbol word);
	static bool isKnownNoun(Symbol word);
	static bool isKnownVerb(Symbol word);
	static bool isPotentialGerund(Symbol word);
	static void setAsStandAloneParser();
	static bool isStandAloneParser();
	static bool isNoun(Symbol tag){
		return isNPtypeLabel(tag);
	}
	static bool isNounPOS(Symbol pos_tag);
	static bool isVerbPOS(Symbol pos_tag);
	
	static Constraint* getConstraints(const NPChunkTheory* theory, const TokenSequence* ts, int& num);
	
	static int findNPHead(ParseNode* arr[], int numNodes);
	static int findNPHead(const SynNode* const arr[], int numNodes);

	static bool matchesHeadToken(Mention *mention, CorrectMention *correctMention) {
		const SynNode *mentionNode = mention->getNode();
		return (mentionNode->getHeadPreterm()->getEndToken() == correctMention->getHeadEndToken());
	}

	static bool matchesHeadToken(const SynNode *node, CorrectMention *correctMention) {
		return (node->getHeadPreterm()->getEndToken() == correctMention->getHeadEndToken());
	}

private:

	static void _fixNameCommaNameHeads(ParseNode* node);
	static void _fixNameCommaNameHeads(SynNode* node, SynNode* children[], 
		int& n_children, int& head_index);
	static SymbolHash * _legalNominalAdjectives;
	static void _fixLegalNominalAdjectives(ParseNode* node);
	static void _fixNPPOS(ParseNode* node);
	static void _fixNPPOS(SynNode* node, CorefItem* coref, SynNode *children[], 
		int &n_children, int &head_index);
	static void _fixNPList(ParseNode* node);
	static void _fixNPList(SynNode* node, SynNode* children[], 
		int& n_children, int& head_index);
	static int _scanForward(ParseNode* nodes[], int numNodes, Symbol syms[], int numSyms);
	static int _scanBackward(ParseNode* nodes[], int numNodes, Symbol syms[], int numSyms);
	static int _scanForward(const SynNode* const nodes[], int numNodes, Symbol syms[], int numSyms);
	static int _scanBackward(const SynNode* const nodes[], int numNodes, Symbol syms[], int numSyms);
	static void _fillNPFromVector(ParseNode* node, ParseNode* kids[], int size);
	static void _fixHyphen(ParseNode* node);
	static void _fixHyphenRegular(ParseNode* node);
	static void _fixHyphenPremod(ParseNode* node);
	static void _splitOrgNames(ParseNode* node);


};


#endif
