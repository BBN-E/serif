// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/ParamReader.h"
#include "Spanish/edt/es_DescLinkFunctions.h"
#include "Spanish/parse/es_STags.h"
#include "Generic/edt/StatDescLinker.h"
#include "Generic/theories/SynNode.h"
#include <boost/scoped_ptr.hpp>
//#include "Generic/wordClustering/WordClusterClass.h"
//#include "Generic/wordClustering/WordClusterTable.h"
#define   swprintf   _snwprintf

SpanishDescLinkFunctions::WordClusterTable *SpanishDescLinkFunctions::_clusterTable = 0;
SymbolHash *SpanishDescLinkFunctions::_stopWords = 0;
const int initialTableSize = 16000;
const int numClusters = 4;

void SpanishDescLinkFunctions::getEventContext(EntitySet* entSet, Mention* mention,
										Entity* entity, OldMaxEntEvent* evt, int maxSize)
{
	const int MAX_RESULTS = 50;

	if (_clusterTable == 0) {
		_clusterTable = _new WordClusterTable(initialTableSize);
		initializeWordClusterTable();
	}
	//if (!WordClusterTable::isInitialized())
	//	initializeWordClusterTable();

	if (_stopWords == 0) {
		_stopWords = _new SymbolHash(initialTableSize);
		initializeStopWords();
	}

	const Symbol HEAD_WORD_MATCH = Symbol(L"HeadWordMatch");
	const Symbol HEAD_WORD_NODE_MATCH = Symbol(L"HeadWordNodeMatch");
	const Symbol EXACT_STRING_MATCH = Symbol(L"ExactStringMatch");
	const Symbol MENT_NUMERIC = Symbol(L"numericPremod");
	const Symbol ENT_NUMERIC = Symbol(L"prevNumeric");
	const Symbol MENT_NOT_NUMERIC = Symbol(L"noNumericPremod");
	const Symbol ENT_NOT_NUMERIC = Symbol(L"noPrevNumeric");
	const Symbol NUMERIC_CLASH = Symbol(L"numericClash");
	const Symbol NUMERIC_MATCH = Symbol(L"numericMatch");
	const Symbol PREMOD_NAME = Symbol(L"premodName");
	const Symbol PREMOD_NAME_CLASH = Symbol(L"premodNameClash");
	const Symbol PREMOD_NAME_MATCH = Symbol(L"premodNameMatch");
	const Symbol PREMOD_MATCH = Symbol(L"premodMatch");
	const Symbol PREMOD_CLASH = Symbol(L"premodClash");
	const Symbol ENT_ONLY_NAMES = Symbol(L"entityOnlyNames");
	const Symbol OVERLAPPING = Symbol(L"Overlapping");
	const Symbol NO_PREMOD = Symbol(L"mentHasNoPremods");
	const Symbol LAST_HANZI_MATCH = Symbol(L"lastHanziMatch");
	const Symbol CLUSTER_MATCH = Symbol(L"clusterMatch");

	Symbol *ngram = _new Symbol[MAX_RESULTS];
	Symbol *ment_premods = _new Symbol[MAX_RESULTS];
	Symbol *ent_headwords = _new Symbol[MAX_RESULTS];
	Symbol *ent_premods = _new Symbol[MAX_RESULTS];
	Symbol *ent_parents = _new Symbol[MAX_RESULTS];
	Symbol *ment_clusters = _new Symbol[MAX_RESULTS];
	Symbol *ent_clusters = _new Symbol[MAX_RESULTS];

	Symbol type = mention->getEntityType().getName();
	Symbol headWord = _findMentHeadWord(mention);
	Symbol numEnts = _getNumEnts(entity->getType(), entSet);
	Symbol uh_ratio = _getUniqueHanziRatio(mention, entity, entSet);
	Symbol um_ratio = _getUniqueModifierRatio(mention, entity, entSet);
	Symbol distance = _getDistance(mention->getSentenceNumber(), entity, entSet);
	Symbol cluster_score = _getClusterScore(mention, entity, entSet);

	bool hw_match = _testHeadWordMatch(mention, entity, entSet);
	bool hwn_match = (hw_match && _testHeadWordNodeMatch(mention, entity, entSet));
	bool es_match = _exactStringMatch(mention, entity, entSet);
	bool hanzi_match = _lastHanziMatch(mention, entity, entSet);

	int n_ment_premods = _findMentPremods(mention, ment_premods, MAX_RESULTS);
	int n_ent_headwords = _findEntHeadWords(entity, entSet, ent_headwords, MAX_RESULTS);
	int n_ent_premods = _findEntPremods(entity, entSet, ent_premods, MAX_RESULTS);
	int n_ment_clusters = _findMentClusters(mention, ment_clusters, MAX_RESULTS);
	int n_ent_clusters = _findEntClusters(entity, entSet, ent_clusters, MAX_RESULTS);

	bool has_premod = (n_ment_premods > 0);
	bool premod_clash = (has_premod && _hasPremodClash(mention, entity, entSet));
	bool premod_match = (has_premod && _hasPremodMatch(mention, entity, entSet));
	bool premod_name = _hasPremodName(mention, entSet);
	bool premod_name_clash = (premod_name && _hasPremodNameEntityClash(mention, entity, entSet));
	bool premod_name_match = (premod_name && !premod_name_clash);
	bool ent_only_names = _entityContainsOnlyNames(entity, entSet);
	bool overlapping = _mentOverlapsEntity(mention, entity, entSet);


	Symbol mentParent = _findMentParent(mention);
	int n_ent_parents = _findEntParents(entity, entSet, ent_parents, MAX_RESULTS);

	bool ment_numeric = _mentHasNumeric(mention);
	bool ent_numeric = _entHasNumeric(entity, entSet);
	bool numeric_clash = (ment_numeric && ent_numeric && _hasNumericClash(mention, entity, entSet));
	bool numeric_match = (ment_numeric && ent_numeric && _hasNumericMatch(mention, entity, entSet));

	evt->addAtomicContextPredicate(type);
	//evt->addAtomicContextPredicate(mentParent);
	evt->addAtomicContextPredicate(numEnts);
	evt->addAtomicContextPredicate(um_ratio);
	evt->addAtomicContextPredicate(uh_ratio);
	evt->addAtomicContextPredicate(distance);
	evt->addAtomicContextPredicate(cluster_score);
	if (hw_match)
		evt->addAtomicContextPredicate(HEAD_WORD_MATCH);
	if (hwn_match)
		evt->addAtomicContextPredicate(HEAD_WORD_NODE_MATCH);
	if (es_match)
		evt->addAtomicContextPredicate(EXACT_STRING_MATCH);
	//if (cluster_match)
	//	evt->addAtomicContextPredicate(CLUSTER_MATCH);
	if (ment_numeric)
		evt->addAtomicContextPredicate(MENT_NUMERIC);
	if (ent_numeric)
		evt->addAtomicContextPredicate(ENT_NUMERIC);
	if (numeric_clash)
		evt->addAtomicContextPredicate(NUMERIC_CLASH);
	if (numeric_match)
		evt->addAtomicContextPredicate(NUMERIC_MATCH);
	if (!has_premod)
		evt->addAtomicContextPredicate(NO_PREMOD);
	if (premod_name)
		evt->addAtomicContextPredicate(PREMOD_NAME);
	if (premod_name_clash)
		evt->addAtomicContextPredicate(PREMOD_NAME_CLASH);
	if (premod_name_match)
		evt->addAtomicContextPredicate(PREMOD_NAME_MATCH);
	if (premod_match)
		evt->addAtomicContextPredicate(PREMOD_MATCH);
	if (premod_clash)
		evt->addAtomicContextPredicate(PREMOD_CLASH);
	if (ent_only_names)
		evt->addAtomicContextPredicate(ENT_ONLY_NAMES);
	if (overlapping)
		evt->addAtomicContextPredicate(OVERLAPPING);
	if (hanzi_match)
		evt->addAtomicContextPredicate(LAST_HANZI_MATCH);
	int i;
	for (i = 0; i < n_ment_premods; i++)
		evt->addAtomicContextPredicate(ment_premods[i]);
	for (i = 0; i < n_ent_premods; i++)
		evt->addAtomicContextPredicate(ent_premods[i]);
	//for (i = 0; i < n_ent_parents; i++)
	//	evt->addAtomicContextPredicate(ent_parents[i]);

	// type, numEnts, distance
	/*ngram[0] = type;
	ngram[1] = numEnts;
	ngram[2] = distance;
	evt->addComplexContextPredicate(3, ngram);*/
	// head word match, numeric clash
	if (hw_match && numeric_clash) {
		ngram[0] = HEAD_WORD_MATCH;
		ngram[1] = NUMERIC_CLASH;
		evt->addComplexContextPredicate(2, ngram);
	}
	// head word node match, numeric clash
	if (hwn_match && numeric_clash) {
		ngram[0] = HEAD_WORD_NODE_MATCH;
		ngram[1] = NUMERIC_CLASH;
		evt->addComplexContextPredicate(2, ngram);
	}
	// head word match, numeric match
	if (hw_match && numeric_match) {
		ngram[0] = HEAD_WORD_MATCH;
		ngram[1] = NUMERIC_MATCH;
		evt->addComplexContextPredicate(2, ngram);
	}
	// head word node match, numeric match
	if (hwn_match && numeric_match) {
		ngram[0] = HEAD_WORD_NODE_MATCH;
		ngram[1] = NUMERIC_MATCH;
		evt->addComplexContextPredicate(2, ngram);
	}
	// head word match, premod name clash
	if (hw_match && premod_name_clash) {
		ngram[0] = HEAD_WORD_MATCH;
		ngram[1] = PREMOD_NAME_CLASH;
		evt->addComplexContextPredicate(2, ngram);
	}
	// head word match, premod name match
	if (hw_match && premod_name_match) {
		ngram[0] = HEAD_WORD_MATCH;
		ngram[1] = PREMOD_NAME_MATCH;
		evt->addComplexContextPredicate(2, ngram);
	}
	// head word node match, premod name clash
	if (hwn_match && numeric_clash) {
		ngram[0] = HEAD_WORD_NODE_MATCH;
		ngram[1] = PREMOD_NAME_CLASH;
		evt->addComplexContextPredicate(2, ngram);
	}
	// head word node match, premod name match
	if (hwn_match && premod_name_match) {
		ngram[0] = HEAD_WORD_NODE_MATCH;
		ngram[1] = PREMOD_NAME_MATCH;
		evt->addComplexContextPredicate(2, ngram);
	}
	// premod clash and match
	if (premod_clash && premod_match) {
		ngram[0] = PREMOD_CLASH;
		ngram[1] = PREMOD_MATCH;
		evt->addComplexContextPredicate(2, ngram);
	}
	// premod clash, unique modifier ratio
	if (premod_clash) {
		ngram[0] = PREMOD_CLASH;
		ngram[1] = um_ratio;
		evt->addComplexContextPredicate(2, ngram);
	}
	// head word node match, unique modifier ratio
	if (hwn_match) {
		ngram[0] = HEAD_WORD_NODE_MATCH;
		ngram[1] = um_ratio;
		evt->addComplexContextPredicate(2, ngram);
	}
	if (ent_numeric && ment_numeric) {
		ngram[0] = MENT_NUMERIC;
		ngram[1] = ENT_NUMERIC;
		evt->addComplexContextPredicate(2, ngram);
		if (numeric_clash) {
			ngram[2] = NUMERIC_CLASH;
			evt->addComplexContextPredicate(3, ngram);
		}
		if (numeric_match) {
			ngram[2] = NUMERIC_MATCH;
			evt->addComplexContextPredicate(3, ngram);
		}
		if (numeric_clash && numeric_match) {
			ngram[2] = NUMERIC_MATCH;
			ngram[3] = NUMERIC_CLASH;
			evt->addComplexContextPredicate(4, ngram);
		}
	}
	else if (ent_numeric && !ment_numeric) {
		ngram[0] = ENT_NUMERIC;
		ngram[1] = MENT_NOT_NUMERIC;
		evt->addComplexContextPredicate(2, ngram);
	}
	else if (!ent_numeric && ment_numeric) {
		ngram[0] = ENT_NOT_NUMERIC;
		ngram[1] = MENT_NUMERIC;
		evt->addComplexContextPredicate(2, ngram);
	}
	if (ent_only_names) {
		ngram[0] = ENT_ONLY_NAMES;
		ngram[1] = type;
		evt->addComplexContextPredicate(2, ngram);
		/*ngram[2] = distance;
		evt->addComplexContextPredicate(3, ngram);
		ngram[2] = numEnts;
		evt->addComplexContextPredicate(3, ngram);
		ngram[3] = distance;
		evt->addComplexContextPredicate(4, ngram);*/
		ngram[1] = um_ratio;
		evt->addComplexContextPredicate(2, ngram);
		if (premod_clash) {
			ngram[1] = PREMOD_CLASH;
			evt->addComplexContextPredicate(2, ngram);
		}
		if (premod_name_clash) {
			ngram[1] = PREMOD_NAME_CLASH;
			evt->addComplexContextPredicate(2, ngram);
		}
		if (premod_name_match) {
			ngram[1] = PREMOD_NAME_MATCH;
			evt->addComplexContextPredicate(2, ngram);
		}
		if (!has_premod) {
			ngram[1] = NO_PREMOD;
			evt->addComplexContextPredicate(2, ngram);
		}
	}
	// ment head word with each ent head word
	ngram[0] = headWord;
	for (i = 0; i < n_ent_headwords; i++) {
		ngram[1] = ent_headwords[i];
		evt->addComplexContextPredicate(2, ngram);
	}
	// ment head word with each ent premod
	for (i = 0; i < n_ent_premods; i++) {
		ngram[1] = ent_premods[i];
		evt->addComplexContextPredicate(2, ngram);
	}
	// each ment premod with each ent premod
	for (i = 0; i < n_ment_premods; i++) {
		ngram[0] = ment_premods[i];
		for (int j = 0; j  < n_ent_premods; j++) {
			ngram[1] = ent_premods[j];
			evt->addComplexContextPredicate(2, ngram);
		}
	}
	// each ment cluster with each ent cluster
	for (i = 0; i < n_ment_clusters; i++) {
		ngram[0] = ment_clusters[i];
		for (int j = 0; j < n_ent_clusters; j++) {
			ngram[1] = ent_clusters[j];
			evt->addComplexContextPredicate(2, ngram);
		}
	}
	// common parent if they're in the same sentence
	/*if (distance == Symbol(L"D0")) {
		ngram[0] = distance;
		ngram[1] = _getCommonParent(mention, entity, entSet);
		evt->addComplexContextPredicate(2, ngram);
	}*/

	delete [] ngram;
	delete [] ment_premods;
	delete [] ent_premods;
	delete [] ent_headwords;
	delete [] ent_parents;
	delete [] ment_clusters;
	delete [] ent_clusters;

	return;
}

Symbol SpanishDescLinkFunctions::_findMentHeadWord(Mention *ment) {
	Symbol headWord = ment->getNode()->getHeadWord();
	wchar_t wordstr[300];
	swprintf(wordstr, 300, L"ment-head[%ls]", headWord.to_string());
	return Symbol(wordstr);
}

bool SpanishDescLinkFunctions::_testHeadWordMatch(Mention *ment, Entity *entity, EntitySet *entitySet) {
	Symbol headWord = ment->getNode()->getHeadWord();
	for (int i = 0; i < entity->getNMentions(); i++) {
		Mention* entMent = entitySet->getMention(entity->getMention(i));
		Symbol entHeadWord = entMent->getNode()->getHeadWord();
		if (headWord == entHeadWord)
			return true;
	}
	return false;
}

bool SpanishDescLinkFunctions::_testHeadWordNodeMatch(Mention *ment, Entity *entity, EntitySet *entitySet) {
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

bool SpanishDescLinkFunctions::_lastHanziMatch(Mention *ment, Entity *entity, EntitySet *entitySet) {
	const wchar_t *ment_head = ment->getNode()->getHeadWord().to_string();
	const wchar_t *ment_hanzi = ment_head + wcslen(ment_head) - 1;
	for (int i = 0; i < entity->getNMentions(); i++) {
		Mention* entMent = entitySet->getMention(entity->getMention(i));
		const wchar_t *ent_head = entMent->getNode()->getHeadWord().to_string();
		const wchar_t *ent_hanzi = ent_head + wcslen(ent_head) - 1;
		if (*ment_hanzi == *ent_hanzi)
			return true;
	}
	return false;
}

// Note: should probably store the words for each entity (and mention?), so we don't have to keep
// recalculating.  Also should add stop words.
Symbol SpanishDescLinkFunctions::_getUniqueHanziRatio(Mention *ment, Entity *entity, EntitySet *entitySet) {
	SymbolHash entityHanzi(150);
	int n_ent_hanzi = 0;
	int unique = 0;
	int total = 0;
	wchar_t str[2];
	str[1] = '\0';

	// collect Entity hanzi
	Symbol entityMentWords[20];
	for (int i = 0; i < entity->getNMentions(); i++) {
		Mention *entityMent = entitySet->getMention(entity->getMention(i));
		int n_words = entityMent->getNode()->getTerminalSymbols(entityMentWords, 20);
		for (int j = 0; j < n_words; j++) {
			const wchar_t *ent_word = entityMentWords[j].to_string();
			int len = static_cast<int>(wcslen(ent_word));
			for (int k = 0; k < len; k++) {
				str[0] = *(ent_word + k);
				Symbol hanzi(str);
				if (! entityHanzi.lookup(hanzi)) {
					entityHanzi.add(hanzi);
					n_ent_hanzi++;
				}
			}
		}
	}

	// collect Mention words
	Symbol mentionWords[20];
	int n_ment_words = ment->getNode()->getTerminalSymbols(mentionWords, 20);

	if (n_ment_words == 0)
		return Symbol(L"uh[0.0]");

	if (n_ent_hanzi == 0)
		return Symbol(L"uh[1.0]");

	for (int k = 0; k < n_ment_words; k++) {
		const wchar_t *ment_word = mentionWords[k].to_string();
		int len = static_cast<int>(wcslen(ment_word));
		for (int l = 0; l < len; l++) {
			str[0] = *(ment_word + l);
			Symbol hanzi(str);
			if (! entityHanzi.lookup(hanzi))
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

// Note: should probably store the modifiers for each entity (and mention?), so we don't have to keep
// recalculating.  Also should add stop words.
Symbol SpanishDescLinkFunctions::_getUniqueModifierRatio(Mention *ment, Entity *entity, EntitySet *entitySet) {
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
			if (_stopWords->lookup(entityMentWords[j]))
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
		if (_stopWords->lookup(mentionWords[k]))
			continue;
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


Symbol SpanishDescLinkFunctions::_getDistance(int sent_num, Entity *entity, EntitySet *entitySet) {
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


bool SpanishDescLinkFunctions::_exactStringMatch(Mention *ment, Entity *entity, EntitySet *entitySet) {
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

int SpanishDescLinkFunctions::_getHeadNPWords(const SynNode* node, Symbol results[], int max_results) {
	if (node->isPreterminal() || node->isTerminal())
		return node->getTerminalSymbols(results, max_results);
	while (!node->getHead()->isPreterminal()) {
		node = node->getHead();
	}
	return node->getTerminalSymbols(results, max_results);
}

bool SpanishDescLinkFunctions::_entHasNumeric(Entity *ent, EntitySet *entitySet) {
	for (int i = 0; i < ent->getNMentions(); i++) {
		Mention *prev = entitySet->getMention(ent->getMention(i));
		const SynNode *num_node = _getNumericPremod(prev);
		if (num_node != NULL)
			return true;
	}
	return false;
}

bool SpanishDescLinkFunctions::_mentHasNumeric(Mention *ment) {
	const SynNode* this_num = _getNumericPremod(ment);
	if (this_num != NULL)
		return true;
	else
		return false;
}


bool SpanishDescLinkFunctions::_hasNumericClash(Mention *ment, Entity *ent, EntitySet *entitySet)
{
	GrowableArray<const SynNode*> numerics;
	for (int i = 0; i < ent->getNMentions(); i++) {
		Mention *prev = entitySet->getMention(ent->getMention(i));
		const SynNode *num_node = _getNumericPremod(prev);
		if (num_node != NULL)
			numerics.add(num_node);
	}
	if (numerics.length() == 0)
		return false;

	const SynNode* this_num = _getNumericPremod(ment);

	if (this_num == NULL)
		return false;

	for (int j = 0; j < numerics.length(); j++) {
		if (this_num->getHeadWord() != numerics[j]->getHeadWord()) {
			return true;
		}
	}
	return false;
}

bool SpanishDescLinkFunctions::_hasNumericMatch(Mention *ment, Entity *ent, EntitySet *entitySet)
{
	GrowableArray<const SynNode*> numerics;
	for (int i = 0; i < ent->getNMentions(); i++) {
		Mention *prev = entitySet->getMention(ent->getMention(i));
		const SynNode *num_node = _getNumericPremod(prev);
		if (num_node != NULL)
			numerics.add(num_node);
	}
	if (numerics.length() == 0)
		return false;

	const SynNode* this_num = _getNumericPremod(ment);

	if (this_num == NULL)
		return false;

	for (int j = 0; j < numerics.length(); j++) {
		if (this_num->getHeadWord() == numerics[j]->getHeadWord()) {
			return true;
		}
	}
	return false;
}


const SynNode* SpanishDescLinkFunctions::_getNumericPremod(Mention *ment) {

	const SynNode *node = ment->getNode();

	// if the node is preterminal, it won't have any numeric premods
	if (node->isPreterminal())
		return 0;

	for (int i = 0; i < node->getHeadIndex(); i++) {
		const SynNode *child = node->getChild(i);
		const SynNode *term = child->getFirstTerminal();
		for (int j = 0; j < child->getNTerminals(); j++) {
			const SynNode *parent = term->getParent();
			if (parent != 0 && parent->getTag() == SpanishSTags::POS_Z)
				return parent;
			if (j < child->getNTerminals() - 1)
				term = term->getNextTerminal();
		}
	}
	return 0;
}

bool SpanishDescLinkFunctions::_hasPremodName(Mention* ment, EntitySet* ents)
{
	const int MAX_PREMOD_NAMES = 5;
	const SynNode* premodNames[MAX_PREMOD_NAMES];

	int n_premod_names = _getPremodNames(ment, ents, premodNames, MAX_PREMOD_NAMES);

	return (n_premod_names > 0);

}

bool SpanishDescLinkFunctions::_hasPremod(Mention* ment)
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
Symbol SpanishDescLinkFunctions::_getParentHead(Mention* ment) {
	const SynNode* node = ment->node;
	if (node->getParent() == NULL)
		return Symbol(L"");
	Symbol mentHead = node->getHeadWord();
	while (node->getParent() != NULL) {
		const SynNode* headNode = node->getParent()->getHeadPreterm();
		Symbol headWord = headNode->getHeadWord();
		if (headWord != mentHead &&
			(headNode->getTag().isInSymbolGroup(SpanishSTags::POS_V_GROUP) ||
			 headNode->getTag() == SpanishSTags::MORFEMA_VERBAL ||
			 headNode->getTag() == SpanishSTags::GRUP_VERB)) {
				 return headWord;
			 }
			 node = node->getParent();
	}
	return Symbol(L"");
}
Symbol SpanishDescLinkFunctions::_findMentParent(Mention* ment) {
	Symbol sym = _getParentHead(ment);
	if (sym != Symbol(L"")) {
		wchar_t wordstr[300];
		swprintf(wordstr, 300, L"ment-parent[%ls]", sym.to_string());
		sym = Symbol(wordstr);
	}
	else {
		wchar_t wordstr[300];
		swprintf(wordstr, 300, L"ment-parent[]");
		sym = Symbol(wordstr);
	}
	return sym;
}

int SpanishDescLinkFunctions::_findEntParents(Entity* ent, EntitySet* ents, Symbol *results, const int max_results) {
	SymbolHash entityPreds(5);
	int n_results = 0;
	for (int i = 0; i < ent->getNMentions(); i++) {
		if (n_results == max_results)
			break;
		Mention* ment = ents->getMention(ent->getMention(i));
		Symbol sym = _getParentHead(ment);
		if (sym != Symbol(L"") && !entityPreds.lookup(sym)) {
			entityPreds.add(sym);
			wchar_t wordstr[300];
			swprintf(wordstr, 300, L"ent-parent[%ls]", sym.to_string());
			results[n_results++] = Symbol(wordstr);
		}
	}
	return n_results;
}

int SpanishDescLinkFunctions::_findMentPremods(Mention* ment, Symbol *premods, const int max_results) {
	int n_premods = _getPremods(ment, premods, max_results);
	int n_final_premods = 0;
	for (int i = 0; i < n_premods; i++) {
		wchar_t wordstr[300];
		if (!_stopWords->lookup(premods[i])) {
			swprintf(wordstr, 300, L"ment[%ls]", premods[i].to_string());
			premods[n_final_premods++] = Symbol(wordstr);
		}
	}
	return n_final_premods;
}

int SpanishDescLinkFunctions::_findEntHeadWords(Entity* ent, EntitySet* ents, Symbol *results, const int max_results) {
	int n_results = 0;

	int i = 0;
	for (i = 0; i < ent->getNMentions(); i++) {
		Mention* ment = ents->getMention(ent->getMention(i));
		Symbol head = ment->getHead()->getHeadWord();
		// filter out duplicates
		bool valid = true;
		for (int m = 0; m < n_results; m++) {
			if (wcscmp(results[m].to_string(), head.to_string()) == 0) {
				valid = false;
				break;
			}
		}
		if (valid)
			results[n_results++] = head;
		if (n_results >= max_results)
			break;
	}
	for (i = 0; i < n_results; i++) {
		wchar_t wordstr[300];
		swprintf(wordstr, 300, L"ent-head[%ls]", results[i].to_string());
		results[i] = Symbol(wordstr);
	}
	return n_results;
}

int SpanishDescLinkFunctions::_findEntPremods(Entity* ent, EntitySet* ents, Symbol *results, const int max_results) {
	// TODO: i have to hash the premods so i'm ensured there's no repeat. that sort of sucks
	const int MAX_PREMODS = 5;
	int n_results = 0;

	int i;
	for (i = 0; i < ent->getNMentions(); i++) {
		Mention* ment = ents->getMention(ent->getMention(i));
		Symbol premods[MAX_PREMODS];
		int n_premods = _getPremods(ment, premods, MAX_PREMODS);
		// filter out duplicates and stop words
		for (int k = 0; k < n_premods; k++) {
			bool valid = true;
			if (_stopWords->lookup(premods[k])) {
				valid = false;
				continue;
			}
			for (int m = 0; m < n_results; m++) {
				if (wcscmp(results[m].to_string(), premods[k].to_string()) == 0) {
					valid = false;
					break;
				}
			}
			if (valid)
				results[n_results++] = premods[k];
			if (n_results >= max_results)
				break;
		}
		if (n_results >= max_results)
			break;
	}
	for (i = 0; i < n_results; i++) {
		wchar_t wordstr[300];
		swprintf(wordstr, 300, L"ent[%ls]", results[i].to_string());
		results[i] = Symbol(wordstr);
	}
	return n_results;
}

int SpanishDescLinkFunctions::_getPremodNames(Mention* ment, EntitySet* ents,
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
	if (hIndex < 1)
		return 0;

	if (max_results < 1)
		return 0;

	int n_names, n_results = 0;
	const SynNode** sub_results = _new const SynNode*[max_results];

	for (int i = 0; i < hIndex; i++) {
		// get all names nested within this node if it has a mention
		n_names = _getMentionInternalNames(node->getChild(i), ms,
		  									sub_results, max_results);
		for (int j = 0; j < n_names; j++) {
			if (n_results == max_results) {
				break;
			}
			results[n_results++] = sub_results[j];
		}
	}

	delete [] sub_results;
	return n_results;
}


int SpanishDescLinkFunctions::_getPremods(Mention* ment, Symbol results[],
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


// This method is necessary because Spanish Treebank NPs have more
// structure than their English counterparts, so premod names can
// get buried inside other premod mentions
int SpanishDescLinkFunctions::_getMentionInternalNames(const SynNode* node,
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
					break;
				}
				results[n_results++] = sub_results[j];
			}
		}
	}
	delete [] sub_results;
	return n_results;
}

bool SpanishDescLinkFunctions::_hasPremodNameEntityClash(Mention* ment, Entity* ent, EntitySet* ents)
{

	if (ment == 0 || ent == 0) {
		return false;
	}

	const int MAX_PREMOD_NAMES = 5;
	const SynNode* ment1PNames[MAX_PREMOD_NAMES];
	const SynNode* ment2PNames[MAX_PREMOD_NAMES];
	bool matches[MAX_PREMOD_NAMES];
	int n_ment1_pnames, n_ment2_pnames;

	n_ment1_pnames = _getPremodNames(ment, ents, ment1PNames, MAX_PREMOD_NAMES);

	if (n_ment1_pnames == 0)
		return false;

	for (int l = 0; l < n_ment1_pnames; l++)
		matches[l] = false;


	// for each existing DESC mention of ent, check to see if its premod names contain
	// references to entities referenced by ment's premod names
	// for each existing NAME mention of ent, check to see if it matches any of
	// ment's premod names
	// i.e. ment cannot introduce any premod names not already associated with ent
	for (int k = 0; k < ent->mentions.length(); k++) {

		Mention* ment2 = ents->getMention(ent->mentions[k]);

		if (ment2->mentionType == Mention::NAME) {
			for (int i = 0; i < n_ment1_pnames; i++) {
				if (ment2->getNode()->getHeadWord() == ment1PNames[i]->getHeadWord()) {
					matches[i] = true;
					break;
				}
			}
		}

		// only look for premods of other DESCs
		if (ment2->mentionType != Mention::DESC)
			continue;

		n_ment2_pnames = _getPremodNames(ment2, ents, ment2PNames, MAX_PREMOD_NAMES);

		for (int i = 0; i < n_ment1_pnames; i++) {

			// search through all premod names of this mention to find matching entity
			// with ment's current premod name
			for (int j = 0; j < n_ment2_pnames; j++) {

				// are these mentions the same?
				if (ment2PNames[j]->getHeadWord() == ment1PNames[i]->getHeadWord()) {
					matches[i] = true;
					break;
				}
			}
		}
	}

	for (int m = 0; m < n_ment1_pnames; m++) {
		if (! matches[m])
			return true;
	}
	// couldn't find a clash
	return false;
}

bool SpanishDescLinkFunctions::_hasPremodClash(Mention* ment, Entity* ent, EntitySet* ents) {
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

bool SpanishDescLinkFunctions::_hasPremodMatch(Mention* ment, Entity* ent, EntitySet* ents) {
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
		// specifically, only DESCS with the same head word - JCS 3/8/04 - Why?
		//if (ment2->getNode()->getHeadWord() != ment1Head)
		//	continue;
		n_ment2_premods = _getPremods(ment2, ment2Premods, MAX_PREMODS);
		if (n_ment2_premods == 0)
			continue;
		// a match exists if any word in ment2 is in ment1
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
	}
	if (n_common_premods < 1)
		return false;
	return true;
}

Symbol SpanishDescLinkFunctions::_getNumEnts(EntityType type, EntitySet *set) {
	int size = set->getEntitiesByType(type).length();
	wchar_t wordstr[20];
	if (size <= 10)
		swprintf(wordstr, 20, L"numEnts[%d]", size);
	else
		swprintf(wordstr, 20, L"numEnts[%ls]", L">10");
	return Symbol(wordstr);
}

bool SpanishDescLinkFunctions::_entityContainsOnlyNames(Entity *entity, EntitySet *entitySet) {
	for (int k = 0; k < entity->mentions.length(); k++) {
		Mention* ment = entitySet->getMention(entity->mentions[k]);
		if (ment->mentionType != Mention::NAME)
			return false;
	}
	return true;
}

bool SpanishDescLinkFunctions::_mentOverlapsEntity(Mention *mention, Entity *entity, EntitySet *entSet) {
	int start = mention->getNode()->getStartToken();
	int end = mention->getNode()->getEndToken();
	for (int k = 0; k < entity->mentions.length(); k++) {
		Mention* ment = entSet->getMention(entity->mentions[k]);
		if (ment->getSentenceNumber() == mention->getSentenceNumber()) {
			int ent_start = ment->getNode()->getStartToken();
			int ent_end = ment->getNode()->getEndToken();
			if (start <= ent_start && end >= ent_start)
				return true;
			if (start > ent_start && start <= ent_end)
				return true;
		}
	}
	return false;
}

Symbol SpanishDescLinkFunctions::_getClusterScore(Mention *ment, Entity *ent, EntitySet *entSet) {
	const int MAX_CLUSTERS = 4;
	int max_agreed = 0;
	int total_ent_clusters = 0;

	Symbol ment_clusters[MAX_CLUSTERS];
	int n_ment_clusters = _getClusters(ment->getNode()->getHeadWord(), ment_clusters, MAX_CLUSTERS);
	if (n_ment_clusters > 0) {
		const wchar_t *ment_str = ment_clusters[0].to_string();
		for (int i = 0; i < ent->getNMentions(); i++) {
			Mention* m = entSet->getMention(ent->getMention(i));
			Symbol ent_clusters[MAX_CLUSTERS];
			int n_ent_clusters = _getClusters(m->getNode()->getHeadWord(), ent_clusters, MAX_CLUSTERS);
			total_ent_clusters += n_ent_clusters;
			if (n_ent_clusters > 0) {
				int agreed = 0;
				const wchar_t *ent_str = ent_clusters[0].to_string();
				const wchar_t *mptr = ment_str;
				while (*mptr != '\0' && *ent_str != '\0') {
					if (*mptr != *ent_str)
						break;
					agreed++;
					mptr++;
					ent_str++;
				}
				if (agreed > max_agreed)
					max_agreed = agreed;
			}
		}
	}
	Symbol score;
	if (n_ment_clusters == 0 || total_ent_clusters == 0)
		score = Symbol(L"NO_CLUSTERS");
	else if (max_agreed < 8)
		score = Symbol(L"clust-agree[<8]");
	else if (max_agreed < 16)
		score = Symbol(L"clust-agree[<16]");
	else if (max_agreed < 24)
		score = Symbol(L"clust-agree[<24]");
	else
		score = Symbol(L"clust-agree[>=24]");
	return score;
}

bool SpanishDescLinkFunctions::_hasClusterMatch(Mention *ment, Entity *ent, EntitySet *entSet) {
	const int MAX_CLUSTERS = 4;

	Symbol ment_clusters[MAX_CLUSTERS];
	int n_ment_clusters = _getClusters(ment->getNode()->getHeadWord(), ment_clusters, MAX_CLUSTERS);

	int i;
	for (i = 0; i < ent->getNMentions(); i++) {
		Mention* m = entSet->getMention(ent->getMention(i));
		Symbol ent_clusters[MAX_CLUSTERS];
		int n_ent_clusters = _getClusters(m->getNode()->getHeadWord(), ent_clusters, MAX_CLUSTERS);
		for (int j = 0; j < n_ment_clusters; j++) {
			for (int k = 0; k < n_ent_clusters; k++) {
				if (ment_clusters[j] == ent_clusters[k])
					return true;
			}
		}
	}
	return false;
}

int SpanishDescLinkFunctions::_findMentClusters(Mention *ment, Symbol *results, const int max_results) {
	const int MAX_CLUSTERS = 4;

	Symbol clusters[MAX_CLUSTERS];
	int n_results = _getClusters(ment->getNode()->getHeadWord(), clusters, MAX_CLUSTERS);

	for (int i = 0; i < n_results; i++) {
		wchar_t wordstr[300];
		swprintf(wordstr, 300, L"ment-clust[%ls]", clusters[i].to_string());
		results[i] = Symbol(wordstr);
	}
	return n_results;
}

int SpanishDescLinkFunctions::_findEntClusters(Entity* ent, EntitySet* ents, Symbol *results, const int max_results) {
	const int MAX_CLUSTERS = 4;
	int n_results = 0;

	int i;
	for (i = 0; i < ent->getNMentions(); i++) {
		Mention* ment = ents->getMention(ent->getMention(i));
		Symbol clusters[MAX_CLUSTERS];
		int n_clusters = _getClusters(ment->getNode()->getHeadWord(), clusters, MAX_CLUSTERS);
		// filter out duplicates
		for (int k = 0; k < n_clusters; k++) {
			bool valid = true;
			for (int m = 0; m < n_results; m++) {
				if (wcscmp(results[m].to_string(), clusters[k].to_string()) == 0) {
					valid = false;
					break;
				}
			}
			if (valid)
				results[n_results++] = clusters[k];
			if (n_results >= max_results)
				break;
		}
		if (n_results >= max_results)
			break;
	}
	for (i = 0; i < n_results; i++) {
		wchar_t wordstr[300];
		swprintf(wordstr, 300, L"ent-clust[%ls]", results[i].to_string());
		results[i] = Symbol(wordstr);
	}
	return n_results;
}


int SpanishDescLinkFunctions::_getClusters(Symbol word, Symbol *results, const int max_results) {
	int n_clusters = 0;

	/*WordClusterClass clusters(word);
	if (!(clusters == WordClusterClass::nullCluster())) {
		wchar_t c[200];
		swprintf(c, 200, L"c8:%d", clusters.c8());
		results[0] = Symbol(c);
		swprintf(c, 200, L"c12:%d", clusters.c12());
		results[1] = Symbol(c);
		swprintf(c, 200, L"c16:%d", clusters.c16());
		results[2] = Symbol(c);
		swprintf(c, 200, L"c20:%d", clusters.c20());
		results[3] = Symbol(c);
		return 4;
	}*/
	Symbol **clusters = _clusterTable->get(word);
	if (clusters != 0) {
		for (int i = 0; i < numClusters - 1 && i < max_results; i++) {
			if (!(*clusters)[i].is_null()) {
				results[n_clusters++] = (*clusters)[i];
			}
		}
	}
	return n_clusters;
}

Symbol SpanishDescLinkFunctions::_getCommonParent(Mention* ment, Entity* ent, EntitySet* ents) {
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

void SpanishDescLinkFunctions::initializeWordClusterTable() {
	std::string buffer = ParamReader::getRequiredParam("word_cluster_table");
	//WordClusterTable::initTable(buffer);

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(buffer.c_str()));
	UTF8InputStream& in(*in_scoped_ptr);
	UTF8Token token;
	wchar_t bitstring[200];

	while (!in.eof()) {
		in >> token;
		Symbol word = token.symValue();
		in >> token;
		wcsncpy(bitstring, token.chars(), 200);
		int total_len = static_cast<int>(wcslen(bitstring));
		Symbol *clusters = _new Symbol[numClusters];
		int clusterBitSize = 32 / numClusters;
		for (int i = 0; i < numClusters; i++) {
			int len = (numClusters - i) * clusterBitSize;
			if (len < total_len) {
				bitstring[len] = '\0';
				clusters[i] = Symbol(bitstring);
			}
			else {
				clusters[i] = Symbol();
			}
		}

		(*_clusterTable)[word] = clusters;
	}
	in.close();

}

void SpanishDescLinkFunctions::initializeStopWords() {
	std::string buffer = ParamReader::getRequiredParam("desclink_stop_words");
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(buffer.c_str()));
	UTF8InputStream& stream(*stream_scoped_ptr);
	int size = -1;
	stream >> size;
	if (size < 0)
		throw UnexpectedInputException("SpanishDescLinkFunctions::loadStopWords()",
									   "couldn't read size of stopWord array");

	UTF8Token token;
	int i;
	for (i = 0; i < size; i++) {
		stream >> token;
		_stopWords->add(token.symValue());
	}

	stream.close();
}
