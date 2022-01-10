// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/WordConstants.h"
#include "edt/KoreanRuleNameLinker.h"

#include <boost/algorithm/string.hpp>

using namespace std;

const int MAX_WORDS_IN_MENTION = 50;
const Symbol KoreanRuleNameLinker::ATTR_FULLEST = Symbol(L"FullestName");
int KoreanRuleNameLinker::initialTableSize = 5;

KoreanRuleNameLinker::KoreanRuleNameLinker() : _debug(Symbol(L"rule-name-link-debug")) { 
	_per_mentions = _new HashTable(initialTableSize);
	_org_mentions = _new HashTable(initialTableSize);
	_gpe_mentions = _new HashTable(initialTableSize);
	_oth_mentions = _new HashTable(initialTableSize);

	_per_attributes = _new HashTable(initialTableSize);
	_org_attributes = _new HashTable(initialTableSize);
	_gpe_attributes = _new HashTable(initialTableSize);
	_oth_attributes = _new HashTable(initialTableSize);
	_unique_per_attributes = _new HashTable(initialTableSize);
}

KoreanRuleNameLinker::~KoreanRuleNameLinker() {
	delete _per_mentions;
	delete _org_mentions;
	delete _gpe_mentions;
	delete _oth_mentions;

	delete _per_attributes;
	delete _org_attributes;
	delete _gpe_attributes;
	delete _oth_attributes;
	delete _unique_per_attributes;
}

int KoreanRuleNameLinker::linkMention(LexEntitySet *currSolution, 
								int currMentionUID, EntityType linkType,
								LexEntitySet *results[], int max_results)
{
	Mention *currMention = currSolution->getMention(currMentionUID);

	Symbol wordSymbols[MAX_WORDS_IN_MENTION];
	int wordCount = currMention->getHead()->getTerminalSymbols(
										wordSymbols, MAX_WORDS_IN_MENTION);

	const wchar_t *wordArray[MAX_WORDS_IN_MENTION];
	for (int i = 0; i < wordCount; i++)
		wordArray[i] = wordSymbols[i].to_string();

	Symbol entID = Symbol();
	wchar_t buffer[11];

	LexEntitySet* newSet = currSolution->fork();

	if (linkType.matchesPER()) {
		generatePERVariations(wordCount, wordArray,
							  _per_attributes, _unique_per_attributes);
		entID = linkPERMention(currSolution, currMention);
		if (entID == Symbol()) {
			newSet->addNew(currMention->getUID(), linkType);
			entID = Symbol(_itow(newSet->getEntityByMention(currMention->getUID())->getID(), buffer, 10));
			_debug << "Adding new entity with mention:\n" << currMention->node->toDebugString(0).c_str() << "\n";
		} else {
			_debug << "Linking:\n" << currMention->node->toDebugString(0).c_str() << "\n";
			_debug << " to entity id: \n";
			_debug << entID.to_debug_string() << "\n\n";
			newSet->add(currMention->getUID(), _wtoi(entID.to_string()));
		}
		mergeMentions(_per_attributes, _per_mentions, entID);
	}
	else if (linkType.matchesORG()) {
		generateORGVariations(wordCount, wordArray, _org_attributes);
		entID = linkORGMention(currSolution, currMention);
		if (entID == Symbol()) {
			newSet->addNew(currMention->getUID(), linkType);
			entID = Symbol(_itow(newSet->getEntityByMention(currMention->getUID())->getID(), buffer, 10));
			_debug << "Adding new entity with mention:\n" << currMention->node->toDebugString(0).c_str() << "\n";
		} else {
			_debug << "Linking:\n" << currMention->node->toDebugString(0).c_str() << "\n";
			_debug << " to entity id: \n";
			_debug << entID.to_debug_string() << "\n\n";
			newSet->add(currMention->getUID(), _wtoi(entID.to_string()));
		}
		mergeMentions(_org_attributes, _org_mentions, entID);
	}
	else if (linkType.matchesGPE()) {
		generateGPEVariations(wordCount, wordArray, _org_attributes);
		entID = linkGPEMention(currSolution, currMention);
		if (entID == Symbol()) {
			newSet->addNew(currMention->getUID(), linkType);
			entID = Symbol(_itow(newSet->getEntityByMention(currMention->getUID())->getID(), buffer, 10));
			_debug << "Adding new entity with mention:\n" << currMention->node->toDebugString(0).c_str() << "\n";
		} else {
			_debug << "Linking:\n" << currMention->node->toDebugString(0).c_str() << "\n";
			_debug << " to entity id: \n";
			_debug << entID.to_debug_string() << "\n\n";
			newSet->add(currMention->getUID(), _wtoi(entID.to_string()));
		}
		mergeMentions(_gpe_attributes, _gpe_mentions, entID);
	}
	else {
		_debug << "Entity type is not PER, ORG, or GPE. Considering generic match search: \n"
			<< currMention->node->toDebugString(0).c_str() << "\n";
		generateOTHVariations(wordCount, wordArray, _oth_attributes);

		// try to link, but only if it is a linkable type
		entID == Symbol();
		if (currMention->getEntityType().isLinkable())
			entID = linkOTHMention(currSolution, currMention);
		if (entID == Symbol()) {
			newSet->addNew(currMention->getUID(), linkType);
			entID = Symbol(_itow(newSet->getEntityByMention(currMention->getUID())->getID(), buffer, 10));
			_debug << "Adding new entity with mention:\n" << currMention->node->toDebugString(0).c_str() << "\n";
		} else {
			_debug << "Linking:\n" << currMention->node->toDebugString(0).c_str() << "\n";
			_debug << " to entity id: \n";
			_debug << entID.to_debug_string() << "\n\n";		
			newSet->add(currMention->getUID(), _wtoi(entID.to_string()));
		}
		mergeMentions(_oth_attributes, _oth_mentions, entID);
	}
	results[0] = newSet;
	return 1;
}
void KoreanRuleNameLinker::cleanUpAfterDocument() { 
	_per_mentions->remove_all();
	_org_mentions->remove_all();
	_gpe_mentions->remove_all();
	_oth_mentions->remove_all();
}

Symbol KoreanRuleNameLinker::linkPERMention(LexEntitySet *currSolution, Mention *currMention) {
	HashTable::iterator iter;
	Symbol entID = Symbol();

	for (iter = _per_attributes->begin(); iter != _per_attributes->end(); ++iter) {
		if (getValueFromTable(_per_mentions, (*iter).first) != Symbol()) {
			// if found a match
			entID = getValueFromTable(_per_mentions, (*iter).first);
		}
	}
	
	return entID;
}

Symbol KoreanRuleNameLinker::linkORGMention(LexEntitySet *currSolution, Mention *currMention) {
	HashTable::iterator iter;
	Symbol entID = Symbol();

	for (iter = _org_attributes->begin(); iter != _org_attributes->end(); ++iter) {
		if (getValueFromTable(_org_mentions, (*iter).first) != Symbol()) {
			// if found a match
			entID = getValueFromTable(_org_mentions, (*iter).first);
		}
	}
	
	return entID;
}

Symbol KoreanRuleNameLinker::linkGPEMention(LexEntitySet *currSolution, Mention *currMention) {
	HashTable::iterator iter;
	Symbol entID = Symbol();
	for (iter = _gpe_attributes->begin(); iter != _gpe_attributes->end(); ++iter) {
		if (getValueFromTable(_gpe_mentions, (*iter).first) != Symbol()) {
			// if found a match
			entID = getValueFromTable(_gpe_mentions, (*iter).first);
		}
	}

	return entID;
}

Symbol KoreanRuleNameLinker::linkOTHMention(LexEntitySet *currSolution, Mention *currMention) {
	HashTable::iterator iter;
	Symbol entID = Symbol();

	for (iter = _oth_attributes->begin(); iter != _oth_attributes->end(); ++iter) {
		if (getValueFromTable(_oth_mentions, (*iter).first) != Symbol()) {
			// if found a match
			entID = getValueFromTable(_oth_mentions, (*iter).first);
		}
	}
	return entID;
}

Symbol KoreanRuleNameLinker::getValueFromTable(HashTable *table, Symbol key) {
	HashTable::iterator iter;
	iter = table->find(key);
	if (iter == table->end()) {
		return Symbol();
	}
	return (*iter).second;
}

void KoreanRuleNameLinker::generatePERVariations(int wordCount,
										   const wchar_t **wordArray,
										   HashTable *attributes,
										   HashTable *uniqAttributes)
{
	attributes->remove_all();
	uniqAttributes->remove_all();

	wstring buffer = wordArray[0];

	for (int i = 1; i < wordCount; i++) {
		buffer += L' ';
		buffer += wordArray[i];
	}
	addAttribute(attributes, Symbol(buffer.c_str()), ATTR_FULLEST);
}

void KoreanRuleNameLinker::generateORGVariations(int wordCount,
										   const wchar_t **wordArray,
										   HashTable *attributes)
{
	attributes->remove_all();

	wstring buffer = wordArray[0];

	for (int i = 1; i < wordCount; i++) {
		buffer += L' ';
		buffer += wordArray[i];
	}
	addAttribute(attributes, Symbol(buffer.c_str()), ATTR_FULLEST);
}

void KoreanRuleNameLinker::generateGPEVariations(int wordCount,
										   const wchar_t **wordArray,
										   HashTable *attributes)
{
	attributes->remove_all();

	wstring buffer = wordArray[0];

	for (int i = 1; i < wordCount; i++) {
		buffer += L' ';
		buffer += wordArray[i];
	}
	addAttribute(attributes, Symbol(buffer.c_str()), ATTR_FULLEST);
}

void KoreanRuleNameLinker::generateOTHVariations(int wordCount,
										   const wchar_t **wordArray,
										   HashTable *attributes)
{
	attributes->remove_all();

	wstring buffer = wordArray[0];

	for (int i = 1; i < wordCount; i++) {
		buffer += L' ';
		buffer += wordArray[i];
	}
	addAttribute(attributes, Symbol(buffer.c_str()), ATTR_FULLEST);
}

void KoreanRuleNameLinker::mergeMentions(HashTable * source, HashTable * destination,
								   Symbol entID)
{
	HashTable::iterator iter;
	for (iter = source->begin(); iter != source->end(); ++iter) {
		(*destination)[(*iter).first] = entID;
	}
}

void KoreanRuleNameLinker::addAttribute(HashTable *table, Symbol key, Symbol value) {
	std::wstring attribute(key.to_string());
	boost::to_lower(attribute);
	replaceChar(attribute, L".", L"");
	Symbol lowerKey = Symbol(attribute.c_str());
	if (getValueFromTable(table, lowerKey) == Symbol()) {
		(*table)[lowerKey] = value;
	}
}

void KoreanRuleNameLinker::replaceChar(wstring &str, const wstring &from,
								 const wstring &to)
{
	size_t iter = 0;
	while( (iter = str.find(from, iter)) != string::npos) {
		str.replace(iter, 1, to);
		iter += 1;
	}
}

Mention *KoreanRuleNameLinker::getMentionFromTerminal(LexEntitySet *currSolution, 
							   Mention *currMention, const SynNode *terminal)
{
	if (terminal == 0 || terminal->getParent() == 0 || 
		terminal->getParent()->getParent() == 0)
		return 0;

	const SynNode *parent = terminal->getParent()->getParent();
	if (parent->hasMention()) {
		int local_id = parent->getMentionIndex();
		int sentence_id = currMention->getSentenceNumber();
		int global_id = MentionUID(sentence_id, local_id).to_int();
		Mention *mention = currSolution->getMention(global_id);
		if (mention->getEntityType() != currMention->getEntityType())
			return 0;
		Entity *ent = currSolution->getEntityByMention(mention->getUID());
		while (ent == 0 && mention->getParent() != 0) {
			mention = mention->getParent();
			ent = currSolution->getEntityByMention(mention->getUID());
		}
		return mention;
	}

	return 0;
}


Symbol KoreanRuleNameLinker::getEntityIDFromTerminal(LexEntitySet *currSolution, 
							   Mention *currMention, const SynNode *terminal) 
{
	Mention *mention = getMentionFromTerminal(currSolution, currMention, terminal);
	if (mention == 0)
		return Symbol();
	Entity *ent = currSolution->getEntityByMention(mention->getUID());
	if (ent != 0) {
		wchar_t buffer[11];
		return Symbol(_itow(ent->getID(), buffer, 10));
	}

	return Symbol();
}

