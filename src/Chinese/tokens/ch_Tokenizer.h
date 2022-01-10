#ifndef CH_TOKENIZER_H
#define CH_TOKENIZER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/theories/Document.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Token.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"
#include "Generic/tokens/Tokenizer.h"
#include "Generic/names/IdFSentenceTokens.h"
#include "Generic/names/IdFDecoder.h"
#include "Generic/common/Symbol.h"
#include <wchar.h>

class SessionLogger;

class ChineseTokenizer : public Tokenizer {
public:
	~ChineseTokenizer();
	ChineseTokenizer();

	
	static Symbol getSubstitutionSymbol(Symbol sym);

protected:
	int _cur_sent_no;

	static SymbolSubstitutionMap *_substitutionMap;
	
	static Token* _tokenBuffer[MAX_SENTENCE_TOKENS];
	static wchar_t _wchar_buffer[MAX_TOKEN_SIZE+1];

	/// Set to false if you want to treat each character as its
	//   own token, for instance if you're only doing
	//   names and value finding.
	bool _do_tokenization;

public:

	void resetForNewSentence(const Document *doc, int sent_no);

	/** 
	  * This does the work. It puts an array of pointers to TokenSequences
	  * where specified by <code>results</code>, and returns its size. The 
	  * client is responsible for deleting the array and the TokenSequences.
	  *
	  * @param results the array of pointers to populate
	  * @param max_theories the maximum number of theories to return
	  * @param string the LocatedString representing source text to tokenize
	  * @param beginOfSentence is true of the source contains the beginning of a sentence
	  * @param endOfSentence true when the source contains the end of a sentence
	  * @return the number of theories produced or 0 for failure
	  */
	int getTokenTheories(TokenSequence **results, int max_theories,
			             const LocatedString *string, bool beginOfSentence=true,
								 bool endOfSentence=true);
private:
	NameClassTags *_POSClassTags;
	IdFDecoder *_decoder;
	IdFSentenceTokens *_sentenceChars;
	IdFSentenceTheory **_POSTheories;

	const Document *_document;

	int getIdFTokenTheories(TokenSequence **results, int max_theories, 
		const LocatedString& string);

	int getCharTokenTheory(TokenSequence **results, int max_theories,
		const LocatedString& string);

	void removeWhitespace(LocatedString *text);
	bool constraintViolation(EDTOffset begin, EDTOffset end);

	bool isConstraintBeginning(const LocatedString *string, int index);
	bool isConstraintEnding(const LocatedString *string, int index);

	bool isASCIIBeginBreak(const LocatedString *string, int index);
	bool isASCIIEndBreak(const LocatedString *string, int index);

	bool isChineseNumberChar(wchar_t c_char) const;
	bool isChineseTimeChar(wchar_t c_char) const;
};


#endif
