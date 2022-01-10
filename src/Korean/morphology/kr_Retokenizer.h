// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KR_RETOKENIZER_H
#define KR_RETOKENIZER_H

#include "morphSelection/Retokenizer.h"
#include "common/limits.h"
#include "common/Symbol.h"

class LexicalEntry;
class Lexicon;

#define MAX_NEW_TOKENS 30
#define MAX_LEX_ENTRIES 50
#define ERROR_MESSAGE_SIZE 1000

class KoreanRetokenizer : public Retokenizer {
public:
	void Retokenize(TokenSequence* origTS, Token** newTokens, int n_new);
	void RetokenizeForNames(TokenSequence *origTS, Token** newTokens, int n_new);
	void reset();

private:
	// This class should only be instantiated by 
	// Retokenizer::FactoryFor<KoreanRetokenizer>:
	KoreanRetokenizer();
	friend struct Retokenizer::FactoryFor<KoreanRetokenizer>;

private:
	Lexicon* lex;

	Token* newTokenBuffer[MAX_NEW_TOKENS];
	const Token* prevTokenBuffer[MAX_NEW_TOKENS];
	LexicalEntry* prevLexEntryBuffer[MAX_LEX_ENTRIES];
	LexicalEntry* newTokenLexEntryBuffer[MAX_SENTENCE_TOKENS][MAX_LEX_ENTRIES];
	int newTokenLexEntryCounts[MAX_SENTENCE_TOKENS];
	Token* final_token_buffer[MAX_SENTENCE_TOKENS];
	LexicalEntry* tempLexEntryBuffer[MAX_LEX_ENTRIES];

	char error_message_buffer[ERROR_MESSAGE_SIZE+1];
	bool dbg;

	void alignLexEntry(LexicalEntry *prevLE, int startNewTok, int newTokCount);
	bool matchLexEntry(Symbol key, LexicalEntry** le, int n, size_t& id);
	size_t addLexEntry(Symbol key, LexicalEntry** le, int n);
	int matchStringInToken(const wchar_t* le_str, const wchar_t* tok_str, int place);
	int getCorrespondingTokens(Token** results, Token** tokens, int orig_id, int start, int max);
	int getCorrespondingTokens(const Token** results, const TokenSequence* tokens, int orig_id, int start);
	int pullLexEntriesUp(LexicalEntry** result, int size, LexicalEntry* initialEntry);
};
#endif



