// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/edt/es_DescLinkFeatureFunctions.h"
#include "Spanish/parse/es_STags.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/SynNode.h"

Symbol SpanishDescLinkFeatureFunctions::LINK_SYM = Symbol(L"LINK");

const SynNode* SpanishDescLinkFeatureFunctions::getNumericMod(const Mention *ment) {	
	const SynNode *node = ment->getNode();

	// if the node is preterminal, it won't have any numeric premods
	if (node->isPreterminal())
		return 0;

	for (int i = 0; i < node->getHeadIndex(); i++) {
		const SynNode *child = node->getChild(i);
		const SynNode *term = child->getFirstTerminal();
		for (int j = 0; j < child->getNTerminals(); j++) {
			const SynNode *parent = term->getParent();
			if (parent != 0 && parent->getTag() == SpanishSTags::POS_Z)
				return parent;
			if (j < child->getNTerminals() - 1)
				term = term->getNextTerminal();
		}
	}

	return 0;
}




std::vector<const Mention*> SpanishDescLinkFeatureFunctions::getModNames(const Mention* ment) {	
	std::vector<const Mention*> results;

	// are there any mentions of entities yet?
	const MentionSet* ms = ment->getMentionSet();
	if (ms == 0)
		return results;

	// does ment have premods?
	const SynNode* node = ment->node;
	int hIndex = node->getHeadIndex();
	for (int i = 0; i < hIndex; i++) {
		// get all names nested within this node if it has a mention
		DescLinkFeatureFunctions::getMentionInternalNames(node->getChild(i), ms, results);
	}

	return results;
}



namespace {
	const Symbol LRB_SYM(L"(");
	const Symbol LRB_SUB_SYM(L"-LRB-");
	const Symbol RRB_SYM(L")");
	const Symbol RRB_SUB_SYM(L"-RRB-");
}

std::vector<Symbol> SpanishDescLinkFeatureFunctions::getMods(const Mention* ment) {	
	std::vector<Symbol> results;

	const SynNode* node = ment->node;
	Symbol headWord = node->getHeadWord();
	int mention_length = node->getNTerminals();
	Symbol* terms = _new Symbol[mention_length];
	node->getTerminalSymbols(terms, mention_length);
	
	for (int i = 0; i < mention_length; i++) {
		if (terms[i] == headWord)
			break;
		else if (terms[i] == LRB_SYM)
			break; //results.push_back(LRB_SUB_SYM);
		else if (terms[i] == RRB_SYM)
			break; //results.push_back(RRB_SUB_SYM);
		else
			results.push_back(terms[i]);
	}
	delete [] terms;
	return results;
}


bool SpanishDescLinkFeatureFunctions::hasMod(const Mention* ment)
{
	// does ment have premods?
	const SynNode* node = ment->node;
	int hIndex = node->getHeadIndex();
	if (hIndex < 1)
		return false;
	// TODO: actually output the words in the event
	return true;
}
