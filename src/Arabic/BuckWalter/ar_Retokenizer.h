// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_RETOKENIZER_H
#define AR_RETOKENIZER_H

#include "Generic/morphSelection/Retokenizer.h"
#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"

class LexicalEntry;
class LexicalToken;

#define MAX_NEW_TOKENS 30
#define MAX_LEX_ENTRIES 50
#define ERROR_MESSAGE_SIZE 1000

class Lexicon;

class ArabicRetokenizer: public Retokenizer {
public:
	void Retokenize(TokenSequence* origTS, Token** newTokens, int n_new);
	void RetokenizeForNames(TokenSequence *origTS, Token** newTokens, int n_new);
	void reset();

private:
	// This class should only be instantiated by 
	// Retokenizer::FactoryFor<ArabicRetokenizer>:
	ArabicRetokenizer();
	friend struct Retokenizer::FactoryFor<ArabicRetokenizer>;

private:
	bool getInstanceIntegrityCheck();

	Lexicon* lex;

	LexicalToken* newTokenBuffer[MAX_NEW_TOKENS];
	const LexicalToken* prevTokenBuffer[MAX_NEW_TOKENS];
	LexicalEntry* prevLexEntryBuffer[MAX_LEX_ENTRIES];
	LexicalEntry* newTokenLexEntryBuffer[MAX_SENTENCE_TOKENS][MAX_LEX_ENTRIES];
	int newTokenLexEntryCounts[MAX_SENTENCE_TOKENS];
	Token* final_token_buffer[MAX_SENTENCE_TOKENS];
	LexicalEntry* tempLexEntryBuffer[MAX_LEX_ENTRIES];

	char error_message_buffer[ERROR_MESSAGE_SIZE];
	bool dbg;

	bool matchLexEntry(Symbol key, LexicalEntry** le, int n, size_t& id);
	size_t addLexEntry(Symbol key, LexicalEntry** le, int n);
	int matchStringInToken(const wchar_t* le_str, const wchar_t* tok_str, int place);
	int getCorrespondingTokens(LexicalToken** results, Token** tokens, int orig_id, int start, int max);
	int getCorrespondingTokens(const LexicalToken** results, const TokenSequence* tokens, int orig_id, int start);
	Symbol getNormalizedLexEntrySymbol(LexicalEntry* le);
	
};

#endif

