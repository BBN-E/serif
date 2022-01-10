// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/edt/ar_DescLinkFeatureFunctions.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/SynNode.h"

#include "Arabic/parse/ar_STags.h"



/*
################### Description of Arabic modification ###################
English:                                                Arabic

The 15 officials                          The 15 officials
The 15 American officials                 The 15 officials American
The 15 government officials               The 15 officials government
The 15 American government officials      The 15 officials government American/The 15 officials American government
*/

//Arabic is usually head initial, but numbers proceed the words they modify
const SynNode* ArabicDescLinkFeatureFunctions::getNumericMod(const Mention *ment) {

	
	const SynNode *node = ment->getNode();

	// if the node is preterminal, it won't have any numeric premods
	if (node->isPreterminal())
		return 0;

	for (int i = 0; i < node->getHeadIndex(); i++) {
		const SynNode *child = node->getChild(i);
		const SynNode *term = child->getFirstTerminal();
		for (int j = 0; j < child->getNTerminals(); j++) {
			const SynNode *parent = term->getParent();
			if (parent != 0 && (parent->getTag() == ArabicSTags::CD ||
				parent->getTag() == ArabicSTags::NUMERIC_COMMA))
				return parent;
			if (j < child->getNTerminals() - 1)
				term = term->getNextTerminal();
		}
	}
	
	return 0;
}




std::vector<const Mention*> ArabicDescLinkFeatureFunctions::getModNames(const Mention* ment) {
	std::vector<const Mention*> results;
	//why???? -Names will be put in entities already
	
	// are there any mentions of entities yet?
	const MentionSet* ms = ment->getMentionSet();
	if (ms == 0)
		return results;
	
	
	//Arabic is head initial in these cases (men American)
	// does ment have premods?
	const SynNode* node = ment->node;
	int hIndex = node->getHeadIndex();
	if(hIndex == (node->getNChildren() - 1))
		return results;

	// this looks like a bug to me - shouldn't i start at hIndex+1?
	for (int i = hIndex; i < node->getNChildren(); i++) {
		// get all names nested within this node if it has a mention
		DescLinkFeatureFunctions::getMentionInternalNames(node->getChild(i), ms, results);
	}
	return results;
}

std::vector<Symbol> ArabicDescLinkFeatureFunctions::getMods(const Mention* ment) {	
	std::vector<Symbol> results;

	const SynNode* node = ment->node;
	Symbol headWord = node->getHeadWord();
	int mention_length = node->getNTerminals();
	Symbol* terms = _new Symbol[mention_length];
	node->getTerminalSymbols(terms, mention_length);

	int n_results = 0;
	int headindex = -1;
	for (int i = 0; i < mention_length; i++) {
		if (terms[i] == headWord) {
			headindex = i;
			break;
		}
		
	}
	if (headindex != -1) {	
		for (int j = headindex; j < mention_length; j++) {
			results.push_back(terms[j]);
		}
	}
	delete [] terms;
	return results;
}


bool ArabicDescLinkFeatureFunctions::hasMod(const Mention* ment)
{	
	std::vector<Symbol> results = getMods(ment);
	return (!results.empty());
}


/*
// the idea being to return predicates only. If it's not a predicate (verb word)
// don't bother 
Symbol ArabicDescLinkFeatureFunctions::_getParentHead(Mention* ment) {
	const SynNode* node = ment->node;
	if (node->getParent() == NULL) 
		return Symbol(L"");
	Symbol mentHead = node->getHeadWord();
	while (node->getParent() != NULL) {
		const SynNode* headNode = node->getParent()->getHeadPreterm();
		Symbol headWord = headNode->getHeadWord();
		if (headWord != mentHead &&
			(headNode->getTag() == ArabicSTags::VA ||
			 headNode->getTag() == ArabicSTags::VC ||
			 headNode->getTag() == ArabicSTags::VCD ||
			 headNode->getTag() == ArabicSTags::VCP ||
			 headNode->getTag() == ArabicSTags::VE ||
			 headNode->getTag() == ArabicSTags::VNV ||
			 headNode->getTag() == ArabicSTags::VP ||
			 headNode->getTag() == ArabicSTags::VPT ||
			 headNode->getTag() == ArabicSTags::VRD ||
			 headNode->getTag() == ArabicSTags::VSB ||
			 headNode->getTag() == ArabicSTags::VV)) {
				 return headWord;
			 }
			 node = node->getParent();
	}
	return Symbol(L"");
}
*/
/*
Symbol ArabicDescLinkFeatureFunctions::_findMentParent(Mention* ment) {
	Symbol sym = _getParentHead(ment);
	if (sym.to_string() != L"") {
		wchar_t wordstr[300];
		swprintf(wordstr, 300, L"ment-parent[%ls]", sym.to_string());
		sym = Symbol(wordstr);
	}
	else {
		wchar_t wordstr[300];
		swprintf(wordstr, 300, L"ment-parent[]");
		sym = Symbol(wordstr);
	}
	return sym;
}
*/
/*
int ArabicDescLinkFeatureFunctions::_findEntParents(Entity* ent, EntitySet* ents, Symbol *results, const int max_results) {
	SymbolHash entityPreds(5);
	int n_results = 0;
	for (int i = 0; i < ent->getNMentions(); i++) {
		if (n_results == max_results)
			break;
		Mention* ment = ents->getMention(ent->getMention(i));
		Symbol sym = _getParentHead(ment);
		if (sym != Symbol(L"") && !entityPreds.lookup(sym)) {
			entityPreds.add(sym);
			wchar_t wordstr[300];
			swprintf(wordstr, 300, L"ent-parent[%ls]", sym.to_string());
			results[n_results++] = Symbol(wordstr);
		}
	}
	return n_results;
}
*/
