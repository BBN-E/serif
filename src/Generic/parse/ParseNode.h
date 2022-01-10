// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARSE_NODE_H
#define PARSE_NODE_H

#include <cstddef>
#include <string>
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"

#define PARSE_NODE_BLOCK_SIZE 1000

class ParseNode {
private:
    static const size_t blockSize;
public:
	Symbol label;
	ParseNode* headNode;
	ParseNode* premods;
	ParseNode* postmods;
	ParseNode* next;
	bool isName;
	//Marjorie added for BW, 
	int chart_start_index;
	int chart_end_index;

	// for use only in Trainer--
	// don't reference these fields in the parser unless you also 
	// put in code to fill them in when constructing ParseNodes
	ParseNode* headWordNode;
	bool isFirstWord;

	ParseNode(Symbol labelArg)
		: label(labelArg), headNode(0), premods(0), postmods(0), 
		next(0), headWordNode(0), isFirstWord(false), isName(false),
		chart_start_index(-1), chart_end_index(-1)
    {}
	ParseNode(Symbol labelArg, int _chart_start_index, int _chart_end_index)
		: label(labelArg), headNode(0), premods(0), postmods(0), 
		next(0), headWordNode(0), isFirstWord(false), isName(false),
		chart_start_index(_chart_start_index), chart_end_index(_chart_end_index)
    {}
	ParseNode() : headNode(0), premods(0), 
		postmods(0), next(0), headWordNode(0), isFirstWord(false), isName(false),
		chart_start_index(-1), chart_end_index(-1)
	{}
	ParseNode(ParseNode* old) : label(old->label), isFirstWord(old->isFirstWord), isName(old->isName) {
		headNode = old->headNode ? _new ParseNode(old->headNode) : 0;
		premods = old->premods ? _new ParseNode(old->premods) : 0;
		postmods = old->postmods ? _new ParseNode(old->postmods) : 0;
		next = old->next ? _new ParseNode(old->next) : 0;
		headWordNode = old->headWordNode ? _new ParseNode(old->headWordNode) : 0;
		chart_start_index = old->chart_start_index;
		chart_end_index = old->chart_end_index;

	}

	~ParseNode()
	{
		delete headNode;
		delete premods;
		delete postmods;
		delete next;
	}
	// EMB 2/26/04: I added an optional parameter here to allow the
	//   parses to be printed out with head information included,
	//   so they can be usefully loaded into other programs w/o re-headfinding.
	std::wstring toWString(bool printHeadInfo = false);
	
	std::string toDebugString();
	void addPremodsToDebugString(ParseNode* premod, std::string& str);
	void addPostmodsToDebugString(ParseNode* postmod, std::string& str);
	
	std::wstring toNPChunkString(Symbol& chunk_tag);
	void addPremodsToNPChunkString(ParseNode* premod, std::wstring& str, Symbol& chunk_tag);
	void addPostmodsToNPChunkString(ParseNode* postmod, std::wstring& str, Symbol& chunk_tag);

	void read_from_file(UTF8InputStream& stream, bool IFW); 
	void premods_reverse(ParseNode* first_node, ParseNode* second_node);
	void addPremodsToString(ParseNode* premod, std::wstring& str, bool printHeadInfo);
	void addPostmodsToString(ParseNode* postmod, std::wstring& str, bool printHeadInfo);
	static void* operator new(size_t n, int, char *, int) { return operator new(n); }
	static void* operator new(size_t);
	static void operator delete(void* object);

	std::wstring to_headified_wstring();
	void addHeadifiedPremodsToString(ParseNode* premod, std::wstring& str);
	void addHeadifiedPostmodsToString(ParseNode* postmod, std::wstring& str);
	//for arabic parsing
	std::wstring readLeaves();
	void getPremodLeaves(ParseNode* premod, std::wstring& str);
	void getPostmodLeaves(ParseNode* postmod, std::wstring& str);
	std::wstring writeTokens();
	void getPremodTokens(ParseNode* premod, std::wstring& str);
	void getPostmodTokens(ParseNode* postmod, std::wstring& str);
	//void makeNPSDefinite();
	ParseNode* getHeadPreterm();





private:
	static ParseNode* freeList;
};

#endif

