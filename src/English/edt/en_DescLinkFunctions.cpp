// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/SymbolHash.h"
#include "English/edt/en_DescLinkFunctions.h"
#include "English/parse/en_STags.h"
#include "Generic/theories/SynNode.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


void EnglishDescLinkFunctions::getEventContext(EntitySet* entSet, Mention* mention, 
										Entity* entity, OldMaxEntEvent* evt, int maxSize) 
{
	//1) mention type
	evt->addAtomicContextPredicate(mention->getEntityType().getName());
	// new test: get all premods of mention
	_addPremodList(mention, evt);
	// also get all premods of entity mentions
	_addPremodList(entity, entSet, evt);
	// get parents of mention (
	_addParentHead(mention, evt);
	_addParentHead(entity, entSet, evt);
	evt->addAtomicContextPredicate(_getNumEnts(entity->getType(), entSet));
	if (_testHeadWordMatch(mention, entity, entSet)) {
		evt->addAtomicContextPredicate(Symbol(L"HeadWordMatch"));
		if (_testHeadWordNodeMatch(mention, entity, entSet)) {
			evt->addAtomicContextPredicate(Symbol(L"HeadWordNodeMatch"));
			if (_exactStringMatch(mention, entity, entSet)) 
				evt->addAtomicContextPredicate(Symbol(L"ExactStringMatch"));
		}
	}
	//if (testTransHeadMatch())
// JM: temporarily turned these off to simplify things...and back on
	evt->addAtomicContextPredicate(_getUniqueModifierRatio(mention, entity, entSet));
	evt->addAtomicContextPredicate(_getDistance(mention->getSentenceNumber(), entity, entSet));	
	//if (testNameHeadMatch()
	//if (numberMatch())
	//if (genderMatch())
	//if (lookDefinite(ment->node))
	//	evt->addContextPredicate(Symbol("LooksDefinite"));
	//if (specialtyHeadType()) (cluster IDs)
	_testNumericPremods(evt, mention, entity, entSet);
	// any premod at all
	if (_hasPremod(mention)) {
		if (_hasPremodClash(mention, entity, entSet))
			evt->addAtomicContextPredicate(Symbol(L"premodClash"));
		// we have the ability to display all the matching premods,
		// but for now, let's just mention that we matched
		Symbol match;
		if (_hasPremodMatch(mention, entity, entSet, &match))
//			evt->addContextPredicate(match);
			evt->addAtomicContextPredicate(Symbol(L"premodMatch"));

	}

	// fill in binary context predicates
	int n_atomic = evt->getNContextPredicates();
	Symbol bigram[2];
	for (int i = 1; i < n_atomic - 1; i++) {
		bigram[0] = evt->getContextPredicate(i).getSymbol(0);
		for (int j = i + 1; j < n_atomic; j++) {
			bigram[1] = evt->getContextPredicate(j).getSymbol(0);
			evt->addComplexContextPredicate(2, bigram);
		}
	}

	return;
}
bool EnglishDescLinkFunctions::_testHeadWordMatch(Mention *ment, Entity *entity, EntitySet *entitySet) {
	Symbol headWord = ment->getNode()->getHeadWord();
	for (int i = 0; i < entity->getNMentions(); i++) {
		Mention* entMent = entitySet->getMention(entity->getMention(i)); 
		Symbol entHeadWord = entMent->getNode()->getHeadWord();
		if (headWord == entHeadWord)
			return true;
	}
	return false;
}

bool EnglishDescLinkFunctions::_testHeadWordNodeMatch(Mention *ment, Entity *entity, EntitySet *entitySet) {
	Symbol mentHeadWords[20];
	Symbol entHeadWords[20];
	int n_ment_words = _getHeadNPWords(ment->getNode(), mentHeadWords, 20);
	for (int i = 0; i < entity->getNMentions(); i++) {
		Mention* entMent = entitySet->getMention(entity->getMention(i)); 
		int n_ent_words = _getHeadNPWords(entMent->getNode(), entHeadWords, 20);
		if (n_ent_words != n_ment_words)
			continue;
		bool match = true;
		for (int j = 0; j < n_ment_words; j++) {
		   if (mentHeadWords[j] != entHeadWords[j]) {
			   match = false;
			   break;
		   }
		}
		if (match)
			return true;
	}
	return false;
}

// Note: should probably store the modifiers for each entity (and mention?), so we don't have to keep 
// recalculating.  Also should add stop words.
Symbol EnglishDescLinkFunctions::_getUniqueModifierRatio(Mention *ment, Entity *entity, EntitySet *entitySet) {
	SymbolHash entityWords(50);
	int n_ent_words = 0;
	int unique = 0;
	int total = 0;

	// collect Entity words
	Symbol entityMentWords[20];
	for (int i = 0; i < entity->getNMentions(); i++) {
		Mention *entityMent = entitySet->getMention(entity->getMention(i));
		int n_words = entityMent->getNode()->getTerminalSymbols(entityMentWords, 20);
		for (int j = 0; j < n_words; j++) {
			if (entityMentWords[j] == entityMent->getHead()->getHeadWord())
				continue;
			if (! entityWords.lookup(entityMentWords[j])) {
				entityWords.add(entityMentWords[j]);
				n_ent_words++;	
			}
		}
	}

	// collect Mention words
	Symbol mentionWords[20];
	int n_ment_words = ment->getNode()->getTerminalSymbols(mentionWords, 20);

	if (n_ment_words == 1)
		return Symbol(L"u[0.0]");

	if (n_ent_words == 0)
		return Symbol(L"u[1.0]");

	for (int k = 0; k < n_ment_words; k++) {
		if (mentionWords[k] == ment->getHead()->getHeadWord())
			continue;
		if (! entityWords.lookup(mentionWords[k]))
			unique++;
		total++;
	}
	
	double ratio = (double)unique/(double)total;
	if (ratio == 0.0) 
		return Symbol(L"u[0.0]");
	else if (ratio < 0.25)
		return Symbol(L"u[<.25]");
	else if (ratio < 0.5)
		return Symbol(L"u[<.5]");
	else if (ratio < 0.75)
		return Symbol(L"u[<.75]");
	else if (ratio < 1.0)
		return Symbol(L"u[<1.0]");
	else
		return Symbol(L"u[1.0]");
	
}


Symbol EnglishDescLinkFunctions::_getDistance(int sent_num, Entity *entity, EntitySet *entitySet) {
	int distance = entitySet->getMention(entity->getMention(0))->getSentenceNumber() - sent_num;
	if (distance < 0)
		distance = -distance;

	for (int i = 1; i < entity->getNMentions(); i++) {
		Mention *ment = entitySet->getMention(entity->getMention(i));
		int tmp = ment->getSentenceNumber() - sent_num;
		if (tmp < 0)
			tmp = -tmp;
		if (tmp < distance)
			distance = tmp;
	}

	if (distance > 5) 
		distance = 5;

	wchar_t number[10];
	swprintf(number, 10, L"D%d", distance);
	return Symbol(number);
}


bool EnglishDescLinkFunctions::_exactStringMatch(Mention *ment, Entity *entity, EntitySet *entitySet) {
	std::wstring mentString = ment->getNode()->toString(0);
	for (int i = 0; i < entity->getNMentions(); i++) {
		Mention *entMention = entitySet->getMention(entity->getMention(i));
		std::wstring entString = entMention->getNode()->toString(0);
		if (entString == mentString)
			return true;
	}
	return false;
}

int EnglishDescLinkFunctions::_getHeadNPWords(const SynNode* node, Symbol results[], int max_results) {
	if (node->isPreterminal() || node->isTerminal()) 
		return node->getTerminalSymbols(results, max_results);
	while (!node->getHead()->isPreterminal()) {
		node = node->getHead();
	}
	return node->getTerminalSymbols(results, max_results);
}

void EnglishDescLinkFunctions::_testNumericPremods(OldMaxEntEvent *e, Mention *ment, Entity *ent, EntitySet *entitySet) {
	GrowableArray<const SynNode*> numerics;
	for (int i = 0; i < ent->getNMentions(); i++) {
		Mention *prev = entitySet->getMention(ent->getMention(i));
		const SynNode *num_node = _getNumericPremod(prev);
		if (num_node != NULL) 
			numerics.add(num_node);
	}
	if (numerics.length() > 0) 
		e->addAtomicContextPredicate(Symbol(L"prevNumeric"));

	const SynNode* this_num = _getNumericPremod(ment);
	if (this_num != NULL) {
		e->addAtomicContextPredicate(Symbol(L"numericPremod"));
		bool match = false;
		bool clash = false;
		for (int j = 0; j < numerics.length(); j++) {
			if (this_num->getHeadWord() != numerics[j]->getHeadWord() && !clash) { 
				e->addAtomicContextPredicate(Symbol(L"numericClash"));
				clash = true;
			}
			else if (!match) { 
				e->addAtomicContextPredicate(Symbol(L"numericMatch"));
				match = true;
			}
		}
	}
}

const SynNode* EnglishDescLinkFunctions::_getNumericPremod(Mention *ment) {

	const SynNode *node = ment->getNode();

	// if the node is preterminal, it won't have any numeric premods
	if (node->isPreterminal())
		return 0;

	for (int i = 0; i < node->getHeadIndex(); i++) {
		const SynNode *child = node->getChild(i);
		if (child->getTag() == EnglishSTags::QP) {
			const SynNode *childHead = child->getHead();
			if (childHead->getTag() == EnglishSTags::CD)
				return childHead;
		}
		else if (child->getTag() == EnglishSTags::CD)
			return child;
	}
	return 0;
}


bool EnglishDescLinkFunctions::_hasPremod(Mention* ment)
{
	// does ment have premods?
	const SynNode* node = ment->node;
	int hIndex = node->getHeadIndex();
	if (hIndex < 1)
		return false;
	// TODO: actually output the words in the event
	return true;
}

// the idea being to return predicates only. If it's not a predicate (verb word)
// don't bother
Symbol EnglishDescLinkFunctions::_getParentHead(Mention* ment) {
	const SynNode* node = ment->node;
	if (node->getParent() == NULL) 
		return Symbol(L"");
	Symbol mentHead = node->getHeadWord();
	while (node->getParent() != NULL) {
		const SynNode* headNode = node->getParent()->getHeadPreterm();
		Symbol headWord = headNode->getHeadWord();
		if (headWord != mentHead &&
			(headNode->getTag() == EnglishSTags::VB ||
			 headNode->getTag() == EnglishSTags::VBD ||
			 headNode->getTag() == EnglishSTags::VBG ||
			 headNode->getTag() == EnglishSTags::VBN ||
			 headNode->getTag() == EnglishSTags::VBP ||
			 headNode->getTag() == EnglishSTags::VBZ)) {
				 return headWord;
			 }
			 node = node->getParent();
	}
	return Symbol(L"");
}
void EnglishDescLinkFunctions::_addParentHead(Mention* ment, OldMaxEntEvent* e) {
	Symbol sym = _getParentHead(ment);
	if (sym != Symbol(L"")) {
		wchar_t wordstr[300];
		swprintf(wordstr, 300, L"ment-parent[%ls]", sym.to_string());
		e->addAtomicContextPredicate(Symbol(wordstr));
	}
}
void EnglishDescLinkFunctions::_addParentHead(Entity* ent, EntitySet* ents, OldMaxEntEvent* e) {
	SymbolHash entityPreds(5);
	int i;
	for (i=0; i < ent->getNMentions(); i++) {
		Mention* ment = ents->getMention(ent->getMention(i));
		Symbol sym = _getParentHead(ment);
		if (sym != Symbol(L"") && !entityPreds.lookup(sym)) {
			entityPreds.add(sym);
			wchar_t wordstr[300];
			swprintf(wordstr, 300, L"ent-parent[%ls]", sym.to_string());
			e->addAtomicContextPredicate(Symbol(wordstr));
		}
	}
}
void EnglishDescLinkFunctions::_addPremodList(Mention* ment, OldMaxEntEvent* e) {
	const int MAX_PREMODS = 5;
	Symbol premods[MAX_PREMODS];
	int n_premods;
	n_premods = _getPremods(ment, premods, MAX_PREMODS);
	for (int i = 0; i < n_premods; i++) {
		wchar_t wordstr[300];
		swprintf(wordstr, 300, L"ment[%ls]", premods[i].to_string());
		e->addAtomicContextPredicate(Symbol(wordstr));
	}
}

void EnglishDescLinkFunctions::_addPremodList(Entity* ent, EntitySet* ents, OldMaxEntEvent* e) {
	// TODO: i have to hash the premods so i'm ensured there's no repeat. that sort of sucks
	const int MAX_PREMODS = 5;
	const int MAX_TOTAL_WORDS = 50;
	Symbol total_words[MAX_TOTAL_WORDS];
	int total_words_idx = 0;
	
	int i;
	for (i=0; i < ent->getNMentions(); i++) {
		Mention* ment = ents->getMention(ent->getMention(i));
		Symbol premods[MAX_PREMODS];
		int n_premods;
		n_premods = _getPremods(ment, premods, MAX_PREMODS);
		// filter out duplicates
		for (int k = 0; k < n_premods; k++) {
			bool valid=true;
			for (int m = 0; m < total_words_idx; m++) {
				if (wcscmp(total_words[m].to_string(), premods[k].to_string()) == 0) {
					valid = false;
					break;
				}
			}
			if (valid)
				total_words[total_words_idx++] = premods[k];
			if (total_words_idx >= MAX_TOTAL_WORDS)
				break;
		}
		if (total_words_idx >= MAX_TOTAL_WORDS)
			break;
	}
	for (i=0; i < total_words_idx; i++) {
		wchar_t wordstr[300];
		swprintf(wordstr, 300, L"ent[%ls]", total_words[i].to_string());
		e->addAtomicContextPredicate(Symbol(wordstr));
	}
}

int EnglishDescLinkFunctions::_getPremods(Mention* ment, Symbol results[], 
										const int max_results)
{	
	if (max_results < 1)
		return 0;

	const SynNode* node = ment->node;
	Symbol headWord = node->getHeadWord();
	int mention_length = node->getNTerminals();
	Symbol* terms = _new Symbol[mention_length];
	node->getTerminalSymbols(terms, mention_length);

	int n_results = 0;
	
	for (int i = 0; i < max_results && i < mention_length; i++) {
		if (terms[i] == headWord)
			break;
		results[n_results++] = terms[i];
	}
	delete [] terms;
	return n_results;
}


bool EnglishDescLinkFunctions::_hasPremodClash(Mention* ment, Entity* ent, EntitySet* ents) {
	if (ment == 0 || ent == 0)
		return false;
	const int MAX_PREMODS = 5;
	Symbol ment1Premods[MAX_PREMODS];
	Symbol ment2Premods[MAX_PREMODS];
	int n_ment1_premods, n_ment2_premods;

	n_ment1_premods = _getPremods(ment, ment1Premods, MAX_PREMODS);
	if (n_ment1_premods == 0)
		return false;
	Symbol ment1Head = ment->getNode()->getHeadWord();
	
	// and now all the premods of each mention of the entity that shares a head
	for (int k = 0; k < ent->mentions.length(); k++) {
		Mention* ment2 = ents->getMention(ent->mentions[k]);
		// only look for premods of other DESCs
		if (ment2->mentionType != Mention::DESC)
			continue;
		// specifically, only DESCS with the same head word
		if (ment2->getNode()->getHeadWord() != ment1Head)
			continue;
		n_ment2_premods = _getPremods(ment2, ment2Premods, MAX_PREMODS);
		if (n_ment2_premods == 0)
			continue;
		// a clash exists if a word in ment2 isn't in ment1 AND vice versa
		bool firstHalfClash = false;
		for (int j = 0; j < n_ment2_premods; j++) {
			bool foundMent = false;
			for (int i = 0; i < n_ment1_premods; i++) {
				if (ment1Premods[i] == ment2Premods[j]) {
					foundMent = true;
					break;
				}
			}
			if (!foundMent) {
				firstHalfClash = true;
				break;
			}
		}
		if (firstHalfClash) {
			for (int i = 0; i < n_ment1_premods; i++) {
				bool foundMent = false;
				for (int j = 0; j < n_ment2_premods; j++) {
					if (ment1Premods[i] == ment2Premods[j]) {
						foundMent = true;
						break;
					}
				}
				if (!foundMent) {
					return true;
				}
			}
		}
	}
	return false;
}






bool EnglishDescLinkFunctions::_hasPremodMatch(Mention* ment, Entity* ent, EntitySet* ents, Symbol* word) {
	if (ment == 0 || ent == 0)
		return false;
	const int MAX_PREMODS = 5;
	Symbol ment1Premods[MAX_PREMODS];
	Symbol ment2Premods[MAX_PREMODS];
	Symbol commonPremods[MAX_PREMODS];
	int n_ment1_premods, n_ment2_premods, n_common_premods=0;

	n_ment1_premods = _getPremods(ment, ment1Premods, MAX_PREMODS);
	if (n_ment1_premods == 0)
		return false;
	Symbol ment1Head = ment->getNode()->getHeadWord();
	// and now all the premods of each mention of the entity that shares a head
	for (int k = 0; k < ent->mentions.length(); k++) {
		Mention* ment2 = ents->getMention(ent->mentions[k]);
		// only look for premods of other DESCs
		if (ment2->mentionType != Mention::DESC)
			continue;
		// specifically, only DESCS with the same head word
		if (ment2->getNode()->getHeadWord() != ment1Head)
			continue;
		n_ment2_premods = _getPremods(ment2, ment2Premods, MAX_PREMODS);
		if (n_ment2_premods == 0)
			continue;
		// a match exists if any word in ment2 is in ment1 OR vice versa
		// for ANY mention
		for (int j = 0; j < n_ment2_premods; j++) {
			for (int i = 0; i < n_ment1_premods; i++) {
				if (ment1Premods[i] == ment2Premods[j]) {
					bool alreadySeen = false;
					for (int m = 0; m < n_common_premods; m++) {
						if (commonPremods[m] == ment1Premods[i]) {
							alreadySeen = true;
							break;
						}
					}
					if (!alreadySeen)
						commonPremods[n_common_premods++] = ment1Premods[i];
				}
			}			
		}
		for (int i = 0; i < n_ment1_premods; i++) {
			for (int j = 0; j < n_ment2_premods; j++) {
				if (ment1Premods[i] == ment2Premods[j]) {
					bool alreadySeen = false;
					for (int m = 0; m < n_common_premods; m++) {
						if (commonPremods[m] == ment1Premods[i]) {
							alreadySeen = true;
							break;
						}
					}
					if (!alreadySeen)
						commonPremods[n_common_premods++] = ment1Premods[i];
				}
			}
		}
	}
	if (n_common_premods < 1)
		return false;
	wchar_t wordstr[300];
	int j = 0;
	j = swprintf(wordstr, 300-j, L"%ls ", L"[");
	for (int i = 0; i < n_common_premods; i++) {
		j += swprintf(wordstr+j, 300-j, L"%ls ", commonPremods[i].to_string());
	}
	j += swprintf(wordstr+j, 300-j, L"%ls", L"]");
    *word = Symbol(wordstr);
	return true;
}

Symbol EnglishDescLinkFunctions::_getNumEnts(EntityType type, EntitySet *set) {
	int size = set->getEntitiesByType(type).length();
	wchar_t wordstr[20];
	if (size <= 10)
		swprintf(wordstr, 20, L"numEnts[%d]", size);
	else
		swprintf(wordstr, 20, L"numEnts[%ls]", L">10");
	return Symbol(wordstr);
}
