// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/SynNode.h"
#include "English/parse/en_STags.h"
#include "English/edt/en_DescLinkFeatureFunctions.h"
#include "English/common/en_WordConstants.h"


const SynNode* EnglishDescLinkFeatureFunctions::getNumericMod(const Mention *ment) {

	
	const SynNode *node = ment->getNode();

	// if the node is preterminal, it won't have any numeric premods
	if (node->isPreterminal())
		return 0;

	for (int i = 0; i < node->getHeadIndex(); i++) {
		const SynNode *child = node->getChild(i);
		const SynNode *term = child->getFirstTerminal();
		for (int j = 0; j < child->getNTerminals(); j++) {
			const SynNode *parent = term->getParent();
			if (parent != 0 && parent->getTag() == EnglishSTags::CD )
				return parent;
			if (j < child->getNTerminals() - 1)
				term = term->getNextTerminal();
		}
	}
	return 0;
}

std::vector<const Mention*> EnglishDescLinkFeatureFunctions::getModNames(const Mention* ment) {
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

std::vector<Symbol> EnglishDescLinkFeatureFunctions::getMods(const Mention* ment) {
	std::vector<Symbol> results;

	const SynNode* node = ment->node;
	Symbol headWord = node->getHeadWord();
	int mention_length = node->getNTerminals();
	Symbol* terms = _new Symbol[mention_length];
	node->getTerminalSymbols(terms, mention_length);
	
	for (int i = 0; i < mention_length; i++) {
		if (terms[i] == headWord)
			break;
		results.push_back(terms[i]);
	}
	delete [] terms;
	return results;
}


bool EnglishDescLinkFeatureFunctions::hasMod(const Mention* ment)
{
	// does ment have premods?
	const SynNode* node = ment->node;
	int hIndex = node->getHeadIndex();
	if (hIndex < 1)
		return false;
	// TODO: actually output the words in the event
	return true;
}


// This function was cut&pasted from en_GenericFilter
// It was changed to work with features and is a little different
// There may be better changes to make here
bool EnglishDescLinkFeatureFunctions::_isGenericMention(const Mention* ment, const EntitySet* ents)
{
	// the syntactic structure test series:
	// these all involve something to do with the syn node
	const SynNode* node = ment->node;

	// it's generic if the descriptor isn't an np (and it's weird)
	if (!_isNPish(node->getTag()))
		return true;
	const SynNode* coreNPA = _getCoreNPA(node);

	// not generic if coreNPA is nppos and npa is "definite"
	// e.g. "The country's leader"
	if (coreNPA->getTag() == EnglishSTags::NPPOS && _isDefinite(coreNPA))
		return false;

	// the next 4 tests look at the premods of the core npa.
	if (coreNPA->getHeadIndex() > 0) {
		int i;
		// this flag needed for a later test
		bool isFirstDT = false;
		for (i=0; i < coreNPA->getHeadIndex(); i++) {
			const SynNode* pre = coreNPA->getChild(i);
			// set flag for later test
			if (i == 0 && pre->getTag() == EnglishSTags::DT) {
				isFirstDT = true;
				continue;
			}
			Symbol tag = pre->getTag();
			// non generic if a premod is:
			// 1) NPPOS: "people's nation"
			// 2) NPPRO: TODO: what's an nppro?
			// 3) CD: "five factories"
			// 4) QP: "a percentage of the population"
			if (tag == EnglishSTags::NPPOS ||
				tag == EnglishSTags::NPPRO ||
				tag == EnglishSTags::CD ||
				tag == EnglishSTags::QP)
				return false;
			// non generic if a premod is a numeric JJ
			// e.g. "5.3 cities"
			if (tag == EnglishSTags::JJ && _isNumeric(pre->getHeadWord()))
				return false;

			// non generic if the first premod was a dt (or nppos, or nppro, but those cases
			// were caught above) and there is a premod with a name. e.g. "the Iraqi president"
			if (isFirstDT && pre->hasMention()) {
				Mention* preMent = ents->getMentionSet(ment->getSentenceNumber())->getMentionByNode(pre);
				if (preMent == 0)
					throw InternalInconsistencyException("EnglishGenericsFilter::_isGenericMention()",
														 "couldn't find premod mention in mention set");
				if (preMent->mentionType == Mention::NAME)
					return false;
			}

		}

		// non generic if the first premod of core was a dt (the flag set before)
		// and the whole node is an np, and the postmods of the node are a pp
		// with a name. e.g. "The president of Iraq"
		if (isFirstDT && node->getTag() == EnglishSTags::NP) {
			for (i = node->getHeadIndex()+1; i < node->getNChildren(); i++) {
				const SynNode* post = node->getChild(i);
				if (post->getTag() == EnglishSTags::PP) {
					int j;
					for (j=0; j < post->getNChildren(); j++) {
						const SynNode* ppNode = post->getChild(j);
						if (ppNode->hasMention()) {
							Mention* ppMent = ents->getMentionSet(ment->getSentenceNumber())->getMentionByNode(ppNode);
							if (ppMent == 0)
								throw InternalInconsistencyException("EnglishGenericsFilter::_isGenericMention()",
																	 "couldn't find postmod mention in mention set");

							if (ppMent->mentionType == Mention::NAME)
								return false;
						}
					}
				}
			}
		}
	}

	// the more broadly-reaching tests. Hypothetical and name bearing were already determined
	// for subjunctive we'll take the time to traverse the tree for each node, for now.
	// many of these tests want to know about definiteness;
//	bool isDef =  _isDefinite(node);
//	bool isSubj = _isInSubjunctive(node);
	// it is generic if node is indefinite and in a subjunctive node
//	if (!isDef && isSubj)
//		return true;

	// Tal's change
	if(_isDefinite(node))
		return true;   // true-this is an error, but works for some bizarre reason

	// this next test isn't actually all that far-reaching
	// it is specific if node is the object of a PP that is the object of something specific
	// e.g. "the son of a king"
	// the method returns the contagious node, for debugging purposes
	const SynNode* contNode = _getContagious(node, ment->getSentenceNumber(), ents);
	if (contNode != 0)
		return false;

	// specific if closest verbs of core are past tense
	// the method returns the verb node, for debugging purposes
	const SynNode* predNode = _getPredicate(coreNPA, ment->getSentenceNumber(), ents);
	if (predNode != 0) {
		if (predNode->getTag() == EnglishSTags::VBD ||
			(predNode->getParent() != 0 && predNode->getParent()->getHead()->getTag() == EnglishSTags::VBD))
			return false;
	}

	// generic if head is plural and no premod or postmod (bare plurals)
	if ((coreNPA->getTag() == EnglishSTags::NNS ||
		coreNPA->getTag() == EnglishSTags::NNPS) && coreNPA->getNChildren() < 2)
		return true;

	// TRUE otherwise
	return true;
}

// traverse nodes for subjunctive
// a subjunctive node is a vp with a md head, or a node with that in its parent
bool EnglishDescLinkFeatureFunctions::_isInSubjunctive(const SynNode* node)
{
	if (node == 0)
		return false;
	if (node->getTag() == EnglishSTags::VP && node->getHead()->getTag() == EnglishSTags::MD)
		return true;
	return _isInSubjunctive(node->getParent());
}


// look for numbers, an optional leading negative, and up to one decimal
bool EnglishDescLinkFeatureFunctions::_isNumeric(Symbol sym)
{
	const wchar_t* symStr = sym.to_string();
	int i = 0;
	if (symStr[0] == L'-')
		i++;
	bool seenDec = false;
	while(&symStr[i] != 0) {
		if (symStr[i] == L'.') {
			if (seenDec)
				return false;
			seenDec = true;
			i++;
			continue;
		}
		if (!iswdigit(symStr[i]))
			return false;
		i++;
	}
	return true;
}

bool EnglishDescLinkFeatureFunctions::_isNPish(Symbol sym)
{
	return (
		sym == EnglishSTags::NP ||
		sym == EnglishSTags::NPA ||
		sym == EnglishSTags::NPP ||
		sym == EnglishSTags::NPPOS ||
		sym == EnglishSTags::NPPRO
		);
}

const SynNode* EnglishDescLinkFeatureFunctions::_getCoreNPA(const SynNode* node)
{
	if (node->getTag() == EnglishSTags::NP &&
		node->getHead()->getTag() == EnglishSTags::NPA)
		return node->getHead();
	return node;
}

// look for a definite article in the first premod of the node
bool EnglishDescLinkFeatureFunctions::_isDefinite(const SynNode* node)
{
	const SynNode* pre = node->getChild(0);
	// if no premod, not definite
	if (pre == 0 || pre == node->getHead())
		return false;
	if (pre->getTag() == EnglishSTags::PRPS)
		return true;
	if (pre->getTag() == EnglishSTags::DT) {
		Symbol preWord = pre->getHeadWord();
		if (preWord == EnglishWordConstants::THE ||
			preWord == EnglishWordConstants::THAT ||
			preWord == EnglishWordConstants::THIS ||
			preWord == EnglishWordConstants::THESE ||
			preWord == EnglishWordConstants::THOSE)
			return true;
	}
	return false;
}

// the node must be child of a pp, and that pp must be the postmod of a node whose head (or head path)
// is a mention of something specific
const SynNode* EnglishDescLinkFeatureFunctions::_getContagious(const SynNode* node, int sentNum, const EntitySet* ents)
{
	const SynNode* parent = node->getParent();
	if (parent == 0 || parent->getTag() != EnglishSTags::PP)
		return 0;
	const SynNode* grandparent = parent->getParent();
	if (grandparent == 0)
		return 0;
	// the pp shouldn't be the head of its parent
	const SynNode* part = grandparent->getHead();
	if (part == parent)
		return 0;
	const MentionSet* ments = ents->getMentionSet(sentNum);
	// find a mention in the parent chain of the part
	Mention* partMent = 0;
	while (part != 0 && _isNPish(part->getTag())) {
		partMent = ments->getMentionByNode(part);
		if (partMent == 0) {
			return 0;
		}
		if (partMent->isPopulated())
			break;
		part = part->getParent();
	}
	if (part == 0 || partMent == 0 || !partMent->isPopulated())
		return 0;
	// NOTE: assume only edt entities count.
	if (!partMent->isOfRecognizedType())
		return 0;

	// if the mention has relevant children, I'd prefer to use them
	while (partMent->getChild() != 0 && partMent->getChild()->mentionType != Mention::NONE)
		partMent = partMent->getChild();
	part = partMent->node;
	// look at the entity of this mention.
	Entity* ent = ents->getEntityByMention(ments->getMentionByNode(part)->getUID());
	// something weird if we can't get the entity
	if (ent == 0) {
		return 0; // this happens in bnews-66 and npaper sets
#if 0
		string err = "unable to find entity given node:\n";
		err.append(part->toDebugString(0).c_str());
		err.append(" when mention is of type ");
		err.append(Mention::getTypeString(partMent->mentionType));
		err.append(", ");
		err.append(partMent->getEntityType().to_debug_string());
		err.append("\n");
		throw InternalInconsistencyException("EnglishGenericsFilter::_getContagious", (char*)(err.c_str()));
#endif
	}
	// if the contagious mention is specific, so are we
	if (!ent->isGeneric())
		return part;
	return 0;
}

// find closest verbs to this node
const SynNode* EnglishDescLinkFeatureFunctions::_getPredicate(const SynNode* node, int sentNum, const EntitySet* ents)
{
	return _getPredicateDriver(node, false, sentNum, ents);

}

// do the work for the above
const SynNode* EnglishDescLinkFeatureFunctions::_getPredicateDriver(const SynNode* node, bool goingDown, int sentNum, const EntitySet* ents)
{
	if (node == 0)
		return 0;
	// we might be an aux. find the real verb in postmode (now descending)
	if (_isVish(node->getTag())) {
		int i;
		for(i = node->getHeadIndex()+1; i < node->getNChildren(); i++) {
			const SynNode* post = node->getChild(i);
			if (post->getTag() == EnglishSTags::VP)
				return _getPredicateDriver(post, true, sentNum, ents);
		}
		return node;
	}
	// look down vp & S chain
	if (node->getTag() == EnglishSTags::VP || _isSish(node->getTag()))
		return _getPredicateDriver(node->getHead(), true, sentNum, ents);

	// bad path to descend
	if (goingDown || node->getParent() == 0)
		return 0;
	const SynNode* parent = node->getParent();
	// pp object of a partitive np  doesn't really care about the verb
	if (node->getTag() == EnglishSTags::PP && _isNPish(parent->getTag())) {
		// checking partitive parent (info is in the mention)
		if (parent->hasMention() && ents->getMentionSet(sentNum)->getMentionByNode(parent)->mentionType == Mention::PART)
			return 0;
	}
	return _getPredicateDriver(parent, goingDown, sentNum, ents);

}

bool EnglishDescLinkFeatureFunctions::_isVish(Symbol sym)
{
	return (
		sym == EnglishSTags::VBD ||
		sym == EnglishSTags::VBG ||
		sym == EnglishSTags::VBN ||
		sym == EnglishSTags::VBP ||
		sym == EnglishSTags::VBZ
		);
}

bool EnglishDescLinkFeatureFunctions::_isSish(Symbol sym)
{
	return (
		sym == EnglishSTags::S ||
		sym == EnglishSTags::SINV ||
		sym == EnglishSTags::SQ
		);
}
