// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/edt/en_PreLinker.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/NodeInfo.h"
#include "English/parse/en_STags.h"
#include "Generic/wordnet/xx_WordNet.h"
#include "English/common/en_NationalityRecognizer.h"
#include "English/common/en_WordConstants.h"
#include "Generic/common/ParamReader.h"

namespace { // private symbols
static Symbol of_symbol(L"of");
static Symbol poss_symbol(L"<poss>");
static Symbol govt_symbol(L"government");
static Symbol regime_symbol(L"regime");
static Symbol body_symbol(L"body");
static Symbol bodies_symbol(L"bodies");
static Symbol capital_symbol(L"capital");

static Symbol division_symbol(L"division");
static Symbol unit_symbol(L"unit");
static Symbol party_symbol(L"party");
static Symbol agency_symbol(L"agency");
static Symbol news_symbol(L"news");
static Symbol company_symbol(L"company");
static Symbol organization_symbol(L"organization");
static Symbol conglomerate_symbol(L"conglomerate");
static Symbol empire_symbol(L"empire");

static Symbol firm_symbol(L"firm");
static Symbol gang_symbol(L"gang");
static Symbol group_symbol(L"group");
static Symbol grouping_symbol(L"grouping");
static Symbol bank_symbol(L"bank");
static Symbol cartel_symbol(L"cartel");
static Symbol solntsevo_symbol(L"solntsevo");

static Symbol territory_symbol(L"territory");
static Symbol village_symbol(L"village");
static Symbol province_symbol(L"province");
static Symbol section_symbol(L"section");
static Symbol island_symbol(L"island");  
static Symbol republic_symbol(L"republic");  
static Symbol town_symbol(L"town");
static Symbol neighborhood_symbol(L"neighborhood");
static Symbol region_symbol(L"region");
static Symbol nation_symbol(L"nation");
static Symbol state_symbol(L"state");

static Symbol states_symbol(L"states");
static Symbol nations_symbol(L"nations");
static Symbol countries_symbol(L"countries");
static Symbol republics_symbol(L"republics");

static Symbol source_symbol(L"source");
static Symbol colon_symbol(L":");

static Symbol geocoord_symbol(L"geocoord");
}

void EnglishPreLinker::preLinkSpecialCases(PreLinker::MentionMap &preLinks,
									const MentionSet *mentionSet,
									const PropositionSet *propSet)
{
	if (!LINK_SPECIAL_CASES) return;

	preLinkContextLinks(preLinks, mentionSet);

	for (int i = 0; i < propSet->getNPropositions(); i++) {
		Proposition *prop = propSet->getProposition(i);

		// Relative pronouns
		if (prop->getNArgs() == 1) {

			/*
			// +++ JJO 29 July 2011 +++
			preLinkRelativePersonPronouns(preLinks, prop, mentionSet);
			*/

			preLinkRelativePronouns(preLinks, prop, mentionSet);

		} else if (prop->getNArgs() >= 2) {
			// body/bodies
			preLinkBody(preLinks, prop, mentionSet);

			// ____ of GPE-NAME and GPE-NAME ____
			preLinkGPELocDescsToNames(preLinks, prop, mentionSet);

			// the GPE-NAME government
			preLinkGovernment(preLinks, prop, mentionSet);

			// the NAME agency, etc.
			preLinkOrgNameDescs(preLinks, prop, mentionSet);
				
			// John who is an engineer
			preLinkWHQCopulas(preLinks, prop, mentionSet);

			// For Ace2004
			// the American People
			preLinkNationalityPeople(preLinks, prop, mentionSet);
			
		}
	}
}


const Mention *EnglishPreLinker::getWHQLink(const Mention *currMention, 
									 const MentionSet *mentionSet) 
{
	const Mention* whqLink = 0;
	const SynNode *pronNode = currMention->node;
	const SynNode *parent = pronNode->getParent();
	if (parent == 0)
		return NULL;

	if (pronNode->getTag() == EnglishSTags::WHNP || pronNode->getTag() == EnglishSTags::WHADVP) {
		if (parent->getTag() == EnglishSTags::SBAR) {
			parent = parent->getParent();
			if (parent == 0) return NULL;
			if (parent->getTag() == EnglishSTags::SBAR) {
				parent = parent->getParent();
				if (parent == 0) return NULL;
			}
			if (parent->getTag() == EnglishSTags::NP) {
				whqLink = mentionSet->getMentionByNode(parent);
			} else return NULL;
		} else return NULL;
	}

	if (pronNode->getTag() == EnglishSTags::WPDOLLAR) {
		if (parent->getTag() == EnglishSTags::WHNP) {
			parent = parent->getParent();
			if (parent == 0) return NULL;
			if (parent->getTag() == EnglishSTags::SBAR) {
				parent = parent->getParent();
				if (parent == 0) return NULL;
				if (parent->getTag() == EnglishSTags::SBAR) {
					parent = parent->getParent();
					if (parent == 0) return NULL;
				}
				if (parent->getTag() == EnglishSTags::NP) {
					whqLink = mentionSet->getMentionByNode(parent);
				} else return NULL;
			} else return NULL;
		} else return NULL;
	}

	if (whqLink != NULL) {

		// Restrict the linking of relative pronouns that normally link to
		//   ACE types. This may need to be loosened if we start adding
		//   new types that are subtypes of our ACE types.
		// This is important because the parse can sometimes be wrong, e.g.:
		//   "people with diabetes who love sugar", sometimes the parse
		//   will have "who" linked to "diabetes"...
		Symbol headword = currMention->getNode()->getHeadWord();			
		if (!whqLink->getEntityType().matchesPER()) {
			if (headword == EnglishWordConstants::WHO || headword == EnglishWordConstants::WHOM || headword == EnglishWordConstants::WHOSE)
				return NULL;
		}
		if (!whqLink->getEntityType().matchesGPE() && !whqLink->getEntityType().matchesFAC() && !whqLink->getEntityType().matchesGPE()) {
			if (headword == EnglishWordConstants::WHERE)
				return NULL;
		}
	}

	return whqLink;
}

/*
// +++ JJO 29 July 2011 +++
// Relative pronouns
void EnglishPreLinker::preLinkRelativePersonPronouns(PreLinker::MentionMap &preLinks,
									   const Proposition *prop,
									   const MentionSet *mentionSet)
{
	if (prop->getPredType() != Proposition::PRONOUN_PRED) return;

	Argument *arg = prop->getArg(0);
	if (arg->getType() != Argument::MENTION_ARG) return;
	if (arg->getRoleSym() != Argument::REF_ROLE) return;

	const Mention *mention = arg->getMention(mentionSet);
	if (!isRelativePronoun(mention->getHead()->getHeadWord())) return;

	// Find the largest parent node that is a mention of recognized type
	// Make sure the pronoun is in a relative clause
	Mention *parentCand = 0;
	Mention *parentMention = 0;
	bool exittedClause = false;
	const SynNode *parent = mention->getNode()->getParent();
	while (parent != 0) {

		if (parent->getTag() == Symbol(L"SBAR"))
			exittedClause = true;

		else if (parent->hasMention()) {
			parentCand = mentionSet->getMention(parent->getMentionIndex());
			if (parentCand->getEntityType().isRecognized() &&
				parentCand->getEntityType() == EntityType::getPERType())
			{
				parentMention = parentCand;
				break;
			}
		}
		parent = parent->getParent();
	}

	if (!exittedClause || parentMention == 0) return;

	preLinks[mention->getIndex()] = parentMention;
	Mention *ment = mentionSet->getMention(arg->getMentionIndex());
	ment->setRoleType(parentMention->getRoleType());
}

bool EnglishPreLinker::isRelativePersonPronoun(const Symbol sym) {
	return (sym == Symbol(L"who"));
}
// --- JJO 29 July 2011
*/

void EnglishPreLinker::preLinkRelativePronouns(PreLinker::MentionMap &preLinks,
									   const Proposition *prop,
									   const MentionSet *mentionSet)
{
	// This only runs for entity types where we are using "simple coref",
	//   i.e. untrained coref.
	// For the other types, this happens either via rule in the PMPronounLinker,
	//   or via model in the DTPronounLinker

	if (prop->getPredType() != Proposition::PRONOUN_PRED || prop->getNArgs() == 0) 
		return;

	Argument *arg = prop->getArg(0);
	if (arg->getType() != Argument::MENTION_ARG || arg->getRoleSym() != Argument::REF_ROLE) 
		return;

	Mention *mention = mentionSet->getMention(arg->getMentionIndex()); // can't be const	

	const Mention *whqLink = getWHQLink(mention, mentionSet);

	if (whqLink != NULL && whqLink->getEntityType().useSimpleCoref()) {
		preLinks[mention->getIndex()] = whqLink;
		mention->setRoleType(whqLink->getRoleType());
	}
}

void EnglishPreLinker::preLinkNationalityPeople(PreLinker::MentionMap &preLinks, 
										 const Proposition *prop, 
								    	 const MentionSet *mentionSet) 

{

	if (prop->getPredType() != Proposition::NOUN_PRED) return;
	if (prop->getNArgs() < 2) return;

	Argument *arg1 = prop->getArg(0);
	Argument *arg2 = prop->getArg(1);

	if (arg1->getType() != Argument::MENTION_ARG ||
		arg2->getType() != Argument::MENTION_ARG) 
		return;

	if (arg1->getRoleSym() != Argument::REF_ROLE ||
		arg2->getRoleSym() != Argument::UNKNOWN_ROLE)
		return;

	const Mention *refMention = arg1->getMention(mentionSet);
	const Mention *unkMention = arg2->getMention(mentionSet);

	if (refMention->getHead()->getHeadWord() != Symbol(L"people"))
		return;

	if (!EnglishNationalityRecognizer::isNationalityWord(unkMention->getHead()->getHeadWord()))
		return;

	if (refMention->getNode()->getHeadIndex() > 3) return;

	if (refMention->getNode()->getHeadIndex() == 3 &&
		refMention->getNode()->getChild(0)->getHeadWord() != EnglishWordConstants::THE)
		return;
	
	preLinks[refMention->getIndex()] = unkMention;
	Mention *ment = mentionSet->getMention(arg1->getMentionIndex());
	ment->setRoleType(EntityType::getPERType());
}

bool EnglishPreLinker::theIsOnlyPremod(const SynNode *node) {
	if (node->getHeadIndex() > 0) {
		for (int i = 0; i < node->getHeadIndex(); i++) {
			const SynNode *child = node->getChild(i);
			if (child->getHeadWord() != EnglishWordConstants::THE)
				return false;
		}
		return true;
	}
	return false;
}

void EnglishPreLinker::preLinkWHQCopulas(PreLinker::MentionMap &preLinks, 
								  const Proposition *prop,
								  const MentionSet *mentionSet) 
{
	if (prop->getPredType() != Proposition::COPULA_PRED) return;
	if (prop->getNegation() != 0) return;

	if (prop->getNArgs() < 2 ||
		prop->getArg(0)->getType() != Argument::MENTION_ARG ||
		prop->getArg(1)->getType() != Argument::MENTION_ARG)
		return;

	const Mention *lhs = prop->getArg(0)->getMention(mentionSet);
	const Mention *rhs = prop->getArg(1)->getMention(mentionSet);

	//std::cout << "found copula " << lhs->node->toDebugString(0) << " and " << rhs->node->toDebugString(0) << "\n";
	const Mention *ment = getWHQLink(lhs, mentionSet);
	if (ment == NULL) return;
	//std::cout << "linking to " << ment->node->toDebugString(0) << "\n";
	if (ment->isOfRecognizedType() && rhs->isOfRecognizedType()) {
		if (rhs->getMentionType() == Mention::LIST) {
			Mention *child = rhs->getChild();
			while (child != 0) {
				preLinks[child->getIndex()] = ment;
				child = child->getNext();
			}
		} else {
			preLinks[rhs->getIndex()] = ment;
		}
	}
}

void EnglishPreLinker::preLinkTitleAppositives(PreLinker::MentionMap &preLinks,
									    const MentionSet *mentionSet) 
{
	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		const Mention *mention = mentionSet->getMention(i);
		getTitle(mentionSet, mention, &preLinks);
	}
		
}

bool EnglishPreLinker::getTitle(const MentionSet *mentionSet, const Mention *mention, 
						 PreLinker::MentionMap *preLinks) {
	if (mention->getMentionType() != Mention::NAME &&
	    mention->getMentionType() != Mention::NEST)
		return false;

	const SynNode *node = mention->node;
	const SynNode *NPPnode = mention->node->getHeadPreterm()->getParent();
	if (node == NPPnode)
		return false;
	node = NPPnode->getParent();
	if (node == 0)
		return false;

	// node must have at least two children
	int nkids = node->getNChildren();
	if (nkids < 2)
		return false;

	// no commas or CCs allowed
	bool continue_flag = false;
	for (int j = 0; j < node->getNChildren(); j++) {
		const SynNode *child = node->getChild(j);
		if (child->getTag() == EnglishSTags::CC || child->getTag() == EnglishSTags::COMMA) {
			continue_flag = true;
			break;
		}
	}
	if (continue_flag) return false;
	//rightmost child (that is a mention) must be a name mention
	//MRF: Modify so that NP-final PPs are ignored 
	int childNum = nkids - 1; 
	const SynNode *child = 0;
	while(childNum > 0){
		child = node->getChild(childNum);
		if(child->getTag() == STags::getPP())
			childNum--;
		else
			break;
	}
	if(child == 0 || childNum == 0)
		return false;
	if (!child->hasMention())
		return false;
	const Mention *rightmost_ment = mentionSet->getMentionByNode(child);
	if (rightmost_ment->getMentionType() == Mention::NAME) {
		// keep it
	}
	else if (rightmost_ment->getMentionType() == Mention::NONE) {
		// make sure this is the name child
		if(rightmost_ment->getParent() == 0){
			return false;
		}
		else if(rightmost_ment->getParent()->getParent() == mention){
			//heaviliy nested, but keep it	
		}
		else if (rightmost_ment->getParent() != mention){
			return false;
		} 
	}
	else {
		// who knows what kind of mention *this* is
		return false;
	}

	if (!rightmost_ment->isOfRecognizedType())
		return false;

	//MRF: look one back from selected child must be a nominal premod descriptor 
	child = node->getChild(childNum-1);
	if (!child->hasMention() || !NodeInfo::isNominalPremod(child))
		return false;

	Mention *next_ment = mentionSet->getMentionByNode(child);
	if (next_ment->getMentionType() != Mention::DESC || isNonTitleWord(next_ment->getNode()->getHeadWord()))
		return false;

	if (preLinks != 0) {
	
		// types should be the same, so we coerce the descriptor
		if (rightmost_ment->getEntityType() != next_ment->getEntityType()) {
			// this looks evil because it *is* evil
			next_ment->setEntityType(rightmost_ment->getEntityType());
			next_ment->setEntitySubtype(rightmost_ment->getEntitySubtype());
		}

		// any other necessary tests? Do I care what's to the left of this?

		// install link (if rightmost is a NONE, use its parent (mention))
		if (rightmost_ment->getMentionType() == Mention::NONE)
			(*preLinks)[next_ment->getIndex()] = mention;
		else
			(*preLinks)[next_ment->getIndex()] = rightmost_ment;
	}

	return true;
}

bool EnglishPreLinker::isNonTitleWord(Symbol word) {
	return word == EnglishWordConstants::AKA2;
}

void EnglishPreLinker::preLinkBody(PreLinker::MentionMap &preLinks,
							const Proposition *prop,
							const MentionSet *mentionSet)
{
	if (prop->getPredType() != Proposition::NOUN_PRED)
		return;
	if (prop->getPredSymbol() != body_symbol &&
		prop->getPredSymbol() != bodies_symbol)
		return;

	if (prop->getArg(0)->getType() != Argument::MENTION_ARG)
		return;
	const Mention *ref = prop->getArg(0)->getMention(mentionSet);
	if (!ref->getEntityType().matchesPER())
		return;

	const Mention *poss = 0;
	for (int i = 1; i < prop->getNArgs(); i++) {
		Argument *arg = prop->getArg(i);

		if (arg->getRoleSym() == Argument::POSS_ROLE ||
			arg->getRoleSym() == of_symbol)
		{
			if (arg->getType() == Argument::MENTION_ARG)
				poss = arg->getMention(mentionSet);
			break;
		}
	}
	if (poss == 0)
		return;

	// pre-link the "body" to the possessive expression
	preLinks[ref->getIndex()] = poss;
}

void EnglishPreLinker::preLinkGPELocDescsToNames(PreLinker::MentionMap &preLinks,
										  const Proposition *prop,
										  const MentionSet *mentionSet)
{
	if (prop->getPredType() != Proposition::NOUN_PRED)
		return;

	if (prop->getArg(0)->getType() != Argument::MENTION_ARG)
		return;

	const Mention *ref = prop->getArg(0)->getMention(mentionSet);
	
	if (ref->getMentionType() != Mention::DESC) return;

	if (prop->getNArgs() < 2) return;

	// EMB: I removed the test that allowed it to get through without being
	// a GPE/LOC if it was a hyponym of "region" -- because we don't change
	// mention types here, so it's bad if an OTH mention gets linked in with a
	// GPE/LOC!

    if (!ref->getEntityType().matchesGPE() && 
		!ref->getEntityType().matchesLOC())
		return;

	// the ____ of GPE/LOC
	if (prop->getPredSymbol() != capital_symbol &&
		ref->getNode()->getHeadPreterm()->getTag() == EnglishSTags::NN)	
	{
		const Mention *name = 0;
		for (int i = 1; i < prop->getNArgs(); i++) {
			const Argument *arg = prop->getArg(i);

			if (arg->getType() != Argument::MENTION_ARG)
				continue;

			const Mention *mention = arg->getMention(mentionSet);
			if ((mention->getEntityType().matchesGPE() ||
				 mention->getEntityType().matchesLOC()) &&
				(mention->getMentionType() == Mention::NAME ||
				 mention->getMentionType() == Mention::NEST) &&
				mention->getChild() == 0)
			{
				if (arg->getRoleSym() == of_symbol) {
					preLinks[ref->getIndex()] = mention;
					return;
				}
			}
		}
	}

	// the GPE republic/etc. -- but only if "the ____ of GPE/LOC" didn't fire!

	if (prop->getPredSymbol() == province_symbol ||
		prop->getPredSymbol() == republic_symbol ||
		prop->getPredSymbol() == island_symbol ||
		prop->getPredSymbol() == territory_symbol ||
		prop->getPredSymbol() == town_symbol ||
		prop->getPredSymbol() == village_symbol ||
		prop->getPredSymbol() == neighborhood_symbol ||
		prop->getPredSymbol() == region_symbol ||
		prop->getPredSymbol() == section_symbol ||
		prop->getPredSymbol() == nation_symbol ||
		prop->getPredSymbol() == state_symbol)
	{

		for (int i = 1; i < prop->getNArgs(); i++) {
			Argument *arg = prop->getArg(i);
			if (arg->getType() != Argument::MENTION_ARG)
				continue;

			const Mention *mention = arg->getMention(mentionSet);

			if ((ref->getEntityType().matchesGPE() ||
				ref->getEntityType().matchesLOC()) &&
				(mention->getMentionType() == Mention::NAME ||
				mention->getMentionType() == Mention::NEST) &&
				arg->getRoleSym() == Argument::UNKNOWN_ROLE)
			{
				Symbol headWord = mention->getNode()->getHeadWord();
				if (prop->getPredSymbol() == territory_symbol ||
					(prop->getPredSymbol() == state_symbol && EnglishNationalityRecognizer::isNationalityWord(headWord)) ||
					(prop->getPredSymbol() == nation_symbol && EnglishNationalityRecognizer::isNationalityWord(headWord)) ||
					!EnglishNationalityRecognizer::isNationalityWord(headWord))
				{
					preLinks[ref->getIndex()] = mention;
					return;
				}
			}
		}
	}

	// the GPE states/nations/countries where GPE is a region word, like "Baltic"

	if (prop->getPredSymbol() == states_symbol ||
		prop->getPredSymbol() == nations_symbol ||
		prop->getPredSymbol() == countries_symbol ||
		prop->getPredSymbol() == republics_symbol)
	{
		for (int i = 1; i < prop->getNArgs(); i++) {
			Argument *arg = prop->getArg(i);
			if (arg->getType() != Argument::MENTION_ARG)
				continue;

			const Mention *mention = arg->getMention(mentionSet);

			if ((ref->getEntityType().matchesGPE() ||
				ref->getEntityType().matchesLOC()) &&
				(mention->getMentionType() == Mention::NAME ||
				mention->getMentionType() == Mention::NEST) &&
				arg->getRoleSym() == Argument::UNKNOWN_ROLE)
			{
				Symbol headWord = mention->getNode()->getHeadWord();
				if (EnglishNationalityRecognizer::isRegionWord(headWord)) {
					preLinks[ref->getIndex()] = mention;
					return;
				}
			}
		}
	}

	return;
}

void EnglishPreLinker::preLinkOrgNameDescs(PreLinker::MentionMap &preLinks,
								  const Proposition *prop,
								  const MentionSet *mentionSet)
{
	// the ORG_NAME ORG_DESC (e.g. the Itar-Tass news agency)

	static int itea = -1;
	if (itea == -1) {
		if (ParamReader::isParamTrue("use_itea_linking"))
			itea = 1;
		else itea = 0;
	}

	if (prop->getPredType() != Proposition::NOUN_PRED)
		return;

	if (prop->getArg(0)->getType() != Argument::MENTION_ARG)
		return;
	const Mention *ref = prop->getArg(0)->getMention(mentionSet);

	if (prop->getPredSymbol() == group_symbol ||
		prop->getPredSymbol() == grouping_symbol ||
		prop->getPredSymbol() == gang_symbol) 
	{
		for (int i = 1; i < prop->getNArgs(); i++) {
			Argument *arg = prop->getArg(i);
			if (arg->getType() != Argument::MENTION_ARG)
				continue;

			const Mention *mention = arg->getMention(mentionSet);
			if (mention->getNode()->getHeadWord() == solntsevo_symbol &&
				(mention->getMentionType() == Mention::NAME ||
				 mention->getMentionType() == Mention::NEST) &&
				arg->getRoleSym() == Argument::UNKNOWN_ROLE)
			{
				if (!mention->getEntityType().matchesORG() &&
					ref->getEntityType().matchesORG())
					preLinks[mention->getIndex()] = ref;
				else preLinks[ref->getIndex()] = mention;
				return;
			}
		}
	}
		
	if (!ref->getEntityType().matchesORG() ||
		ref->getMentionType() != Mention::DESC)
	{
		return;
	}

	// gang or group?
	if (prop->getPredSymbol() != division_symbol &&
		prop->getPredSymbol() != unit_symbol &&
		prop->getPredSymbol() != party_symbol &&
		prop->getPredSymbol() != agency_symbol &&
		prop->getPredSymbol() != company_symbol &&
		prop->getPredSymbol() != organization_symbol &&
		prop->getPredSymbol() != conglomerate_symbol &&
		prop->getPredSymbol() != empire_symbol &&
		prop->getPredSymbol() != firm_symbol &&
		prop->getPredSymbol() != bank_symbol &&
		prop->getPredSymbol() != cartel_symbol)
	{
		return;
	}

	if (itea == 1 &&
		(prop->getPredSymbol() == division_symbol  ||
		prop->getPredSymbol() == unit_symbol))
		return;

	for (int i = 1; i < prop->getNArgs(); i++) {
        Argument *arg = prop->getArg(i);
		if (arg->getType() != Argument::MENTION_ARG)
			continue;

		const Mention *mention = arg->getMention(mentionSet);

		if (mention->getEntityType().matchesORG() &&
			(mention->getMentionType() == Mention::NAME ||
			 mention->getMentionType() == Mention::NEST) &&
			arg->getRoleSym() == Argument::UNKNOWN_ROLE)
		{
			preLinks[ref->getIndex()] = mention;
			return;
		}
	}
}

void EnglishPreLinker::preLinkGovernment(PreLinker::MentionMap &preLinks,
								  const Proposition *prop,
								  const MentionSet *mentionSet)
{
	// the government/regime of GPE
	// the GPE's government/regime
	// the GPE government/regime

	if (prop->getPredType() != Proposition::NOUN_PRED)
		return;

	if (prop->getArg(0)->getType() != Argument::MENTION_ARG)
		return;
	const Mention *ref = prop->getArg(0)->getMention(mentionSet);

	if (!ref->getEntityType().matchesGPE())
	{
		return;
	}

	if (prop->getPredSymbol() != govt_symbol &&
		prop->getPredSymbol() != regime_symbol)
	{
		return;
	}

	for (int i = 1; i < prop->getNArgs(); i++) {
        Argument *arg = prop->getArg(i);
		if (arg->getType() != Argument::MENTION_ARG)
			continue;

		const Mention *mention = arg->getMention(mentionSet);

		if (mention->getEntityType().matchesGPE() &&
			(arg->getRoleSym() == Argument::UNKNOWN_ROLE ||
			 arg->getRoleSym() == of_symbol ||
			 arg->getRoleSym() == poss_symbol))
		{
			preLinks[ref->getIndex()] = mention;
			return;
		}
	}
}

void EnglishPreLinker::preLinkContextLinks(PreLinker::MentionMap &preLinks,
								const MentionSet *mentionSet)
{
	static int init = -1;
	if (init == 0)
		return;
	if (init == -1) {
		if (ParamReader::isParamTrue("use_itea_linking")) {
			init = 1;
		} else {
			init = 0;
			return;
		}
	}

	int nmentions = mentionSet->getNMentions();
	for (int i = 0; i < nmentions; i++) {
		Mention *mention = mentionSet->getMention(i);
		if (mention->getEntityType().isRecognized()) {
			const SynNode *node = mention->getNode();
			while (!node->isPreterminal()) {
				if (preLinkContextLinksGivenNode(preLinks, mentionSet, mention, node))
					break;
				node = node->getHead();
			}
		}
	}
}

bool EnglishPreLinker::preLinkContextLinksGivenNode(PreLinker::MentionMap &preLinks,
											 const MentionSet *mentionSet,
											 Mention *mention,
											 const SynNode *node)

{
	if (node->getHeadWord() == source_symbol &&
		node->getChild(0) == node->getHead()) 
	{
		if (node->getNChildren() >= 3 &&
			node->getChild(1)->getHeadWord() == colon_symbol &&
			node->getChild(2)->hasMention())
		{
			preLinks[node->getChild(2)->getMentionIndex()] = mention;
		} else if (node->getNChildren() >= 2 &&
			node->getChild(0)->getLastTerminal()->getTag() == colon_symbol &&
			node->getChild(1)->hasMention()) 
		{
			preLinks[node->getChild(1)->getMentionIndex()] = mention;
		}
	}

	const SynNode *prev = node->getPrevTerminal();
	const SynNode *next = node->getNextTerminal();

	if (prev != 0 && next != 0 &&
		((prev->getTag() == EnglishWordConstants::LEFT_BRACKET &&
		  next->getTag() == EnglishWordConstants::RIGHT_BRACKET) ||
	     (prev->getTag() == EnglishWordConstants::LEFT_CURLY_BRACKET &&
		  next->getTag() == EnglishWordConstants::RIGHT_CURLY_BRACKET)))
	{
		Mention *mention2 = getMentionFromTerminal(mentionSet, prev->getPrevTerminal());
		if (mention2 != 0 && mention2->getEntityType().isRecognized()) 
		{
			// sets type to that of name, or of full mention if both are descs
			// if both are names, just resets type to that of full mention
			if (mention->getMentionType() == Mention::NAME &&
				mention2->getMentionType() == Mention::DESC) 
				preLinks[mention2->getIndex()] = mention;
			else if (mention2->getMentionType() == Mention::NAME &&
					 mention->getMentionType() == Mention::DESC)
				preLinks[mention->getIndex()] = mention2;
			else if (mention2->getMentionType() == Mention::DESC &&
					 mention->getMentionType() == Mention::DESC)
				preLinks[mention->getIndex()] = mention2;	

			return true;
		}
	}

	// no type changing for AKA
	while (next != 0 && WordConstants::isPunctuation(next->getTag()))
		next = next->getNextTerminal();
	if (next != 0 && 
		(next->getTag() == EnglishWordConstants::IS || next->getTag() == EnglishWordConstants::WAS))
		next = next->getNextTerminal();
	if (next != 0 && 
		(next->getTag() == EnglishWordConstants::AKA1 ||
		next->getTag() == EnglishWordConstants::AKA2 ||
		next->getTag() == EnglishWordConstants::AKA3 ||
		next->getTag() == EnglishWordConstants::AKA4))
	{
		next = next->getNextTerminal();
		while (next != 0 && WordConstants::isPunctuation(next->getTag()))
			next = next->getNextTerminal();	
		Mention *mention2 = getMentionFromTerminal(mentionSet, next);
		if (mention2) {
			if (mention->getEntityType() == mention2->getEntityType() ||
				(mention->getEntityType() == EntityType::getORGType() && mention2->getEntityType() == EntityType::getFACType()) ||
				(mention->getEntityType() == EntityType::getFACType() && mention2->getEntityType() == EntityType::getORGType()) ||
				mention2->getEntityType().getName() == Symbol(L"POG"))
			{
				if (mention->getMentionType() == Mention::NAME &&
					mention2->getMentionType() == Mention::DESC) 
					preLinks[mention2->getIndex()] = mention;
				else if (mention2->getMentionType() == Mention::NAME &&
					mention->getMentionType() == Mention::DESC)
					preLinks[mention->getIndex()] = mention2;
				else if (mention2->getMentionType() == Mention::DESC &&
					mention->getMentionType() == Mention::DESC)
					preLinks[mention2->getIndex()] = mention;	

				return true;
			}
		}
	}

	// geocoord linking
	if (mention->getEntityType() == EntityType::getLOCType() &&	
	    (mention->getMentionType() == Mention::NAME ||
	     mention->getMentionType() == Mention::NEST) && 
	    isGeocoord(mention)) 
	{
		Mention *lowestMention = mention;
		while (lowestMention->getChild() != 0)
			lowestMention = lowestMention->getChild();

		prev = lowestMention->getNode()->getPrevTerminal();	
		while (prev != 0 && WordConstants::isPunctuation(prev->getTag()))
			prev = prev->getPrevTerminal();
		Mention *mention2 = getAppropriateGeocoordLink(mentionSet, prev, mention);
		if (mention2) {
			mention->setEntityType(mention2->getEntityType());
			mention->setEntitySubtype(mention2->getEntitySubtype());
			preLinks[mention->getIndex()] = mention2;
			return true;
		}
	}

	return false;
}

bool EnglishPreLinker::isGeocoord(const Mention *mention) {
	
	// test terminal symbols
	const SynNode *node = mention->getNode();
	Symbol symArray[MAX_SENTENCE_TOKENS];
	int n_tokens = node->getTerminalSymbols(symArray, MAX_SENTENCE_TOKENS);

	for (int i = 0; i < n_tokens; i++) {
		Symbol sym = symArray[i];
		
		std::wstring wordString(sym.to_string());
		if (wordString.find(geocoord_symbol.to_string()) != std::wstring::npos)
			return true;
	}

	return false;
}

Mention *EnglishPreLinker::getAppropriateGeocoordLink(const MentionSet *mentionSet, const SynNode *terminalNode, Mention *mention) {
	if (terminalNode == 0)
		return 0;

	int token_number = terminalNode->getStartToken();

	// get longest LOC, FAC or GPE mention that contains terminalNode
	Mention *longestMention = 0;
	int longestMentionLength = 0;
	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		Mention *candidateMention = mentionSet->getMention(i);
		if (candidateMention == mention)
			continue;

		if (candidateMention->getMentionType() != Mention::NAME &&
			candidateMention->getMentionType() != Mention::NEST &&
			candidateMention->getMentionType() != Mention::DESC)
		{
			continue;
		}

		if (candidateMention->getNode()->getStartToken() > token_number ||
			candidateMention->getNode()->getEndToken() < token_number) 
		{
			continue;
		}

		if (candidateMention->getEntityType() == EntityType::getLOCType() || 
			candidateMention->getEntityType() == EntityType::getGPEType() || 
			candidateMention->getEntityType() == EntityType::getFACType())
		{
			const SynNode *candidateMentionNode = candidateMention->getNode();
			int candidateMentionLength = candidateMentionNode->getEndToken() - candidateMentionNode->getStartToken() + 1;
			if (candidateMentionLength > longestMentionLength) {
				longestMention = candidateMention;
				longestMentionLength = candidateMentionLength;
			}
		}
	}

	return longestMention;
}

Mention *EnglishPreLinker::getMentionFromTerminal(const MentionSet *mentionSet,
										   const SynNode *terminal)
{
	if (terminal == 0 || terminal->getParent() == 0 || 
		terminal->getParent()->getParent() == 0)
		return 0;

	const SynNode *parent = terminal->getParent()->getParent();
	if (parent->hasMention()) {
		Mention *mention = mentionSet->getMention(parent->getMentionIndex());
		while (mention != 0 &&
			mention->getMentionType() != Mention::NAME &&
			mention->getMentionType() != Mention::NEST &&
			mention->getMentionType() != Mention::DESC) 
		{
			mention = mention->getParent();
		}
		return mention;
	}

	return 0;
}
