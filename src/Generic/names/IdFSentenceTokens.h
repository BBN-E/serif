// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef IDF_SENTENCE_TOKENS_H
#define IDF_SENTENCE_TOKENS_H

#include "Generic/common/limits.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include <wchar.h>

class NameClassTags;
class UnexpectedInputException;

/**
 * A simple representation of the words in an IdF sentence (stored as Symbols).
 */
class IdFSentenceTokens {
private:
	int _length;
	Symbol _wordArray[MAX_SENTENCE_TOKENS];
	bool _constraints[MAX_SENTENCE_TOKENS];
public:
	IdFSentenceTokens();
	const int MAX_LENGTH;

	// not used in SERIF
	bool readDecodeSentence(UTF8InputStream& stream);

	Symbol getWord(int index);
	void setWord(int index, Symbol word);
	bool isConstrained(int index);
	void constrainToBreakBefore(int index, bool constrain);
	int getLength();
	void setLength(int length);

	std::wstring to_string();

};


#endif
