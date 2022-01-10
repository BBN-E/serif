// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

/**
* This is a very much simplified KoreanTokenizer.  It
* 1) makes each punc. mark its own token (except ...,---,?!,!!)
* 2) replaces '(' with -RBR- and ')' with -LBR-
* 3) makes all things that are space delimited a token
* It does not do anything special with dollar amounts, years, dates, names,
**/

#ifndef KR_TOKENIZER_H
#define KR_TOKENIZER_H

#include "theories/Document.h"
#include "theories/TokenSequence.h"
#include "theories/Token.h"
#include "common/LocatedString.h"
#include "common/limits.h"
#include "tokens/SymbolSubstitutionMap.h"

#include <wchar.h>

class KoreanTokenizer : public Tokenizer{
private:
	friend class KoreanTokenizerFactory;

public:

	~KoreanTokenizer();

	static Symbol getSubstitutionSymbol(Symbol sym);

protected:
	/// Eliminates occurrences of double-quotation marks.
	void eliminateDoubleQuotes(LocatedString *string);

	void normalizeWhitespace(LocatedString *string);

	int skipWhitespace(const LocatedString *string, int index) const;

	static SymbolSubstitutionMap *_substitutionMap;

protected:
	int _cur_sent_no;
	static Token *_tokenBuffer[MAX_SENTENCE_TOKENS+1];
	static wchar_t _char_buffer[MAX_TOKEN_SIZE+1];

	Token* getNextToken(const LocatedString *string, int *index) const;


private:
	KoreanTokenizer();

	void resetForNewSentenceRaw(Document *doc, int sent_no);

	/// Splits an input string into a sequence of tokens.
	int getTokenTheoriesRaw(TokenSequence **results, int max_theories,
			                const LocatedString *string, 
						    bool beginOfSentence = true, bool endOfSentence = true);

	// These all match at the index of the punctuation mark.
	bool matchNumberSeparator(const LocatedString *string, int index) const;
	bool matchMultiCharPunct(const LocatedString *string, int index) const;
	bool matchTwoOpenQuotes(const LocatedString *string, int index) const;
	bool matchTwoCloseQuotes(const LocatedString *string, int index) const;
	bool matchThreeOpenQuotes(const LocatedString *string, int index) const;
	bool matchThreeCloseQuotes(const LocatedString *string, int index) const;
	bool matchEllipsis(const LocatedString *string, int index) const;
	bool isSplitChar(wchar_t c) const;
};

class KoreanTokenizerFactory: public Tokenizer::Factory {
	virtual Tokenizer *build() { return _new KoreanTokenizer(); }
};


#endif
