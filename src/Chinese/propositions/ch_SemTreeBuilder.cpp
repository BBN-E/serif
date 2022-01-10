// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

//#include <cassert>

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/WordConstants.h"
#include "Chinese/propositions/ch_SemTreeBuilder.h"
#include "Generic/propositions/sem_tree/SemBranch.h"
#include "Generic/propositions/sem_tree/SemReference.h"
#include "Generic/propositions/sem_tree/SemMention.h"
#include "Generic/propositions/sem_tree/SemTrace.h"
#include "Generic/propositions/sem_tree/SemOPP.h"
#include "Generic/propositions/sem_tree/SemLink.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Proposition.h"
#include "Chinese/parse/ch_STags.h"


static Symbol sym_not(L"\x4e0d");
//int ChineseSemTreeBuilder::analyzeChildrenStackDepth = 0;
//int ChineseSemTreeBuilder::maxAnalyzeChildrenStackDepth = 0;

void ChineseSemTreeBuilder::initialize() {}

SemNode *ChineseSemTreeBuilder::buildSemTree(const SynNode *synNode, const MentionSet *mentionSet) {
    //analyzeChildrenStackDepth = 0;
    //maxAnalyzeChildrenStackDepth = 0;
    SemNode* rootNode = 0;
    //try {
        rootNode = _new SemBranch(buildSemSubtree(synNode, mentionSet), synNode);
    //    assert(analyzeChildrenStackDepth == 0);
    //} catch (InternalInconsistencyException &e) {
    //    std::cerr << "\nWarning, setting ChineseSemTreeBuilder setting root node to null: " << e.getMessage() << "\n";
    //    rootNode = 0;
    //} 
    return rootNode;
}

bool ChineseSemTreeBuilder::isCopula(const SynNode &node) {
	Symbol tag = node.getTag();
	// TODO: check to make sure it isn't not_be
	return (tag == ChineseSTags::VC);
}

bool ChineseSemTreeBuilder::isNegativeAdverb(const SynNode &node) {
	//TODO: also check for negative VEs
	const SynNode *foo;
	if (node.getTag() == ChineseSTags::ADVP)
		foo = node.getHead();
	else
		foo = &node;

	if (foo->getTag() == ChineseSTags::AD) {
		Symbol head = foo->getHeadWord();
		if (head == sym_not)
			return true;
	}
	return false;
}

bool ChineseSemTreeBuilder::isModalVerb(const SynNode &node) {
	const SynNode *parent = node.getParent();
	if (parent->getTag() == ChineseSTags::VP && parent->getNChildren() == 2) {
		if ((node.getTag() == ChineseSTags::VA ||
			 node.getTag() == ChineseSTags::VC ||
			 node.getTag() == ChineseSTags::VCD ||
		     node.getTag() == ChineseSTags::VCP ||
		     node.getTag() == ChineseSTags::VE ||
			 node.getTag() == ChineseSTags::VNV ||
			 node.getTag() == ChineseSTags::VPT ||
			 node.getTag() == ChineseSTags::VRD ||
			 node.getTag() == ChineseSTags::VSB ||
			 node.getTag() == ChineseSTags::VV) &&
			parent->getChild(0) == &node && 
			parent->getChild(1)->getTag() == ChineseSTags::VP)
		{
			return true;
		}
	}
	return false;
}

bool ChineseSemTreeBuilder::canBeAuxillaryVerb(const SynNode &node) {
	return false;
} 

bool ChineseSemTreeBuilder::isTemporalNP(const SynNode &node) {
	Symbol headTag = node.getHeadPreterm()->getTag();
	return (headTag == ChineseSTags::NT);
}


bool ChineseSemTreeBuilder::isLocationNP(const SynNode &node) {
	// return true if node is premodifier in VP 
	const SynNode *parent = node.getParent();
	
	if (parent == 0 || parent->getTag() != ChineseSTags::VP)
		return false;
	
	for (int i = 0; i < parent->getHeadIndex(); i++) {
		if (parent->getChild(i) == &node)
			return true;
	}
	return false;
}

SemOPP *ChineseSemTreeBuilder::findAssociatedPredicateInNP(SemLink &link) {
	if (link.getParent() != 0 &&
		link.getParent()->getSemNodeType() == SemNode::MENTION_TYPE)
	{
		SemMention &parent = link.getParent()->asMention();

		SemOPP *noun = 0;
		if (link.getSymbol() == Argument::POSS_ROLE ||
			link.getSymbol() == Argument::UNKNOWN_ROLE)
		{
			// for possessives and premod/unknown's, search forwards
			for (SemNode *sib = &link; sib; sib = sib->getNext()) {
				if (sib->getSemNodeType() == SemNode::OPP_TYPE &&
					sib->asOPP().getPredicateType() == Proposition::NOUN_PRED)
				{
					noun = &sib->asOPP();
					break;
				}
			}
		}
		else {
			// for other links, search among previous sibs
			for (SemNode *sib = link.getParent()->getFirstChild(); sib;
				 sib = sib->getNext())
			{
				if (sib->getSemNodeType() == SemNode::OPP_TYPE &&
					sib->asOPP().getPredicateType() == Proposition::NOUN_PRED)
				{
					noun = &sib->asOPP();
					break;
				}
			}
		}

		return noun;
	}

	return 0;
}

bool ChineseSemTreeBuilder::uglyBranchArgHeuristic(SemOPP &parent, SemBranch &child) {
	return (parent.getArg1() == 0);
}

int ChineseSemTreeBuilder::mapSArgsToLArgs(SemNode **largs, SemOPP &opp,
				   SemNode *ssubj, SemNode *sarg1, SemNode *sarg2,
				   int n_links, SemLink **links)
{
	// active verb, or not a verb at all

	largs[0] = ssubj;
	largs[1] = sarg1;
	largs[2] = sarg2;
	for (int i = 0; i < n_links; i++)
		largs[3+i] = links[i];

	return 3+n_links;
}

SemNode *ChineseSemTreeBuilder::buildSemSubtree(const SynNode *synNode,
										 const MentionSet *mentionSet)
{
	SessionLogger &logger = *SessionLogger::logger;

#if defined(_WIN32)
//	HeapChecker::checkHeap("ChineseSemTreeBuilder::buildSemTree()");
#endif

/*	std::cout << "/----\n" << *synNode << "\n";
	std::cout.flush();
*/

	if (synNode->isTerminal())
		return 0;

	// branch out according to constituent type:
	Symbol synNodeTag = synNode->getTag();
	if (synNodeTag == ChineseSTags::TOP) {
		return analyzeChildren(synNode, mentionSet);
	}
	else if ((synNodeTag == ChineseSTags::IP && isList(ChineseSTags::IP, *synNode)) ||
		     (synNodeTag == ChineseSTags::VP && isList(ChineseSTags::VP, *synNode)) )
	{
		// looking at conjoined list of predicates

		// try to find head, which is a CC
		const SynNode *head = 0;
		for (int i = 0; i < synNode->getNChildren(); i++) {
			if (synNode->getChild(i)->getTag() == ChineseSTags::CC) {
				head = synNode->getChild(i);
				break;
			}
		}

		SemOPP *result = _new SemOPP(0, synNode, Proposition::COMP_PRED,
									 head);

		SemNode *children = analyzeChildren(synNode, mentionSet);
		for (SemNode::SiblingIterator iter(children);
			 iter.more(); ++iter)
		{
			SemNode *child = &*iter;

			// break the node off to do with as we please
			child->setNext(0);

			if (child->getSemNodeType() == SemNode::OPP_TYPE) {
				// for the OPP, create a branch for it to live in
				child = _new SemBranch(child, child->getSynNode());
			}
			if (child->getSemNodeType() == SemNode::BRANCH_TYPE) {
				// make a branch child the object of a <member> link
				SemLink *newLink = _new SemLink(child, child->getSynNode(),
												Argument::MEMBER_ROLE, 0);
				result->appendChild(newLink);
			}
			else {
				result->appendChild(child);
			}
		}

		return result;
	}
	else if (synNodeTag == ChineseSTags::IP ||
			 synNodeTag == ChineseSTags::FRAG)
	{
		// for sentence, make a branch node ordinarily; but if there's a 
		// preposition or WH-word linking it back to it's parent, make
		// a link

		Symbol first_child_sym = Symbol();
		if (synNode->getNChildren() > 0)
			first_child_sym = synNode->getChild(0)->getTag();

		if (first_child_sym == ChineseSTags::P) {
			return _new SemLink(analyzeChildren(synNode, mentionSet),
								synNode,
								synNode->getChild(0)->getHeadWord(),
								synNode->getChild(0));
		}
		else {
			// no linker word -- just a regular branch

			SemNode *semChildren = analyzeChildren(synNode, mentionSet);

			// if there's a negative here, move it to any opp we find
			const SynNode *negative = 0;
			for (int i = 0; i < synNode->getNChildren(); i++) {
				if (isNegativeAdverb(*synNode->getChild(i))) {
					negative = synNode->getChild(i);
					break;
				}
			}
			if (negative != 0) {
				for (SemNode *semChild = semChildren; semChild;
					 semChild = semChild->getNext())
				{
					if (semChild->getSemNodeType() == SemNode::OPP_TYPE)
						semChild->asOPP().addNegative(negative);
				}
			}

			return _new SemBranch(semChildren, synNode);
		}
	}
	else if (synNodeTag == ChineseSTags::PRN) {
		// a parenthetical -- pass up the children
		return analyzeChildren(synNode, mentionSet);
	}
	else if (synNode->hasMention())
	{
		const Mention *mention = mentionSet->getMentionByNode(synNode);

		if (mention == 0) {
			throw InternalInconsistencyException(
				"ChineseSemTreeBuilder::buildSemSubtree()",
				"Given SynNode thinks it has a mention but no mention found");
		}

		if (mention->getParent() != 0 &&
			mention->getParent()->getMentionType() == Mention::APPO)
		{
			// this is part of an appositive, so don't treat it as its own
			// mention
//			return analyzeChildren(synNode, mentionSet);
			// this is commented out because it is done in the simplify() 
			// stage
		}

		if (mention->getMentionType() == Mention::NONE) {
			// this may be the head of a name
			Mention *parent = mention->getParent();
			Mention *child = mention->getChild();
			while (parent && parent->getMentionType() == Mention::NONE)
				parent = parent->getParent();

			if (parent && parent->getMentionType() == Mention::NAME && child == 0)
			{
				// The name OPP is generated when the parent name is
				// processed, so don't analyze any further
				return 0;
			}
			else {
				// This is an empty mention, so just go through it
				return analyzeChildren(synNode, mentionSet);
			}
		}
		else if (mention->getMentionType() == Mention::LIST) {
			// looking at list of conjoined mentions

			SemNode *semChildren = analyzeChildren(synNode, mentionSet);

			// This OPP will be the "set" defining the mention
			SemOPP *opp = _new SemOPP(semChildren, synNode,
									  Proposition::SET_PRED, 0);

			// replace all mentions with a "member" link to that mention
			for (SemNode::SiblingIterator iter(semChildren); iter.more();
				 ++iter)
			{
				SemNode &child = *iter;

				if (child.getSemNodeType() == SemNode::MENTION_TYPE) {
					SemLink *newLink = _new SemLink(0, child.getSynNode(),
													Argument::MEMBER_ROLE,
													child.getSynNode());
					child.replaceWithNode(newLink);
					newLink->appendChild(&child);
				}
			}

			return _new SemMention(opp, synNode, mention, false);
		}
		else if (mention->getMentionType() == Mention::PART) {
/*
			// for partitives, we have to decide whether to create a 
			// separate mention or not

			// get partitive head
			SynNode *headNode = mention->getChild()->getNode();

			SemNode *object = buildSemTree(headNode, mentionSet);
			// this is only a single-mention partitive if the object
			// is an indefinite NP

			// otherwise, create a separate mention with a partitive predicate
			SemLink *of_link = _new SemLink(object, partitive_head,
											Argument::poss(), synNode);
			SemOPP partitive_opp = _new SemOPP(_new SemUnit[] {of_link}, synNode, synNode.getEnglishNPHeadPreterm(),
				PredTypes.PARTITIVE_PREDICATION);
			result = _new SemMention(_new SemUnit[] {partitive_opp}, synNode, is_definite);
*/
			// Start by creating a SemMention for this node
			SemMention *result = _new SemMention(
				analyzeChildren(synNode, mentionSet), synNode, mention, false);

			return result;
		}
		else if (mention->getMentionType() == Mention::APPO) {
			SemMention *result =
				_new SemMention(analyzeChildren(synNode, mentionSet),
								synNode, mention, false);

			return result;
		}
		else if (mention->getMentionType() == Mention::NAME) {

			// locate the node which we want to associate with the name
			const SynNode *nameNode = synNode;
			Mention *child = mention->getChild();
			while (child) {
				nameNode = child->getNode();
				child = child->getChild();
			}

			// JCS - changed to make nameNode the head of the new SemOPP (instead 
			// of a null head) to allow it to act as the predicate of the resulting
			// proposition
			SemOPP *nameOPP = 
				_new SemOPP(0, nameNode, Proposition::NAME_PRED, nameNode);

			if (nameNode == synNode) {
				// name is atomic, so we're done

				return _new SemMention(nameOPP, synNode, mention, true);
			}
			else {
				// this name has other stuff in it, so use that big 
				// name-noun-name heuristic

				SemMention *result =
					_new SemMention(0, synNode, mention, true);					

				// put together list of non-name children
				SemNode *children = analyzeChildren(synNode, mentionSet);

				applyNameNounNameHeuristic(*result, children, mentionSet);

				result->appendChild(nameOPP);

				return result;
			}
		}
		else if ((synNodeTag == ChineseSTags::NP || synNodeTag == ChineseSTags::NPA) &&
				  synNode->getNChildren() == 1 &&
				  synNode->getHead()->getTag() == ChineseSTags::PN &&
				  WordConstants::isPossessivePronoun(synNode->getHeadWord()))
		{
			// if we have a possessive pronoun, make a special link

			SemOPP *pronOPP = _new SemOPP(0, synNode,
										  Proposition::PRONOUN_PRED, synNode);
			SemMention *pronoun = _new SemMention(pronOPP, synNode,
												  mention, true);
			return _new SemLink(pronoun, synNode, Argument::POSS_ROLE, synNode);
		}
		else if (mention->getMentionType() == Mention::PRON) {
			// if we have a regular pronoun, just make a pronoun pred and 
			// SemMention

			SemOPP *pronOPP = _new SemOPP(0, synNode,
										  Proposition::PRONOUN_PRED, synNode);
			SemMention *pronoun = _new SemMention(pronOPP, synNode,
												  mention, true);
			return pronoun;
		}
		else if (isTemporalNP(*synNode)) {
			// make a link for the temporal np

			SemMention *semMention =
				_new SemMention(0, synNode, mention, false);

			return _new SemLink(semMention, synNode, Argument::TEMP_ROLE,
														synNode, true);
		}
		else if (isLocationNP(*synNode)) {
			// make a link for the location np
			SemMention *semMention = 
				_new SemMention(0, synNode, mention, false);

			return _new SemLink(semMention, synNode, Argument::LOC_ROLE,
													    synNode, true);
		}
		else {
			SemMention *result =
				_new SemMention(0, synNode, mention, false);
			applyNameNounNameHeuristic(*result,
				analyzeChildren(synNode, mentionSet), mentionSet);
			return result;
		}
	}
	else if (synNodeTag == ChineseSTags::VP ||
			 synNodeTag == ChineseSTags::ADJP ||
			 synNodeTag == ChineseSTags::DVP ||
			 synNodeTag == ChineseSTags::QP ||
			 synNodeTag == ChineseSTags::JJ ||
			 synNodeTag == ChineseSTags::CD ||
			 synNodeTag == ChineseSTags::OD ||
			 synNodeTag == ChineseSTags::NN ||
			 synNodeTag == ChineseSTags::NR ||
			 synNodeTag == ChineseSTags::NT ||
			 synNodeTag == ChineseSTags::PN)
	{
		// looking at (non-list) predicate -- create SemOPP

		// depending on the constit type, we set the predicate type, head
		// node, and for verbs, negative modifier
		Proposition::PredType predType;
		const SynNode *head;
		const SynNode *negative = 0;

		if (synNodeTag == ChineseSTags::VP) {
			if (isCopula(*synNode->getHead()))
				predType = Proposition::COPULA_PRED;
			else
				predType = Proposition::VERB_PRED;
			head = synNode->getHead();

			// for verb, we try to find a negative adverb as well
			for (int i = 0; i < synNode->getNChildren(); i++) {
				if (isNegativeAdverb(*synNode->getChild(i))) {
					negative = synNode->getChild(i);
					break;
				}
			}
		}
		else if (synNodeTag == ChineseSTags::ADJP || 
			     synNodeTag == ChineseSTags::DVP  ||
				 synNodeTag == ChineseSTags::QP) 
		{
			predType = Proposition::MODIFIER_PRED;
			head = synNode->getHead();
		}
		else if (synNodeTag == ChineseSTags::JJ ||
				 synNodeTag == ChineseSTags::CD ||
				 synNodeTag == ChineseSTags::OD)
		{
			predType = Proposition::MODIFIER_PRED;
			head = synNode;
		}
		else if (synNodeTag == ChineseSTags::PN) {
			// XXX
			return 0;
		}
		else /* only types left are noun types NN, NR, NT */ {
		// should we make only head nouns noun predicates?		
			predType = Proposition::NOUN_PRED;
			head = synNode;
		}

		SemOPP *result = _new SemOPP(analyzeChildren(synNode, mentionSet), 
										synNode, predType, head);
		result->addNegative(negative);
		return result;
	}
	else if (synNodeTag == ChineseSTags::PP  || 
		     synNodeTag == ChineseSTags::LCP || 
			 synNodeTag == ChineseSTags::CP)
	{
		// looking at a PP or LCP or CP -- create SemLink
		
		// If CP has only CP child -> pass up that child
		if (synNodeTag == ChineseSTags::CP &&  
		    synNode->getNChildren() == 1 &&
			synNode->getChild(0)->getTag() == ChineseSTags::CP)
		{
			return analyzeChildren(synNode, mentionSet);
		} 

		const SynNode *head = synNode->getHead(); // XXX will this find VBG head?
		// XXX Also, it won't be null when it's supposed to

		SemNode *children = analyzeChildren(synNode, mentionSet);

		// we also want to see if there's a nested link
		if (children &&
			children->getNext() == 0 &&
			children->getSemNodeType() == SemNode::LINK_TYPE)
		{
			// there's a nested link, so merge the two
			if (children->asLink().getSymbol() == Argument::TEMP_ROLE) {
				// actually, temporals don't get merged -- instead, create
				// a new temporal which contains the whole expression
				
				SemLink *nested = &children->asLink();

				SemLink *result = _new SemLink(
					children, synNode, Argument::TEMP_ROLE, synNode, true);

				return result;
			}
			else {
				// false alarm -- wasn't a temporal -- just go on with the 
				// nested-link merging:
				SemLink &inner = children->asLink();

				inner.setSynNode(synNode);
				
				wchar_t buf[100];
				wcsncpy(buf, inner.getSymbol().to_string(), 50);
				wcscat(buf, L"_");
				wcsncat(buf, head->getHeadWord().to_string(), 49);
				Symbol headSym = Symbol(buf);
				inner.setSymbol(headSym);

				return &inner;
			}
		}
		else {
			return _new SemLink(children, synNode,
								head->getHeadWord(), head);

		}
	}
	else if (synNodeTag == ChineseSTags::ADVP) {
		// XXX
		return analyzeChildren(synNode, mentionSet);
#if 0
		if (isLocativePronoun(synNode)) {
			// "here" or "there" -- not sure how to handle these in Serif yet
			return 0;
		}
		else {
			// for other ADVPs, it's possible that we'll want to create a link

			// first thing we need is an adv head
			SynNode *head = findAmongChildren(synNode,
					{ChineseSTags::RB, ChineseSTags::RBR, Symbol()});

			// second thing we need is a link
			SemLink *link = 0;
			Vector other_children = _new Vector(child_array.length);
			for (int i = 0; i < child_array.length; i++) {
				if (link == null && child_array[i] instanceof SemLink)
					link = (SemLink) child_array[i];
				else
					other_children.addElement(child_array[i]);
			}

			if (link != null && head != null) {
				// if we have both, then make a new Link

				// put together list of all children of both this node and the link child
				child_array = _new SemUnit[/*other_children.size() +*/ link.getNChildren()];
				int j = 0;
				/*        for (int i = 0; i < other_children.size(); i++)
				child_array[j++] = (SemUnit) other_children.elementAt(i); */
				for (int i = 0; i < link.getNChildren(); i++)
					child_array[j++] = link.getChild(i);

				if (isLocativePronoun(head)) {
					// this is a case of "here in Bangkok"
					SemOPP pronoun_opp = _new SemOPP(_new SemUnit[] {link}, head,
						head, PredTypes.PRONOUN_PREDICATION);
					SemMention pronoun_mention = _new SemMention(_new SemUnit[] {pronoun_opp},
						synNode, true);
					result = _new SemLink(_new SemUnit[] {pronoun_mention}, synNode,
						SemLink.LOCATION_LINK, head);
				}
				else {
					// this a case of "out of ..." (double-preposition)
					// _new head symbol is combination of this head and the link's head
					Symbol head_sym = SymbolMap.add(head.getHeadWord().toString() + '_' + link.getHeadSymbol().toString());

					result = _new SemLink(child_array, synNode, SemLink.PREPOSITION_LINK, head, head_sym);
				}
			}
			else {
				result = null;

				// it's also possible that there's a name here which we should keep
				for (int i = 0; i < child_array.length; i++) {
					if (child_array[i] instanceof SemMention)
						result = child_array[i];
				}
			}
		}
#endif
	}
	else if (synNodeTag == ChineseSTags::DNP) {
		// we have a possessive, so make a special link
			
		SemNode *children = analyzeChildren(synNode, mentionSet);

		// first, we want to see if there's a nested link
		if (children &&
			children->getNext() == 0 &&
			children->getSemNodeType() == SemNode::LINK_TYPE)
		{
			SemLink &inner = children->asLink();

			inner.setSynNode(synNode);
			
			wchar_t buf[100];
			wcsncpy(buf, inner.getSymbol().to_string(), 50);
			wcscat(buf, L"_");
			const SynNode *head = synNode->getHead();
			wcsncat(buf, head->getHeadWord().to_string(), 49);
			Symbol headSym = Symbol(buf);
			inner.setSymbol(headSym);

			return &inner;
		}
		else {
			return _new SemLink(children, synNode, Argument::POSS_ROLE, synNode);
		}
	}
	else {
/*		logger.beginWarning();
		logger << "ChineseSemTreeBuilder::buildSemTree(): Could not create SemNode for:\n";
		logger << "    " << synNode->toString(4).c_str() << "\n";
*/
		return analyzeChildren(synNode, mentionSet);
	}
}

// If you add extra return statements, please decrement analyzeChildrenStackDepth before returning.
SemNode *ChineseSemTreeBuilder::analyzeChildren(const SynNode *synNode, const MentionSet *mentionSet) {
    //analyzeChildrenStackDepth++;

    // Check if we have gone too deep
    //if (analyzeChildrenStackDepth > 25) {
    //    throw InternalInconsistencyException(
    //        "ChineseSemTreeBuilder::analyzeChildren()",
    //        "analyzeChildrenStackDepth got too big - terminating now before stack overflow");
    //}
    //if (analyzeChildrenStackDepth > maxAnalyzeChildrenStackDepth) { 
    //    maxAnalyzeChildrenStackDepth = analyzeChildrenStackDepth;
    //}
    if (synNode->isTerminal()) {
    //    analyzeChildrenStackDepth--;
        return 0;
    }

	SemNode *firstResult = 0;
	SemNode *lastResult = 0;

	for (int i = 0; i < synNode->getNChildren(); i++) {
		// append the linked list returned by buildSemTree to our result list
		SemNode *result = buildSemSubtree(synNode->getChild(i), mentionSet);
//		std::cout << "--\n" << *result << "\n";
		if (lastResult) {
			lastResult->setNext(result);
		}
		else {
			firstResult = result;
			lastResult = result;
		}
		if (lastResult) {
			while (lastResult->getNext() != 0)
				lastResult = lastResult->getNext();
		}
	}
    //analyzeChildrenStackDepth--;
	return firstResult;
}

bool ChineseSemTreeBuilder::isList(Symbol memberTag, const SynNode &node) {
	int members_seen = 0;
	int separators_seen = 0;
	for (int i = 0; i < node.getNChildren(); i++) {
		Symbol tag = node.getChild(i)->getTag();
		if (tag == memberTag) {
			members_seen++;
		}
		else if (tag == ChineseSTags::PU || tag == ChineseSTags::CC)
		{
			separators_seen++;
		}
		else {
			// if we encounter other type of node once we've seen first member, it's not a list
			if (members_seen != 0)
				return false;
		}
	}
	if (members_seen > 1)
		return true;
	else
		return false;
}




// apply name/noun/name heuristic for NP's 
// (For stuff like "IBM president Bob Smith" and its sundry variations)
void ChineseSemTreeBuilder::applyNameNounNameHeuristic(SemMention &mentionNode,
												SemNode *children,
												const MentionSet *mentionSet)
{
#if 0
	std::cout << "\n***\n";
	mentionNode.dump(std::cout);
	std::cout << "\n---\n";
	for (SemNode *child = children; child; child = child->getNext()) {
		std::cout << " -";
		child->dump(std::cout, 2);
		std::cout << "\n";
	}
	std::cout << "\n";
#endif
	
	// the bins we'll be putting nodes into:
	int n_premods = 0;
	SemNode *premods[100];
	int n_postmods = 0;
	SemNode *postmods[100];
	SemReference *premodRef = 0;
	SemReference *premodRef2 = 0;
	SemOPP *noun = 0;

	// for each node, assign it to one of the bins
	for (SemNode::SiblingIterator iter(children); iter.more(); ++iter) {
		if (n_premods >= 100 || n_postmods >= 100)
			break; // unlikely, but just in case....

		(*iter).setNext(0); // break node off from child list

		if ((*iter).isReference()) {
			if (premodRef == 0) {
				premodRef = &(*iter).asReference();
			}
			else {
				if (premodRef2) {
					// move premodRef2 into premodRef's slot
					//delete premodRef;
					premodRef = premodRef2;
				}
				premodRef2 = &(*iter).asReference();
			}
		}
		else if ((*iter).getSemNodeType() == SemNode::OPP_TYPE) {
			SemOPP *opp = &(*iter).asOPP();
			if (opp->getPredicateType() == Proposition::NOUN_PRED) {
				//if (noun)
					//delete noun;
				noun = opp;
			}
			else if (opp->getPredicateType() == Proposition::MODIFIER_PRED ||
					 opp->getPredicateType() == Proposition::NAME_PRED)
			{
				// do nothing with modifiers and names
				//delete opp;
			}
			else {
				if (noun == 0)
					premods[n_premods++] = opp;
				else
					postmods[n_postmods++] = opp;
			}
		}
		else {
			if (noun == 0)
				premods[n_premods++] = &*iter;
			else
				postmods[n_postmods++] = &*iter;
		}
	}

	// make an unknown-role link for the premodifier reference
	SemLink *premodLink = 0;
	if (premodRef != 0) {
		premodLink = _new SemLink(premodRef,
								  premodRef->getSynNode(),
								  Argument::UNKNOWN_ROLE,
								  premodRef->getSynNode());
	}
	SemLink *premodLink2 = 0;
	if (premodRef2 != 0) {
		premodLink2 = _new SemLink(premodRef2,
								   premodRef2->getSynNode(),
								   Argument::UNKNOWN_ROLE,
								   premodRef2->getSynNode());
	}

	// now add what we found to new list of children
	for (int i = 0; i < n_premods; i++)
		mentionNode.appendChild(premods[i]);
	if (premodLink)
		mentionNode.appendChild(premodLink);
	if (premodLink2)
		mentionNode.appendChild(premodLink2);
	if (noun)
		mentionNode.appendChild(noun);
	for (int j = 0; j < n_postmods; j++)
		mentionNode.appendChild(postmods[j]);
}
