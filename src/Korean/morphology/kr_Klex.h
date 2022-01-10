// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KR_KLEX_H
#define KR_KLEX_H

#include "common/limits.h"
#include "theories/Lexicon.h"
#include <windows.h>
#include <string>

class LexicalEntry;
class UnicodeEucKrEncoder;
class Lexicon;
class TokenSequence;
class Token;

class Klex {

public:
	static int analyzeSentence(TokenSequence *tokens, Lexicon *lexicon);
	static int analyzeWord(const wchar_t* input, Lexicon *lexicon, LexicalEntry** result, const int max_results);

private:
	/** Maximum number of characters in a sentence. */
	static const int max_sentence_chars = 1000;

	static UnicodeEucKrEncoder *_encoder;

	static HANDLE hChildProcess;

	static char _cmd_str[1000];
	static Token* _tokenBuffer[MAX_SENTENCE_TOKENS];
	static LexicalEntry* _lexBuffer[MAX_ENTRIES_PER_SYMBOL];
	static unsigned char _euc_buffer[max_sentence_chars];
	static wchar_t _buffer[max_sentence_chars];

	static std::string runKlex();
	static void prepAndLaunchKlex(HANDLE hChildStdOut,
                                 HANDLE hChildStdIn,
                                 HANDLE hChildStdErr);
	static std::string readAndHandleOutput(HANDLE hPipeRead);
	static DWORD WINAPI sendInputThread(LPVOID lpvThreadParam);

	static LexicalEntry *createLexicalEntry(Lexicon *lexicon, wchar_t *entry_str);

	static bool isAllWhitespace(wchar_t *str);

};
#endif
