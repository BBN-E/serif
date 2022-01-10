// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Arabic/parse/ar_STags.h"
#include "Arabic/edt/ar_RuleDescLinker.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/edt/EntityGuess.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/DebugStream.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SessionLogger.h"




ArabicRuleDescLinker::ArabicRuleDescLinker() : _debug(Symbol(L"desc_link_debug")) { }

int ArabicRuleDescLinker::linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, 
							 EntityType linkType, LexEntitySet *results[], int max_results) 
{
	// only DESC mentions are here - no appos, etc.
	//first, determine the mention corresponding to this UID (Mike)
	Mention *currMention = currSolution->getMention(currMentionUID);
	EntityGuess* guess = 0;

	_debug << "Linking:\n" << currMention->node->toString(0) << "\n";
	_debug << "In sentence: " << currMention->getSentenceNumber() << "\n";
	_debug << " to:\n**********************\n";
	
	// MEMORY: a guess may be created  
	//         If it is, it is guaranteed to be deleted at the end of this method
	if (guess == 0)
		guess = _headNPMatch(currMention, currSolution, linkType);
	
	// MEMORY: if a guess was not created before, it is guaranteed to be created now and
	// deleted at the end of this
	if (guess == 0)
		guess = _guessNewEntity(currMention, linkType);

	// only one branch here - desc linking is deterministic
	LexEntitySet* newSet = currSolution->fork();
	if (guess->id == EntityGuess::NEW_ENTITY) {
		newSet->addNew(currMention->getUID(), guess->type);
		_debug << "Itself: entity with id " << guess->id << " and of type " << guess->type.getName().to_string() << "\n";
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
						_debug << thisMention->node->toString(0) << "\n";
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
EntityGuess* ArabicRuleDescLinker::_headNPMatch(Mention* ment, LexEntitySet *ents, EntityType linkType)
{
	const int MAX_HEAD_WORDS = 5;
	Symbol headNPWords[MAX_HEAD_WORDS];
	Symbol prevHeadNPWords[MAX_HEAD_WORDS];

	// if there is a numeric premod we'll look for clashes later
	const SynNode* numeric = _getNumericMod(ment->node);
	if (numeric != 0) 
		_debug << "Numeric Premod: " << numeric->toString(0) << "\n";
	
	// if there are premod names, we'll look for clashes later
	bool has_premod_name = _hasModName(ment, ents);
	if (has_premod_name)
		_debug << "Premod name(s) found.\n";

	//Arabic Determiners are attached to words, so all modifiers will be non determiners
	//bool has_non_DP_premod = _hasNonDPPremod(ment->node);
	bool has_non_DP_premod = false;

	int n_head_words = _getHeadNPWords(ment->node, headNPWords, MAX_HEAD_WORDS);

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

			_debug << "Considering linking:\n" << ment->node->toString(0) << "\n";
			_debug << " to:\n";
			_debug << prevMent->node->toString(0) << "\n\n";

			// evaluate possible match 
			int n_prev_head_words = _getHeadNPWords(prevMent->node, prevHeadNPWords, MAX_HEAD_WORDS);
			bool match = false;
			// do all of the words in the Core NP match?
			if (n_head_words == n_prev_head_words) {
				match = true;
				for (int k = 0; k < n_head_words; k++) {
					if (prevHeadNPWords[k] != headNPWords[k]) {
						match = false;
						break;
					}
				}
			// or, does the mention we're considering have only one head
			// word (and no premods other than determiners) which does
			// match the head word of the previous mention?
			} else if (n_head_words == 1 && !has_non_DP_premod) {
				Symbol prevHead = prevMent->node->getHeadWord();
				if (headNPWords[0] == prevHead) {
					match = true;
					_debug << "Allowing single head word match\n";
				}
			} 


			if (match) {

				if (prevMent->mentionType == Mention::DESC) {
					
					// TODO: as has been said in ace, it would be more efficient to ban the entity 
					// this came from, rather than testing every mention every time.

					// TODO: also, shouldn't this test and the next one be applied
					// even when linking to a name mention? Being that it's a global test?

					if (numeric != 0) {
						// if current mention has a numeric premod, so must previous mention
						// (i.e. "diplomat" then "5 diplomats" should not link)
						const SynNode* prevNumeric = _getNumericMod(prevMent->node);
						if (prevNumeric == 0) {
							_debug << "REJECTED (desc-desc): previous mention does "
								   << "not contain a numeric premod\n";
							continue;
						}
						// mentions can't have numeric premods that clash
						if (_hasNumericClash(numeric, ent, ents)) {
							_debug << "REJECTED (desc-desc): premods have clashing numerics\n";
							continue;
						}
					}

					// mentions can't have entity name premods that clash
					if (has_premod_name && _hasModNameEntityClash(ment, ent, ents)) {
							_debug << "REJECTED (desc-desc): premods have clashing name entities\n";
							continue;
					}

			 

				}

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

const SynNode* ArabicRuleDescLinker::_getNumericMod(const SynNode* node) const
{
	// if the node is preterminal, it won't have any numeric premods
	if (node->isPreterminal())
		return 0;
	// first, check children for numeric premods
	if (!node->getHead()->isPreterminal()) {
		const SynNode* pre = _getNumericMod(node->getHead());
		if (pre != 0)
			return pre;
	}
	int headIndex = node->getHeadIndex();
	for (int i = headIndex+1; i <node->getNChildren(); i++) {
		const SynNode* mod = node->getChild(i);
		if (mod->isPreterminal()) {
			Symbol tag = mod->getTag();
			if (tag == ArabicSTags::CD || tag == ArabicSTags::NUMERIC_COMMA)
				return mod;
		}
		// it seems like I should check the premod branch for this as well, 
		// even though this wasn't done in java (Jon)
		else if (!mod->isTerminal()) {
			const SynNode* deepMod = _getNumericMod(mod);
			if (deepMod != 0)
				return deepMod;
		}
		// if premod is a QP, check for a numeric head (since it won't
		// be found by searching the premod branch)
		if (mod->getTag() == ArabicSTags::QP) {
			const SynNode* head = mod->getHead();
			Symbol headTag = head->getTag();
			if (headTag == ArabicSTags::CD || headTag == ArabicSTags::NUMERIC_COMMA)
				return head;
		}
	}
	return 0;
}

bool ArabicRuleDescLinker::_hasModName(Mention* ment, EntitySet* ents)
{
	const int MAX_MOD_NAMES = 5;
	const SynNode* modNames[MAX_MOD_NAMES];

	int n_mod_names = _getModNames(ment, ents, modNames, MAX_MOD_NAMES);
	
	return (n_mod_names > 0);

}

int ArabicRuleDescLinker::_getModNames(Mention* ment, EntitySet* ents,
								const SynNode* results[], 
								const int max_results)
{	

	// are there any mentions of entities yet?
	const MentionSet* ms = ents->getMentionSet(ment->getSentenceNumber());
	if (ms == 0)
		return 0;

	// does ment have premods?
	const SynNode* node = ment->node;
	int hIndex = node->getHeadIndex();
	if (hIndex == (node->getNChildren() -1))
		return 0;

	if (max_results < 1)
		return 0;
	
	int n_names, n_results = 0;
	const SynNode** sub_results = _new const SynNode*[max_results];
	
	for (int i = hIndex+1; i <node->getNChildren(); i++) {
		// get all names nested within this node if it has a mention
		n_names = _getMentionInternalNames(node->getChild(i), ms, 
		  									sub_results, max_results);
		for (int j = 0; j < n_names; j++) {
			if (n_results == max_results) {
				SessionLogger::warn("premod_names") << "ArabicRuleDescLinker::_getPremodNames(): "
										<< " number of premod names exceeded max_results\n";
				break;
			} 
			results[n_results++] = sub_results[j];
		}
	}

	delete [] sub_results;
	return n_results;
}

// This method is necessary because Arabic Treebank NPs have more 
// structure than their English counterparts, so (post)mod names can
// get buried inside other premod mentions

//In cases where NP chunks are used, the NP's will be less complex
int ArabicRuleDescLinker::_getMentionInternalNames(const SynNode* node, 
										 const MentionSet* ms,
										 const SynNode* results[], 
										 const int max_results) 
{
	if (!node->hasMention() || max_results < 1)
		return 0;
	
	int n_names, n_results = 0;
	const SynNode** sub_results = _new const SynNode*[max_results];

	Mention* ment = ms->getMentionByNode(node);

	if (ment->mentionType == Mention::NAME) {
		results[n_results++] = node;
	}
	else {
		for (int i = 0; i < node->getNChildren(); i++) {
			const SynNode* child = node->getChild(i);
			n_names = _getMentionInternalNames(child, ms, sub_results, max_results);
			for (int j = 0; j < n_names; j++) {
				if (n_results == max_results) {
					SessionLogger::warn("mention_internal_names") << "ArabicRuleDescLinker::_getMentionInternalNames: "
										  << " number of internal names exceeded max_results\n";
					break;
				} 
				results[n_results++] = sub_results[j];
			}
		}
	}
	delete [] sub_results;
	return n_results;
}

bool ArabicRuleDescLinker::_hasModNameEntityClash(Mention* ment, Entity* ent, EntitySet* ents) 
{
	
	if (ment == 0 || ent == 0) {
		return false;
	}

	const int MAX_MOD_NAMES = 5;
	const SynNode* ment1Names[MAX_MOD_NAMES];
	const SynNode* ment2Names[MAX_MOD_NAMES];
	int n_ment1_names, n_ment2_names;

	n_ment1_names = _getModNames(ment, ents, ment1Names, MAX_MOD_NAMES);
	const MentionSet* ms1 = ents->getMentionSet(ment->getSentenceNumber());

	if (n_ment1_names == 0)
		return false;

	// for each existing DESC mention of ent, make sure its premod names contain 
	// a reference to every entity referenced by ment's premod names
	// i.e. ment cannot introduce any premod names not already associated with ent
	for (int k = 0; k < ent->mentions.length(); k++) {

		Mention* ment2 = ents->getMention(ent->mentions[k]);
		
		// only look for mods of other DESCs
		if (ment2->mentionType != Mention::DESC)
			continue;

		n_ment2_names = _getModNames(ment2, ents, ment2Names, MAX_MOD_NAMES);
		const MentionSet* ms2 = ents->getMentionSet(ment2->getSentenceNumber());

		for (int i = 0; i < n_ment1_names; i++) {
			_debug << "In clash: ment1:\n" << ment1Names[i]->toString(1) << "\n";

			Mention* m1 = ms1->getMentionByNode(ment1Names[i]);
			// TODO: need to account for metonymyic mentions
			Entity* e1 = ents->getEntityByMention(m1->getUID());
			// find the entity of this mention
			if (e1 == 0)
				continue;

			// search through all premod names of this mention to find matching entity
			// with ment's current premod name
			bool match_found = false;
			for (int j = 0; j < n_ment2_names; j++) {
				_debug << "In clash: ment2:\n" << ment2Names[j]->toString(1) << "\n";
				
				Mention* m2 = ms2->getMentionByNode(ment2Names[j]);
				// TODO: need to account for metonymyic mentions
				Entity* e2 = ents->getEntityByMention(m2->getUID());
				if (e2 == 0)
					continue;

				_debug << e1->getID() << " =? " << e2->getID() << "\n";
				// are the entities of these mentions the same?
				if (e1 == e2) {
					_debug << "Match Found!\n";
					match_found =  true;
					break;
				}
			}
			if (!match_found) {
				_debug << "No matching entity name found\n";
				return true;
			}
		}
	}
	// couldn't find a clash
	return false;
}

bool ArabicRuleDescLinker::_hasNumericClash(const SynNode* node, Entity* ent, LexEntitySet *ents) 
{
	if (ent == 0 || node == 0) {
		return false;
	}
	int i;
	for (i=0; i < ent->mentions.length(); i++) {

		Mention* ment = ents->getMention(ent->mentions[i]);
		const SynNode* mentNode = _getNumericMod(ment->node);
		if (mentNode != 0) {
			Symbol nodeHead = node->getHeadWord();
			Symbol mentHead = mentNode->getHeadWord();
			_debug << "Comparing for numeric clash: " << nodeHead.to_string();
			_debug << " and " << mentHead.to_string() << "\n";
			if (mentNode->getHeadWord() != node->getHeadWord())
				return true;
		}
	}
	return false;
}

int ArabicRuleDescLinker::_getHeadNPWords(const SynNode* node, Symbol results[], int max_results) {
	if (node->isPreterminal() || node->isTerminal()) 
		return node->getTerminalSymbols(results, max_results);
	while (!node->getHead()->isPreterminal()) {
		node = node->getHead();
	}
	return node->getTerminalSymbols(results, max_results);
}


//Always create a new entity
int ArabicRuleDescLinker::linkNoMentions (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType linkType, 
							 LexEntitySet *results[], int max_results) 
{ 
	Mention *currMention = currSolution->getMention(currMentionUID);
	EntityGuess* guess = 0;

	guess = _guessNewEntity(currMention, linkType);
		// only one branch here - desc linking is deterministic
	LexEntitySet* newSet = currSolution->fork();
	if (guess->id == EntityGuess::NEW_ENTITY) {
		newSet->addNew(currMention->getUID(), guess->type);
	}
	results[0] = newSet;

	// MEMORY: a guess was definitely created above, and is removed now.
	delete guess;
	return 1;
}

EntityGuess* ArabicRuleDescLinker::_guessNewEntity(Mention *ment, EntityType linkType)
{
	// MEMORY: linkMention, the caller of this method, is guaranteed to delete this
	EntityGuess* guess = _new EntityGuess();
	guess->id = EntityGuess::NEW_ENTITY;
	guess->score = 1;
	guess->type = linkType;
	return guess;
}
