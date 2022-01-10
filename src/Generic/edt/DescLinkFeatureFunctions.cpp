// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/edt/xx_DescLinkFeatureFunctions.h"
#include "Generic/common/Assert.h"
#include "Generic/common/NationalityRecognizer.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Sexp.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/DocumentRelMentionSet.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/edt/discmodel/DocumentMentionInformationMapper.h"
#include "Generic/common/SymbolHash.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

#define   swprintf   _snwprintf

SymbolHash *DescLinkFeatureFunctions::_stopWords = 0;
const int initialTableSize = 16000;
const Symbol DescLinkFeatureFunctions::NONE_SYMBOL = (L"NONE_S");
const Symbol DescLinkFeatureFunctions::LEFT = (L"LEFT");
const Symbol DescLinkFeatureFunctions::RIGHT = (L"RIGHT");

std::map<std::wstring, std::wstring> DescLinkFeatureFunctions::_nationalities;
std::map<std::wstring, std::vector<std::wstring> > DescLinkFeatureFunctions::_name_gpe_affiliations;

Symbol DescLinkFeatureFunctions::findMentHeadWord(const Mention *ment) {
	Symbol headWord = ment->getNode()->getHeadWord();
	wchar_t wordstr[300];
	swprintf(wordstr, 300, L"ment-head[%ls]", headWord.to_string());
	return Symbol(wordstr);
}

int DescLinkFeatureFunctions::testHeadWordsMatch(MentionUID mentUID, Entity *entity, const MentionSymArrayMap *map) {
	SymbolArray **mentArray = map->get(mentUID);
	SerifAssert (mentArray != NULL);
	SerifAssert ((*mentArray) != NULL);
	const Symbol *headWords = (*mentArray)->getArray();
	int nHeadWords = (*mentArray)->getLength();
	const Symbol *entHeadWords;
	for (int i = 0; i < (entity->getNMentions()); i++) {
		SymbolArray **entityArray = map->get(entity->getMention(i));
		if (entityArray == NULL)
			continue;
		SerifAssert ((*entityArray) != NULL);
		entHeadWords =  (*entityArray)->getArray();
		size_t n_ent_words = (*entityArray)->getLength();
		if ((size_t)nHeadWords != n_ent_words)
			continue;
		bool found = true;
		for (int j=0; j<nHeadWords; j++) {
			if (headWords[j] != entHeadWords[j]) {
				found = false;
				break;
			}
		}
		if (found) return nHeadWords;
	}
	return -1;
}

bool DescLinkFeatureFunctions::testHeadWordMatch(const Mention *ment, Entity *entity, const EntitySet *entitySet){
	return testHeadWordMatch(ment->getNode()->getHeadWord(), entity, entitySet);
}

bool DescLinkFeatureFunctions::testHeadWordMatch(Symbol headWord, Entity *entity, const EntitySet *entitySet) {
	for (int i = 0; i < entity->getNMentions(); i++) {
		Mention* entMent = entitySet->getMention(entity->getMention(i)); 
		Symbol entHeadWord = entMent->getNode()->getHeadWord();
		if (headWord == entHeadWord)
			return true;
	}
	return false;
}

bool DescLinkFeatureFunctions::testHeadWordNodeMatch(const Mention *ment, Entity *entity, const EntitySet *entitySet) {
	Symbol mentHeadWords[20];
	Symbol entHeadWords[20];
	int n_ment_words = getHeadNPWords(ment->getNode(), mentHeadWords, 20);
	for (int i = 0; i < entity->getNMentions(); i++) {
		Mention* entMent = entitySet->getMention(entity->getMention(i)); 
		int n_ent_words = getHeadNPWords(entMent->getNode(), entHeadWords, 20);
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

bool DescLinkFeatureFunctions::testLastCharMatch(const Mention *ment, Entity *entity, const EntitySet *entitySet) {
	const wchar_t *head_word = ment->getNode()->getHeadWord().to_string();
	const wchar_t *last_char = head_word + wcslen(head_word) - 1;
	for (int i = 0; i < entity->getNMentions(); i++) {
		Mention* entMent = entitySet->getMention(entity->getMention(i)); 
		const wchar_t *ent_head_word = entMent->getNode()->getHeadWord().to_string();
		const wchar_t *ent_last_char = ent_head_word + wcslen(ent_head_word) - 1;
		if (*last_char  == *ent_last_char)
			return true;
	}
	return false;
}


// Note: should probably store the modifiers for each entity (and mention?), so we don't have to keep 
// recalculating.  Also should add stop words.
Symbol DescLinkFeatureFunctions::getUniqueModifierRatio(const Mention *ment, Entity *entity, const EntitySet *entitySet) {
	if (_stopWords == 0) {
		_stopWords = _new SymbolHash(initialTableSize);
		//initializeStopWords();
	}
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
			// JCS - 03/08/04 - Don't want this to count against a link if modifier occured in head word of ent
			//if (entityMentWords[j] == entityMent->getHead()->getHeadWord())
			//	continue;
			//if (_stopWords->lookup(entityMentWords[j]))
			//	continue;
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
		//if (_stopWords->lookup(mentionWords[k]))
		//	continue;
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

Symbol DescLinkFeatureFunctions::getUniqueCharRatio(const Mention *ment, Entity *entity, const EntitySet *entitySet) {
	SymbolHash entityChars(150);
	int n_ent_chars = 0;
	int unique = 0;
	int total = 0;
	wchar_t str[2];
	str[1] = '\0';

	// collect Entity chars
	Symbol entityMentWords[20];
	for (int i = 0; i < entity->getNMentions(); i++) {
		Mention *entityMent = entitySet->getMention(entity->getMention(i));
		int n_words = entityMent->getNode()->getTerminalSymbols(entityMentWords, 20);
		for (int j = 0; j < n_words; j++) {
			const wchar_t *ent_word = entityMentWords[j].to_string();
			int len = static_cast<int>(wcslen(ent_word));
			for (int k = 0; k < len; k++) {
				str[0] = *(ent_word + k);
				Symbol character(str);
				if (! entityChars.lookup(character)) {
					entityChars.add(character);
					n_ent_chars++;	
				}
			}
		}
	}

	// collect Mention words
	Symbol mentionWords[20];
	int n_ment_words = ment->getNode()->getTerminalSymbols(mentionWords, 20);

	if (n_ment_words == 0)
		return Symbol(L"uh[0.0]");

	if (n_ent_chars == 0)
		return Symbol(L"uh[1.0]");

	for (int k = 0; k < n_ment_words; k++) {
		const wchar_t *ment_word = mentionWords[k].to_string();
		int len = static_cast<int>(wcslen(ment_word)); 
		for (int l = 0; l < len; l++) {
			str[0] = *(ment_word + l);
			Symbol character(str);
			if (! entityChars.lookup(character))
				unique++;
			total++;
		}
	}
	
	double ratio = (double)unique/(double)total;
	if (ratio == 0.0) 
		return Symbol(L"uh[0.0]");
	else if (ratio < 0.25)
		return Symbol(L"uh[<.25]");
	else if (ratio < 0.5)
		return Symbol(L"uh[<.5]");
	else if (ratio < 0.75)
		return Symbol(L"uh[<.75]");
	else if (ratio < 1.0)
		return Symbol(L"uh[<1.0]");
	else
		return Symbol(L"uh[1.0]");
}

/* computes the distance in mentions between the entity's last mention and the mention.
* In arabic this enables to utilize forward coreference of nominals to names (appositives)
*/
/*** this function is deprecated and is here for backward compatibility ***/
Symbol DescLinkFeatureFunctions::getMentionDistance4(const Mention *mention, Entity *entity, const EntitySet *entitySet/*, GrowableArray<const MentionSet *> &mentionSets*/) {

	int sent_num = mention->getSentenceNumber();
	if (sent_num >= entitySet->getNMentionSets()) {
		char tempBuffer[500];
		sprintf(tempBuffer, "sentNum(%d) >= number of mention sets(%d)", sent_num, entitySet->getNMentionSets());
		throw InternalInconsistencyException(
			"DescLinkFeatureFunctions::getMentionDistance()",
			tempBuffer);
	}
	// get the last entity mention
	// assuming that metions index in the same sentence correspond to relative order in the sentence
	Mention *lastMention = entitySet->getMention(entity->getMention(0));
	MentionUID lastMentionUID = lastMention->getUID();
	int lastMentionIndex = lastMention->getIndex();
	int	lastMentionSentenceNumber = lastMention->getSentenceNumber();
	for (int i=1; i<entity->getNMentions(); i++){
		Mention *ment = entitySet->getMention(entity->getMention(i));
		if(lastMentionUID<ment->getUID()){
			lastMention = ment;
			lastMentionUID = ment->getUID();
			lastMentionIndex = ment->getIndex();
			lastMentionSentenceNumber = ment->getSentenceNumber();
		}
	}

	// compute the mention distance
	int mentDistance = 0;
	if (lastMentionSentenceNumber < sent_num) {
		mentDistance = entitySet->getMentionSet(lastMentionSentenceNumber)->getNMentions()-lastMentionIndex-1;
	}
	for (int lsn=lastMentionSentenceNumber+1; lsn<sent_num; lsn++) {
		mentDistance += entitySet->getMentionSet(lsn)->getNMentions();
	}
	if (lastMentionSentenceNumber < sent_num) {
		mentDistance += mention->getIndex()+1; // index starts from 0
	} else { // == (deal with forward coref within a sentence
		mentDistance += mention->getIndex() - lastMentionIndex;
	}

	// cutoff
	if (mentDistance > 15) mentDistance = 15;
	if (mentDistance < -8) mentDistance = -4; // *** maybe -4 is not enough

	wchar_t number[10];
	swprintf(number, 10, L"D%d", mentDistance);
	return Symbol(number);
}

/* computes the distance in mentions between the entity's last mention and the mention.
* In arabic this enables to utilize forward coreference of nominals to names (appositives)
*/
Symbol DescLinkFeatureFunctions::getMentionDistance8(const Mention *mention, Entity *entity, const EntitySet *entitySet) {

	int sent_num = mention->getSentenceNumber();
	if (sent_num >= entitySet->getNMentionSets()) {
		char tempBuffer[500];
		sprintf(tempBuffer, "sentNum(%d) >= number of mention sets(%d)", sent_num, entitySet->getNMentionSets());
		throw InternalInconsistencyException(
			"DescLinkFeatureFunctions::getMentionDistance()",
			tempBuffer);
	}
	// get the last entity mention
	// assuming that metions index in the same sentence correspond to relative order in the sentence
	Mention *lastMention = entitySet->getMention(entity->getMention(0));
	MentionUID lastMentionUID = lastMention->getUID();
	int lastMentionIndex = lastMention->getIndex();
	int	lastMentionSentenceNumber = lastMention->getSentenceNumber();
	for (int i=1; i<entity->getNMentions(); i++){
		Mention *ment = entitySet->getMention(entity->getMention(i));
		if(lastMentionUID<ment->getUID()){
			lastMention = ment;
			lastMentionUID = ment->getUID();
			lastMentionIndex = ment->getIndex();
			lastMentionSentenceNumber = ment->getSentenceNumber();
		}
	}

	// compute the mention distance
	int mentDistance = 0;
	if (lastMentionSentenceNumber < sent_num) {
		mentDistance = entitySet->getMentionSet(lastMentionSentenceNumber)->getNMentions()-lastMentionIndex-1;
	}
	for (int lsn=lastMentionSentenceNumber+1; lsn<sent_num; lsn++) {
		mentDistance += entitySet->getMentionSet(lsn)->getNMentions();
	}
	if (lastMentionSentenceNumber < sent_num) {
		mentDistance += mention->getIndex()+1; // index starts from 0
	} else { // == (deal with forward coref within a sentence
		mentDistance += mention->getIndex() - lastMentionIndex;
	}

	// cutoff
	if (mentDistance > 15) mentDistance = 15;
	// -8 turned out to be better but we leave also -4 because it was used in the 2007 eval
	if (mentDistance < -8) mentDistance = -8; 

	wchar_t number[10];
	swprintf(number, 10, L"D%d", mentDistance);
	return Symbol(number);
}


Symbol DescLinkFeatureFunctions::getSentenceDistance(int sent_num, Entity *entity, const EntitySet *entitySet) {
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

Symbol DescLinkFeatureFunctions::getSentenceDistanceWide(int sent_num, Entity *entity, const EntitySet *entitySet) {
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

	if (distance > 10) 
		distance = 10;

	if (distance > 5 && distance < 10) 
		distance = 5;

	wchar_t number[10];
	swprintf(number, 10, L"D%d", distance);
	return Symbol(number);
}



bool DescLinkFeatureFunctions::exactStringMatch(const Mention *ment, Entity *entity, const EntitySet *entitySet) {
	std::wstring mentString = ment->getNode()->toTextString();
	size_t ws = mentString.find(L" ");
	while (ws != std::wstring::npos) {
		mentString.erase(ws, 1);
		ws = mentString.find(L" ");
	}
	for (int i = 0; i < entity->getNMentions(); i++) {
		Mention *entMention = entitySet->getMention(entity->getMention(i));
		std::wstring entString = entMention->getNode()->toTextString();
		ws = entString.find(L" ");
		while (ws != std::wstring::npos) {
			entString.erase(ws, 1);
			ws = entString.find(L" ");
		}
		if (entString.compare(mentString) == 0)
			return true;
	}
	return false;
}

int DescLinkFeatureFunctions::getHeadNPWords(const SynNode* node, Symbol results[], int max_results) {
	if (node->isPreterminal() || node->isTerminal()) 
		return node->getTerminalSymbols(results, max_results);
	while (!node->getHead()->isPreterminal()) {
		node = node->getHead();
	}
	return node->getTerminalSymbols(results, max_results);
}

bool DescLinkFeatureFunctions::entHasNumeric(const Entity *ent, const EntitySet *entitySet) {
	for (int i = 0; i < ent->getNMentions(); i++) {
		const Mention *prev = entitySet->getMention(ent->getMention(i));
		const SynNode *num_node = DescLinkFeatureFunctions::getNumericMod(prev);
		if (num_node != NULL) 
			return true;
	}
	return false; 
}

bool DescLinkFeatureFunctions::mentHasNumeric(const Mention *ment) {
	const SynNode* this_num = DescLinkFeatureFunctions::getNumericMod(ment);
	if (this_num != NULL) 
		return true;
	else
		return false;
}


bool DescLinkFeatureFunctions::hasNumericClash(const Mention *ment, Entity *ent, const EntitySet *entitySet) 
{
	GrowableArray<const SynNode*> numerics;
	for (int i = 0; i < ent->getNMentions(); i++) {
		Mention *prev = entitySet->getMention(ent->getMention(i));
		const SynNode *num_node = DescLinkFeatureFunctions::getNumericMod(prev);
		if (num_node != NULL) 
			numerics.add(num_node);
	}
	if (numerics.length() == 0) 
		return false;

	const SynNode* this_num = DescLinkFeatureFunctions::getNumericMod(ment);
	
	if (this_num == NULL)
		return false;

	for (int j = 0; j < numerics.length(); j++) {
		if (this_num->getHeadWord() != numerics[j]->getHeadWord()) { 
			return true;
		}
	}
	return false;
}

bool DescLinkFeatureFunctions::hasNumericMatch(const Mention *ment, Entity *ent, const EntitySet *entitySet) 
{
	GrowableArray<const SynNode*> numerics;
	for (int i = 0; i < ent->getNMentions(); i++) {
		Mention *prev = entitySet->getMention(ent->getMention(i));
		const SynNode *num_node = DescLinkFeatureFunctions::getNumericMod(prev);
		if (num_node != NULL) 
			numerics.add(num_node);
	}
	if (numerics.length() == 0) 
		return false;

	const SynNode* this_num = DescLinkFeatureFunctions::getNumericMod(ment);
	
	if (this_num == NULL)
		return false;

	for (int j = 0; j < numerics.length(); j++) {
		if (this_num->getHeadWord() == numerics[j]->getHeadWord()) { 
			return true;
		}
	}
	return false;
}
		
bool DescLinkFeatureFunctions::hasModName(const Mention* ment) {
	std::vector<const Mention*> premodNames = DescLinkFeatureFunctions::getModNames(ment);
	return (!premodNames.empty());
}

std::set<Symbol> DescLinkFeatureFunctions::getMentMods(const Mention* ment) {
	std::set<Symbol> premods;
	std::vector<Symbol> tmp_premods = DescLinkFeatureFunctions::getMods(ment);
	BOOST_FOREACH(Symbol tmp_mod, tmp_premods) {
		wchar_t wordstr[300];
		//if (!_stopWords->lookup(premods[i])) {
			swprintf(wordstr, 300, L"ment[%ls]", tmp_mod.to_string());
			premods.insert(Symbol(wordstr));
		//}
	}
	return premods;
}

/* returns the last pronoun mention of an entity if such exist
*/
Symbol DescLinkFeatureFunctions::findEntLastPronoun(const Entity* ent, const EntitySet* ents) {
	for (int i = 0; i < ent->getNMentions(); i++) {
		Mention* ment = ents->getMention(ent->getMention(i));
		Symbol head = ment->getHead()->getHeadWord();
		if (WordConstants::isLinkingPronoun(head))
//		if (WordConstants::isLinkingPronounTal(head))
			return head;
	}//else
	return Symbol();
}

std::set<Symbol> DescLinkFeatureFunctions::getEntHeadWords(Entity* ent, const EntitySet* ents) {
	std::set<Symbol> results;
	wchar_t wordstr[300];
	for (int i = 0; i < ent->getNMentions(); i++) {
		Mention* ment = ents->getMention(ent->getMention(i));
		Symbol head = ment->getHead()->getHeadWord();
		swprintf(wordstr, 300, L"ent-head[%ls]", head.to_string());
		results.insert(Symbol(wordstr));
	}
	return results;
}

std::set<Symbol> DescLinkFeatureFunctions::getEntMods(Entity* ent, const EntitySet* ents) {
	std::set<Symbol> results;
	
	for (int i = 0; i < ent->getNMentions(); i++) {
		Mention* ment = ents->getMention(ent->getMention(i));
		std::vector<Symbol> premods = DescLinkFeatureFunctions::getMods(ment);
		
		BOOST_FOREACH(Symbol premod, premods) {
		// filter out stop words
			bool valid = true;
			//if (_stopWords->lookup(premods[k])) {
			//	valid = false;
			//	continue;
			//}
			if (valid) {
				wchar_t wordstr[300];
				swprintf(wordstr, 300, L"ent[%ls]", premod.to_string());
				results.insert(Symbol(wordstr));
			}
		}
	}
	return results;
}




// This method is necessary because Chinese Treebank NPs have more 
// structure than their English counterparts, so premod names can
// get buried inside other premod mentions
void DescLinkFeatureFunctions::getMentionInternalNames(const SynNode* node, 
												const MentionSet* ms,
												std::vector<const Mention*>& results) 
{
	if (!node->hasMention())
		return;

	Mention* ment = ms->getMentionByNode(node);

	if (ment->mentionType == Mention::NAME ||
	    ment->mentionType == Mention::NEST) 
	{
		results.push_back(ment);
	}
	else {
		for (int i = 0; i < node->getNChildren(); i++) {
			const SynNode* child = node->getChild(i);
			getMentionInternalNames(child, ms, results);
		}
	}
}

bool DescLinkFeatureFunctions::hasModNameEntityClash(const Mention* ment, Entity* ent, const EntitySet* ents) 
{
	
	if (ment == 0 || ent == 0) {
		return false;
	}

	std::vector<const Mention*> ment1PNames;
	std::vector<const Mention*> ment2PNames;
	std::vector<bool> matches;

	ment1PNames = DescLinkFeatureFunctions::getModNames(ment);

	if (ment1PNames.empty()) 
		return false;

	// Note: matches should be the same size as ment1PNames
	for (size_t l = 0; l < ment1PNames.size(); l++)
		matches.push_back(false);
	

	// for each existing DESC mention of ent, check to see if its premod names contain 
	// references to entities referenced by ment's premod names
	// for each existing NAME mention of ent, check to see if it matches any of
	// ment's premod names
	// i.e. ment cannot introduce any premod names not already associated with ent
	for (int k = 0; k < ent->mentions.length(); k++) {

		Mention* ment2 = ents->getMention(ent->mentions[k]);
		
		if (ment2->mentionType == Mention::NAME) {
			for (size_t i = 0; i < ment1PNames.size(); i++) {
				if (ment2->getNode()->getHeadWord() == ment1PNames[i]->getNode()->getHeadWord()) {
					matches[i] = true;
					break;
				}
			}
		}

		// only look for premods of other DESCs
		if (ment2->mentionType != Mention::DESC)
			continue;

		ment2PNames = DescLinkFeatureFunctions::getModNames(ment2);

		for (size_t i = 0; i < ment1PNames.size(); i++) {

			// search through all premod names of this mention to find matching entity
			// with ment's current premod name
			for (size_t j = 0; j < ment2PNames.size(); j++) {
			
				// are these mentions the same?
				if (ment2PNames[j]->getNode()->getHeadWord() == ment1PNames[i]->getNode()->getHeadWord()) {
					matches[i] = true;
					break;
				}
			}
		}
	}

	for (size_t m = 0; m < ment1PNames.size(); m++) {
		if (! matches[m])
			return true;
	}
	// couldn't find a clash
	return false;
}

bool DescLinkFeatureFunctions::hasModClash(const Mention* ment, Entity* ent, const EntitySet* ents) {
	if (ment == 0 || ent == 0)
		return false;

	std::vector<Symbol> ment1Premods = DescLinkFeatureFunctions::getMods(ment);
	if (ment1Premods.empty())
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
		std::vector<Symbol> ment2Premods = DescLinkFeatureFunctions::getMods(ment2);
		if (ment2Premods.empty())
			continue;
		// a clash exists if a word in ment2 isn't in ment1 AND vice versa
		bool firstHalfClash = false;
		for (size_t j = 0; j < ment2Premods.size(); j++) {
			bool foundMent = false;
			for (size_t i = 0; i < ment1Premods.size(); i++) {
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
			for (size_t i = 0; i < ment1Premods.size(); i++) {
				bool foundMent = false;
				for (size_t j = 0; j < ment2Premods.size(); j++) {
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

bool DescLinkFeatureFunctions::hasModMatch(const Mention* ment, Entity* ent, const EntitySet* ents) {
	if (ment == 0 || ent == 0)
		return false;

	std::vector<Symbol> commonPremods;
	std::vector<Symbol> ment1Premods = DescLinkFeatureFunctions::getMods(ment);
	if (ment1Premods.empty())
		return false;
	Symbol ment1Head = ment->getNode()->getHeadWord();
	// and now all the premods of each mention of the entity that shares a head
	for (int k = 0; k < ent->mentions.length(); k++) {
		Mention* ment2 = ents->getMention(ent->mentions[k]);
		// only look for premods of other DESCs
		if (ment2->mentionType != Mention::DESC)
			continue;
		// specifically, only DESCS with the same head word - JCS 3/8/04 - Why?
		//if (ment2->getNode()->getHeadWord() != ment1Head)
		//	continue;
		std::vector<Symbol> ment2Premods = DescLinkFeatureFunctions::getMods(ment2);
		if (ment2Premods.empty())
			continue;
		// a match exists if any word in ment2 is in ment1 
		for (size_t j = 0; j < ment2Premods.size(); j++) {
			for (size_t i = 0; i < ment1Premods.size(); i++) {
				if (ment1Premods[i] == ment2Premods[j]) {
					bool alreadySeen = false;
					for (size_t m = 0; m < commonPremods.size(); m++) {
						if (commonPremods[m] == ment1Premods[i]) {
							alreadySeen = true;
							break;
						}
					}
					if (!alreadySeen)
						commonPremods.push_back(ment1Premods[i]);
				}
			}			
		}
	}
	if (commonPremods.empty())
		return false;
	return true;
}

Symbol DescLinkFeatureFunctions::getNumEnts(const EntitySet *set) {
	int size = set->getNEntities();
	wchar_t wordstr[20];
	if (size <= 10)
		swprintf(wordstr, 20, L"numEnts[%d]", size);
	else
		swprintf(wordstr, 20, L"numEnts[%ls]", L">10");
	return Symbol(wordstr);
}


Symbol DescLinkFeatureFunctions::getNumEntsByType(EntityType type, const EntitySet *set) {
	int size = set->getNEntitiesByType(type);
	wchar_t wordstr[20];
	if (size <= 10)
		swprintf(wordstr, 20, L"numEnts[%d]", size);
	else
		swprintf(wordstr, 20, L"numEnts[%ls]", L">10");
	return Symbol(wordstr);
}

bool DescLinkFeatureFunctions::entityContainsOnlyNames(Entity *entity, const EntitySet *entitySet) {
	for (int k = 0; k < entity->mentions.length(); k++) {
		Mention* ment = entitySet->getMention(entity->mentions[k]);
		if (ment->mentionType != Mention::NAME)
			return false;
	}
	return true;
}

bool DescLinkFeatureFunctions::mentOverlapsEntity(const Mention *mention, Entity *entity, const EntitySet *entSet) {
	int start = mention->getNode()->getStartToken();
	int end = mention->getNode()->getEndToken();
	for (int k = 0; k < entity->mentions.length(); k++) {
		Mention* ment = entSet->getMention(entity->mentions[k]);
		if (ment->getSentenceNumber() == mention->getSentenceNumber()) {
			int ent_start = ment->getNode()->getStartToken();
			int ent_end = ment->getNode()->getEndToken();
			if (start == ent_start && end == ent_end)
				;// do nothing
			else if (start <= ent_start && end >= ent_start)
				return true;
			else if (start > ent_start && start <= ent_end)
				return true;
		}
	}
	return false;
}

bool DescLinkFeatureFunctions::mentIncludedInEntity(const Mention *mention, Entity *entity, const EntitySet *entSet) {
	int start = mention->getNode()->getStartToken();
	int end = mention->getNode()->getEndToken();
	for (int k = 0; k < entity->mentions.length(); k++) {
		Mention* ment = entSet->getMention(entity->mentions[k]);
		if (ment->getSentenceNumber() == mention->getSentenceNumber()) {
			int ent_start = ment->getNode()->getStartToken();
			int ent_end = ment->getNode()->getEndToken();
			if (start >= ent_start && end <= ent_end)
				return true;
		}
	}
	return false;
}

Symbol DescLinkFeatureFunctions::getClusterScore(const Mention *ment, Entity *ent, const EntitySet *entSet) {
	const int MAX_CLUSTERS = 4;
	int max_agreed = 0;
	bool ent_clusters_exist = 0;

	WordClusterClass mentionWordClass(ment->getNode()->getHeadWord(), true);
	if (mentionWordClass.c8() == 0)
		return Symbol(L"NO_CLUSTERS");

	for (int i = 0; i < ent->getNMentions(); i++) {
		Mention* m = entSet->getMention(ent->getMention(i));
		WordClusterClass entWordClass(m->getNode()->getHeadWord(), true);
		if (entWordClass.c8() == 0)
			continue;
		else ent_clusters_exist = true;
		int agreed = 0;
		if (entWordClass.c20() == mentionWordClass.c20())
			agreed = 20;
		else if (entWordClass.c16() == mentionWordClass.c16())
			agreed = 16;
		else if (entWordClass.c12() == mentionWordClass.c12())
			agreed = 12;
		else if (entWordClass.c8() == mentionWordClass.c8())
			agreed = 8;
		else agreed = 0;
		
		if (agreed > max_agreed)
			max_agreed = agreed;
	}

	Symbol score;
	if (max_agreed == 8)
		score = Symbol(L"clust-agree[<12]");
	else if (max_agreed == 12)
		score = Symbol(L"clust-agree[<16]");
	else if (max_agreed == 16)
		score = Symbol(L"clust-agree[<20]");
	else if (max_agreed == 20)
		score = Symbol(L"clust-agree[20+]");
	else score = Symbol(L"clust-agree[<8]");

	return score;
}

bool DescLinkFeatureFunctions::hasClusterMatch(const Mention *ment, Entity *ent, const EntitySet *entSet) {
	std::vector<Symbol> ment_clusters = getClusters(ment->getNode()->getHeadWord());
	for (int i = 0; i < ent->getNMentions(); i++) {
		Mention* m = entSet->getMention(ent->getMention(i));
		std::vector<Symbol> ent_clusters = getClusters(m->getNode()->getHeadWord());
		BOOST_FOREACH(Symbol ment_cluster, ment_clusters) {
			BOOST_FOREACH(Symbol ent_cluster, ent_clusters) {
				if (ment_cluster == ent_cluster)
					return true;
			}
		}
	}
	return false;
}

std::set<Symbol> DescLinkFeatureFunctions::findMentClusters(const Mention *ment) {
	std::set<Symbol> results;
	std::vector<Symbol> clusters = getClusters(ment->getNode()->getHeadWord());
	BOOST_FOREACH(Symbol cluster, clusters) {
		wchar_t wordstr[300];
		swprintf(wordstr, 300, L"ment-clust[%ls]", cluster.to_string());
		results.insert(Symbol(wordstr));
	}
	return results;
}

std::set<Symbol> DescLinkFeatureFunctions::findEntClusters(Entity* ent, const EntitySet* ents) {
	std::set<Symbol> results;
	wchar_t wordstr[300];
	for (int i = 0; i < ent->getNMentions(); i++) {
		Mention* ment = ents->getMention(ent->getMention(i));
		std::vector<Symbol> clusters = getClusters(ment->getNode()->getHeadWord());
		BOOST_FOREACH(Symbol cluster, clusters) {
			swprintf(wordstr, 300, L"ent-clust[%ls]", cluster.to_string());
			results.insert(Symbol(wordstr));
		}
	}
	return results;
}


std::vector<Symbol> DescLinkFeatureFunctions::getClusters(Symbol word) {
	std::vector<Symbol> results;
	wchar_t c[200];
	WordClusterClass wordClass(word, true);
	if (wordClass.c8() > 0) {
		swprintf(c, 200, L"c8:%d", wordClass.c8());
		results.push_back(Symbol(c));
	}
	if (wordClass.c12() > 0) {
		swprintf(c, 200, L"c12:%d", wordClass.c12());
		results.push_back(Symbol(c));
	}
	if (wordClass.c16() > 0) {
		swprintf(c, 200, L"c16:%d", wordClass.c16());
		results.push_back(Symbol(c));
	}
	if (wordClass.c20() > 0) {
		swprintf(c, 200, L"c20:%d", wordClass.c20());
		results.push_back(Symbol(c));
	}
	return results;
}

Symbol DescLinkFeatureFunctions::getCommonParent(const Mention* ment, Entity* ent, EntitySet* ents) {
	wchar_t wordstr[300];
	for (int i = 0; i < ent->getNMentions(); i++) {
		Mention* entMent = ents->getMention(ent->getMention(i));
		if (entMent->getSentenceNumber() == ment->getSentenceNumber()) {
			const SynNode *m_node = ment->getNode();
			const SynNode *e_node = entMent->getNode();
			while (m_node->getStartToken() > e_node->getStartToken() || 
				   m_node->getEndToken() < e_node->getEndToken())
			{
				m_node = m_node->getParent();
			}
			swprintf(wordstr, 300, L"pred[%ls]", m_node->getHeadWord().to_string());
			return Symbol(wordstr);
		}
	}
	swprintf(wordstr, 300, L"pred[]");
	return Symbol(wordstr);
}

std::vector<const Mention*> DescLinkFeatureFunctions::getGPEModNames(const Mention* ment) {
	std::vector<const Mention*> results;
	std::vector<const Mention*> nameresults = DescLinkFeatureFunctions::getModNames(ment);

	BOOST_FOREACH(const Mention *name, nameresults) {
		if (name->getEntityType().matchesGPE()) {
			results.push_back(name);
		}
	}
	return results;
}

std::vector<const Mention*> DescLinkFeatureFunctions::getNationalityModNames(const Mention* ment) {
	std::vector<const Mention*> results;
	std::vector<const Mention*> nameresults = DescLinkFeatureFunctions::getModNames(ment);

	BOOST_FOREACH(const Mention *name, nameresults) {
		if (NationalityRecognizer::isNationalityWord(name->getNode()->getHeadWord())){
			results.push_back(name);
		}
	}
	return results;
}

int DescLinkFeatureFunctions::getMentionStartToken(const Mention *m1){
	int start_ment1 = -1;
	if(m1->mentionType == Mention::NAME){
		if(m1->getHead() != 0){
			start_ment1 = m1->getHead()->getStartToken();
		}
		else{
			start_ment1 = m1->getNode()->getStartToken();
		}
	}
	else{
		if(m1->getHead() != 0){
			start_ment1 = m1->getHead()->getStartToken();
		}
		else if(m1->getNode() != 0){
			if(m1->getNode()->getHead() != 0){
				start_ment1 = m1->getNode()->getHead()->getStartToken();
			}
			else{
				start_ment1 = m1->getNode()->getStartToken();
			}
		}
	}
	return start_ment1;
}
int DescLinkFeatureFunctions::getMentionEndToken(const Mention *m1){
	int end_ment1 = -1;
	if(m1->mentionType == Mention::NAME){
		if(m1->getHead() != 0){
			end_ment1 = m1->getHead()->getEndToken();
		}
		else{
			end_ment1 = m1->getNode()->getEndToken();
		}
	}
	else{
		if(m1->getHead() != 0){
			end_ment1 = m1->getHead()->getEndToken();
		}
		else if(m1->getNode() != 0){
			if(m1->getNode()->getHead() != 0){
				end_ment1 = m1->getNode()->getHead()->getEndToken();
			}
			else{
				end_ment1 = m1->getNode()->getEndToken();
			}
		}
	}
	return end_ment1;
}
std::wstring DescLinkFeatureFunctions::addTextToPatternString(const SynNode *node, 
																	  const MentionSet* mentset,
																	  int& count, std::wstring str,
																	  int start_tok, int end_tok)
																	  
{
	if(node->getEndToken() < start_tok){
		return str;
	}
	if(node->getStartToken() > end_tok){
		return str;
	}
	if(node->hasMention()){
		Mention* ment = mentset->getMentionByNode(node);
		
		if(ment->getMentionType() == Mention::NAME){
				str += L"NAME:" ;
				str += ment->getEntityType().getName().to_string();
				str += L"_";
				count++;
				return str;
		}
		
		if(ment->getMentionType() ==Mention::DESC)
		{//get sub mentions <man> in the <company>
			if((ment->getEntityType() != EntityType::getUndetType()) &&
				(ment->getEntityType() != EntityType::getOtherType()))
			{
				std::wstring str2 = L"";
				const SynNode* descnode = ment->getNode();
				int i =0;
				//add premod txt
				while((i < descnode->getNChildren()) &&
					(descnode->getChild(i) != ment->getHead()))
				{
					str2= addTextToPatternString(descnode->getChild(i), mentset, count, str2, start_tok, end_tok);
					i++;
				}
				str2 +=L"DESC:";
				str2 += ment->getEntityType().getName().to_string();
				str2 += L"_";
				count++;
				i++;
				while(i < descnode->getNChildren()){
					str2 = addTextToPatternString(descnode->getChild(i), mentset, count, str2, start_tok, end_tok);
					i++;
				}
				str = str+ str2;
				
				return str;
			}
		}
	}
	if(node->isPreterminal()){
		str+= node->getTag().to_string();
		str+= L"_";
		count++;
		return str;
	}


	for(int i =0; i< node->getNChildren(); i++){
		str = addTextToPatternString(node->getChild(i), mentset, count, str, start_tok, end_tok);
	}
	return str;


}

Symbol DescLinkFeatureFunctions::getPatternSymbol(const MentionSet* mentset, const Mention *ment1, 
																const Mention* ment2)
{
	const SynNode* root = ment1->getNode();
	while(root->getParent() != 0){
		root = root->getParent();
	}
	int start1 = getMentionStartToken(ment1);
	int end1 = getMentionEndToken(ment1);
	int start2 = getMentionStartToken(ment2);
	int end2 = getMentionEndToken(ment2);
	if(end1 == (start2 -1)){
		return Symbol(L"ADJACENT");
	}
	

	const SynNode* top_node = root->getCoveringNodeFromTokenSpan(ment1->getNode()->getStartToken(), 
		ment2->getNode()->getEndToken());
	int count = 0;
	if((end1+1) == start2){
		return Symbol(L"ADJACENT");
	}
	std::wstring str = L"";
	str = addTextToPatternString(top_node, mentset, count, str, end1+1, start2-1 );
	if(count < 10){
		if(wcslen(str.c_str()) < 2){
			std::ostringstream ostr;
			ostr<<"found bad str: "<<Symbol(str.c_str()).to_debug_string()<<" "<<
				end1+1<<" "<<start2-1<<std::endl;
			ostr<<"Ment1: "<<start1<<" to "<<end1<<" ";
			ment1->dump(ostr);
			ostr<<std::endl;
			ment1->getNode()->dump(ostr);
			ostr<<std::endl;
			ostr<<"Ment2: "<<start2<<" to "<<end2<<" ";
			ment2->dump(ostr);
			ostr<<std::endl;
			ment2->getNode()->dump(ostr);
			ostr<<std::endl;
			SessionLogger::info("SERIF") << ostr.str();
		}
		return Symbol(str.c_str());
	}
	else{
		return Symbol(L"TOO_LONG");
	}
}
	






Symbol DescLinkFeatureFunctions::getPatternBetween(const MentionSet* mentset, 
														  const Mention* ment1, 
			   										      const Mention* ment2)
{
	if(ment1->getSentenceNumber() != ment2->getSentenceNumber())
	{
		return Symbol(L"DIFF_SENT");
	}
	int start1 = getMentionStartToken(ment1);
	int end1 = getMentionEndToken(ment1);
	int start2 = getMentionStartToken(ment2);
	int end2 = getMentionEndToken(ment2);
	if(start1 < start2){
		return getPatternSymbol(mentset, ment1, ment2);
	}
	else{
		return getPatternSymbol(mentset, ment2, ment1);
	}
}


Symbol DescLinkFeatureFunctions::checkCommonRelationsTypes(const Mention *ment, 
  											  const Entity *entity,
											  const EntitySet* ents,
											  DocumentRelMentionSet *docRelMentionSet)
{
	const Mention *left, *right;
	Symbol type;
	const int MAX_REL_TYPES=7;
	int n_entMentions = entity->getNMentions();
	MentionUID mentUID = ment->getUID();
	int n_mentionRelTypes = 0;
	int n_entityRelTypes = 0;
	Symbol entityRelTypes[MAX_REL_TYPES];
	Symbol mentionRelTypes[MAX_REL_TYPES];

	for (int k = 0; k < docRelMentionSet->getNRelMentions(); k++) {
		RelMention *rm = docRelMentionSet->getRelMention(k);
		left = rm->getLeftMention();//->getIndex();
		right = rm->getRightMention();//->getIndex();
		type = rm->getType();
		// collect types for the mention
		if(n_mentionRelTypes < MAX_REL_TYPES && (left->getUID() == mentUID || right->getUID() == mentUID)){
			// check for uniqueness
			bool newType = true;
			for(int i=0; i<n_mentionRelTypes; i++){
				if(mentionRelTypes[i]== type){
					newType = false;
					break;
				}
			}
			if(newType)
				mentionRelTypes[n_mentionRelTypes++] = type;
		}
		//collect types for the entity
		for(int m=0;m<entity->getNMentions(); m++){
			Mention* entMent = ents->getMention(entity->getMention(m)); 
			MentionUID entMentUID = entMent->getUID();
			if(n_entityRelTypes < MAX_REL_TYPES && (left->getUID() == entMentUID || right->getUID() == entMentUID)){
				// check for uniqueness
				bool newType = true;
				for(int i=0; i<n_entityRelTypes; i++){
					if(entityRelTypes[i]== type){
						newType = false;
						break;
					}
				}
				if(newType)
					entityRelTypes[n_entityRelTypes++] = type;
			}
		}
	}

	for(int i=0; i< n_mentionRelTypes ; i++){
		for(int j=0; j<n_entityRelTypes; j++){
			if(mentionRelTypes[i] == entityRelTypes[j])
				return type;
		}
	}
	return NONE_SYMBOL;
}

bool DescLinkFeatureFunctions::checkCommonRelationsTypesPosAnd2ndHW(
	const Mention *mention,	const Entity *entity, const EntitySet *entSet,
	DocumentRelMentionSet *docRelMentionSet,
	Symbol &type, Symbol &hw2, Symbol &pos)
{
	const Mention *left, *right;
	if(mention->getHead() == NULL)
		return false;
	for (int m = 0; m < docRelMentionSet->getNRelMentions(); m++) {
		bool isLeft=false, isRight=false;
		RelMention *rm = docRelMentionSet->getRelMention(m);
		left = rm->getLeftMention();
		right = rm->getRightMention();
		if(left->getUID()== mention->getUID())
			isLeft = true;
		if(right->getUID()== mention->getUID())
			isRight = true;

		const Mention *mentSecondMention = isLeft ? right : left;
		Entity *mentSecondEntity = entSet->getEntityByMention(mentSecondMention->getUID(),mentSecondMention->getEntityType());
		if(mentSecondEntity==NULL)// didn't find the entity in the set
			continue;

		if(!isLeft && !isRight) // could not match the mention to the relation
			continue;
		for(int k=0; k<docRelMentionSet->getNRelMentions(); k++){
			RelMention *rm2 = docRelMentionSet->getRelMention(k);
			if(rm->getType()==rm2->getType() && rm != rm2){ // matched the type of the first relation
				for(int m=0;m<entity->getNMentions(); m++){
					const Mention *rm2Ment = isLeft ? rm2->getLeftMention() : rm2->getRightMention();
					Mention* entMent = entSet->getMention(entity->getMention(m)); 
					// the mention and entity share the same relation type and pos
					if(rm2Ment->getUID() == entMent->getUID()){ // check the entity mention is in this relation
						const Mention *otherMention = isLeft ? rm2->getRightMention() : rm2->getLeftMention();
						Entity *entSecondEntity = entSet->getEntityByMention(otherMention->getUID(),otherMention->getEntityType());
						if(otherMention->getHead() == NULL)
							continue;
						pos = isLeft ? LEFT : RIGHT;
						type = rm->getType();
						hw2 = otherMention->getHead()->getHeadWord();
						if(hw2 == mention->getHead()->getHeadWord()
							|| (entSecondEntity != NULL && mentSecondEntity->getID() == entSecondEntity->getID()))
						{
							return true;
						}
					}
				}
			}
		}
	}
    return false;
}						  

bool DescLinkFeatureFunctions::checkEntityHasHW(const Entity *entity,
													   const EntitySet *entSet, const Symbol hw)
{
	for(int i=0; i<entity->getNMentions(); i++){
		Mention* ment = entSet->getMention(entity->getMention(i));
		if(hw == ment->getHead()->getHeadWord())
			return true;
	}
	return false;
}

bool DescLinkFeatureFunctions::mentOverlapInAnotherMention(const Mention *mention, const MentionSet *mentSet) {
	int start = mention->getNode()->getStartToken();
	int end = mention->getNode()->getEndToken();
	for (int k = 0; k < mentSet->getNMentions(); k++) {
		Mention* ment = mentSet->getMention(k);
		if(ment->getUID() == mention->getUID())
			continue;
		if (ment->getSentenceNumber() == mention->getSentenceNumber()) {
			int other_start = ment->getNode()->getStartToken();
			int other_end = ment->getNode()->getEndToken();
			if (start == other_start && end == other_end)
				;// do nothing
			else if (start <= other_start && end >= other_start)
				return true;
			else if (start > other_start && start <= other_end)
				return true;
		}
	}
	return false;
}

void DescLinkFeatureFunctions::loadNationalities() {
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		std::string filename = ParamReader::getRequiredParam("dt_coref_nationalities");
		
		boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& uis(*uis_scoped_ptr);
		uis.open(filename.c_str());

		int line_count = 0;	
		while (!uis.eof()) {
			Sexp *line = _new Sexp(uis);
			if (line->isVoid()) break;
			line_count++;
			if(line->getNumChildren() != 2) {
				SessionLogger::warn("desc_link_feature_functions")<<"invalid line in the nationalities file: line: "<<line_count<<std::endl;
				continue;
			}
			Sexp* s_key = line->getFirstChild();
			if ( s_key->isVoid() ) continue;

			std::wstring name = L"";
			for (int i = 0; i < s_key->getNumChildren(); i++) {
				if (i > 0)
					name += L" ";
				name += s_key->getNthChild(i)->to_string();
			}
			std::vector<std::wstring> affiliated_gpes;
			Sexp* s_values = line->getSecondChild();
			std::wstring gpe = L"";
			for (int i = 0; i < s_values->getNumChildren(); i++) {
				if (i > 0)
					gpe += L" ";
				gpe += s_values->getNthChild(i)->to_string();
			}
			_nationalities[name] = gpe;
			delete line;
		}
		uis.close();
	}
}

void DescLinkFeatureFunctions::loadNameGPEAffiliations() {
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		std::string filename = ParamReader::getRequiredParam("dt_coref_name_gpe_affiliations");

		boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& uis(*uis_scoped_ptr);
		uis.open(filename.c_str());

		int line_count = 0;	
		while (!uis.eof()) {
			Sexp *line = _new Sexp(uis);
			if (line->isVoid()) break;
			line_count++;
			if(line->getNumChildren() != 2) {
				SessionLogger::warn("desc_link_feature_functions")<<"invalid line in the name-gpe affiliations file: line: "<<line_count<<std::endl;
				continue;
			}
			Sexp* s_key = line->getFirstChild();
			if ( s_key->isVoid() ) continue;

			std::wstring name = L"";
			for (int i = 0; i < s_key->getNumChildren(); i++) {
				if (i > 0)
					name += L" ";
				name += s_key->getNthChild(i)->to_string();
			}
			std::vector<std::wstring> affiliated_gpes;
			Sexp* s_values = line->getSecondChild();
			for (int i = 0; i < s_values->getNumChildren(); i++) {
				Sexp * s_gpe = s_values->getNthChild(i);
				std::wstring gpe = L"";
				for (int j = 0; j < s_gpe->getNumChildren(); j++) {
					if (j > 0)
						gpe += L" ";
					gpe += s_gpe->getNthChild(j)->to_string();
				}
				affiliated_gpes.push_back(gpe);
			}
			_name_gpe_affiliations[name] = affiliated_gpes;
			delete line;
		}
		uis.close();
	}
}

bool DescLinkFeatureFunctions::globalGPEAffiliationClash(const DocTheory *docTheory, EntitySet *currSolution, Entity *entity, Mention *mention) {
	if (!entity->getType().matchesPER() || !mention->getEntityType().matchesPER())
		return false;
	
	std::set<std::wstring> mentionGPEmods;	
	std::set<std::wstring> matchedMentionGPEmods;	

	// Collect all GPE modifiers (premod, possessive, "of") of this person
	// If we know the affiliation of the person in the entity, then each of these GPE modifiers must match
	//   that affiliation.
	// It's not totally clear to me whether this could ever suceeed if there is more than one modifier, but
	//   this is rare anyway. Probably we wouldn't want these to match, they would be things like 
	//   "French-American team"
	getGPEModNames(mention, docTheory, mentionGPEmods);

	if (mentionGPEmods.size() == 0) 
		return false;

	// Find any name-gpe affiliations in the entity
	bool found_name = false;
	for (int i = 0; i < entity->getNMentions(); i++) {
		Mention * entity_mention = currSolution->getMention(entity->getMention(i));
		std::vector<std::wstring> gpe_names = getGPEAffiliations(entity_mention, docTheory);
		if (!gpe_names.empty())
			found_name = true;
		BOOST_FOREACH(std::wstring mod, mentionGPEmods) {
			if (std::find(gpe_names.begin(), gpe_names.end(), mod) != gpe_names.end()) {
				matchedMentionGPEmods.insert(mod);
			}
		}
	}
	if (found_name && mentionGPEmods.size() != matchedMentionGPEmods.size())
		return true;

	return false;
}

std::vector<std::wstring> DescLinkFeatureFunctions::getGPEAffiliations(const Mention *mention, const DocTheory *docTheory) {
	std::vector<std::wstring> results;
	
	if (mention->getMentionType() != Mention::NAME)
			return results;

	std::wstring name_ment_str = L"";
	TokenSequence *ts = docTheory->getSentenceTheory(mention->getSentenceNumber())->getTokenSequence();
	for (int k = mention->getNode()->getStartToken(); k <= mention->getNode()->getEndToken(); k++) {
		if (k > mention->getNode()->getStartToken())
			name_ment_str += L" ";
		name_ment_str += ts->getToken(k)->getSymbol().to_string();
	}
	std::transform(name_ment_str.begin(), name_ment_str.end(), name_ment_str.begin(), towlower);
	// Find the name in the name-gpe affiliations list
	std::map<std::wstring, std::vector<std::wstring> >::const_iterator iter = _name_gpe_affiliations.find(name_ment_str);
	if (iter == _name_gpe_affiliations.end())
		return results;
	
	results.insert(results.end(), (iter->second).begin(), (iter->second).end());
	return results;
}

bool DescLinkFeatureFunctions::localGPEAffiliationClash(const Mention* ment, const Entity* ent, const EntitySet* ents, const DocTheory *docTheory) {

	// Return true if mention has GPE mods and entity has GPE mods and not all
	//   of the mention's GPE mods are covered in the entity

	// too dangerous without nationalities defined?
	if (_nationalities.size() == 0)
		return false;

	if (ment == 0 || ent == 0)
		return false;

	// Don't want to apply this to GPEs
	if (ment->getEntityType().matchesGPE())
		return false;

	// Get all GPE modifiers for our mention, names only
	std::set<std::wstring> mentionMods;	
	getGPEModNames(ment, docTheory, mentionMods);
	if (mentionMods.size() == 0) 
		return false;

	// Get all GPE modifiers for the mentions of the entity we want to link to, names only
	std::set<std::wstring> entityMods;	
	for (int k = 0; k < ent->mentions.length(); k++) {
		Mention* ment2 = ents->getMention(ent->mentions[k]);
		if (ment2->mentionType == Mention::DESC)
			getGPEModNames(ment2, docTheory, entityMods);
	}

	if (entityMods.size() == 0)
		return false;

	// If _both_ have GPE modifiers, then every one of the mention's modifiers
	//   must be found as a modifier to the entity as well
	BOOST_FOREACH(std::wstring name, mentionMods) {		
		if (entityMods.find(name) == entityMods.end()) {
			return true;
		}
	}
	return false;
}

bool DescLinkFeatureFunctions::hasHeadwordClash(const Mention* ment, const Entity* ent, const EntitySet* ents, const DocTheory *docTheory) {

	// Return true if descriptor mention has a headword that clashes with one in the entity

	if (ment == 0 || ent == 0)
		return false;

	// Only descriptors and partitives
	if (ment->getMentionType() != Mention::DESC && ment->getMentionType() != Mention::PART)
		return false;

	// Get all headwords for the entity
	std::set<Symbol> headwords;	
	for (int k = 0; k < ent->mentions.length(); k++) {
		Mention* ment2 = ents->getMention(ent->mentions[k]);
		if (ment2->mentionType == Mention::DESC || ment2->mentionType == Mention::PART)
			headwords.insert(ment2->getNode()->getHeadWord());
	}

	// If there are some headwords in the entity and ours doesn't match, we have a clash
	if (headwords.size() != 0 && headwords.find(ment->getNode()->getHeadWord()) == headwords.end())
		return true;

	return false;
}


void DescLinkFeatureFunctions::getGPEModNames(const Mention* ment, const DocTheory *docTheory, std::set<std::wstring>& results)  {
	PropositionSet * ps = docTheory->getSentenceTheory(ment->getSentenceNumber())->getPropositionSet();
	MentionSet * ms = docTheory->getSentenceTheory(ment->getSentenceNumber())->getMentionSet();
	TokenSequence * ts = docTheory->getSentenceTheory(ment->getSentenceNumber())->getTokenSequence();

	// We use two methods here. One is derived from Alex Baron's global GPE affiliation code, which
	//    uses propositions and covers premods, possessives, and "of" attachments. The other calls
	//    a function in DescLinkFeatureFunctions that gets SynNodes that are named premodifiers; this
	//    one does not require propositions, which is important for languages other than English.

	std::vector<const Mention *> modMentions;

	// Proposition-based method
	for (int i = 0; i < ps->getNPropositions(); i++) {
		Proposition * prop = ps->getProposition(i);
		const Mention * ref_mention = prop->getMentionOfRole(Argument::REF_ROLE, ms);
		if (ref_mention == 0 || ref_mention->getUID() != ment->getUID())
			continue;
		for (int j = 0; j < prop->getNArgs(); j++) {
			Argument * arg = prop->getArg(j);
			if (arg->getType() != Argument::MENTION_ARG)
				continue;
			// NOTE: This is against policy because it uses an English Symbol "of" in generic code
			if (arg->getRoleSym() == Argument::POSS_ROLE || arg->getRoleSym() == Argument::UNKNOWN_ROLE || arg->getRoleSym() == Symbol(L"of")) {
				modMentions.push_back(arg->getMention(ms));
			}
		}
	}

	// SynNode-based method (will overlap with the above method significantly)
	std::vector<const Mention *> synMentions = DescLinkFeatureFunctions::getModNames(ment);
	modMentions.insert(modMentions.end(), synMentions.begin(), synMentions.end());

	BOOST_FOREACH(const Mention *modMent, modMentions) {
		if (!modMent->getEntityType().matchesGPE() && !modMent->getEntityType().matchesPER())
			continue;
		std::wstring mention_text = modMent->getAtomicHead()->toTextString();
		boost::trim(mention_text);
		std::transform(mention_text.begin(), mention_text.end(), mention_text.begin(), towlower);		
		std::map<std::wstring, std::wstring>::const_iterator nation_iter = _nationalities.find(mention_text);
		if (nation_iter != _nationalities.end())
			mention_text = nation_iter->second;
		else if (!modMent->getEntityType().matchesGPE() || modMent->getMentionType() != Mention::NAME)
			continue;
		results.insert(mention_text);
	}
}

boost::shared_ptr<DescLinkFeatureFunctions::DescLinkFeatureFunctionsInstance> &DescLinkFeatureFunctions::getInstance() {
    static boost::shared_ptr<DescLinkFeatureFunctionsInstance> instance(_new DescLinkFeatureFunctionsInstanceFor<GenericDescLinkFeatureFunctions>());
    return instance;
}

