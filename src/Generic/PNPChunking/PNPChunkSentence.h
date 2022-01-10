// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P_NPCHUNK_SENTENCE_H
#define P_NPCHUNK_SENTENCE_H

#include "Generic/common/limits.h"

class UTF8InputStream;
class UTF8OutputStream;
class TagSet;
class TokenSequence;


class PNPChunkSentence {
public:
	PNPChunkSentence(const DTTagSet *tagSet, const int max_length);
	
	PNPChunkSentence(const DTTagSet *tagSet, const int max_length, const TokenSequence* tokens, const Symbol *pos);
	int getLength() const { return _length; }
	Symbol getWord(int i) const;
	int getTag(int i) const;
	Symbol getPOS(int i) const;

	void clear() { _length = 0; }
	void addWord(Symbol word);
	void setTag(int i, int tag);
	
	bool readSexpSentence(UTF8InputStream &in);
	bool readTrainingSentence(UTF8InputStream &in);
	bool readPOSTrainingSentence(UTF8InputStream &in);
	bool readPOSTestSentence(UTF8InputStream &in);
	void writeSexp(UTF8OutputStream &out);

private:
	static const Symbol NONE_ST;
	static const Symbol NONE_CO;

	const int _max_length;
	const DTTagSet *_tagSet;
	int _NONE_ST_index;
	int _NONE_CO_index;

	int _length;
	Symbol _words[MAX_SENTENCE_TOKENS];
	Symbol _pos[MAX_SENTENCE_TOKENS];
	int _tags[MAX_SENTENCE_TOKENS];
};

#endif
