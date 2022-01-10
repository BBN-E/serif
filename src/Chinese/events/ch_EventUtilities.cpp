// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Chinese/events/ch_EventUtilities.h"
#include "Generic/events/EventFinder.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/common/SymbolHash.h"
#include "Chinese/parse/ch_STags.h"
#include "Chinese/parse/ch_LanguageSpecificFunctions.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/MentionSet.h"

SymbolHash * ChineseEventUtilities::_nonAssertedIndicators;

// This function was initially lifted from ChineseRelationUtilities, but has been modified
void ChineseEventUtilities::identifyNonAssertedProps(const PropositionSet *propSet, 
													 const MentionSet *mentionSet, 
													 bool *isNonAsserted)

{
	static bool init = false;
	if (!init) {
		init = true;
		std::string filename = ParamReader::getParam("non_asserted_event_indicators");
		if (!filename.empty())
		{
			_nonAssertedIndicators = _new SymbolHash(filename.c_str());
		} else _nonAssertedIndicators = _new SymbolHash(5);
	}
	
	for (int i = 0; i < propSet->getNPropositions(); i++) {
		isNonAsserted[i] = false;
	}

	/*for (int k = 0; k < propSet->getNPropositions(); k++) {
		Proposition *prop = propSet->getProposition(k);

		// allegedly
		if (prop->getAdverb() && prop->getAdverb()->getHeadWord() == allegedly_sym)
			isNonAsserted[prop->getIndex()] = true;

		// should, could, might, may
		const SynNode *modal = prop->getModal();
		if (modal != 0) {
			Symbol modalWord = modal->getHeadWord();
			if (modalWord == ChineseWordConstants::SHOULD ||
				modalWord == ChineseWordConstants::COULD ||
				modalWord == ChineseWordConstants::MIGHT ||
				modalWord == ChineseWordConstants::MAY)
				isNonAsserted[prop->getIndex()] = true;
		}

		// if/whether: p#
		// not ALWAYS accurate, but close enough		
		for (int j = 0; j < prop->getNArgs(); j++) {
			Argument *arg = prop->getArg(j);
			if (arg->getType() == Argument::PROPOSITION_ARG &&
				(arg->getRoleSym() == ChineseWordConstants::IF || 
				arg->getRoleSym() == ChineseWordConstants::WHETHER))
			{
				isNonAsserted[arg->getProposition()->getIndex()] = true;
			}
		}

		if (prop->getPredType() == Proposition::VERB_PRED) {
			// get "if..." unrepresented in propositions
			const SynNode* node = prop->getPredHead();
			while (node != 0) {
				if (node->getTag() != ChineseSTags::VP &&
					node->getTag() != ChineseSTags::S &&
					node->getTag() != ChineseSTags::SBAR &&
					!node->isPreterminal())
					break;
				if (node->getTag() == ChineseSTags::SBAR &&
					(node->getHeadWord() == ChineseWordConstants::IF ||
					(node->getHeadIndex() == 0 &&
					node->getNChildren() > 1 &&
					node->getChild(1)->getTag() == ChineseSTags::IN &&
					node->getChild(1)->getHeadWord() == ChineseWordConstants::IF)))
				{
					isNonAsserted[prop->getIndex()] = true;
					break;
				}
				node = node->getParent();								
			}
		}
	}

	for (int l = 0; l < propSet->getNPropositions(); l++) {
		Proposition *prop = propSet->getProposition(l);
		
		Symbol predicate = prop->getPredSymbol();
		if (predicate.is_null())
			continue;
		Symbol stemmed_predicate = WordNet::getInstance()->stem_verb(predicate);

		// [non-asserted prop] that: p# -- e.g. he might plan an attack
		// ... but only allow verbs, no modifiers, e.g. he might be happy that...
		// OR
		// [non-asserted word] that: p# -- he guessed that X

		if (_nonAssertedIndicators->lookup(stemmed_predicate) ||
			(isNonAsserted[prop->getIndex()] && prop->getPredType() == Proposition::VERB_PRED))
		{
			for (int j = 0; j < prop->getNArgs(); j++) {
				Argument *arg = prop->getArg(j);
				if (arg->getType() == Argument::PROPOSITION_ARG &&
					(arg->getRoleSym() == Argument::OBJ_ROLE ||
					 arg->getRoleSym() == ChineseWordConstants::THAT ||
					 arg->getRoleSym() == Argument::IOBJ_ROLE))					  
				{
					isNonAsserted[arg->getProposition()->getIndex()] = true;
				}
			}
		}
	}*/
}

bool ChineseEventUtilities::includeInConnectingString(Symbol tag, Symbol next_tag) {
	/*if (tag == ChineseSTags::DATE_NNP || 
		tag == ChineseSTags::DATE_NNPS ||
		tag == ChineseSTags::RB)
		return false;
	if (tag == ChineseSTags::IN && (next_tag == ChineseSTags::DATE_NNP || next_tag == ChineseSTags::DATE_NNPS))
		return false;*/
	return true;
}

bool ChineseEventUtilities::includeInAbbreviatedConnectingString(Symbol tag, Symbol next_tag) {
	if (!includeInConnectingString(tag, next_tag))
		return false;
	return (LanguageSpecificFunctions::isVerbPOSLabel(tag) ||
		LanguageSpecificFunctions::isPrepPOSLabel(tag));
}
