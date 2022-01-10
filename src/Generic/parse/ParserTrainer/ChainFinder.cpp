// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/ParserTrainer/ChainFinder.h"
#include "Generic/parse/ParseNode.h"
#include <string>


// given a chain A - B - C - D - E, where we pass in A,
// and C is the nearest node with modifiers (or a preterminal),
// this returns C and sets str to A=B=C
ParseNode* ChainFinder::find(ParseNode* parse, std::wstring& str)
{
	str += parse->label.to_string();
	if (parse->premods != 0 || parse->postmods != 0 ||
			parse->headNode->headNode == 0)
	{
		return parse;
	} else {
		str += '=';
		return find (parse->headNode, str);
	}
}
