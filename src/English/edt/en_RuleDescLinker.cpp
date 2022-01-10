// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/common/en_WordConstants.h"
#include "English/edt/en_RuleDescLinker.h"
#include "English/parse/en_STags.h"
#include "English/parse/en_LanguageSpecificFunctions.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/edt/Guesser.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/DebugStream.h"
#include "Generic/common/InternalInconsistencyException.h"

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

using namespace std;

static Symbol mafia_symbol(L"mafia");
static Symbol crime_symbol(L"crime");
static Symbol mob_symbol(L"mob");
static Symbol of(L"of");
static Symbol symFor(L"for");

EnglishRuleDescLinker::EnglishRuleDescLinker() : _debug(Symbol(L"desc_link_debug"))
{
	_noise = _new HashTable(50);
	_require_close_match = ParamReader::isParamTrue("desc_link_require_close_match");
	if (_require_close_match) {
		std::string buffer = ParamReader::getRequiredParam("desc_link_noise_file");
		_loadFileIntoTable(buffer.c_str(), _noise);
	}
}

EnglishRuleDescLinker::~EnglishRuleDescLinker()
{
	delete _noise;
}



int EnglishRuleDescLinker::linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType linkType, LexEntitySet *results[], int max_results) {
	// TODO: victim hack (which needs to be done in a different manner than we can do here...)

	// only DESC mentions are here - no appos, etc.
	//first, determine the mention corresponding to this UID (Mike)
	Mention *currMention = currSolution->getMention(currMentionUID);
	EntityGuess* guess = 0;

	// Don't link types that are supposed to be using simple coref
	if (currMention->getEntityType().useSimpleCoref())
		guess = _guessNewEntity(currMention, linkType);

	// if premods have "anti-link" words, don't even bother looking for a match
	// NOTE: preventing anything from linking to this node was disabled in the old system
	//       and as such is disabled here too
	// MEMORY: a guess may be created here.
	//         If it is, it is guaranteed to be deleted at the end of this method
	if (guess == 0 && _premodsHaveAntiLinkWords(currMention)) {
		_debug << "Not considering on account of anti link words:\n"
			   << currMention->node->toDebugString(0).c_str()
			   << "\n";
		guess = _guessNewEntity(currMention, linkType);
	}

	// MEMORY: a guess may be created by head match.
	//         If it is, it is guaranteed to be deleted at the end of this method
	if (guess == 0)
		guess = _headMatch(currMention, currSolution, linkType);

	// MEMORY: a guess may be created
	//         If it is, it is guaranteed to be deleted at the end of this method
	if (guess == 0 && !_require_close_match)
		guess = _unlinkedPremodNameMatch(currMention, currSolution, linkType);

	if (guess == 0 && !_require_close_match)
		guess = _subtypeMatch(currMention, currSolution, linkType);

	// TODO: match to gpe antecedents. this is based on the whole 3 sentence thingy
	// nothing found, so create a new entity

	// MEMORY: if a guess was not created before, it is guaranteed to be created now and
	// deleted at the end of this
	if (guess == 0)
		guess = _guessNewEntity(currMention, linkType);
	
	_debug << "Linking:\n" << currMention->node->toDebugString(0).c_str() << "\n";
	_debug << " to:\n**********************\n";
	// only one branch here - desc linking is deterministic
	LexEntitySet* newSet = currSolution->fork();
	if (guess->id == EntityGuess::NEW_ENTITY) {
		newSet->addNew(currMention->getUID(), guess->type);

		_debug << "Itself: entity with id " << guess->id << " and of type " << guess->type.getName().to_debug_string() << "\n";
	}
	else {
		// debug: what are we linking?
		if (_debug.isActive()) {
			int i;
			for (i=0; i < currSolution->getNEntities(); i++) {
				Entity* ent = currSolution->getEntity(i);
				if (ent->getID() == guess->id) {
					GrowableArray<MentionUID> &ments = ent->mentions;
					int j;
					for (j=0; j < ments.length(); j++) {
						Mention *thisMention = currSolution->getMention(ments[j]);
						_debug << thisMention->node->toDebugString(0).c_str() << "\n";
					}
					break;
				}
			}
		}
		newSet->add(currMention->getUID(), guess->id);
	}
	_debug << "**********************\n";

	results[0] = newSet;

	// MEMORY: a guess was definitely created above, and is removed now.
	delete guess;
	return 1;

}

EntityGuess* EnglishRuleDescLinker::_headMatch(Mention* ment, LexEntitySet *ents, EntityType linkType)
{
	// NOTE: the following commented code is a restrictive test that was in the old
	// code but as of the last run was turned off, and replaced with the less restrictive
	// numeric-matching stuff below

//	// we don't want to link anything that's partitive or has a numeric premod
//	if (_getNumericPremod(ment->node) != 0)
//		return 0;
//	if (ment->mentionType == Mention::PART)
//		return 0;

	const SynNode* numericPremod = _getNumericPremod(ment->node);
	const SynNode* numericPostmod = _getNumericPostmod(ment->node);

	// if there are premod names, we'll have to make sure there are no clashes later on
	bool has_premod_name = _hasPremodName(ment, ents);
	Symbol headWord = ment->node->getHeadWord();

	bool is_NAMED_mob_word = false;
	if (has_premod_name &&
		(headWord == mafia_symbol ||
		headWord == crime_symbol ||
		headWord == mob_symbol))
		is_NAMED_mob_word = true;

	// now iterate through all mentions of the proper entity type and valid mention type
	// to try and link this mention to one of them
	// THIS IS AN INNER LOOP, INTENSE CALCULATION AREA
	// backwards through sentences
	int i;
	for (i = ents->getNMentionSets()-1; i >=0; i--) {
		const MentionSet* prevMents = ents->getMentionSet(i);
		// skipping through the current sentence
		// until we are before the mention being considered
		bool skipping = false;
		if (i == ment->getSentenceNumber())
			skipping = true;
		int j;
		bool matchedMention = false;
		// we want most recent mentions first
		for (j=prevMents->getNMentions()-1; j >= 0; j--) {
			Mention* prevMent = prevMents->getMention(j);
			if (skipping) {
				// we're done skipping!
				if (prevMent == ment)
					skipping = false;
				continue;
			}

			if (prevMent->mentionType != Mention::DESC &&
				prevMent->mentionType != Mention::NAME)
				continue;

			Entity* ent = ents->getEntityByMention(prevMent->getUID(), linkType);
			if (ent == 0) // SRS -- this results from pre-linked mentions
				continue; // JCS -- or from mentions with the wrong entity type

			if (_isSpeakerEntity(ent, ents))
				continue;

			_debug << "Considering linking:\n" << ment->node->toDebugString(0).c_str() << "\n";
			_debug << " to:\n";
			_debug << prevMent->node->toDebugString(0).c_str() << "\n\n";

			// NOTE: the following commented code is a restrictive test that was in the old
			// code but as of the last run was turned off, and replaced with the less restrictive
			// numeric-matching stuff below

			//		// don't link to anything that's partitive or has numeric premod
			//		if (_getNumericPremod(prevMent->node) != 0)
			//			continue;
			//		if (prevMent->mentionType == Mention::PART)
			//			continue;

			// Desc-Desc case: we get a few ways to do it
			if (prevMent->mentionType == Mention::DESC) {
				Symbol prevHeadWord = prevMent->node->getHeadWord();
				// TODO: special case (mafia & crime) matching

				bool special_case_match = false;

				// the XXXX mob ~ the XXXX mafia ~ XXXX organized crime
				// (here we just make sure they have name premods;
				//  clashes will get thrown out below)
				if (is_NAMED_mob_word &&
					(prevHeadWord == mafia_symbol ||
					 prevHeadWord == crime_symbol ||
					 prevHeadWord == mob_symbol) &&
					 _hasPremodName(prevMent, ents))
					special_case_match = true;

				if (special_case_match || prevHeadWord == headWord) {


					// TODO: shouldn't this test across every descriptor mention
					// of the proposed ent (i.e. as done in numeric below)?

					// TODO: also, shouldn't this test and the next one be applied
					// even when linking to a name mention? Being that it's a global test?

					// mentions can't have entity name premods that clash
					if (has_premod_name &&
						_hasPremodName(prevMent, ents) &&
						_hasPremodNameEntityClash(ment, prevMent, ents))
					{
						_debug << "REJECTED (desc-desc): premods have clashing entities\n";
						continue;
					}

					// TODO: as has been said in ace, it would be more efficient to ban the entity
					// this came from, rather than testing every mention every time.


					// mentions can't have numeric premods that clash
					// this is the less restrictive form of the "no numerics/no partitives above"
					if (numericPremod != 0 && _hasNumericPremodClash(numericPremod, ent, ents)) {
						_debug << "REJECTED (desc-desc): premods have clashing numerics\n";
						continue;
					}

					if (numericPostmod != 0 && _hasNumericPostmodClash(numericPostmod, ent, ents)) {
						_debug << "REJECTED (desc-desc): postmods have clashing numerics\n";
						continue;
					}

					// close match test
					if (_require_close_match && _closeMatchClash(ment, prevMent)) {
						_debug << "REJECTED (desc-desc): mentions do not closely match\n";
						continue;
					}	

					matchedMention = true;
				}
				else {
					_debug << "REJECTED (desc-desc): headwords " << headWord.to_debug_string()
						<< " and " << prevHeadWord.to_debug_string() << " don't match\n";
				}

			}
			else if (prevMent->mentionType == Mention::NAME) {

				Symbol prevHeadWord = prevMent->node->getHeadWord();
				Symbol finalPremodWord = _getFinalPremodWord(prevMent->node);
				if (headWord == prevHeadWord || headWord == finalPremodWord) {
					if (_require_close_match && _closeMatchClash(ment, prevMent)) {
						_debug << "REJECTED (desc-name): mentions do not closely match\n";
						continue;
					}
					matchedMention = true;
				}
				else {
					_debug << "REJECTED (desc-name): headword " << headWord.to_debug_string()
						<< " doesn't match previous head word: " << prevHeadWord.to_debug_string()
						<< " or previous final premod: " << finalPremodWord.to_debug_string() << "\n";
				}
			}
			else {
				_debug << "REJECTED: Mention is of type "
					<< Mention::getTypeString(prevMent->mentionType)
					<< "; Can only link to DESC or NAME\n";
			}

			if (matchedMention && _hasRelationClash(ment, prevMent, ents)) {
				_debug << "REJECTED: Relation mention clash\n";
				continue;
			}

			if (matchedMention) {

				// create a guess and return it
				// MEMORY: linkMention, the caller of this method, is guaranteed to delete this
				EntityGuess* guess = _new EntityGuess();
				guess->id = ent->ID;
				guess->score = 1;
				guess->type = ent->getType();
				return guess;
			}
		}
	}
	// couldn't find anything
	return 0;

}

EntityGuess* EnglishRuleDescLinker::_subtypeMatch(Mention* ment, LexEntitySet *ents, EntityType linkType)
{
	// if there are premod names, we'll have to make sure there are no clashes later on
	bool has_premod_name = _hasPremodName(ment, ents);

	EntitySubtype subtype = ment->getEntitySubtype();
	int i;
	for (i = ents->getNMentionSets()-1; i >=0; i--) {
		const MentionSet* prevMents = ents->getMentionSet(i);
		// skipping through the current sentence
		// until we are before the mention being considered
		bool skipping = false;
		if (i == ment->getSentenceNumber())
			skipping = true;
		int j;

		for (j=prevMents->getNMentions()-1; j >= 0; j--) {
			Mention* prevMent = prevMents->getMention(j);
			if (skipping) {
				// we're done skipping!
				if (prevMent == ment)
					skipping = false;
				continue;
			}

			if (prevMent->mentionType != Mention::NAME) continue;

			Entity* ent = ents->getEntityByMention(prevMent->getUID(), linkType);
			if (ent == 0) // SRS -- this results from pre-linked mentions
				continue; // JCS -- or from mentions with the wrong entity type

			if (_isSpeakerEntity(ent, ents))
				continue;

			_debug << "Considering linking (subtype match):\n" << ment->node->toDebugString(0).c_str() << "\n";
			_debug << " to:\n";
			_debug << prevMent->node->toDebugString(0).c_str() << "\n\n";

			Symbol mentNumber = Guesser::guessNumber(ment->node, ment);
			Symbol prevMentNumber = Guesser::guessNumber(prevMent->node, prevMent);

			// AGGRESSIVE LINKING:
			// This function was originally designed to link a mention with a known subtype
			// to the nearest entity that is also known to have this subtype.
			//
			// As it is checked in, however, UNDET also functions as a subtype.
			// So, if we have a mention whose subtype we don't know, and there is
			// a previous entity whose subtype we also don't know,
			// it will link those too. This is very aggressive! It makes coref
			// performance WORSE, but it helps relation scores significantly,
			// which is why we did it.
			//
			// IF, however, you want real coref performance scores, you should
			// uncomment the first line of the following if statement.
			//
			if (subtype != EntitySubtype::getUndetType() &&
				subtype == prevMent->getEntitySubtype() &&
				(mentNumber == prevMentNumber ||
				 mentNumber == Guesser::UNKNOWN ||
				 prevMentNumber == Guesser::UNKNOWN))

			{
				if (_hasRelationClash(ment, prevMent, ents)) {
					_debug << "REJECTED: Relation mention clash\n";
				} else {
					EntityGuess* guess = _new EntityGuess();
					guess->id = ent->ID;
					guess->score = 1;
					guess->type = ent->getType();
					return guess;
				}
			} else {
				_debug << "REJECTED (desc-name): subtype " << subtype.getName().to_debug_string()
					<< " doesn't match previous subtype: " << prevMent->getEntitySubtype().getName().to_debug_string() << "\n";
			}
		}

	}

	return 0;
}

// match ment to an entity if there is a singleton name mention in the mention's premod
EntityGuess* EnglishRuleDescLinker::_unlinkedPremodNameMatch(Mention* ment, LexEntitySet* ents, EntityType linkType)
{
	// structure of mention's node must be correct:
	// the mention must have a name mention as premod and it must be a singleton
	const SynNode* descNode = ment->node;

	// if this node starts with "the", we can only have one premod. which means the
	// name must also start with "the"
	Symbol* start = _new Symbol[1];
	descNode->getTerminalSymbols(start, 1);
	if (*start == EnglishWordConstants::THE && descNode->getHeadIndex() > 1) {
		delete [] start;
		return 0;
	}
	delete [] start;
	int i;
	for (i=0; i < descNode->getHeadIndex(); i++) {
		const SynNode* nameNode = descNode->getChild(i);
		// hopeful naming here...let's see if this is in fact a name
		if (!nameNode->hasMention())
			continue;
		Mention* nameMent = ents->getMentionSet(ment->getSentenceNumber())->getMentionByNode(nameNode);
		if (!nameMent->isOfRecognizedType() || nameMent->mentionType != Mention::NAME)
			continue;
		// can only link gpe, org, fac, loc
		if (nameMent->getEntityType().matchesPER())
			continue;
		// TODO: if the descriptor is a FAC or LOC, originally the type of the name could be
		//       coerced. For now, we're not doing that
		if (linkType != nameMent->getEntityType() && linkType != nameMent->getIntendedType())
			continue;

		// the name entity must be a singleton
		Entity* ent = ents->getEntityByMention(nameMent->getUID(), linkType);
		if (ent == 0)
			throw InternalInconsistencyException("EnglishRuleDescLinker::_unlinkedPremodNameMatch()",
												 "couldn't get entity from mention");
		if (ent->mentions.length() > 1)
			continue;

		// MEMORY: linkMention, the caller of this method, is guaranteed to delete this
		EntityGuess* guess = _new EntityGuess();
		guess->id = ent->ID;
		guess->score = 1;
		guess->type = ent->getType();
		return guess;
	}
	// couldn't find a premod singleton name
	return 0;
}


EntityGuess* EnglishRuleDescLinker::_guessNewEntity(Mention *ment, EntityType linkType)
{
	// MEMORY: linkMention, the caller of this method, is guaranteed to delete this
	EntityGuess* guess = _new EntityGuess();
	guess->id = EntityGuess::NEW_ENTITY;
	guess->score = 1;
	guess->type = linkType;
	return guess;
}
Symbol EnglishRuleDescLinker::_getFinalPremodWord(const SynNode* node) const
{
	// TODO: this algorithm is originally coded as follows.
	// however, it would be better to start at the head word's parent,
	// look for premods, if not finding any go up until found, etc.
	if (node->getHeadIndex()-1 >= 0)
		return node->getChild(node->getHeadIndex()-1)->getHeadWord();
	// otherwise just return the head
	return node->getHeadWord();
}

const SynNode* EnglishRuleDescLinker::_getNumericPremod(const SynNode* node) const
{
	// if the node is preterminal, it won't have any numeric premods
	if (node->isPreterminal())
		return 0;
	// check children for numeric premods
	int headIndex = node->getHeadIndex();
	for (int i = 0; i < headIndex; i++) {
		const SynNode* pre = node->getChild(i);
		if (!pre->isTerminal()) {
			// check to see if head word of child is numeric
			const SynNode* preterm = pre->getHeadPreterm();
			Symbol tag = preterm->getTag();
			if (tag == EnglishSTags::CD)
				return preterm;
			// it seems like I should check the premod and postmod branch for this as well,
			// even though this wasn't done in java
			const SynNode* deepPre = _getNumericPremod(pre);
			if (deepPre != 0)
				return deepPre;
			const SynNode* deepPost = _getNumericPostmod(pre);
			if (deepPost != 0)
				return deepPost;
		}
	}
	// check head for numeric premods
	if (!node->getHead()->isPreterminal()) {
		const SynNode* pre = _getNumericPremod(node->getHead());
		if (pre != 0)
			return pre;
	}
	return 0;
}

const SynNode* EnglishRuleDescLinker::_getNumericPostmod(const SynNode* node) const
{
	// if the node is preterminal, it won't have any numeric postmods
	if (node->isPreterminal())
		return 0;
	// check children for numeric postmods
	int headIndex = node->getHeadIndex();
	for (int i = node->getNChildren() - 1; i > headIndex; i--) {
		const SynNode* post = node->getChild(i);
		if (!post->isTerminal()) {
			// check to see if head word of child is numeric
			const SynNode* preterm = post->getHeadPreterm();
			Symbol tag = preterm->getTag();
			if (tag == EnglishSTags::CD)
				return preterm;
			// it seems like I should check the premod and postmod branch for this as well,
			// even though this wasn't done in java
			const SynNode* deepPre = _getNumericPremod(post);
			if (deepPre != 0)
				return deepPre;
			const SynNode* deepPost = _getNumericPostmod(post);
			if (deepPost != 0)
				return deepPost;
		}
	}
	//	check head for numeric postmods
	if (!node->getHead()->isPreterminal()) {
		const SynNode* post = _getNumericPostmod(node->getHead());
		if (post != 0)
			return post;
	}
	return 0;
}

bool EnglishRuleDescLinker::_hasPremodName(Mention* ment, LexEntitySet* ents)
{
	// are there any mentions of entities yet?

	const MentionSet* ms = ents->getMentionSet(ment->getSentenceNumber());

	if (ms == 0)
		return false;

	const SynNode* node = ment->node;
	// first, check head for premod names
	if (node->getHead()->hasMention()) {
		Mention* headMent = ms->getMentionByNode(node->getHead());
		if (_hasPremodName(headMent, ents))
			return true;
	}
	// does ment have premods, and is there at least one name mention?
	int hIndex = node->getHeadIndex();
	if (hIndex < 1)
		return false;
	int i;
	for (i=0; i < hIndex; i++) {
		const SynNode* pre = node->getChild(i);
		if (!pre->hasMention())
			continue;
		Mention* preMen = ms->getMentionByNode(pre);
		if (preMen->mentionType == Mention::NAME) {
			_debug << "premod Name found:\n" << preMen->node->toDebugString(0).c_str() << "\n";
			return true;
		}
	}
	return false;
}

bool EnglishRuleDescLinker::_hasPremodNameEntityClash(Mention* ment1, Mention* ment2, LexEntitySet* ents)
{
	const MentionSet* ms = ents->getMentionSet(ment1->getSentenceNumber());
	const SynNode* n1 = ment1->node;
	int h1 = n1->getHeadIndex();

	for (int i = 0 ; i < h1; i++) {
		const SynNode* pre1 = n1->getChild(i);
		if (!pre1->hasMention())
			continue;
		Mention* pre1ment = ents->getMentionSet(ment1->getSentenceNumber())->getMentionByNode(pre1);

		if (pre1ment->mentionType != Mention::NAME)
			continue;
		_debug << "In clash: ment1:\n" << pre1ment->node->toDebugString(1).c_str() << "\n";
		if (_nameClashesWithPremods(pre1ment, ment2, ents))
			return true;
	}

	// do premods of head node contain a name that clashes?
	if (n1->getHead()->hasMention()) {
		Mention* headMent = ms->getMentionByNode(n1->getHead());
		if (_hasPremodNameEntityClash(headMent, ment2, ents))
			return true;
	}

	// couldn't find a clash
	return false;
}

bool EnglishRuleDescLinker::_hasRelationClash(Mention* ment1, Mention *ment2, LexEntitySet* ents) {

	// This was a failed experiment...

	/*RelMentionSet *rmSet
		= _docTheory->getSentenceTheory(ment1->getSentenceNumber())->getRelMentionSet();

	//std::cerr << ment->getNode()->toDebugTextString() << "\n";
	
	Symbol relation_types[10];
	int relation_ents[10];
	int n_relations = 0;
	for (int i = 0; i < rmSet->getNRelMentions(); i++) {
		RelMention *rm = rmSet->getRelMention(i);
		if ((rm->getType() == Symbol(L"ORG-AFF.Employment") ||
			 rm->getType() == Symbol(L"GEN-AFF.Citizen-Resident-Religion-Ethnicity")) &&
			rm->getLeftMentionID() == ment1->getUID()) 
		{
			Entity *ent = ents->getEntityByMention(rm->getRightMentionID());
			if (ent != 0) {
				relation_types[n_relations] = rm->getType();			
				relation_ents[n_relations] = ent->getID();
				n_relations++;
			}
		} 
		if (n_relations > 10)
			break;
	}

	rmSet = _docTheory->getSentenceTheory(ment2->getSentenceNumber())->getRelMentionSet();	
	for (int i = 0; i < rmSet->getNRelMentions(); i++) {
		RelMention *rm = rmSet->getRelMention(i);
		if (rm->getLeftMentionID() == ment2->getUID()) {
			Entity *otherEntity = ents->getEntityByMention(rm->getRightMentionID());
			for (int j = 0; j < n_relations; j++) {
				if (relation_types[j] == rm->getType() && relation_ents[j] != otherEntity->getID()) {
					std::cerr << "RELATION CLASH:\nMention 1: " << ment1->getNode()->toDebugTextString() << ": ";
					std::cerr << "Mention 2: " << ment2->getNode()->toDebugTextString() << ": ";std::cerr << "Other entity: " << otherEntity->getID() << "\n";
					std::cerr << "Relation type: " << rm->getType() << "\n";
					return true;
				}
			}
		}
			
	}




	// THIS IS HIDEOUSLY LAZY -- comparing mention to an entity
	for (int sentno = 0; sentno <= ment->getSentenceNumber(); sentno++) {
		const RelMentionSet *relMentionSet
			= _docTheory->getSentenceTheory(sentno)->getRelMentionSet();
		for (int i = 0; i < relMentionSet->getNRelMentions(); i++) {
			RelMention *rm = relMentionSet->getRelMention(i);
			Entity *left = ents->getEntityByMention(rm->getLeftMentionID());
			Entity *right = ents->getEntityByMention(rm->getRightMentionID());
			if (left == 0 || right == 0)
				continue;
			Entity *otherEntity = 0;
			if (ent == left)
				otherEntity = right;
			else if (ent == right)
				otherEntity = left;
			else continue;
			for (int j = 0; j < n_relations; j++) {
				if (relation_types[j] == rm->getType() && relation_ents[j] != otherEntity->getID()) {
					std::cerr << "RELATION CLASH:\nMention: " << ment->getNode()->toDebugTextString() << ": ";
					std::cerr << relation_ents[j] << "\n";
					std::cerr << "Entity:\n  ";
					for (int temp = 0; temp < ent->getNMentions(); temp++) {
						std::cerr << ents->getMention(ent->getMention(temp))->getNode()->toDebugTextString() << "\n  ";
					}
					std::cerr << "Other entity: " << otherEntity->getID() << "\n";
					std::cerr << "Relation type: " << rm->getType() << "\n";
					return true;
				}
			}
		}
	}*/

	return false;

}


bool EnglishRuleDescLinker::_nameClashesWithPremods(Mention* name, Mention* ment, LexEntitySet* ents) {
	if (name->mentionType != Mention::NAME)
		return false;

	// make sure name is associated with an entity
	Entity* e1 = ents->getEntityByMention(name->getUID());
	Entity* ie1 = ents->getIntendedEntityByMention(name->getUID());
	if (e1 == 0)
		return false;

	const MentionSet* ms = ents->getMentionSet(ment->getSentenceNumber());
	const SynNode* node = ment->node;
	int headIndex = node->getHeadIndex();

	bool found_match = false;
	bool found_clash = false;

	for (int j = 0; j < headIndex; j++) {
		const SynNode* pre = node->getChild(j);
		if (!pre->hasMention())
			continue;
		Mention* prement = ents->getMentionSet(ment->getSentenceNumber())->getMentionByNode(pre);

		if (prement->mentionType != Mention::NAME)
			continue;
		_debug << "In clash: ment2:\n" << prement->node->toDebugString(1).c_str() << "\n";
		Entity* e2 = ents->getEntityByMention(prement->getUID());
		Entity* ie2 = ents->getIntendedEntityByMention(prement->getUID());
		if (e2 == 0)
			continue;
		// are the entities of these mentions different?
		if ((e1 != e2) && (ie1 != e2) && (e1 != ie2) && (ie1 == 0 || ie1 != ie2)) {
			_debug << "Mentions Clash: ";
			if (ie1 != 0)
				_debug << "literal " << e1->getID() << ", intended " << ie1->getID();
			else
				_debug << e1->getID();
			if (ie2 != 0)
				_debug << " and literal " << e2->getID() << ", intended " << ie2->getID() << "\n";
			else
				_debug << " and " << e2->getID() << "\n";
			
			found_clash = true;
			continue;
		}
		_debug << "Mentions are of the same entity: ";
		if (ie1 != 0)
			_debug << "literal " << e1->getID() << ", intended " << ie1->getID();
		else
			_debug << e1->getID();
		if (ie2 != 0)
			_debug << " and literal " << e2->getID() << ", intended " << ie2->getID() << "\n";
		else
			_debug << " and " << e2->getID() << "\n";
		found_match = true;
		break;
	}

	if (found_clash && !found_match)
		return true;

	// check against premods of head node
	if (node->getHead()->hasMention()) {
		Mention* headMent = ms->getMentionByNode(node->getHead());
		if (_nameClashesWithPremods(name, headMent, ents))
			return true;
	}

	return false;
}

bool EnglishRuleDescLinker::_hasNumericPremodClash(const SynNode* node, Entity* ent, LexEntitySet *ents)
{
	if (ent == 0 || node == 0) {
		return false;
	}
	int i;

	for (i=0; i < ent->mentions.length(); i++) {

		Mention* ment = ents->getMention(ent->mentions[i]);
		const SynNode* mentNode = _getNumericPremod(ment->node);
		if (mentNode != 0) {
			Symbol nodeHead = node->getHeadWord();
			Symbol mentHead = mentNode->getHeadWord();
			_debug << "Comparing for numeric premod clash: " << nodeHead.to_debug_string();
			_debug << " and " << mentHead.to_debug_string() << "\n";
			if (mentNode->getHeadWord() != node->getHeadWord())
				return true;
		}
	}
	return false;
}



bool EnglishRuleDescLinker::_isSpeakerMention(Mention *ment) {
	return (_docTheory->isSpeakerSentence(ment->getSentenceNumber()));
}

bool EnglishRuleDescLinker::_isSpeakerEntity(Entity *ent, LexEntitySet *ents) {
	if (ent == 0)
		return false;
	for (int i=0; i < ent->mentions.length(); i++) {
		Mention* ment = ents->getMention(ent->mentions[i]);
		if (_isSpeakerMention(ment))
			return true;
	}
	return false;
}


bool EnglishRuleDescLinker::_hasNumericPostmodClash(const SynNode* node, Entity* ent, LexEntitySet *ents)
{
	if (ent == 0 || node == 0) {
		return false;
	}
	int i;
	for (i=0; i < ent->mentions.length(); i++) {

		Mention* ment = ents->getMention(ent->mentions[i]);
		const SynNode* mentNode = _getNumericPostmod(ment->node);
		if (mentNode != 0) {
			Symbol nodeHead = node->getHeadWord();
			Symbol mentHead = mentNode->getHeadWord();
			_debug << "Comparing for numeric postmod clash: " << nodeHead.to_debug_string();
			_debug << " and " << mentHead.to_debug_string() << "\n";
			if (mentNode->getHeadWord() != node->getHeadWord())
				return true;
		}
	}
	return false;
}

bool EnglishRuleDescLinker::_premodsHaveAntiLinkWords(Mention* ment)
{
	// first get the token ids we need
	// we want from the beginning of the node to
	// the beginning of the head word
	int start = ment->node->getStartToken();
	int end = ment->node->getHeadPreterm()->getStartToken();
	// no problem if no premods
	if (end <= start)
		return false;
	// build (then delete) an array with exactly the words we want
	// to look at
	Symbol* premodWords = _new Symbol[end-start];
	ment->node->getTerminalSymbols(premodWords, end-start);
	int i;
	bool foundWord = false;
	for (i = 0; i < end-start; i++) {
		if (premodWords[i] == EnglishWordConstants::OTHER ||
			premodWords[i] == EnglishWordConstants::ADDITIONAL ||
			premodWords[i] == EnglishWordConstants::EARLIER ||
			premodWords[i] == EnglishWordConstants::PREVIOUS ||
			premodWords[i] == EnglishWordConstants::FORMER ||
			premodWords[i] == EnglishWordConstants::ANOTHER ||
			premodWords[i] == EnglishWordConstants::MANY ||
			premodWords[i] == EnglishWordConstants::FEW ||
			premodWords[i] == EnglishWordConstants::A ||
			premodWords[i] == EnglishWordConstants::SEVERAL) {
			foundWord = true;
			break;
			}
	}
	delete[] premodWords;
	return foundWord;

}

bool EnglishRuleDescLinker::_closeMatchClash(Mention *ment1, Mention *ment2)
{
	const SynNode *node1 = ment1->getNode();
	const SynNode *node2 = ment2->getNode();

	const SynNode *node1NumericPremod = _getNumericPremod(node1);
	const SynNode *node2NumericPremod = _getNumericPremod(node2);

	if (node1NumericPremod &&
		(node2NumericPremod == 0 || node1NumericPremod->getHeadWord() != node2NumericPremod->getHeadWord()))
	{
		return true;
	}

	//std::cout << "Comparing: " << node1->toDebugTextString() << "\n";
	//std::cout << "       To: " << node2->toDebugTextString() << "\n\n";

	int start1 = node1->getStartToken();
	int head1 = node1->getHeadPreterm()->getStartToken();
	int end1 = node1->getEndToken();

	Symbol* node1Symbols = _new Symbol[end1-start1+1];
	node1->getTerminalSymbols(node1Symbols, end1-start1+1);

	int start2 = node2->getStartToken();
	int head2 = node2->getHeadPreterm()->getStartToken();
	int end2 = node2->getEndToken();

	Symbol* node2Symbols = _new Symbol[end2-start2+1];
	node2->getTerminalSymbols(node2Symbols, end2-start2+1);

	head1 = _advanceHeadForPostmods(node1, node1Symbols, start1, head1, end1);
	head2 = _advanceHeadForPostmods(node2, node2Symbols, start2, head2, end2);
	//std::cout << "node1: " << (head1 - start1 + 1) << " tokens\n";
	//std::cout << "node2: " << (head2 - start2 + 1) << " tokens\n";

	bool clash = false;
	int i, j;
	for (i = 0; i < head1 - start1 + 1; i++) {
		Symbol s1 = node1Symbols[i];

		if (_noise->get(s1)) continue;

		bool matched = false;
		for(j = 0; j < head2 - start2 + 1; j++) {
			Symbol s2 = node2Symbols[j];
			if (s1 == s2 ||
				WordConstants::getNumericPortion(s1) == s2 ||
				s1 == WordConstants::getNumericPortion(s2))
			{
				matched = true;
				break;
			}
		}
		if (!matched) {
			clash = true;
			break;
		}
	}

	delete [] node1Symbols;
	delete [] node2Symbols;

	/*if (clash)
		std::cout << "CLASH\n\n";
	else
		std::cout << "NO CLASH\n\n";*/

	return clash;
}

int EnglishRuleDescLinker::_advanceHeadForPostmods(const SynNode *node, Symbol *symbols,
											int start, int head, int end)
{
	// if the there are no postmods then do nothing
	if (head == end)
		return head;

	// if the next word after head is not "of" or "for" then do nothing
	if (symbols[head - start + 1] != of &&
		symbols[head - start + 1] != symFor)
		return head;

    // get first postmod
	int i;
	for (i = 0; i < node->getNChildren(); i++) {
		const SynNode* child = node->getChild(i);

		// if it's after the head, and it contains the "of" (or "for")
		if (child->getStartToken() > head &&
			child->getStartToken() <= head + 1 &&
			child->getEndToken() >= head + 1)
		{
			// we have first postmod
			return head + child->getNTerminals();

		}
	}
	return head;
}

void EnglishRuleDescLinker::_loadFileIntoTable(const char * filepath, HashTable * table) {
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	// XXX put error checking here -- SRS
	in.open(filepath);
	std::wstring entry;
	while (!in.eof()) {
		in.getLine(entry);
		std::transform(entry.begin(), entry.end(), entry.begin(), towlower);
		(*table)[Symbol(entry.c_str())] = Symbol(L"1");
	}
}

