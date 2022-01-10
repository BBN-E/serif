// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/common/en_WordConstants.h"
#include "English/edt/en_DescLinker.h"
#include "English/parse/en_STags.h"
#include "English/parse/en_LanguageSpecificFunctions.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/edt/EntityGuess.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/DebugStream.h"
#include "Generic/common/InternalInconsistencyException.h"

using namespace std;

static Symbol mafia_symbol(L"mafia");
static Symbol crime_symbol(L"crime");
static Symbol mob_symbol(L"mob");


EnglishDescLinker::EnglishDescLinker() : _debug(Symbol(L"desc_link_debug")) { }
int EnglishDescLinker::linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType linkType, LexEntitySet *results[], int max_results) {
	// TODO: victim hack (which needs to be done in a different manner than we can do here...)

	// only DESC mentions are here - no appos, etc.
	//first, determine the mention corresponding to this UID (Mike)
	Mention *currMention = currSolution->getMention(currMentionUID);
	EntityGuess* guess = 0;
	// if premods have "anti-link" words, don't even bother looking for a match
	// NOTE: preventing anything from linking to this node was disabled in the old system
	//       and as such is disabled here too
	// MEMORY: a guess may be created here. 
	//         If it is, it is guaranteed to be deleted at the end of this method
	if (_premodsHaveAntiLinkWords(currMention)) {
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
	if (guess == 0)
		guess = _unlinkedPremodNameMatch(currMention, currSolution, linkType);

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

EntityGuess* EnglishDescLinker::_headMatch(Mention* ment, LexEntitySet *ents, EntityType linkType)
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
						_hasPremodNameEntityClash(ment, prevMent, ents)) {
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
// match ment to an entity if there is a singleton name mention in the mention's premod
EntityGuess* EnglishDescLinker::_unlinkedPremodNameMatch(Mention* ment, LexEntitySet* ents, EntityType linkType)
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
			throw InternalInconsistencyException("EnglishDescLinker::_unlinkedPremodNameMatch()",
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


EntityGuess* EnglishDescLinker::_guessNewEntity(Mention *ment, EntityType linkType)
{
	// MEMORY: linkMention, the caller of this method, is guaranteed to delete this
	EntityGuess* guess = _new EntityGuess();
	guess->id = EntityGuess::NEW_ENTITY;
	guess->score = 1;
	guess->type = linkType;
	return guess;
}
Symbol EnglishDescLinker::_getFinalPremodWord(const SynNode* node) const
{
	// TODO: this algorithm is originally coded as follows.
	// however, it would be better to start at the head word's parent,
	// look for premods, if not finding any go up until found, etc.
	if (node->getHeadIndex()-1 >= 0)
		return node->getChild(node->getHeadIndex()-1)->getHeadWord();
	// otherwise just return the head
	return node->getHeadWord();
}

const SynNode* EnglishDescLinker::_getNumericPremod(const SynNode* node) const
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

const SynNode* EnglishDescLinker::_getNumericPostmod(const SynNode* node) const
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

bool EnglishDescLinker::_hasPremodName(Mention* ment, LexEntitySet* ents)
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

bool EnglishDescLinker::_hasPremodNameEntityClash(Mention* ment1, Mention* ment2, LexEntitySet* ents) 
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

bool EnglishDescLinker::_nameClashesWithPremods(Mention* name, Mention* ment, LexEntitySet* ents) {
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
			return true;
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
	}

	// check against premods of head node
	if (node->getHead()->hasMention()) {
		Mention* headMent = ms->getMentionByNode(node->getHead());
		if (_nameClashesWithPremods(name, headMent, ents))
			return true;
	}

	return false;
}

bool EnglishDescLinker::_hasNumericPremodClash(const SynNode* node, Entity* ent, LexEntitySet *ents) 
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

bool EnglishDescLinker::_hasNumericPostmodClash(const SynNode* node, Entity* ent, LexEntitySet *ents) 
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

bool EnglishDescLinker::_premodsHaveAntiLinkWords(Mention* ment)
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




