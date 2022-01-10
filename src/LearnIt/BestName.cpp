#include "Generic/common/leak_detection.h"

#include "BestName.h"

#include "common/ParamReader.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/MentionConfidence.h"

using namespace std;

BestName BestName::calcBestNameForMention(const Mention* mention, 
		const SentenceTheory* st, const DocTheory* dt, bool fullText,
		bool prefer_better_coref_name) 
{
	BestName bestMentionName = _findBestMentionName(mention, st, fullText,
		prefer_better_coref_name);

	if (ParamReader::isParamTrue("no_coref")) {
		return bestMentionName;
	} else {
		BestName bestEntityName = _findBestEntityName(mention, st, dt, 
			prefer_better_coref_name);

		// sometimes we want to try extra-hard to return a name
		if (bestEntityName.mentionType() == Mention::NAME &&
			bestMentionName.mentionType() != Mention::NAME &&
			ParamReader::isParamTrue("force_name_for_bn")) {
			return bestEntityName;
		} else {
			return (bestEntityName.confidence()>=bestMentionName.confidence())
				? bestEntityName
				: bestMentionName;
		}
	}
}

// Get a best name from the mention. Use its head if it has one and full text 
//    is not requested, otherwise use full text.
//	Give it a confidence based on the mention type.
BestName BestName::_findBestMentionName(const Mention* mention,
	const SentenceTheory* st, bool fullText, bool prefer_better_coref_name)
{
	const SynNode* mention_head = mention->getHead();
	Mention::Type mention_type = mention->getMentionType();
	EDTOffset start_offset = st->getTokenSequence()->
		getToken(mention->getNode()->getStartToken())->getStartEDTOffset();
	EDTOffset end_offset = st->getTokenSequence()->
		getToken(mention->getNode()->getEndToken())->getEndEDTOffset();
	double confidence=0.0;

	switch(mention_type) {
		case Mention::NAME: // E.g.: John
			if (mention_head) {
				// E.g.: President John
				start_offset = st->getTokenSequence()->getToken(
					mention_head->getStartToken())->getStartEDTOffset();
				end_offset = st->getTokenSequence()->getToken(
					mention_head->getEndToken())->getEndEDTOffset();
			}
			if (prefer_better_coref_name) {
				confidence = 0.9999;
			} else {
				confidence = 1.0;
			}
			break;
		case Mention::APPO: // E.g.: John, the baker
			if (mention_head && mention_head->hasMention()) {
				const MentionSet* mention_set = st->getMentionSet();
				Mention *m = mention_set->getMention(mention_head->getMentionIndex());
				mention_type = m->getMentionType();
				start_offset = st->getTokenSequence()->getToken(
					mention_head->getStartToken())->getStartEDTOffset();
				end_offset = st->getTokenSequence()->getToken(
					mention_head->getEndToken())->getEndEDTOffset();
				if (m->getMentionType() == Mention::NAME)
					confidence = 1.0;
				else
					confidence = 0.5;
			}
			break;
		case Mention::DESC: // E.g.: the baker
			confidence = 0.4; break;
		case Mention::LIST: // E.g.: (??)
			confidence = 0.2; break;
		case Mention::PRON: // E.g.: he
			confidence = 0.0; break; // A pronoun w/ no referent is not useful!
		case Mention::PART: // E.g.: ??
			confidence = 0.0; break;
		case Mention::INFL: // E.g.: ??
			confidence = 0.0; break;
		default:
			confidence = 0.0; break;
	}

	wstring bestNameString=L"";

	if (mention_head && !fullText) {
		bestNameString = (mention_head->toCasedTextString(st->getTokenSequence()));
	} else {
		bestNameString = mention->getNode()->toCasedTextString(st->getTokenSequence());;
	}

	return BestName(bestNameString, confidence, mention_type, 
		start_offset, end_offset);
}

BestName BestName::_findBestEntityName(const Mention* mention,
	const SentenceTheory* st, const DocTheory* dt, 
	bool prefer_better_coref_name) 
{
	// Get a best name from the entity that this mention is a part of, 
	// and give it a confidence score based on determineMentionConfidence().
	EntitySet* entity_set = dt->getEntitySet();
	const Entity* entity = entity_set->lookUpEntityForMention(mention->getUID());

	if (!entity) {
		return BestName();
	} 
	else {
		std::pair<std::wstring, const Mention*> bestNameAndMention =
			entity->getBestNameWithSourceMention(dt);
		
		std::wstring bestName = bestNameAndMention.first;
		if (bestName == L"NO_NAME") {
			return BestName();
		}

		// Determine mention node to use for offsets (favor head name, to strip e.g. titles); could be in a different sentence
		const SynNode* offset_node = bestNameAndMention.second->getHead();
		if (!offset_node)
			offset_node = bestNameAndMention.second->getNode();
		EDTOffset start_offset = dt->getSentenceTheory(bestNameAndMention.second->getSentenceNumber())->getTokenSequence()->getToken(offset_node->getStartToken())->getStartEDTOffset();
		EDTOffset end_offset = dt->getSentenceTheory(bestNameAndMention.second->getSentenceNumber())->getTokenSequence()->getToken(offset_node->getEndToken())->getEndEDTOffset();

		Mention::Type mention_type = bestNameAndMention.second->getMentionType();
		MentionConfidenceAttribute mention_confidence = mention->brandyStyleConfidence(dt, st, std::set<Symbol>());

		double confidence=0.0;
		if (mention_confidence == MentionConfidenceStatus::ANY_NAME) {  // any name mention
			if (prefer_better_coref_name && 
				mention->getMentionType()==Mention::NAME) 
			{
				confidence = 1.0;
			} else {
				confidence = 0.8; 
			}
		} else if (mention_confidence == MentionConfidenceStatus::ONLY_ONE_CANDIDATE_PRON) { // PER pronoun co-referent with the only PER entity in the document
			confidence = 0.6;
		} else if (mention_confidence ==  MentionConfidenceStatus::ONLY_ONE_CANDIDATE_DESC ||
			mention_confidence ==  MentionConfidenceStatus::PREV_SENT_DOUBLE_SUBJECT_DESC ||
			mention_confidence ==  MentionConfidenceStatus::TITLE_DESC || // e.g. _President_ Barack Obama
			mention_confidence ==  MentionConfidenceStatus::COPULA_DESC || // e.g. Microsoft is _a big software company_
			mention_confidence ==  MentionConfidenceStatus::APPOS_DESC || // e.g. Microsoft, _a big software company_
			mention_confidence ==  MentionConfidenceStatus::WHQ_LINK_PRON || // Microsoft, _which_ is a big software company
			mention_confidence ==  MentionConfidenceStatus::NAME_AND_POSS_PRON) // Bob and _his_ dog
		{
			confidence = 0.5; 
		} else if (mention_confidence == MentionConfidenceStatus::AMBIGUOUS_NAME) {
			confidence = 0.4;
		} else if (mention_confidence == MentionConfidenceStatus::OTHER_DESC) { // any other descriptor
			confidence = 0.2; 
		} else if (mention_confidence == MentionConfidenceStatus::PREV_SENT_DOUBLE_SUBJECT_PRON ||
			mention_confidence == MentionConfidenceStatus::OTHER_PRON) {
				confidence = 0.1;
		} else {
			confidence = 0.0;
		}

		return BestName(bestName, confidence, mention_type, 
			start_offset, end_offset);
	}
}
