// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_PIDF_CHAR_SENTENCE_H
#define CH_PIDF_CHAR_SENTENCE_H

#include "Generic/common/Symbol.h"

class UTF8InputStream;
class UTF8OutputStream;
class DTTagSet;
class TokenSequence;


class PIdFCharSentence {

public:
	PIdFCharSentence(const DTTagSet *tagSet, int max_length);
	PIdFCharSentence(const DTTagSet *tagSet,
				 const TokenSequence &tokenSequence);
	// if you use the default constructor, you must populate the object
	// with populate()
	PIdFCharSentence() : _tagSet(0), _length(0), _chars(0), _char_token_map(0), _tags(0), marginScore(0) {}

	~PIdFCharSentence() {
		delete[] _chars;
		delete[] _char_token_map;
		delete[] _tags;
	}

	const TokenSequence* getTokenSequence() const { return _tokenSequence; }

	double marginScore;
	/** Populates from given sentence, but only allocates just enough
	  * space for its tokens -- you can't re-use the object on a longer
	  * sentence later. */
	void populate(const PIdFCharSentence &other);

	int getLength() const { return _length; }
	Symbol getChar(int i) const;
	int getTag(int i) const;

	int getTokenFromChar(int i) const;

	/// Initialize for a use on a fresh new sentence
	void clear() { _length = 0; }
	void setTag(int i, int tag);

	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out, const PIdFCharSentence &it)
		{ it.dump(out, 0); return out; }

private:
	static const Symbol NONE_ST;
	static const Symbol NONE_CO;

	int _max_length;
	const DTTagSet *_tagSet;
	int _NONE_ST_index;
	int _NONE_CO_index;

	int _length;
	Symbol *_chars;
	int *_char_token_map;
	int *_tags;

	wchar_t _word_buffer[MAX_TOKEN_SIZE+1];
	wchar_t _char_buffer[2];

	const TokenSequence* _tokenSequence;
};

#endif
