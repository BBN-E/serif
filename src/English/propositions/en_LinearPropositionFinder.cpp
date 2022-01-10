// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/propositions/en_LinearPropositionFinder.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/DebugStream.h"



static Symbol sym_who = Symbol(L"who");
static Symbol sym_which = Symbol(L"which");
static Symbol sym_COMMA = Symbol(L",");

// known-to-be transitive past participles
static int n_xitive_past_parts;
static Symbol xitivePastParts[1000];

void EnglishLinearPropositionFinder::initializeSymbols() {
	n_xitive_past_parts = 0;
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"killed");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"murdered");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"assassinated");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"slew");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"slayed");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"executed");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"eliminated");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"shot");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"stabbed");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"gunned");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"attacked");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"assaulted");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"knifed");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"bayonetted");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"stabbed");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"hit");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"beat");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"battered");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"mutilated");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"trampled");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"tortured");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"martyred");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"maimed");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"bit");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"poisoned");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"abused");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"injured");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"harmed");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"wounded");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"damaged");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"gouged");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"crippled");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"lamed");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"disabled");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"incapacitated");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"bruised");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"contused");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"crushed");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"lacerated");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"scalded");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"disfigured");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"scarred");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"smashed");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"sliced");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"raped");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"grazed");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"given");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"kidnapped");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"abducted");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"snatched");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"siezed");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"captured");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"accused");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"indicted");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"blamed");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"charged");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"incriminated");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"arrested");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"detained");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"convicted");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"apprehended");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"collared");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"caught");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"pinched");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"nailed");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"nabbed");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"grabbed");
//	xitivePastParts[n_xitive_past_parts++] = Symbol(L"brought");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"conveyed");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"transported");
	xitivePastParts[n_xitive_past_parts++] = Symbol(L"displaced");
}


EnglishLinearPropositionFinder::EnglishLinearPropositionFinder()
	: LinearPropositionFinder(),
	  _debug(Symbol(L"linear_propfinder_debug"))
{
	initializeSymbols();
}

void EnglishLinearPropositionFinder::augmentPropositionTheory(PropositionSet *propSet, const SynNode *root,const MentionSet *mentionSet) {
//	doTransitiveVerbs(propSet, parse, mentionSet);
//	doProximityTemps(propSet, parse, mentionSet);
}

void EnglishLinearPropositionFinder::doTransitiveVerbs(PropositionSet *propSet,
												const SynNode *root,
												const MentionSet *mentionSet)
{
	for (const SynNode *node = root->getFirstTerminal();
		 node != 0; node = node->getNextTerminal())
	{
		Symbol word = node->getTag();

		// see if this token looks like one of our transitive verbs
		bool matches = false;
		for (int i = 0; i < n_xitive_past_parts; i++) {
			if (word == xitivePastParts[i]) {
				matches = true;
				break;
			}
		}

		if (matches) {
			// now see if the verb has mentions on both sides

			// for the left-hand mention, skip over "who" and "which"
			const SynNode *leftEdge = node;
			for (;;) {
				const SynNode *prev = leftEdge->getPrevTerminal();
				if (prev != 0 &&
					(prev->getTag() == sym_who ||
					 prev->getTag() == sym_which))
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


void EnglishLinearPropositionFinder::doProximityTemps(PropositionSet *propSet,
											   const SynNode* root,
											   const MentionSet *mentionSet)
{
	for (const SynNode *node = root->getFirstTerminal();
		 node != 0; node = node->getNextTerminal())
	{
		Symbol word = node->getTag();
	}
}


const SynNode *EnglishLinearPropositionFinder::findPrecedingMentionNode(
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

const SynNode *EnglishLinearPropositionFinder::findFollowingMentionNode(
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

Proposition *EnglishLinearPropositionFinder::findPropositionForPredicate(
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

