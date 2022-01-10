// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_IDF_NAME_RECOGNIZER_H
#define ch_IDF_NAME_RECOGNIZER_H

#include "Chinese/names/ch_NameRecognizer.h"
#include <wchar.h>

class NameTheory;
class IdFSentenceTokens;
class IdFSentenceTheory;
class IdFDecoder;
class TokenSequence;
class NameClassTags;

class ChineseIdFNameRecognizer : public NameRecognizer {
public:
	ChineseIdFNameRecognizer();

	void resetForNewSentence(const Sentence *sentence);

	void cleanUpAfterDocument(){}
	void resetForNewDocument(class DocTheory *docTheory){}

	/** 
	  * This does the work. It populates an array of pointers to NameTheorys
	  * specified by <code>results</code> with up to <code>max_theories</code> 
	  * NameTheory pointers. The client is responsible for deleting the NameTheorys.
	  *
	  * @param results the array of pointers to populate
	  * @param max_theories the maximum number of theories to return
	  * @param tokenSequence the TokenSequence representing source text 
	  * @return the number of theories produced or 0 for failure
	  */
	int getNameTheories(NameTheory **results, int max_theories,
						TokenSequence *tokenSequence);

	static const int MAX_NAMES = 1000;
	static const int MAX_CHARACTERS_PER_SENTENCE = 3000;
	static const int OVERLAPPING_SPAN = -1;

private:

	IdFDecoder *_decoder;
	IdFSentenceTokens *_sentenceTokens;
	IdFSentenceTheory **_IdFTheories;
	NameClassTags *_nameClassTags;

	int _getNameTheoriesRunOnKanji(NameTheory **results, int max_theories,
								const TokenSequence *tokenSequence);

	int _getNameTheoriesRunOnTokens(NameTheory **results, int max_theories,
								const TokenSequence *tokenSequence);

	NameSpan* _spanBuffer[MAX_NAMES];
	wchar_t _word_buffer[MAX_TOKEN_SIZE+1];
	wchar_t _char_buffer[2];
	bool _run_on_tokens;

	bool firstNameHasRightsToToken(int first_token_char_index, int second_token_char_index, int* token_cache);

};

#endif
