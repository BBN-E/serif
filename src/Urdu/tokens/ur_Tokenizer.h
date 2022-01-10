// Copyright 2013 by Raytheon BBN Technologies
// All Rights Reserved.

#include "Generic/common/LocatedString.h"
#include "Generic/common/limits.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Token.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"
#include "Generic/tokens/SymbolList.h"
#include "Generic/tokens/Tokenizer.h"

#include <wchar.h>

class UrduTokenizer : public Tokenizer {

public:
	UrduTokenizer();
	~UrduTokenizer();

protected:
	/// The index number of the sentence being tokenized.
	int _cur_sent_no;

	/// The internal buffer of tokens used to construct the token sequence.
	static Token *_tokenBuffer[MAX_SENTENCE_TOKENS+1];
	static Token *_tempTokenBuffer[MAX_SENTENCE_TOKENS+1];

	/// The internal buffer of characters used to construct individual tokens.
	static wchar_t _char_buffer[MAX_TOKEN_SIZE+1];

	// The token substitution map.
	SymbolSubstitutionMap *_substitutionMap;

	/// Finds the next token in the string.
	Token* getNextToken(const LocatedString *string, int *index) const;

private:

	/// Resets the state of the tokenizer to prepare for a new sentence.
	void resetForNewSentence(const Document *doc, int sent_no);

	/// Splits an input string into a sequence of tokens.
	int getTokenTheories(TokenSequence **results, int max_theories,
			             const LocatedString *string, bool beginOfSentence = true,
								 bool endOfSentence= true );

	// Perform forced replacements before tokenization
	void forceReplacements(void);
	
	bool enforceTokenBreak(const LocatedString *string, int i) const;
	const Document *_document;	


	void splitPunctuationWithConstraints(LocatedString *string, const wchar_t punc[], bool checkURL, bool checkDigits);
	void replaceDirectionalChar(LocatedString *string);
	void splitNumbersFromWords(LocatedString *string);
	void splitCurrencyAmounts(LocatedString *string);
	
	bool isRelevantPunctuation(const wchar_t c, const wchar_t punc[]) const;
	bool isCommaBreakable(LocatedString *string, int index);
	bool matchEmail(LocatedString *string, int index);
};
