// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P_IDF_SENTENCE_H
#define P_IDF_SENTENCE_H

#include "Generic/common/Symbol.h"

class UTF8InputStream;
class UTF8OutputStream;
class DTTagSet;
class TokenSequence;

class PIdFSentence {
private:
	bool _in_training_set;
	const TokenSequence* _tokenSequence;

public:
	PIdFSentence(const DTTagSet *tagSet, int max_length);
	PIdFSentence(const DTTagSet *tagSet,
				 const TokenSequence &tokenSequence,
				 bool force_tokens_lowercase = false);
	// if you use the default constructor, you must populate the object
	// with populate()
	PIdFSentence() : _tagSet(0), _length(0), _words(0), _tags(0), marginScore(0) {}

	~PIdFSentence() {
		delete[] _words;
		delete[] _tags;
	}

	const TokenSequence* getTokenSequence() const { return _tokenSequence; }

	double marginScore;
	/** Populates from given sentence, but only allocates just enough
	  * space for its tokens -- you can't re-use the object on a longer
	  * sentence later. */
	void populate(const PIdFSentence &other);

	int getLength() const { return _length; }
	Symbol getWord(int i);
	int getTag(int i) const;

	/// Initialize for a use on a fresh new sentence
	void clear() { _length = 0; }
	void addWord(Symbol word);
	void setTag(int i, int tag);
	void changeToStartTag(int i);
	void changeToContinueTag(int i);

	bool readSexpSentence(UTF8InputStream &in);
	bool readTrainingSentence(UTF8InputStream &in);
	void writeSexp(UTF8OutputStream &out);
	bool inTraining(){ return _in_training_set;};
	void addToTraining(){ _in_training_set = true;};
	void removeFromTraining(){ _in_training_set = false;};

private:
	static const Symbol NONE_ST;
	static const Symbol NONE_CO;

	int _max_length;
	const DTTagSet *_tagSet;
	int _NONE_ST_index;
	int _NONE_CO_index;

	int _length;
	Symbol *_words;
	int *_tags;
};

#endif
