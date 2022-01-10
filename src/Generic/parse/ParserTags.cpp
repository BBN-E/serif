// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/ParserTags.h"

Symbol ParserTags::adjSymbol = Symbol(L":ADJ");
Symbol ParserTags::exitSymbol = Symbol(L":EXIT");
Symbol ParserTags::TOP = Symbol(L"TOP");
Symbol ParserTags::TOPTAG = Symbol(L"TOPTAG");
Symbol ParserTags::TOPWORD = Symbol(L"TOPWORD");
Symbol ParserTags::FRAGMENTS = Symbol(L"FRAGMENTS");
Symbol ParserTags::nullSymbol = Symbol(L":NULL");
Symbol ParserTags::leftParen = Symbol(L"(");
Symbol ParserTags::rightParen = Symbol(L")");
Symbol ParserTags::LEFT = Symbol(L"LEFT");
Symbol ParserTags::RIGHT = Symbol(L"RIGHT");
Symbol ParserTags::BOTH = Symbol(L"BOTH");
Symbol ParserTags::EOFToken = Symbol(L"EOF");
Symbol ParserTags::unknownTag = Symbol(L"UNKNOWN_TAG");
Symbol ParserTags::HYPHEN = Symbol(L"HYPHEN");
Symbol ParserTags::SPLIT = Symbol(L"SPLIT");
Symbol ParserTags::HEAD_CONSTRAINT = Symbol(L"HEAD_CONSTRAINT");
Symbol ParserTags::LIST = Symbol(L"LIST");
Symbol ParserTags::DATE_CONSTRAINT = Symbol(L"DATE_CONSTRAINT");
Symbol ParserTags::CONSTIT_CONSTRAINT = Symbol(L"CONSTIT_CONSTRAINT");
Symbol ParserTags::NESTED_NAME_CONSTRAINT = Symbol(L"NESTED_NAME_CONSTRAINT");
