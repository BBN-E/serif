// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/parse/en_NodeInfo.h"

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/theories/SynNode.h"
#include "English/parse/en_STags.h"
#include "Generic/wordnet/xx_WordNet.h"
#include "English/common/en_WordConstants.h"


bool EnglishNodeInfo::isReferenceCandidate(const SynNode *node) {
	Symbol tag = node->getTag();

	if (tag == EnglishSTags::RB ||
		/* this other condition includes "home", which can be found as
		   (ADVP (NN home)) */
		(node->getParent() != 0 &&
		 node->getParent()->getTag() == EnglishSTags::ADVP &&
		 node->getParent()->getHead() == node))
	{
		if (node->getHeadWord() == EnglishWordConstants::HERE ||
			node->getHeadWord() == EnglishWordConstants::THERE ||
			node->getHeadWord() == EnglishWordConstants::ABROAD||
			node->getHeadWord() == EnglishWordConstants::OVERSEAS || 
			node->getHeadWord() == EnglishWordConstants::HOME)
		{
			return true;
		}
	}

	// NOMINAL PREMODS
	if (useNominalPremods() && isNominalPremod(node)) {
		return true;
	}

	if (useWHQMentions() && isWHQNode(node)) {
		return true;
	}
	if(tag == EnglishSTags::NP ||
			tag == EnglishSTags::NPA ||
			tag == EnglishSTags::NPP ||
			tag == EnglishSTags::NPPRO ||
			tag == EnglishSTags::PRP ||
			tag == EnglishSTags::PRPS ||
			tag == EnglishSTags::NX ||
			tag == EnglishSTags::NAC ||
			tag == EnglishSTags::DATE)
	{
				Symbol head_preterm_tag = node->getHeadPreterm()->getTag();
				//Serif finds "huh ." etc as an NP headed by '.', 
				//really we don't want punctuation mentions
				if((head_preterm_tag == EnglishSTags::DASH) || 
					(head_preterm_tag == EnglishSTags::DOT) ||
					(head_preterm_tag == EnglishSTags::COMMA))
				{
						return false;
				}
				else return true;
	}
	return false;


}

bool EnglishNodeInfo::isOfNPKind(const SynNode *node) {
	Symbol tag = node->getTag();
	return (tag == EnglishSTags::NP ||
			tag == EnglishSTags::NPP ||
			tag == EnglishSTags::NPA ||
			tag == EnglishSTags::NPPOS ||
			tag == EnglishSTags::WHNP ||
			tag == EnglishSTags::NPPRO);
}

bool EnglishNodeInfo::isOfHobbsNPKind(const SynNode *node) {
	Symbol tag = node->getTag();
	return (tag == EnglishSTags::NP ||
			tag == EnglishSTags::NPP ||
			tag == EnglishSTags::NPA);
}

bool EnglishNodeInfo::isOfHobbsSKind(const SynNode *node) {
	Symbol tag = node->getTag();
	return (tag == EnglishSTags::S ||
			tag == EnglishSTags::SINV ||
			tag == EnglishSTags::SQ);
}

bool EnglishNodeInfo::canBeNPHeadPreterm(const SynNode *node) {
	Symbol tag = node->getTag();
	if (tag == EnglishSTags::NN ||
		tag == EnglishSTags::NNS ||
		tag == EnglishSTags::NNP ||
		tag == EnglishSTags::NNPS)
		return true;
	if (tag == EnglishSTags::JJ) {
		Symbol headword = node->getHeadWord();
		return isValidAdjectiveNomPremod(node->getHeadWord());
	}
	return false;
}

bool EnglishNodeInfo::isNominalPremod(const SynNode *node) {
	if (canBeNPHeadPreterm(node) &&
		node->getParent() != 0 &&
		node->getParent()->getTag() != EnglishSTags::NPP &&
		node->getParent()->getHead() != node)
	{
		const SynNode *parent = node->getParent();
		for (int i = parent->getHeadIndex();
			 i > 0 && parent->getChild(i) != node; i--)
		{
			if (parent->getChild(i)->getTag() == EnglishSTags::COMMA)
				return false;
		}

		return true;
	}
	else {
		return false;
	}
}

bool EnglishNodeInfo::isWHQNode(const SynNode *node) {	
	Symbol tag = node->getTag();
	if (tag == EnglishSTags::WPDOLLAR)
		return true;
	if (node->getHead() == 0)
		return false;
	Symbol headTag = node->getHead()->getTag();
	if (tag == EnglishSTags::WHNP &&
		(headTag == EnglishSTags::WDT ||
		 headTag == EnglishSTags::WP))
		 return true;
	if (tag == EnglishSTags::WHADVP &&
		headTag == EnglishSTags::WRB &&
		node->getHeadPreterm()->getHeadWord() == EnglishWordConstants::WHERE)
		return true;
	return false;
}

bool EnglishNodeInfo::useWHQMentions() {
	static bool init = false;
	static bool use_whq;
	if (!init) {
		init = true;
		use_whq = ParamReader::getRequiredTrueFalseParam("use_whq_mentions");
	}
	return use_whq;
}

bool EnglishNodeInfo::useNominalPremods() {
	static bool init = false;
	static bool use_nompre;
	if (!init) {
		use_nompre = ParamReader::getRequiredTrueFalseParam("use_nominal_premods");
		init = true;
	}
	return use_nompre;
}

bool EnglishNodeInfo::isValidAdjectiveNomPremod(Symbol headword) {
	static bool init = false;
	static SymbolHash *table = 0;
	if (!init) {
		std::string param = ParamReader::getParam("adjectival_nominal_premods");
		if (!param.empty()) {
			table = _new SymbolHash(param.c_str());
		} else table = 0;
	}
	init = true;
	if (table == 0)
		return false;
	return table->lookup(headword);
}

