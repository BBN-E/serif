// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEADLESS_PARSE_NODE_H
#define HEADLESS_PARSE_NODE_H

#include <cstddef>
#include <string>
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/trainers/HeadFinder.h"
#define HEADLESS_PARSE_NODE_BLOCK_SIZE 1000
//NOTE:  add _headFinder as a class member because chinese headfinder is not 
//static
class HeadlessParseNode {
private:
	HeadFinder* _headFinder;

    static const size_t blockSize;
public:
    Symbol label;
    HeadlessParseNode* children;
    HeadlessParseNode* next;
	bool isPreTerminal;

    HeadlessParseNode(Symbol labelArg, HeadFinder* hf)
        : label(labelArg), children(0), next(0), isPreTerminal(false)
    {_headFinder = hf;}
    HeadlessParseNode(HeadFinder* hf)
        : children(0), next(0), isPreTerminal(false)
    {_headFinder = hf;}

   ~HeadlessParseNode()
    {
        delete children;
        delete next;
    }
	std::wstring Headify_to_string();
	void read(UTF8InputStream& stream);
	void addModsToString(HeadlessParseNode* modifier, std::wstring& result);
    static void* operator new(size_t);
    static void operator delete(void* object);
private:
    static HeadlessParseNode* freeList;
	bool containsNP();
};

#endif

