// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// Copyright (c) 2006 by BBN Technologies, Inc.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "ATEASerif_generic/titles/TitleListNode.h"
#include "ATEASerif_generic/titles/TitleMap.h"
#include "common/UTF8InputStream.h"
#include "common/UTF8Token.h"
#include "common/Symbol.h"
#include "common/UnexpectedInputException.h"
#include <string>

#define MAX_TITLE_LENGTH 10

const float TitleMap::_target_loading_factor = static_cast<float>(0.7);

using namespace std;

TitleMap::TitleMap(UTF8InputStream &in) 
{
	int numBuckets = static_cast<int>(1000 / _target_loading_factor);

	if (numBuckets < 5)
		numBuckets = 5;
	_map = _new Map(numBuckets);

	UTF8Token token;
	Symbol buffer[MAX_TITLE_LENGTH];

	std::wstring line;
	while (!in.eof()) {
		in.getLine(line);
		line = trim(line);

		size_t pos = line.find(L" ", 0);
		size_t last_pos = -1;
		int title_length = 0;

		while (pos != wstring::npos) {
			wstring word = line.substr(last_pos + 1, pos - last_pos - 1);
			word = trim(word);

			if (word.length() > 0) {
				if (title_length >= MAX_TITLE_LENGTH)
					throw UnexpectedInputException("TitleMap::TitleMap", "title too long, limit is 10");
				buffer[title_length++] = Symbol(word.c_str());
				//std::cout << Symbol(word.c_str()).to_debug_string() << " ";
			}

			last_pos = pos;
			pos = line.find(L" ", last_pos + 1);
		}
		wstring last_word = line.substr(last_pos + 1);
		last_word = trim(last_word);
		if (last_word.length() > 0) {
			if (title_length >= MAX_TITLE_LENGTH)
				throw UnexpectedInputException("TitleMap::TitleMap", "title too long, limit is 10");
			buffer[title_length++] = Symbol(last_word.c_str());
			//std::cout << Symbol(last_word.c_str()).to_debug_string() << " ";
		}

		if (title_length > 0) {
			TitleListNode* titleNode = new TitleListNode(buffer, title_length);
			TitleListNode* existingTitle = lookup(buffer[0]);
			if (existingTitle) 
				_insertNewNode(titleNode, existingTitle);
			else {
				(*_map)[buffer[0]] = titleNode;
				//std::cout << "adding " << buffer[0] << "... length: " << title_length << "\n";  
			}
		}
		//std::cout << "\n";
	}
}

TitleListNode* TitleMap::lookup(const Symbol s) const
{
	Map::iterator iter;

	iter = _map->find(s);
	if (iter == _map->end()) {
		return NULL;
	}
	return (*iter).second;
}

void TitleMap::_insertNewNode(TitleListNode *newNode, TitleListNode *node) {
	while (node != NULL) {
		if (newNode->getTitleLength() > node->getTitleLength()) {
			if (node->getPrev()) {
				node->getPrev()->setNext(newNode);
				newNode->setPrev(node->getPrev());
			} else {
				(*_map)[newNode->getTitleWord(0)] = newNode;
			}
			newNode->setNext(node);
			node->setPrev(newNode);
			return;
		}
		
		if (node->getNext() == NULL) {
			node->setNext(newNode);
			newNode->setPrev(node);
			return;
		}
		node = node->getNext();
	}
}

wstring TitleMap::trim(wstring line)
{
	while (line.length() > 0 && iswspace(line.at(0)))
		line = line.substr(1);
	
	while (line.length() > 0 && iswspace(line.at(line.length() - 1)))
		line = line.substr(0, line.length() - 1);

	return line;
}
