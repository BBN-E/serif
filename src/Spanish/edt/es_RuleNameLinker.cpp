// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UTF8InputStream.h"
#include "Spanish/common/es_WordConstants.h"
#include "Spanish/edt/es_RuleNameLinker.h"
#include "Generic/edt/AcronymMaker.h"
#include "Generic/theories/SynNode.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>


using namespace std;

// stub implementation of rule name linker

const int MAX_WORDS_IN_MENTION = 50;

const Symbol SpanishRuleNameLinker::ATTR_FULLEST = Symbol(L"FullestName");
const Symbol SpanishRuleNameLinker::ATTR_FIRST_MI_LAST = Symbol(L"FirstMILastName");
const Symbol SpanishRuleNameLinker::ATTR_FIRST_LAST = Symbol(L"FirstLastName");
const Symbol SpanishRuleNameLinker::ATTR_LAST = Symbol(L"LastName");
const Symbol SpanishRuleNameLinker::ATTR_MI = Symbol(L"MiddleInitial");
const Symbol SpanishRuleNameLinker::ATTR_MIDDLE = Symbol(L"MiddleName");
const Symbol SpanishRuleNameLinker::ATTR_SUFFIX = Symbol(L"Suffix");
const Symbol SpanishRuleNameLinker::ATTR_NO_DESIG = Symbol(L"NoDesignators");
const Symbol SpanishRuleNameLinker::ATTR_SINGLE_WORD = Symbol(L"SingleWord");
const Symbol SpanishRuleNameLinker::ATTR_ABBREV = Symbol(L"Abbreviation");
const Symbol SpanishRuleNameLinker::ATTR_MILITARY = Symbol(L"Military");

const int SpanishRuleNameLinker::RANK_LEVEL_0 = 0;
const int SpanishRuleNameLinker::RANK_LEVEL_10 = 10;
const int SpanishRuleNameLinker::RANK_LEVEL_20 = 20;
const int SpanishRuleNameLinker::RANK_LEVEL_30 = 30;
const int SpanishRuleNameLinker::RANK_LEVEL_40 = 40;
const int SpanishRuleNameLinker::RANK_LEVEL_50 = 50;
const int SpanishRuleNameLinker::RANK_LEVEL_60 = 60;
const int SpanishRuleNameLinker::RANK_LEVEL_70 = 70;
const int SpanishRuleNameLinker::RANK_LEVEL_80 = 80;
const int SpanishRuleNameLinker::RANK_LEVEL_90 = 90;
const int SpanishRuleNameLinker::RANK_LEVEL_100 = 100;

int SpanishRuleNameLinker::initialTableSize = 5;

SpanishRuleNameLinker::SpanishRuleNameLinker() : _debug(Symbol(L"rule_name_link_debug")) { 
	
	_noise = _new HashTable(50);
	loadFileIntoTable(ParamReader::getParam(L"linker_noise"), _noise);
	_alternateSpellings = _new HashTable(initialTableSize);
	loadAlternateSpellings(ParamReader::getParam(L"linker_alt_spellings"));
	loadAlternateSpellings(ParamReader::getParam(L"linker_nations"));

	_suffixes = _new HashTable(initialTableSize);
	loadFileIntoTable(ParamReader::getParam(L"linker_suffixes"), _suffixes);

	_designators = _new HashTable(initialTableSize);
	loadFileIntoTable(ParamReader::getParam(L"linker_designators"), _designators);

	_per_mentions = _new HashTable(initialTableSize);
	_org_mentions = _new HashTable(initialTableSize);
	_gpe_mentions = _new HashTable(initialTableSize);
	_oth_mentions = _new HashTable(initialTableSize);

	_per_attributes = _new HashTable(initialTableSize);
	_org_attributes = _new HashTable(initialTableSize);
	_gpe_attributes = _new HashTable(initialTableSize);
	_oth_attributes = _new HashTable(initialTableSize);
	_unique_per_attributes = _new HashTable(initialTableSize);

	if (ParamReader::isParamTrue("use_itea_linking")) {
		ITEA_LINKING = true;
	} else ITEA_LINKING = false;

	_use_rules = ParamReader::getRequiredTrueFalseParam("use_simple_rule_namelink");
	if (_use_rules)
		_simpleRuleNameLinker = _new SimpleRuleNameLinker();

}

SpanishRuleNameLinker::~SpanishRuleNameLinker() {
	delete _per_mentions;
	delete _org_mentions;
	delete _gpe_mentions;
	delete _oth_mentions;

	delete _per_attributes;
	delete _org_attributes;
	delete _gpe_attributes;
	delete _oth_attributes;
	delete _unique_per_attributes;

	if (_use_rules)
		delete _simpleRuleNameLinker;
}

int SpanishRuleNameLinker::linkMention(LexEntitySet *currSolution, 
								MentionUID currMentionUID, EntityType linkType,
								LexEntitySet *results[], int max_results)
{
	if (_use_rules)
		_simpleRuleNameLinker->setCurrSolution(currSolution);

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
		if (entID.is_null()) {
			newSet->addNew(currMention->getUID(), linkType);
#if defined(_WIN32)
			entID = Symbol(_itow(newSet->getEntityByMention(currMention->getUID())->getID(), buffer, 10));
#else
			swprintf (buffer, sizeof(buffer)/sizeof(buffer[0]),
				  L"%d", newSet->getEntityByMention
				  (currMention->getUID())->getID());
			entID = Symbol(buffer);
#endif
			_debug << "Adding new entity #" << entID.to_string() << " with mention:\n" << currMention->node->toDebugString(0).c_str() << "\n";
		} else {
			if (_use_rules) {
				EntityGuess * newGuess = _new EntityGuess();
				newGuess->id = _wtoi(entID.to_string());
				newGuess->type = linkType;
				if (_simpleRuleNameLinker->isOKToLink(currMention, linkType, newGuess)) {
					_debug << "Linking Mention #" << currMention->getUID() << ":\n" << currMention->node->toDebugString(0).c_str() << "\n";
					_debug << " to entity id: \n";
					_debug << entID.to_debug_string() << "\n\n";
					newSet->add(currMention->getUID(), _wtoi(entID.to_string()));
				} else {
					newSet->addNew(currMention->getUID(), linkType);
#ifdef _WIN32
					entID = Symbol(_itow(newSet->getEntityByMention(currMention->getUID())->getID(), buffer, 10));
#else
					swprintf (buffer, sizeof(buffer)
						  /sizeof(buffer[0]), L"%d",
						  newSet->getEntityByMention
						  (currMention->getUID())->getID());
					entID = Symbol(buffer);
#endif
					_debug << "Adding new entity #" << entID.to_string() << " with mention:\n" << currMention->node->toDebugString(0).c_str() << "\n";
				}
				delete newGuess;
			} else {
				_debug << "Linking Mention #" << currMention->getUID() << ":\n" << currMention->node->toDebugString(0).c_str() << "\n";
				_debug << " to entity id: \n";
				_debug << entID.to_debug_string() << "\n\n";
				newSet->add(currMention->getUID(), _wtoi(entID.to_string()));
			}
		}
		mergeMentions(_per_attributes, _per_mentions, entID);
	}
	else if (linkType.matchesORG()) {
		generateORGVariations(wordCount, wordArray, _org_attributes);
		addAcronymAttributes(wordCount, wordSymbols, _org_attributes);
		entID = linkORGMention(currSolution, currMention);
		if (entID.is_null()) {
			newSet->addNew(currMention->getUID(), linkType);
#ifdef _WIN32
			entID = Symbol(_itow(newSet->getEntityByMention(currMention->getUID())->getID(), buffer, 10));
#else
			swprintf (buffer, sizeof(buffer)/sizeof(buffer[0]),
				  L"%d", newSet->getEntityByMention
				  (currMention->getUID())->getID());
			entID = Symbol(buffer);
#endif
			_debug << "Adding new entity #" << entID.to_string() << " with mention:\n" << currMention->node->toDebugString(0).c_str() << "\n";
		} else {
			if (_use_rules) {
				EntityGuess * newGuess = _new EntityGuess();
				newGuess->id = _wtoi(entID.to_string());
				newGuess->type = linkType;
				if (_simpleRuleNameLinker->isOKToLink(currMention, linkType, newGuess)) {
					_debug << "Linking Mention #" << currMention->getUID() << ":\n" << currMention->node->toDebugString(0).c_str() << "\n";
					_debug << " to entity id: \n";
					_debug << entID.to_debug_string() << "\n\n";
					newSet->add(currMention->getUID(), _wtoi(entID.to_string()));
				} else {
					newSet->addNew(currMention->getUID(), linkType);
#ifdef _WIN32
					entID = Symbol(_itow(newSet->getEntityByMention(currMention->getUID())->getID(), buffer, 10));
#else
					swprintf (buffer, sizeof(buffer)
						  /sizeof(buffer[0]), L"%d",
						  newSet->getEntityByMention
						  (currMention->getUID())->getID());
					entID = Symbol(buffer);
#endif
					_debug << "Adding new entity #" << entID.to_string() << " with mention:\n" << currMention->node->toDebugString(0).c_str() << "\n";
				}
				delete newGuess;
			} else {
				_debug << "Linking Mention #" << currMention->getUID() << ":\n" << currMention->node->toDebugString(0).c_str() << "\n";
				_debug << " to entity id: \n";
				_debug << entID.to_debug_string() << "\n\n";
				newSet->add(currMention->getUID(), _wtoi(entID.to_string()));
			}
		}
		mergeMentions(_org_attributes, _org_mentions, entID);
	}
	else if (linkType.matchesGPE()) {
		generateGPEVariations(wordCount, wordArray, _gpe_attributes);
		entID = linkGPEMention(currSolution, currMention);
		if (entID.is_null()) {
			newSet->addNew(currMention->getUID(), linkType);
#ifdef _WIN32
			entID = Symbol(_itow(newSet->getEntityByMention(currMention->getUID())->getID(), buffer, 10));
#else
			swprintf (buffer, sizeof(buffer)/sizeof(buffer[0]),
				  L"%d", newSet->getEntityByMention
				  (currMention->getUID())->getID());
			entID = Symbol(buffer);
#endif
			_debug << "Adding new entity #" << entID.to_string() << " with mention:\n" << currMention->node->toDebugString(0).c_str() << "\n";
		} else {
			if (_use_rules) {
				EntityGuess * newGuess = _new EntityGuess();
				newGuess->id = _wtoi(entID.to_string());
				newGuess->type = linkType;
				if (_simpleRuleNameLinker->isOKToLink(currMention, linkType, newGuess)) {
					_debug << "Linking Mention #" << currMention->getUID() << ":\n" << currMention->node->toDebugString(0).c_str() << "\n";
					_debug << " to entity id: \n";
					_debug << entID.to_debug_string() << "\n\n";
					newSet->add(currMention->getUID(), _wtoi(entID.to_string()));
				} else {
					newSet->addNew(currMention->getUID(), linkType);
#ifdef _WIN32
					entID = Symbol(_itow(newSet->getEntityByMention(currMention->getUID())->getID(), buffer, 10));
#else
					swprintf (buffer, sizeof(buffer)
						  /sizeof(buffer[0]), L"%d",
						  newSet->getEntityByMention
						  (currMention->getUID())->getID());
					entID = Symbol(buffer);
#endif
					_debug << "Adding new entity #" << entID.to_string() << " with mention:\n" << currMention->node->toDebugString(0).c_str() << "\n";
				}
				delete newGuess;
			} else {
				_debug << "Linking Mention #" << currMention->getUID() << ":\n" << currMention->node->toDebugString(0).c_str() << "\n";
				_debug << " to entity id: \n";
				_debug << entID.to_debug_string() << "\n\n";
				newSet->add(currMention->getUID(), _wtoi(entID.to_string()));
			}
		}
		mergeMentions(_gpe_attributes, _gpe_mentions, entID);
	}
	else {
		_debug << "Entity type is not PER, ORG, or GPE. Considering generic match search: \n"
			<< currMention->node->toDebugString(0).c_str() << "\n";
		generateOTHVariations(wordCount, wordArray, _oth_attributes);
		addAcronymAttributes(wordCount, wordSymbols, _oth_attributes);

		// try to link, but only if it is a linkable type
		entID.is_null();
		if (currMention->getEntityType().isLinkable())
			entID = linkOTHMention(currSolution, currMention);
		if (entID.is_null()) {
			newSet->addNew(currMention->getUID(), linkType);
#ifdef _WIN32
			entID = Symbol(_itow(newSet->getEntityByMention(currMention->getUID())->getID(), buffer, 10));
#else
			swprintf (buffer, sizeof(buffer)/sizeof(buffer[0]),
				  L"%d", newSet->getEntityByMention
				  (currMention->getUID())->getID());
			entID = Symbol(buffer);
#endif
			_debug << "Adding new entity #" << entID.to_string() << " with mention:\n" << currMention->node->toDebugString(0).c_str() << "\n";
		} else {
			if (_use_rules) {
				EntityGuess * newGuess = _new EntityGuess();
				newGuess->id = _wtoi(entID.to_string());
				newGuess->type = linkType;
				if (_simpleRuleNameLinker->isOKToLink(currMention, linkType, newGuess)) {
					_debug << "Linking Mention #" << currMention->getUID() << ":\n" << currMention->node->toDebugString(0).c_str() << "\n";
					_debug << " to entity id: \n";
					_debug << entID.to_debug_string() << "\n\n";		
					newSet->add(currMention->getUID(), _wtoi(entID.to_string()));
				} else {
					newSet->addNew(currMention->getUID(), linkType);
#ifdef _WIN32
					entID = Symbol(_itow(newSet->getEntityByMention(currMention->getUID())->getID(), buffer, 10));
#else
					swprintf (buffer, sizeof(buffer)
						  /sizeof(buffer[0]), L"%d",
						  newSet->getEntityByMention
						  (currMention->getUID())->getID());
					entID = Symbol(buffer);
#endif
					_debug << "Adding new entity #" << entID.to_string() << " with mention:\n" << currMention->node->toDebugString(0).c_str() << "\n";
				}
				delete newGuess;
			} else {
				_debug << "Linking Mention #" << currMention->getUID() << ":\n" << currMention->node->toDebugString(0).c_str() << "\n";
				_debug << " to entity id: \n";
				_debug << entID.to_debug_string() << "\n\n";		
				newSet->add(currMention->getUID(), _wtoi(entID.to_string()));
			}
		}
		mergeMentions(_oth_attributes, _oth_mentions, entID);
	}
	results[0] = newSet;
	return 1;
}
void SpanishRuleNameLinker::cleanUpAfterDocument() { 
	_per_mentions->clear();
	_org_mentions->clear();
	_gpe_mentions->clear();
	_oth_mentions->clear();
}

void SpanishRuleNameLinker::loadAlternateSpellings(Symbol filepath) {
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(filepath.to_string());
	// XXX put error checking here -- SRS
	std::wstring buf;
	std::wstring entry;
	while (!in.eof()) {
		in.getLine(buf);
		size_t begin = 0;
		size_t iter = buf.find(L';', 0);
		if (iter != string::npos) {
			entry = buf.substr(begin, iter - begin);
			std::transform(entry.begin(), entry.end(), entry.begin(), towlower);
			Symbol head(entry.c_str());
			iter += 2;
			begin = iter;
			while ((iter = buf.find(L';', iter)) != string::npos) {
				entry = buf.substr(begin, iter - begin).c_str();
				std::transform(entry.begin(), entry.end(), entry.begin(), towlower);
				(*_alternateSpellings)[Symbol(entry.c_str())] = head;
				iter += 2;
				begin = iter;
			}
			entry = buf.substr(begin).c_str();
			std::transform(entry.begin(), entry.end(), entry.begin(), towlower);
			(*_alternateSpellings)[Symbol(entry.c_str())] = head;
		}
	}
}

void SpanishRuleNameLinker::loadFileIntoTable(Symbol filepath, HashTable * table) {
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	// XXX put error checking here -- SRS
	in.open(filepath.to_string());
	std::wstring entry;
	while (!in.eof()) {
		in.getLine(entry);
		std::transform(entry.begin(), entry.end(), entry.begin(), towlower);
		(*table)[Symbol(entry.c_str())] = Symbol(L"1");
	}
}

Symbol SpanishRuleNameLinker::linkPERMention(LexEntitySet *currSolution, Mention *currMention) {
	HashTable::iterator iter;
	int rank = 0;
	int newRank = 0;
	Symbol entID = Symbol();
	bool reject = false;
	bool has_first_last_name = false;
	for (iter = _per_attributes->begin(); iter != _per_attributes->end(); ++iter) {
		if (reject)
			continue;
		Symbol namePart = (*iter).second;
		if (!getValueFromTable(_per_mentions, (*iter).first).is_null()) {
			// if found a match
			newRank = 0;
			if (namePart == ATTR_FULLEST)
				newRank = RANK_LEVEL_100;
			else if (namePart == ATTR_FIRST_MI_LAST)
				newRank = RANK_LEVEL_90;
			else if (namePart == ATTR_FIRST_LAST)
				newRank = RANK_LEVEL_60;
			else if (namePart == ATTR_LAST)
				newRank = RANK_LEVEL_10;
		}
		else {
			if (namePart == ATTR_LAST)
				reject = true;
			if (namePart == ATTR_FIRST_LAST)
				has_first_last_name = true;
		}
		if (newRank > rank) {
			rank = newRank;
			entID = getValueFromTable(_per_mentions, (*iter).first);
		}
	}

	// this says John Smith can't link to Jane Smith
	//   it also prevents it from linking to Smith -- is that OK? 
	//   I would think yes, since we generally never expand names later in the doc
	if (has_first_last_name && rank <= RANK_LEVEL_10)
		entID = Symbol();

	if (reject) {
		entID = Symbol();
	}

	if (!entID.is_null())
		return entID;

	return checkForContextLinks(currSolution, currMention);
}

Symbol SpanishRuleNameLinker::linkORGMention(LexEntitySet *currSolution, Mention *currMention) {
	HashTable::iterator iter;
	int rank = 0;
	int newRank = 0;
	Symbol entID = Symbol();
	for (iter = _org_attributes->begin(); iter != _org_attributes->end(); ++iter) {
		if (!getValueFromTable(_org_mentions, (*iter).first).is_null()) {
			// if found a match
			newRank = 0;
			Symbol namePart = (*iter).second;
			if (namePart == ATTR_FULLEST)
				newRank = RANK_LEVEL_100;
			else if (namePart == ATTR_NO_DESIG)
				newRank = RANK_LEVEL_90;
			else if (namePart == ATTR_SINGLE_WORD)
				newRank = RANK_LEVEL_50;
			// EMB 1/6/04: don't want the _acronym_ of a full name to be linked _back_
			//   (If the name actually is an acronym, it would be ATTR_FULLEST)
			//   we assume that the full name would be listed first
			//   This eliminates "Iraqi Air Force" and "Irani Air Force" spurious link.
			//   What we really would want is to allow this iff we were linking back
			//     to a ATTR_FULLEST -- but there's no way to know that.
			//else if (namePart == ATTR_ABBREV)
			//	newRank = RANK_LEVEL_20;
		}
		if (newRank > rank) {
			rank = newRank;
			entID = getValueFromTable(_org_mentions, (*iter).first);
		}
	}
	if (!entID.is_null())
		return entID;

	return checkForContextLinks(currSolution, currMention);
}

Symbol SpanishRuleNameLinker::linkGPEMention(LexEntitySet *currSolution, Mention *currMention) {
	HashTable::iterator iter;
	int rank = 0;
	int newRank = 0;
	Symbol entID = Symbol();
	for (iter = _gpe_attributes->begin(); iter != _gpe_attributes->end(); ++iter) {
		if (!getValueFromTable(_gpe_mentions, (*iter).first).is_null()) {
			// if found a match
			newRank = 0;
			Symbol namePart = (*iter).second;
			if (namePart == ATTR_FULLEST)
				newRank = RANK_LEVEL_100;
			else if (namePart == ATTR_SINGLE_WORD)
				newRank = RANK_LEVEL_90;
		}
		if (newRank > rank) {
			rank = newRank;
			entID = getValueFromTable(_gpe_mentions, (*iter).first);
		}
	}
	if (!entID.is_null())
		return entID;

	return checkForContextLinks(currSolution, currMention);
}

Symbol SpanishRuleNameLinker::linkOTHMention(LexEntitySet *currSolution, Mention *currMention) {
	HashTable::iterator iter;
	Symbol entID = Symbol();
	for (iter = _oth_attributes->begin(); iter != _oth_attributes->end(); ++iter) {
		if (!getValueFromTable(_oth_mentions, (*iter).first).is_null()) {
			// if found a match
			// EMB 1/7/04: see comment in linkORGMention
			if ((*iter).second != ATTR_ABBREV)
				entID = getValueFromTable(_oth_mentions, (*iter).first);
		}
	}

	if (!entID.is_null())
		return entID;

	return checkForContextLinks(currSolution, currMention);
}

void SpanishRuleNameLinker::mergeMentions(HashTable * source, HashTable * destination,
								   Symbol entID)
{
	HashTable::iterator iter;
	for (iter = source->begin(); iter != source->end(); ++iter) {
		(*destination)[(*iter).first] = entID;
	}
}

void SpanishRuleNameLinker::generateOTHVariations(int wordCount, 
										   const wchar_t **wordArray,
										   HashTable *attributes)
{
	attributes->clear();
	wstring str = wordArray[0];
	for (int i = 1; i < wordCount; i++) {
		str += L" ";
		str += wordArray[i];
	}
	addAttribute(attributes, Symbol(str.c_str()), ATTR_FULLEST);
}

void SpanishRuleNameLinker::generateGPEVariations(int wordCount,
										   const wchar_t **wordArray,
										   HashTable *attributes)
{
	attributes->clear();
	
	wstring str = L"";
	for (int i = 0; i < wordCount - 1; i++) {
		str += wordArray[i];
		str += L" ";
	}

	// check alternate spelling for the last word in a GPE name. 
	// (for example, add "north korea" in place of "north koreans")
	std::wstring word(wordArray[wordCount - 1]);
	std::transform(word.begin(), word.end(), word.begin(), towlower);
	Symbol wordSym = Symbol(word.c_str());
	if (!getValueFromTable(_alternateSpellings, wordSym).is_null()) {
		wstring norm_str = str;
		norm_str += getValueFromTable(_alternateSpellings, wordSym).to_string();
		addAttribute(attributes, Symbol(norm_str.c_str()), ATTR_FULLEST);
	}

	str += wordArray[wordCount - 1];

	if (wordCount == 1)
		addAttribute(attributes, Symbol(str.c_str()), ATTR_SINGLE_WORD);
	else
		addAttribute(attributes, Symbol(str.c_str()), ATTR_FULLEST);
}

void SpanishRuleNameLinker::generateORGVariations(int wordCount,
										   const wchar_t **wordArray,
										   HashTable *attributes)
{
	attributes->clear();

	wstring buffer = L"";
	wstring fullest_str = L"";
	wstring no_desig_str = L"";
	wstring abbrev_str = L"";
	wstring first_word = L"";
	wstring second_word = L"";
	wstring last_word = L"";

	buffer = wordArray[wordCount - 1];
	fullest_str = buffer;

	if (getValueFromTable(_designators, Symbol(buffer.c_str())).is_null()) {
		// corporate designator match
		no_desig_str = buffer;
	}
	for (int i = wordCount - 2; i >= 0; i--) {
		// construct fullest name
		buffer = fullest_str;
		fullest_str = wordArray[i];
		fullest_str += L" ";
		fullest_str += buffer;

		if (!getValueFromTable(_designators, Symbol(fullest_str.c_str())).is_null())
		{ 
			no_desig_str = L"";
			abbrev_str = L"";
		}
		else {
			if (no_desig_str.length() == 0) {
				no_desig_str = wordArray[i];
			}
			else {
				buffer = no_desig_str;
				no_desig_str = wordArray[i];
				no_desig_str += L" ";
				no_desig_str += buffer;
			}

			// abbrevs turned off for now by SRS

			// construct abbreviation name
//			wcscpy(buffer, abbreviationName);
//			firstChar[0] = wordArray[i][0];
//			if ((firstChar[0] != '\'') && (firstChar[0] != '-')) {
//				wcscpy(abbreviationName, firstChar);
//				wcscat(abbreviationName, buffer);
//			}
			
		}
	}

	if (wordCount == 1)
		addAttribute(attributes, Symbol(fullest_str.c_str()), ATTR_SINGLE_WORD);
	else
		addAttribute(attributes, Symbol(fullest_str.c_str()), ATTR_FULLEST);
	addAttribute(attributes, Symbol(no_desig_str.c_str()), ATTR_NO_DESIG);

	// "The 1st Battalion" --> "The 1st", "1 Battalion"
	if (ITEA_LINKING && (wordCount >= 2)) {
		first_word = wordArray[0];
		second_word = wordArray[1];
		last_word = wordArray[wordCount - 1];

		Symbol first = Symbol(first_word.c_str());
		Symbol second = Symbol(second_word.c_str());
		Symbol last = Symbol(last_word.c_str());
		wstring ordinal = L"";

		if (WordConstants::isOrdinal(first)) ordinal = first_word;
		if (WordConstants::isOrdinal(second)) ordinal = second_word;

		if (WordConstants::isMilitaryWord(last) &&
			(WordConstants::isOrdinal(first) || 
			 (!first_word.compare(L"the") && WordConstants::isOrdinal(second))))
		{
			wstring abbrev = L"the " + ordinal;
			addAttribute(attributes, Symbol(abbrev.c_str()), ATTR_MILITARY);
			addAttribute(attributes, Symbol(ordinal.c_str()), ATTR_MILITARY);

			Symbol numberSym = WordConstants::getNumericPortion(Symbol(ordinal.c_str()));
			wstring number = wstring(numberSym.to_string());
			wstring abbrev2 = number + L" " + last_word;
			addAttribute(attributes, Symbol(abbrev2.c_str()), ATTR_MILITARY);
			//std::cout << "added " << Symbol(abbrev2.c_str()).to_debug_string() << "\n";
		}
	}
	
	// abbrevs turned off for now by SRS
//	if (wcslen(abbreviationName) > 1)
//		addAttribute(attributes, Symbol(abbreviationName), Symbol(L"AbbreviationName"));
}

namespace {
	Symbol FNU(L"FNU"); // first name unknown
	Symbol LNU(L"LNU"); // last name unknown
}

void SpanishRuleNameLinker::generatePERVariations(int word_count,
										   const wchar_t **wordArray,
										   HashTable *attributes,
										   HashTable *uniqAttributes)
{
	attributes->clear();
	uniqAttributes->clear();

	// remove any funny markers in the name e.g. LNU, FNU, parens
	if (ITEA_LINKING) {
		int current = 0;
		int new_word_count = word_count;
		for (int i = 0; i < word_count; i++) {
			Symbol word = Symbol(wordArray[i]);
			if (SpanishWordConstants::isParenOrBracket(word) || (word==FNU) || (word==LNU)) {
				new_word_count--;
				continue;
			}
			wordArray[current++] = wordArray[i];
		}
		word_count = new_word_count;
		if (word_count == 0) return;
	}

	wstring buffer;
	wstring middleNameBuffer;
	bool has_suffix = !getValueFromTable(_suffixes, Symbol(wordArray[word_count-1])).is_null();

	// Determine which word in the word array corresponds to which name
	int first_name = -1;
	int middle_name_start = -1;
	int middle_name_end = -1;
	int last_name = -1;
	int suffix = -1;
	
	if (isReversedCommaName(wordArray, word_count)) {
		last_name = 0;
		first_name = 2;
		if (word_count >= 4) middle_name_start = 3;
		if (word_count >= 4) middle_name_end = word_count - 1;
	} else if (word_count == 1) {
		last_name = 0;
	} else if (word_count == 2 && !has_suffix) {
		first_name = 0;
		last_name = 1;
	} else if (word_count == 2 && has_suffix) {
		first_name = 0;
		suffix = 1;
	} else if (word_count > 2 && !has_suffix) {
		first_name = 0;
		last_name = word_count - 1;
		middle_name_start = 1;
		middle_name_end = word_count - 2;
	} else if (word_count > 2 && has_suffix) {
		first_name = 0;
		last_name = word_count - 2;
		suffix = word_count - 1;
		if (word_count > 3) middle_name_start = 1;
		if (word_count > 3) middle_name_end = word_count - 3;
	}

	// Add attributes for names where we have data available
	if (last_name != -1) {
		addAttribute(attributes, Symbol(wordArray[last_name]), ATTR_LAST);
	}
	if (first_name != -1 && last_name != -1) {
		buffer = wordArray[first_name];
		buffer += L' ';
		buffer += wordArray[last_name];
		addAttribute(attributes, Symbol(buffer), ATTR_FIRST_LAST);
	}
	if (middle_name_start != -1) {
		wstring middleName = wordArray[middle_name_start];
		wstring middleInitial = middleName.substr(0, 1);
		addAttribute(uniqAttributes, Symbol(middleInitial), ATTR_MI);
	}
	if (first_name != -1 && middle_name_start != -1 && last_name != -1) {
		wstring middleName = wordArray[middle_name_start];
		wstring middleInitial = middleName.substr(0, 1);
		buffer = wordArray[first_name];
		buffer += L' ';
		buffer += middleInitial;
		buffer += L' ';
		buffer += wordArray[last_name];
		addAttribute(attributes, Symbol(buffer), ATTR_FIRST_MI_LAST);
	}
	if (middle_name_start != -1 && middle_name_end != -1) {
		middleNameBuffer = wordArray[middle_name_start];
		for (int i = middle_name_start + 1; i <= middle_name_end; i++) {
			middleNameBuffer += L' ';
			middleNameBuffer += wordArray[i];
		}
		addAttribute(uniqAttributes, Symbol(middleNameBuffer), ATTR_MIDDLE);
	}
	if (suffix != -1) {
		addAttribute(uniqAttributes, Symbol(wordArray[suffix]), ATTR_SUFFIX);
	}
	// Make ATTR_FULLEST
	buffer = wstring();
	if (first_name != -1) 
		buffer += wordArray[first_name];
	if (middle_name_start != -1 && middle_name_end != -1) {
		if (buffer.length() != 0) buffer += L' ';
		buffer += middleNameBuffer;
	}
	if (last_name != -1) {
		if (buffer.length() != 0) buffer += L' ';
		buffer += wordArray[last_name];
	}
	if (suffix != -1) {
		if (buffer.length() != 0) buffer += L' ';
		buffer += wordArray[suffix];
	}	
	if (buffer.length() != 0) {
		addAttribute(attributes, Symbol(buffer), ATTR_FULLEST);
	}
}

// Check for names of the sort "Smith, John"
bool SpanishRuleNameLinker::isReversedCommaName(const wchar_t **wordArray, int word_count) {
	static const Symbol _COMMA_(L",");
	return word_count > 2 && Symbol(wordArray[1]) == _COMMA_;
}

void SpanishRuleNameLinker::addAcronymAttributes(int wordCount,
										   Symbol *wordArray,
										   HashTable *attributes)
{
	if (!ITEA_LINKING)
		return;
	Symbol acronyms[16];
	int nAcronyms = AcronymMaker::getSingleton().generateAcronyms(wordArray, wordCount, acronyms, 16);
	for(int i=0; i<nAcronyms; i++) {
		addAttribute(attributes, acronyms[i], ATTR_ABBREV);
	}

}

void SpanishRuleNameLinker::addAttribute(HashTable *table, Symbol key, Symbol value) {
	std::wstring attribute(key.to_string());
	std::transform(attribute.begin(), attribute.end(), attribute.begin(), towlower);
	replaceChar(attribute, L".", L"");
	Symbol lowerKey = Symbol(attribute.c_str());
	if (getValueFromTable(table, lowerKey).is_null()) {
		(*table)[lowerKey] = value;

		// check for alternate spellings
		Symbol altSpelling = getValueFromTable(_alternateSpellings, lowerKey); 
		if (!altSpelling.is_null()) {
			(*table)[altSpelling] = value;
		}
	}
}

void SpanishRuleNameLinker::replaceChar(wstring &str, const wstring &from,
								 const wstring &to)
{
	size_t iter = 0;
	while( (iter = str.find(from, iter)) != string::npos) {
		str.replace(iter, 1, to);
		iter += 1;
	}
}


Symbol SpanishRuleNameLinker::getValueFromTable(HashTable *table, Symbol key) {
	HashTable::iterator iter;
	iter = table->find(key);
	if (iter == table->end()) {
		return Symbol();
	}
	return (*iter).second;
}

Mention *SpanishRuleNameLinker::getMentionFromTerminal(LexEntitySet *currSolution, 
							   Mention *currMention, const SynNode *terminal)
{
	if (terminal == 0 || terminal->getParent() == 0 || 
		terminal->getParent()->getParent() == 0)
		return 0;

	const SynNode *parent = terminal->getParent()->getParent();
	if (parent->hasMention()) {
		int local_id = parent->getMentionIndex();
		int sentence_id = currMention->getSentenceNumber();
		MentionUID global_id(sentence_id, local_id);
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


Symbol SpanishRuleNameLinker::getEntityIDFromTerminal(LexEntitySet *currSolution, 
							   Mention *currMention, const SynNode *terminal) 
{
	Mention *mention = getMentionFromTerminal(currSolution, currMention, terminal);
	if (mention == 0)
		return Symbol();
	Entity *ent = currSolution->getEntityByMention(mention->getUID());
	if (ent != 0) {
		wchar_t buffer[11];
#ifdef _WIN32
		return Symbol(_itow(ent->getID(), buffer, 10));
#else
		swprintf (buffer, sizeof(buffer)/sizeof(buffer[0]), L"%d",
			  ent->getID());
		return Symbol(buffer);
#endif
	}
	return Symbol();
}

Symbol SpanishRuleNameLinker::checkForContextLinks(LexEntitySet *currSolution, Mention *currMention) {
	if (!ITEA_LINKING)
		return Symbol();

	Symbol entID = checkForAKA(currSolution, currMention);
	if (!entID.is_null())
		return entID;

	entID = checkForParenDef(currSolution, currMention);
	if (!entID.is_null())
		return entID;

	return Symbol();
}

namespace {
	const Symbol ES_SYM(L"es");   // is: ...is aka...
	const Symbol ERA_SYM(L"era"); // was: ...was aka...
}


Symbol SpanishRuleNameLinker::checkForAKA(LexEntitySet *currSolution, 
							   Mention *currMention) 
{
	if (!currMention->getEntityType().matchesFAC() &&
		!currMention->getEntityType().matchesGPE() &&
		!currMention->getEntityType().matchesPER() &&
		!currMention->getEntityType().matchesORG())
		return Symbol();

	// not sure whether to check forward or backward -- not sure which will have an ID already
	// also, have to go inside complex mentions, or we'll miss things
	const SynNode *node = currMention->getNode();
	while (!node->isPreterminal()) {
		// going forward
		const SynNode *next = node->getNextTerminal();
		while (next != 0 && WordConstants::isPunctuation(next->getTag()))
			next = next->getNextTerminal();
		if (next != 0) {
			Symbol potentialAKA = next->getTag();
			if (potentialAKA == ES_SYM || potentialAKA == ERA_SYM) {
				next = next->getNextTerminal();
				if (next != 0) 
					potentialAKA = next->getTag();
			}
			if (SpanishWordConstants::isAKA(potentialAKA))
			{
				next = next->getNextTerminal();
				while (next != 0 && WordConstants::isPunctuation(next->getTag()))
					next = next->getNextTerminal();
				Symbol entID = getEntityIDFromTerminal(currSolution, currMention, next);
				
				if (entID.is_null() && next != 0 && SpanishWordConstants::isDefiniteArticle(next->getTag())) {
					next = next->getNextTerminal();
					entID = getEntityIDFromTerminal(currSolution, currMention, next);
				}

				if (!entID.is_null())
					return entID;
			}
		}

		// going backward
		// deal with fact that AKA will often get absorbed into name (e.g. "AKA My Company")
		Symbol potentialAKA = node->getFirstTerminal()->getTag();
		const SynNode *prev = node->getPrevTerminal();
		bool found_aka = false;
		if (SpanishWordConstants::isAKA(potentialAKA)) {
			found_aka = true;
		} else {
			while (prev != 0 && 
				   (WordConstants::isPunctuation(prev->getTag()) || SpanishWordConstants::isDefiniteArticle(prev->getTag())))
				prev = prev->getPrevTerminal();
			if (prev != 0) {
				potentialAKA = prev->getTag();
				if (SpanishWordConstants::isAKA(potentialAKA))
					found_aka = true;
				prev = prev->getPrevTerminal();
			}			
		}
		if (prev != 0 && found_aka) {
			if (prev->getTag() == ES_SYM || prev->getTag() == ERA_SYM)
				prev = prev->getPrevTerminal();
			while (prev != 0 && WordConstants::isPunctuation(prev->getTag()))
				prev = prev->getPrevTerminal();
			Symbol entID = getEntityIDFromTerminal(currSolution, currMention, prev);

			if (!entID.is_null())
				return entID;
		}
		
		node = node->getHead();
	}

	return Symbol();
}


Symbol SpanishRuleNameLinker::checkForParenDef(LexEntitySet *currSolution, Mention *currMention) 
{
	if (!currMention->getEntityType().matchesFAC() &&
		!currMention->getEntityType().matchesGPE() &&
		!currMention->getEntityType().matchesPER() &&
		!currMention->getEntityType().matchesORG())
		return Symbol();

	const SynNode *node = currMention->getNode();
	while (!node->isPreterminal()) {


		const SynNode *next = node->getNextTerminal();
		if (next != 0 && SpanishWordConstants::isOpenBracket(next->getTag())) {
			next = next->getNextTerminal();
			Mention *mention = getMentionFromTerminal(currSolution, currMention, next);
			if (mention != 0) {
				const SynNode* bracket = mention->getNode()->getNextTerminal();
				if (bracket != 0 && SpanishWordConstants::isClosedBracket(bracket->getTag())) {
					Symbol entID = getEntityIDFromTerminal(currSolution, currMention, next);
					if (!entID.is_null())
						return entID;		
				}
			}
		}

		const SynNode *prev = node->getPrevTerminal();
		next = node->getNextTerminal();
		if (prev != 0 && SpanishWordConstants::isOpenBracket(prev->getTag()) &&
			next != 0 && SpanishWordConstants::isClosedBracket(next->getTag())) {
			prev = prev->getPrevTerminal();
			Symbol entID = getEntityIDFromTerminal(currSolution, currMention, prev);
			if (!entID.is_null())
				return entID;		
		}

		next = node->getNextTerminal();
		if (next != 0 && SpanishWordConstants::isClosedDoubleBracket(next->getTag())) {
			next = next->getNextTerminal();
			if (next != 0 && SpanishWordConstants::isOpenBracket(next->getTag())) {
				Mention *mention = getMentionFromTerminal(currSolution, currMention, next);
				if (mention != 0) {
					const SynNode* bracket = mention->getNode()->getNextTerminal();
					if (bracket != 0 && SpanishWordConstants::isClosedBracket(bracket->getTag())) {
						Symbol entID = getEntityIDFromTerminal(currSolution, currMention, next);
						if (!entID.is_null())
							return entID;		
					}
				}
			}
		}

		prev = node->getPrevTerminal();
		next = node->getNextTerminal();
		if (prev != 0 && SpanishWordConstants::isOpenBracket(prev->getTag()) &&
			next != 0 && SpanishWordConstants::isClosedBracket(next->getTag())) {
			prev = prev->getPrevTerminal();
			if (prev != 0 && SpanishWordConstants::isClosedDoubleBracket(prev->getTag())) {
				prev = prev->getPrevTerminal();
				Symbol entID = getEntityIDFromTerminal(currSolution, currMention, prev);
				if (!entID.is_null())
					return entID;
			}
		}

		node = node->getHead();
	}

	return Symbol();
}

