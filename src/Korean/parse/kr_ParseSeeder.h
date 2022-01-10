// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KR_PARSE_SEEDER_H
#define KR_PARSE_SEEDER_H

#include "common/limits.h"
#include "common/Symbol.h"
#include "morphSelection/KoreanParseSeeder.h"
#include <iostream>

class Token;
class TokenSequence;
class LexicalEntry;

class KoreanParseSeeder : public ParseSeeder {

private:
	friend class KoreanParseSeederFactory;

public:
	KoreanParseSeeder(){};

	virtual void processToken(const LocatedString& sentenceString, const Token* t);
	
	virtual void dumpBothCharts(UTF8OutputStream &out) { dumpAllCharts(out); };
	virtual void dumpBothCharts(std::ostream &out) { dumpAllCharts(out); };

	void dumpAllCharts(UTF8OutputStream &out);
	void dumpAllCharts(std::ostream &out);

private:
	int _wordNumSeg[MAX_POSSIBILITIES_PER_WORD];
	int _wordLength[MAX_POSSIBILITIES_PER_WORD];
	ParseToken _wordChart[MAX_SEGMENTS_PER_WORD][MAX_POSSIBILITIES_PER_WORD];
	
	~KoreanParseSeeder(){};

	void reset();

	void populateWordChart(const Token* t);
	void alignWordChart();
	void mergeSegments();
	void addOffsets(const Token* t);
	void groupParseTokens();

	bool alignTokenWithLexicalSegments(const wchar_t *tokstr, const wchar_t *segstr, int *map);

	static const bool DBG = false;
};


class KoreanParseSeederFactory: public ParseSeeder::Factory {
	virtual ParseSeeder *build() { return _new KoreanParseSeeder(); }
};


#endif
