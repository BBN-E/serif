// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
#include "Generic/common/leak_detection.h"

#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/RelationSet.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/RelMentionSet.h"
#include "English/docRelationsEvents/en_StructuralRelationFinder.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/DebugStream.h"
#include "English/common/en_WordConstants.h"
#include "English/relations/en_RelationUtilities.h"

EnglishStructuralRelationFinder::EnglishStructuralRelationFinder()
{
	_debugStream.init(Symbol(L"docrelfinder_debug"));

	_find_itea_document_relations = false;
	if (ParamReader::isParamTrue("find_itea_document_relations")) {
		_find_itea_document_relations = true;
	}

}

RelMentionSet * EnglishStructuralRelationFinder::findRelations(DocTheory* docTheory) {
	if (!_find_itea_document_relations)
		return _new RelMentionSet();

	_n_relations = 0;

	int i;
	for (i = 0; i < docTheory->getNSentences(); i++) {
		SentenceTheory *st = docTheory->getSentenceTheory(i);
		const Mention* arg2 = findCandidateArg2Mention(st);
		if (!arg2) continue;

		createStructureBasedRelations(arg2, docTheory, i + 1);
	}

	// add relations in _relMentionList to docTheory's document RelMentionSet
	RelMentionSet* set = _new RelMentionSet();
	for (int j = 0; j < _n_relations; j++) {
		set->takeRelMention(_relMentionList[j]);
		_relMentionList[j] = 0;
	}
	return set;
}

void EnglishStructuralRelationFinder::createStructureBasedRelations(const Mention *arg2, DocTheory *docTheory, int start_sentence)
{
	// skip over any sentences that do not have any alphanumeric characters
	while (start_sentence < docTheory->getNSentences() &&
		   sentenceIsTrivial(docTheory->getSentenceTheory(start_sentence)))
	{
		start_sentence++;
	}

	if (start_sentence >= docTheory->getNSentences() - 1) return;

	// figure out which type of list, if any, starts at sentence i, or i+1
	if (sentenceStartsAlphaNumericList(docTheory->getSentenceTheory(start_sentence)))
		createAlphaNumericListRelations(arg2, docTheory, start_sentence);
	else if (sentenceStartsWithDash(docTheory->getSentenceTheory(start_sentence)))
		createDashListRelations(arg2, docTheory, start_sentence);
	else if (sentenceStartsAlphaNumericList(docTheory->getSentenceTheory(start_sentence + 1)))
		createAlphaNumericListRelations(arg2, docTheory, start_sentence + 1);
	else if (sentenceStartsWithDash(docTheory->getSentenceTheory(start_sentence + 1)))
		createDashListRelations(arg2, docTheory, start_sentence + 1);
}

// This procedure may be replaced with a rules system
const Mention* EnglishStructuralRelationFinder::findCandidateArg2Mention(SentenceTheory* st)
{
	const Mention *result = findOrgConsisting(st);
	if (result) {
		_debugStream << L"\nfound candidate mention (OrgConsisting) " << result->getNode()->toFlatString() << L"\n";
		return result;
	}

	result = findOrgIsMadeOf(st);
	if (result) {
		_debugStream << L"\nfound candidate mention (OrgIsMadeOf) " << result->getNode()->toFlatString() << L"\n";
		return result;
	}

    result = findForOrgFollows(st);
	if (result) {
		_debugStream << L"\nfound candidate mention (ForOrgFollows) " << result->getNode()->toFlatString() << L"\n";
		return result;
	}

	result = findFollowingOrgPerson(st);
	if (result) {
		_debugStream << "\nfound candidate mention (FollowingOrgPerson) " << result->getNode()->toFlatString() << L"\n";
		return result;
	}

	return NULL;
}

const Mention* EnglishStructuralRelationFinder::findFollowingOrgPerson(SentenceTheory *st)
{
	PropositionSet *ps = st->getPropositionSet();
	MentionSet *ms = st->getMentionSet();

	// following ORG PER_DESCs
	int i;
	for (i = 0; i < ps->getNPropositions(); i++) {
		Proposition *p = ps->getProposition(i);
		if (p->getPredType() != Proposition::NOUN_PRED) continue;

		const Mention *candidate = p->getMentionOfRole(Argument::UNKNOWN_ROLE, ms);
		const Mention *possPerDesc = p->getMentionOfRole(Argument::REF_ROLE, ms);

		if (!candidate ||
			candidate->getMentionType() != Mention::NAME ||
			candidate->getEntityType() != EntityType::getORGType())
		{ continue; }


		if (!possPerDesc ||
			possPerDesc->getMentionType() != Mention::DESC ||
			possPerDesc->getEntityType() != EntityType::getPERType())
		{ continue; }

		// we have an ORG PER_DESC, make sure there's a proposition that modifies
		// the PER_DESC with the word "following"
		int j;
		for (j = 0; j < ps->getNPropositions(); j++) {
			Proposition *p = ps->getProposition(j);
			if (p->getPredType() != Proposition::MODIFIER_PRED) continue;
			if (p->getPredSymbol() != EnglishWordConstants::FOLLOWING) continue;
			if (p->getMentionOfRole(Argument::REF_ROLE, ms) != possPerDesc) continue;

			return candidate;
		}
	}

	return NULL;

}

const Mention* EnglishStructuralRelationFinder::findForOrgFollows(SentenceTheory *st)
{
	PropositionSet *ps = st->getPropositionSet();
	MentionSet *ms = st->getMentionSet();

	const Mention *result = NULL;
	int last_token = 0;

	int i;
	for (i = 0; i < ps->getNPropositions(); i++) {
		Proposition *p = ps->getProposition(i);
		const Mention *candidate = p->getMentionOfRole(EnglishWordConstants::FOR, ms);
		if (!candidate) continue;

		if (candidate->getEntityType().matchesORG() ||
			candidate->getEntityType().matchesFAC())
		{
			int last = candidate->getNode()->getEndToken();
			if (last >= last_token) {
				result = candidate;
				last_token = last;
			}
		}
	}

	if (!result) return NULL;

	// now we have an ORG consisting, make sure there is a "follows" word after it
	const SynNode *node = result->getNode();
	while ((node = node->getNextTerminal()) != NULL) {
		if (RelationUtilities::get()->stemPredicate(node->getSingleWord(), Proposition::NOUN_PRED) == EnglishWordConstants::FOLLOW ||
			RelationUtilities::get()->stemPredicate(node->getSingleWord(), Proposition::VERB_PRED) == EnglishWordConstants::FOLLOW)
			return result;
	}

	return NULL;
}

const Mention* EnglishStructuralRelationFinder::findOrgConsisting(SentenceTheory* st)
{
	PropositionSet *ps = st->getPropositionSet();
	MentionSet *ms = st->getMentionSet();
	const Mention *result = NULL;

	// "ORG consists ... following/follows"
	int i;
	for (i = 0; i < ps->getNPropositions(); i++) {
		Proposition *p = ps->getProposition(i);
		if (p->getPredType() != Proposition::VERB_PRED) continue;
		Symbol stemmedVerb = RelationUtilities::get()->stemPredicate(p->getPredSymbol(), Proposition::VERB_PRED);
		if (stemmedVerb != EnglishWordConstants::CONSIST &&
			stemmedVerb != EnglishWordConstants::CONTAIN &&
			stemmedVerb != EnglishWordConstants::APPEAR) continue;

		// now we have something consisting, make sure it's an ORG
		const Mention* candidate = p->getMentionOfRole(Argument::SUB_ROLE, ms);
		if (!candidate) continue;
		if (candidate->getEntityType().matchesORG() || candidate->getEntityType().matchesFAC()) {
			result = candidate;
			break;
		}
	}

	if (!result) return NULL;

	// now we have an ORG consisting, make sure there is a "follows" word after it
	const SynNode *node = result->getNode();
	while ((node = node->getNextTerminal()) != NULL) {
		if (RelationUtilities::get()->stemPredicate(node->getSingleWord(), Proposition::NOUN_PRED) == EnglishWordConstants::FOLLOW ||
			RelationUtilities::get()->stemPredicate(node->getSingleWord(), Proposition::VERB_PRED) == EnglishWordConstants::FOLLOW)
			return result;
	}

	// there was no "follows" word after the ORG mention
	return NULL;
}

const Mention* EnglishStructuralRelationFinder::findOrgIsMadeOf(SentenceTheory* st)
{
	PropositionSet *ps = st->getPropositionSet();
	MentionSet *ms = st->getMentionSet();
	const Mention *result = NULL;

	// "ORG is comprised ... following/follows"
	int i;
	for (i = 0; i < ps->getNPropositions(); i++) {
		Proposition *p = ps->getProposition(i);
		if (p->getPredType() != Proposition::VERB_PRED) continue;
		Symbol stemmedVerb = RelationUtilities::get()->stemPredicate(p->getPredSymbol(), Proposition::VERB_PRED);
		if (stemmedVerb != EnglishWordConstants::MAKE &&
			stemmedVerb != EnglishWordConstants::COMPRISE &&
			stemmedVerb != EnglishWordConstants::ORGANIZE) continue;

		// now we have something consisting, make sure it's an ORG
		const Mention* candidate = p->getMentionOfRole(Argument::OBJ_ROLE, ms);
		if (!candidate) continue;
		if (candidate->getEntityType().matchesORG() || candidate->getEntityType().matchesFAC()) {
			result = candidate;
			break;
		}
	}

	if (!result) return NULL;

	// now we have an OrgIsMadeOf, make sure there is a "follows" word after it
	const SynNode *node = result->getNode();
	while ((node = node->getNextTerminal()) != NULL) {
		if (RelationUtilities::get()->stemPredicate(node->getSingleWord(), Proposition::NOUN_PRED) == EnglishWordConstants::FOLLOW ||
			RelationUtilities::get()->stemPredicate(node->getSingleWord(), Proposition::VERB_PRED) == EnglishWordConstants::FOLLOW)
			return result;
	}

	// there was no "follows" word after the ORG mention
	return NULL;
}

bool EnglishStructuralRelationFinder::sentenceIsTrivial(SentenceTheory* st)
{
	TokenSequence *ts = st->getTokenSequence();
	int i;
	for (i = 0; i < ts->getNTokens(); i++) {
		const Token *token = ts->getToken(i);
		if (!WordConstants::isPunctuation(token->getSymbol()))
			return false;
	}
	return true;
}


bool EnglishStructuralRelationFinder::sentenceStartsWithDash(SentenceTheory* st)
{
	TokenSequence *ts = st->getTokenSequence();
	if (ts->getNTokens() < 1) return false;

	return WordConstants::startsWithDash(ts->getToken(0)->getSymbol());
}


bool EnglishStructuralRelationFinder::sentenceStartsAlphaNumericList(SentenceTheory* st)
{

	// sentence must be "A ." or "1 ."
	TokenSequence *ts = st->getTokenSequence();
	if (ts->getNTokens() != 2) return false;

	Symbol first = ts->getToken(0)->getSymbol();
	Symbol second = ts->getToken(1)->getSymbol();

	return ((first == EnglishWordConstants::A || first == EnglishWordConstants::UPPER_A || first == EnglishWordConstants::ONE) &&
		second == EnglishWordConstants::PERIOD);

}



void EnglishStructuralRelationFinder::createDashListRelations(const Mention *arg2, DocTheory *docTheory, int start_sentence)
{
	_debugStream << L"Found Dash List\n";

	EntitySet *es = docTheory->getEntitySet();

	int sentences_since_last = 0;
	int i;
	for (i = start_sentence; i < docTheory->getNSentences(); i++) {
		SentenceTheory *st = docTheory->getSentenceTheory(i);
		TokenSequence *ts = st->getTokenSequence();

		if (sentenceIsTrivial(st)) continue;
		const Mention *ment = NULL;
		if (sentenceStartsWithDash(st)) {
			ment = grabMentionFromSentence(st);
			while (ment && ment->getMentionType() == Mention::LIST)
				ment = ment->getChild();

			sentences_since_last = 0;
		} else
			sentences_since_last++;

		if (sentences_since_last > 8 ||
			isListItemElement(st) ||
		    containsListStopper(st))
			return;

		if (ment) {
			Symbol relationType = getRelationType(ment, arg2);
			if (relationType.is_null()) continue;

			if (_n_relations < MAX_DOCUMENT_RELATIONS) {
				_relMentionList[_n_relations] = _new RelMention(ment, arg2, relationType, i, _n_relations, 1);
				_debugStream << _relMentionList[_n_relations]->toString() << "\n";
				_n_relations++;

			}
		}
	}
}



void EnglishStructuralRelationFinder::createAlphaNumericListRelations(const Mention *arg2, DocTheory *docTheory, int start_sentence)
{
	_debugStream << L"Found Alphanumeric List\n";

	// indicates whether last sentence is "A.", "B.", "1.", etc.
	bool item_flag = false;
	bool first_item = true;

	EntitySet *es = docTheory->getEntitySet();

	int i;
	for (i = start_sentence; i < docTheory->getNSentences(); i++) {
		SentenceTheory *st = docTheory->getSentenceTheory(i);
		TokenSequence *ts = st->getTokenSequence();

		if (!first_item && sentenceStartsAlphaNumericList(st))
			return;

		if (item_flag == true) {

			first_item = false;
			item_flag = false;

			const Mention *ment = grabMentionFromSentence(st);
			while (ment && ment->getMentionType() == Mention::LIST)
				ment = ment->getChild();

			if (ment) {
				Symbol relationType = getRelationType(ment, arg2);
				if (relationType.is_null()) continue;

				if (_n_relations < MAX_DOCUMENT_RELATIONS) {
					_relMentionList[_n_relations] = _new RelMention(ment, arg2, relationType, i, _n_relations, 1);
					_debugStream << _relMentionList[_n_relations]->toString() << "\n";
					_n_relations++;
				}
			}
		}

		if (isListItemElement(st))
			item_flag = true;
	}
}

Symbol EnglishStructuralRelationFinder::getRelationType(const Mention *arg1, const Mention *arg2)
{
	if ((arg1->getEntityType().matchesORG() || arg1->getEntityType().matchesFAC()) &&
		(arg2->getEntityType().matchesORG() || arg2->getEntityType().matchesFAC()))
	{
		return Symbol(L"PART-WHOLE.Subsidiary");
	}

	if (arg1->getEntityType().matchesPER() && arg2->getEntityType().matchesPER()) 
	    return Symbol(L"PER-SOC.Business");

	if (arg1->getEntityType().matchesPER() &&
		(arg2->getEntityType().matchesORG() || arg2->getEntityType().matchesFAC())) 
	{
		return Symbol(L"ORG-AFF.Employment");
	}

	return Symbol();
}

bool EnglishStructuralRelationFinder::isListItemElement(SentenceTheory *st)
{
	TokenSequence *ts = st->getTokenSequence();
	if (ts->getNTokens() != 2) return false;

	Symbol first = ts->getToken(0)->getSymbol();
	Symbol second = ts->getToken(1)->getSymbol();

	if (second != EnglishWordConstants::PERIOD) return false;

	return
		(WordConstants::isNumeric(first) ||
		 (WordConstants::isAlphabetic(first) && WordConstants::isSingleCharacter(first)));
}

const Mention* EnglishStructuralRelationFinder::grabMentionFromSentence(SentenceTheory *st)
{
	TokenSequence *ts = st->getTokenSequence();
	MentionSet *ms = st->getMentionSet();

	// skip over any whitespace
	int i = 0;
	while (i < ts->getNTokens()) {
		Symbol word = ts->getToken(i)->getSymbol();
		if (!WordConstants::isPunctuation(word))
			break;
		i++;
	}
	if (i >= ts->getNTokens())
		return NULL;

	int j;
	for (j = 0; j < ms->getNMentions(); j++) {
		Mention *ment = ms->getMention(j);
		const SynNode *node = ment->getNode();

		if (node->getStartToken() <= i && node->getEndToken() >= i)
			return ment;
	}

	return NULL;

}


const Proposition* EnglishStructuralRelationFinder::findPropositionArg(Proposition *prop,
															  Symbol roleSym,
															  Proposition::PredType predType,
															  Symbol stemmedWord) const
{
	int i;
	for (i = 0; i < prop->getNArgs(); i++) {
		Argument *arg = prop->getArg(i);
		if (arg->getRoleSym() == roleSym &&
			arg->getType() == Argument::PROPOSITION_ARG)
		{
			const Proposition *innerProp = arg->getProposition();
			if (innerProp->getPredType() == predType &&
				RelationUtilities::get()->stemPredicate(innerProp->getPredSymbol(), predType) == stemmedWord)
				return innerProp;
		}
	}
	return 0;

}

bool EnglishStructuralRelationFinder::containsListStopper(SentenceTheory *st)
{
	TokenSequence *ts = st->getTokenSequence();

	int i;
	for (i = 0; i < ts->getNTokens(); i++)
	{
		Symbol word = ts->getToken(i)->getSymbol();
		if (RelationUtilities::get()->stemPredicate(word, Proposition::NOUN_PRED) == EnglishWordConstants::FOLLOW ||
			RelationUtilities::get()->stemPredicate(word, Proposition::VERB_PRED) == EnglishWordConstants::FOLLOW)
			return true;
	}
	return false;
}
