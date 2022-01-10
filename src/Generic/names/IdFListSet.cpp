// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/names/IdFListSet.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/IdFSentenceTokens.h"
#include "Generic/names/discmodel/PIdFSentence.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/ParamReader.h"
#include <iostream>
#include <boost/scoped_ptr.hpp>

IdFListSet::IdFListSet(const char *file_name) 
	: numLists(0)
{
	if (file_name == 0)
		return;

	boost::scoped_ptr<UTF8InputStream> countStream_scoped_ptr(UTF8InputStream::build(file_name));
	UTF8InputStream& countStream(*countStream_scoped_ptr);
	UTF8Token token;
	numLists = 0;
	int numListItems = 0;
	// count lists (for feature value array),
	//   and count number of items in lists (for approximate hash size)
	while (!countStream.eof()) {
		countStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		if (token.chars()[0] == '#')
			continue;

		// Convert to ASCII string so ParamReader may expand the path
		std::wstring ws = token.chars();
		std::string s(ws.begin(), ws.end());
		s.assign(ws.begin(), ws.end());

		s = ParamReader::expand(s);

		numLists++;
		boost::scoped_ptr<UTF8InputStream> listStream_scoped_ptr(UTF8InputStream::build(s.c_str()));
		UTF8InputStream& listStream(*listStream_scoped_ptr);
		UTF8Token listToken;
		while (!listStream.eof()) {
			listStream >> listToken;
			if (listToken.symValue() == SymbolConstants::leftParen)
				numListItems++;
		}
		listStream.close();
	}
	countStream.close();

	// allocate feature value array
	listFeatures = _new Symbol[numLists];
	for (int i = 0; i < numLists; i++) {
		wchar_t feature[100];
#ifdef _WIN32
		swprintf(feature, L":listFeature%d", i);
#else
		swprintf(feature, 100, L":listFeature%d", i);
#endif
		listFeatures[i] = Symbol(feature);
	}

	//std::cout << "found " << numListItems << " names in " << numLists << " lists\n";

	_graphHash = _new GraphHash(numListItems, hasher, eqTester);
	_featureTable = _new FeatureTable(static_cast<int>(numListItems / .7), hasher, eqTester);	

	boost::scoped_ptr<UTF8InputStream> fileStream_scoped_ptr(UTF8InputStream::build(file_name));
	UTF8InputStream& fileStream(*fileStream_scoped_ptr);

	// read in actual lists
	int count = 0;
	while (!fileStream.eof()) {
		fileStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		if (token.chars()[0] == '#')
			continue;

		// Convert to ASCII string so ParamReader may expand the path
		std::wstring ws = token.chars();
		std::string s(ws.begin(), ws.end());
		s.assign(ws.begin(), ws.end());

		s = ParamReader::expand(s);

		// Convert back to wide character string so we can read the list
		std::wstring ws2(s.length(),L' ');
		std::copy(s.begin(), s.end(), ws2.begin());

		readList(ws2.c_str(), count);
		count++;
	}
	fileStream.close();
}

Symbol IdFListSet::getFeature(Symbol sym) const
{
	FeatureTable::const_iterator iter;
	iter = _featureTable->find(sym);
	if (iter == _featureTable->end())
		return Symbol();
	else return (*iter).second;
}

int IdFListSet::isListMember(IdFSentenceTokens *tokens, int start_index) const 
{
	Symbol firstWord = tokens->getWord(start_index);

	GraphHash::const_iterator iter;
	iter = _graphHash->find(firstWord);
	if (iter == _graphHash->end())
		return 0;
	else return matchNode((*iter).second, tokens, start_index + 1);		

}
int IdFListSet::isListMember(PIdFSentence *tokens, int start_index) const 
{
	Symbol firstWord = tokens->getWord(start_index);

	GraphHash::const_iterator iter;
	iter = _graphHash->find(firstWord);
	if (iter == _graphHash->end())
		return 0;
	else return matchNode((*iter).second, tokens, start_index + 1);		

}

int IdFListSet::matchNode(ListNode *node, IdFSentenceTokens *tokens, int index) const
{
	// shouldn't ever happen
	if (index > tokens->getLength())
		return 0;

	if (index == tokens->getLength()) {
		for (ListNode *node_iter = node; node_iter != 0; node_iter = node_iter->next) {
			if (node_iter->children == 0)
				return 1;
		}
		return 0;
	}
	bool end_found = false;
	int length = 0;
	for (ListNode *node_iter = node; node_iter != 0; node_iter = node_iter->next) {
		if (node_iter->children == 0)
			end_found = true;
		if (node_iter->word == tokens->getWord(index)) {
			length = matchNode(node_iter->children, tokens, index + 1);
			break;
		}
	}

	if (length != 0)
		return length + 1;
	else if (end_found)
		return 1;
	else return 0;

}

int IdFListSet::matchNode(ListNode *node, PIdFSentence *tokens, int index) const
{
	// shouldn't ever happen
	if (index > tokens->getLength())
		return 0;

	if (index == tokens->getLength()) {
		for (ListNode *node_iter = node; node_iter != 0; node_iter = node_iter->next) {
			if (node_iter->children == 0)
				return 1;
		}
		return 0;
	}
	bool end_found = false;
	int length = 0;
	for (ListNode *node_iter = node; node_iter != 0; node_iter = node_iter->next) {
		if (node_iter->children == 0)
			end_found = true;
		if (node_iter->word == tokens->getWord(index)) {
			length = matchNode(node_iter->children, tokens, index + 1);
			break;
		}
	}

	if (length != 0)
		return length + 1;
	else if (end_found)
		return 1;
	else return 0;

}

void IdFListSet::readList(const wchar_t *list_file_name, int list_num) {

	boost::scoped_ptr<UTF8InputStream> listStream_scoped_ptr(UTF8InputStream::build(list_file_name));
	UTF8InputStream& listStream(*listStream_scoped_ptr);
	UTF8Token token;
	Symbol extra_words_ngram[MAX_IDF_LIST_NAME_LENGTH];

	int count = 0;
	while (!listStream.eof()) {
		listStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		if (token.symValue() != SymbolConstants::leftParen) {
          char c[1000];
          sprintf( c, "ERROR: ill-formed list entry in %s: %d\n", 
			  Symbol(list_file_name).to_debug_string(), count);
		  throw UnexpectedInputException("IdFListSet::readList", c);
        }

		listStream >> token;
		Symbol firstWord = token.symValue();
		
		listStream >> token;
		Symbol nextWord = token.symValue();
		int extra_words = 0;
		while (nextWord != SymbolConstants::rightParen) {
			extra_words_ngram[extra_words] = nextWord;
			extra_words++;
			listStream >> token;
			nextWord = token.symValue();
			if (listStream.eof() ||	nextWord == SymbolConstants::leftParen) {
				char c[1000];
				sprintf( c, "ERROR: list entry without close paren in %s at line %d.  nextWord is '%s'\n", 
					Symbol(list_file_name).to_debug_string(), count, nextWord.to_debug_string());
				throw UnexpectedInputException("IdFListSet::readList", c);
			}
		}

		addToTable(firstWord, list_num, extra_words_ngram, extra_words);
		count++;

	}

}

void IdFListSet::addToTable(Symbol firstWord, int list_num, Symbol* ngram, int extra_words) {
	
	(*_featureTable)[barSymbols(firstWord, ngram, extra_words)] = listFeatures[list_num];

	GraphHash::iterator iter;
	iter = _graphHash->find(firstWord);
	if (iter == _graphHash->end()) {
		// "France" --> X
		if (extra_words == 0)
			(*_graphHash)[firstWord] = _new ListNode();
		else {
			// "United" --> "Nations" --> X
			ListNode *node = _new ListNode(ngram[0]);
			(*_graphHash)[firstWord] = node;
			for (int i = 1; i < extra_words; i++) {
				ListNode *new_node = _new ListNode(ngram[i]);
				node->children = new_node;
				node = new_node;
			}
			node->children = _new ListNode();
		}
	} else {
		ListNode *node = (*iter).second;
		ListNode *node_iter = node;
		ListNode *end_of_chain = node;
		bool finished_name = false;
		for (int i = 0; i < extra_words; i++) {
			node_iter = node;			
			while (node_iter != 0) { 
				end_of_chain = node_iter;
				if (node_iter->word == ngram[i]) {
					node = node_iter->children;
					break;
				} else {
					node_iter = node_iter->next;
				}
			}
			if (node_iter == 0) {
				// this is the case where part of the name
				// is already present, but not the whole thing
				ListNode *node2 = _new ListNode(ngram[i]);
				end_of_chain->next = node2;
				for (int j = i + 1; j < extra_words; j++) {
					ListNode *new_node = _new ListNode(ngram[j]);
					node2->children = new_node;
					node2 = new_node;
				}
				node2->children = _new ListNode();
				finished_name = true;
				break;
			} 
		}
		if (!finished_name) {
			// so, we have all the words, but do we have an "end here" node?
			node_iter = node;
			while (node_iter != 0) { 
				end_of_chain = node_iter;
				if (node_iter->children == 0) {
					// yes, we do
					break;
				} else {
					node_iter = node_iter->next;
				}
			}
			// no, we don't
			if (node_iter == 0)
				end_of_chain->next = _new ListNode();
		}
	}

}


Symbol IdFListSet::barSymbols(IdFSentenceTokens *tokens, int start_index, int length) const
{
	std::wstring wstr = tokens->getWord(start_index).to_string();
	for (int j = start_index + 1; j < start_index + length; j++) {
		wstr += L"_";
		wstr += tokens->getWord(j).to_string();
	}
	return Symbol(wstr.c_str());
}

Symbol IdFListSet::barSymbols(Symbol firstWord, Symbol* ngram, int extra_words)
{
	std::wstring wstr = firstWord.to_string();
	for (int i = 0; i < extra_words; i++) {
		wstr += L"_";
		wstr += ngram[i].to_string();
	}
	return Symbol(wstr.c_str());
}
