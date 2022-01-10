// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

/**
* This is a very much simplified ArabicTokenizer.  It
* 1) makes each punc. mark its own token (except ...)
* 2) replaces '(' with -RBR- and ')' with -LBR-
* 3) makes all things that are space delimited a token
* 4) replaces " with '' (this matches treebank training, it does 
*		not make use of open and close like in English
* It does not do anything special with dollar amounts, years, dates, names,
* TODO: add decimal numbers! 
**/
#ifndef AR_TOKENIZER_H
#define AR_TOKENIZER_H


#include "Generic/theories/Document.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Token.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/limits.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"
#include "Generic/tokens/Tokenizer.h"

#include <wchar.h>

// Back-doors for unit tests:

#define MAX_SPLIT_ENDINGS      10

// TODO: mark all const methods as such
class ArabicTokenizer : public Tokenizer{
public:

	ArabicTokenizer();
	~ArabicTokenizer();

	static void removeLinesWithoutArabicCharacters(LocatedString *string);

protected:
	// True if you want to use itea specific tokenization rules
	bool _use_itea_tokenization;
	// True if you want to use GALE-specific tokenization rules
	bool _use_GALE_tokenization;

	/// Eliminates occurrences of double-quotation marks.
	void eliminateDoubleQuotes(LocatedString *string);

	/// Attempts to convert close-quotes to open-quotes where appropriate.
	//NOTE: ARABIC PARSER ONLY USES ''
	//void recoverOpenQuotes(LocatedString *string);
	void normalizeWhitespace(LocatedString *string);
	void splitFinalPeriod(LocatedString *string);

	//-generic int skipCloseQuotes(const LocatedString *string, int index) const;
	//-generic int skipWhitespace(const LocatedString *string, int index) const;

	SymbolSubstitutionMap *_substitutionMap;
	Symbol _splitEndings[MAX_SPLIT_ENDINGS];
	int _split_endings_count;

	int _cur_sent_no;
	static Token *_tokenBuffer[MAX_SENTENCE_TOKENS+1];
	static wchar_t _char_buffer[MAX_TOKEN_SIZE+1];

	Token* getNextToken(const LocatedString *string, int *index) const;

public:
	void resetForNewSentence(const Document *doc, int sent_no);

	/// Splits an input string into a sequence of tokens.
	int getTokenTheories(TokenSequence **results, int max_theories,
			             const LocatedString *string, bool beginOfSentence=true,
								 bool endOfSentence=true);
private:
	// These all match at the index of the punctuation mark.
	bool matchNumberSeparator(const LocatedString *string, int index) const;
	//-generic bool matchTwoOpenQuotes(const LocatedString *string, int index) const;
	//-generic bool matchTwoCloseQuotes(const LocatedString *string, int index) const;
	//-generic bool matchThreeOpenQuotes(const LocatedString *string, int index) const;
	//-generic bool matchThreeCloseQuotes(const LocatedString *string, int index) const;
	//-generic bool matchEllipsis(const LocatedString *string, int index) const;
	bool isSplitChar(wchar_t c) const;
	//-generic bool matchURL(const LocatedString *string, int index) const;
	//-generic void replaceHyphenGroups(LocatedString *input);
	//-generic bool _use_GALE_tokenization;
};

#endif
