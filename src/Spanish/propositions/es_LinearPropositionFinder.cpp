// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/propositions/es_LinearPropositionFinder.h"
#include "Spanish/propositions/es_SemTreeUtils.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/DebugStream.h"
#include "Generic/common/WordConstants.h"


static Symbol sym_COMMA = Symbol(L",");


void SpanishLinearPropositionFinder::initializeSymbols() {
	// use a conjugator here to process infinitives for other forms; right now just did past parts for list by hand
}


SpanishLinearPropositionFinder::SpanishLinearPropositionFinder()
	: LinearPropositionFinder(),
	  _debug(Symbol(L"linear_propfinder_debug"))
{
	initializeSymbols();
}

void SpanishLinearPropositionFinder::augmentPropositionTheory(PropositionSet *propSet, const SynNode *root,const MentionSet *mentionSet) {
	///// need to set up the parse before trying these
//	doTransitiveVerbs(propSet, parse, mentionSet);
//	doProximityTemps(propSet, parse, mentionSet);
}

void SpanishLinearPropositionFinder::doTransitiveVerbs(PropositionSet *propSet,
												const SynNode *root,
												const MentionSet *mentionSet)
{
	for (const SynNode *node = root->getFirstTerminal();
		 node != 0; node = node->getNextTerminal())
	{
		Symbol word = node->getTag();

		// see if this token looks like one of our transitive verbs
		bool matches = SpanishSemTreeUtils::isKnownTransitivePastParticiple(word);

		if (matches) {
			// now see if the verb has mentions on both sides

			// for the left-hand mention, skip over "who" and "which" type pronouns
			const SynNode *leftEdge = node;
			for (;;) {
				const SynNode *prev = leftEdge->getPrevTerminal();
				if (prev != 0 && WordConstants::isRelativePronoun(prev->getTag()))
				{
					leftEdge = prev;
				}
				else {
					break;
				}
			}
			const SynNode *lhs = findPrecedingMentionNode(leftEdge, mentionSet);
			if (lhs == 0)
				continue;

			const SynNode *rhs = findFollowingMentionNode(node, mentionSet);
			if (rhs == 0)
				continue;

			_debug << "In: \n";
			_debug << root->toString(0) << "\n";
			_debug << "Found predicate head '"
				   << node->getTag().to_string() << "' "
				   << "with mentions " << lhs->getMentionIndex()
				   << ", " << rhs->getMentionIndex() << ".\n";

			Proposition *prop = findPropositionForPredicate(node->getParent(),
															propSet);
			if (prop != 0) {
				_debug << "Proposition found headed by this predicate: \n";
				_debug << prop->toString() << "\n";
			}
			_debug << "__________________________________________________\n";
			_debug << "\n";
		}
	}
}


void SpanishLinearPropositionFinder::doProximityTemps(PropositionSet *propSet,
											   const SynNode* root,
											   const MentionSet *mentionSet)
{
	for (const SynNode *node = root->getFirstTerminal();
		 node != 0; node = node->getNextTerminal())
	{
		Symbol word = node->getTag();
	}
}


const SynNode *SpanishLinearPropositionFinder::findPrecedingMentionNode(
	const SynNode *node, const MentionSet *mentionSet)
{
	int last_token = node->getStartToken() - 1;

	// we can also match another possible last-token if the difference
	// is covered by a mere comma
	int alt_last_token = last_token;
	if (node->getPrevTerminal() != 0 &&
		node->getPrevTerminal()->getTag() == sym_COMMA)
	{
		alt_last_token--;
	}

	const SynNode *result = 0;

	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		const Mention *mention = mentionSet->getMention(i);
		if (mention->getMentionType() != Mention::NONE &&
			(mention->getNode()->getEndToken() == last_token ||
			 mention->getNode()->getEndToken() == alt_last_token) &&
			(result == 0 ||
			 mention->getNode()->getStartToken() < result->getStartToken()))
		{
			result = mention->getNode();
		}
	}
	
	return result;
}

const SynNode *SpanishLinearPropositionFinder::findFollowingMentionNode(
	const SynNode *node, const MentionSet *mentionSet)
{
	const SynNode *result = 0;

	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		const Mention *mention = mentionSet->getMention(i);
		if (mention->getMentionType() != Mention::NONE &&
			mention->getNode()->getStartToken() == node->getEndToken() + 1 &&
			(result == 0 ||
			 mention->getNode()->getEndToken() > result->getEndToken()))
		{
			result = mention->getNode();
		}
	}
	
	return result;
}

Proposition *SpanishLinearPropositionFinder::findPropositionForPredicate(
	const SynNode *node, PropositionSet *propSet)
{
	Proposition *result = 0;

	for (int i = 0; i < propSet->getNPropositions(); i++) {
		Proposition *prop = propSet->getProposition(i);
		if (prop->getPredHead() == node) {
			result = prop;
			break;
		}
	}

	return result;
}

