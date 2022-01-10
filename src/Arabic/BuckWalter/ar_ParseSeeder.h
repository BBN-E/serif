// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_PARSE_SEEDER_H
#define AR_PARSE_SEEDER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/limits.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/morphSelection/ParseSeeder.h"
#include "Arabic/BuckWalter/ar_BuckWalterLimits.h"
#include <iostream>

class Token;
class LexicalToken;
class TokenSequence;
class LexicalEntry;


class ArabicParseSeeder : public ParseSeeder {
private:
	friend class ArabicParseSeederFactory;

public:

	const static int PREFIX = 0;
	const static int STEM = 1;
	const static int SUFFIX = 2;
	const static int RETOKENIZED = 3;

	~ArabicParseSeeder(){};

	//static int _pullLexEntriesUp(LexicalEntry** result, LexicalEntry* initialEntry);

	virtual void processToken(const LocatedString& sentenceString, const Token* t) throw (UnrecoverableException);
	virtual void dumpBothCharts(UTF8OutputStream &out);
	virtual void dumpBothCharts(std::ostream &out);

	void dumpNoVowelWordChart(std::ostream &out);
	void dumpNoVowelWordChart(UTF8OutputStream &out);

private:
	ArabicParseSeeder();

	struct NoVowelParseToken {
        Symbol normString;
        int start;
        int end;
        int word_part;
        LexicalEntry* lex_ent;
		NoVowelParseToken() {}
		NoVowelParseToken(const ParseToken &tok)
			: normString(tok.normString), start(tok.start), end(tok.end),
			  word_part(tok.word_part), lex_ent(tok.lex_ents[0]) {}
    };

	int _noVowelNumSeg[MAX_POSSIBILITIES_PER_WORD];
	int _noVowelWordLength[MAX_POSSIBILITIES_PER_WORD];
	NoVowelParseToken _noVowelWordChart[MAX_SEGMENTS_PER_WORD][MAX_POSSIBILITIES_PER_WORD];
	void _reset();

	wchar_t _word_buffer[MAX_CHAR_PER_WORD];
	void _populateNoVowelWordChart(const LexicalToken* t) throw (UnrecoverableException);
	void _alignNoVowelWordChart(int nposs) throw (UnrecoverableException);
	void _mergeSegments()throw (UnrecoverableException);
	void _groupParseTokens();
	void _addOffsets(const LocatedString &sentenceString, const Token* t);
	int _getVowelessEntries(ParseToken* results, int size, LexicalEntry* initialEntry) throw (UnrecoverableException);
	void _updateMergedWordChart(int w_count, int l_count, int nv_start_index, int nv_end_index);
	bool _isPrefInfl(const NoVowelParseToken* le);
	bool _isSuffInfl(const NoVowelParseToken* le);	
	bool _equivalentChars(wchar_t a, wchar_t b);

	bool _use_GALE_tokenization;
};


class ArabicParseSeederFactory: public ParseSeeder::Factory {
	virtual ParseSeeder *build() { return _new ArabicParseSeeder(); }
};


#endif
