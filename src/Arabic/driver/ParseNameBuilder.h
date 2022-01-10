#ifndef AR_PARSE_NAME_BUILDER_H
#define AR_PARSE_NAME_BUILDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/LocatedString.h"
#include "Generic/common/IDGenerator.h"
#include "Generic/parse/ParseNode.h"
#include "Generic/theories/NameSpan.h"
#include "Generic/theories/SynNode.h"

#include <wchar.h>
#include <stdio.h>
#include <iostream>
#include <string>

class ParseNameBuilder {
public:
	ParseNameBuilder();
	~ParseNameBuilder();

	void Reset();
	/**
	*Copied from ArabicParser.cpp 
	**/
	SynNode* convertParseToSynNode(ParseNode* parse, SynNode* parent, int start_token);
	SynNode* convertParseToSynNode(SynNode* parent, int start_token){
					return convertParseToSynNode(_tree, parent,start_token);
	}
	/**
	*@param sym the Symbol of the word to add to parse tree and name span
	**/
	void addWord(Symbol sym);
	/**
	*@param ns NameSpan result
	*@param i index of NameSpan
	**/
	void getNameSpan(NameSpan* &ns, int i);
	/**
	*@param symTag symbol corresponding to xml tag- ignored if not one of 5
			enamex types
	*return the name_type of the tag- an int
	*
	**/
	int processTag(Symbol symTag);
	/**
	*@param thisTag the name_type of this xml tag (returned by processTag)
	*@return thisTag	
	**/
	int addTag(int thisTag);

	int _comp_name_count;
	int _name_count;
	int _word_count;
	int _nnp_count;
	int _top_node_count;
private:
	const static int MAX_NAMES =100;
	int _start_tag;
	int _start_loc;
	int _begin_list[MAX_NAMES];
	int _end_list[MAX_NAMES];
	Entity::Type _type_list[MAX_NAMES];
	Symbol _tags[6];
	int _nodeID;
	ParseNode* _tree;
	ParseNode* _placeholder;
	ParseNode* _lastpostmod;
	ParseNode* addNode(int count, ParseNode* placeholder);
	ParseNode* addLevel(int count, ParseNode* result);


	typedef enum {
				OTH,
				PER,
				ORG,
				GPE,
				FAC,
				LOC,
			} 
	name_type;
};



#endif
