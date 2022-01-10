// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARSE_SEEDER_H
#define PARSE_SEEDER_H

#include <boost/shared_ptr.hpp>

#include "Generic/common/Symbol.h"
#include "Generic/common/limits.h"
#include "Generic/common/Offset.h"
#include <iostream>
#include <vector>

class LexicalEntry;
class TokenSequence;
class Token;
class LocatedString;

#define MAX_SEGMENTS_PER_WORD 10
#define MAX_POSSIBILITIES_PER_WORD 50
#define MAX_CHAR_PER_WORD 500

class ParseSeeder {
public:
	/** Create and return a new ParseSeeder. */
	static ParseSeeder *build() { return _factory()->build(); }
	/** Hook for registering new ParseSeeder factories */
	struct Factory { virtual ParseSeeder *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }


	struct ParseToken {
		Symbol normString;
		int start;
		int end;
		int word_part;
		Symbol pos;
		Symbol cat;
		OffsetGroup start_offset;
		OffsetGroup end_offset;
		std::vector<LexicalEntry*> lex_ents;
	} ;

	typedef struct {
		int start;
		int end;
		Symbol nvString;
		OffsetGroup start_offset;
		OffsetGroup end_offset;
		std::vector<Symbol> postags;

		// [EL 3/15/11] SameSpanParseToken used to include an array of ParseToken
		// objects; but the *only* field we ever accessed in them was the
		// pos field; so I replaced this array (pt) with postags, which takes up 
		// *much* less memory.  There is some commented-out code in ar_Parser.cpp
		// with the comment "need to add lexical entry info here" which accessed
		// pt.lex_ents -- if this is needed in the future, then we should add a 
		// vector of lex_ents, rather than bringing back an array of ParseTokens.
		//     int num_ent;
		//     ParseToken pt[MAX_POSSIBILITIES_PER_WORD];
	} SameSpanParseToken;

	virtual ~ParseSeeder() {};

	virtual void processToken(const LocatedString& sentenceString, const Token* t) = 0;

	virtual void dumpBothCharts(UTF8OutputStream &out) = 0;
	virtual void dumpBothCharts(std::ostream &out) = 0;

	const SameSpanParseToken& groupedWordChart(int i) { return _groupedWordChart[i]; }
	int numGrouped() { return _num_grouped; }

protected:
	ParseSeeder() { 
		_n_lex_entries = 0;
		_num_grouped = 0;
	};

	SameSpanParseToken _groupedWordChart[MAX_POSSIBILITIES_PER_WORD];
	int _num_grouped;

public:
	// public for MorphSelection
	ParseToken _mergedWordChart[MAX_SEGMENTS_PER_WORD][MAX_POSSIBILITIES_PER_WORD];
	int _mergedNumSeg[MAX_POSSIBILITIES_PER_WORD];
	int _n_lex_entries;
private:
	static boost::shared_ptr<Factory> &_factory();
};
//#if defined(ARABIC_LANGUAGE)
//	#include "Arabic/BuckWalter/ar_ParseSeeder.h"
//#elif defined(KOREAN_LANGUAGE)
//	#include "Korean/parse/kr_ParseSeeder.h"
//#else
//	#include "Generic/morphSelection/xx_ParseSeeder.h"
//#endif

#endif
