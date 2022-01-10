// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARSER_TAGS_H
#define PARSER_TAGS_H

#include "Generic/common/Symbol.h"

class ParserTags {
public:

	static Symbol adjSymbol;
	static Symbol exitSymbol;
	static Symbol TOP;
	static Symbol TOPTAG;
	static Symbol TOPWORD;
	static Symbol unknownTag;
	static Symbol FRAGMENTS;
	static Symbol nullSymbol;
	static Symbol leftParen;
	static Symbol rightParen;
	static Symbol LEFT;
	static Symbol RIGHT;
	static Symbol BOTH;
	static Symbol EOFToken;
	static Symbol HYPHEN;
	static Symbol SPLIT;	
	static Symbol LIST;
	//MRF: this is for adding constraints in the standalone parser
	static Symbol CONSTIT_CONSTRAINT;
	// EMB: these are for encouraging the parse to generate specific nodes
	static Symbol HEAD_CONSTRAINT;
	static Symbol DATE_CONSTRAINT;
	// this is for conveying constraints to post-processing
	static Symbol NESTED_NAME_CONSTRAINT;

};



#endif
