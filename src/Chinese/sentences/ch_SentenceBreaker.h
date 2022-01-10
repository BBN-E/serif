#ifndef CH_SENTENCE_BREAKER_H
#define CH_SENTENCE_BREAKER_H

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/LocatedString.h"
#include "Generic/common/limits.h"
#include "Generic/theories/Sentence.h"
#include "Generic/sentences/DefaultSentenceBreaker.h"

class Region;
class Metadata;


class ChineseSentenceBreaker : public DefaultSentenceBreaker {
public:
	ChineseSentenceBreaker();
protected:
	Metadata *_metadata;
	int _cur_sent_no;
	int _max_desirable_sentence_len;

public:
	/**
	 *  Sets the current document pointer to point to the 
	 *	document passed in and resets the current sentence
	 *	number to 0.
	 **/
	void resetForNewDocument(const Document *doc);

    /** 
	 *  This does the work. It passes in an array of pointers to LocatedStrings
	 *  corresponding to the regions of the document text and the number of regions.
	 *  It puts an array of pointers to Sentences where specified by results arg, and 
	 *  returns its size. It returns 0 if something goes wrong. The client is 
	 *  responsible for deleting the array and the Sentences. 
	 **/
	int getSentencesRaw(Sentence **results, int max_sentences, const Region* const* regions, int num_regions);

private:
	/**
	 *  Reads the next complete sentence from input, beginning at 
	 *  offset and creates a new Sentence object in the location
	 *  specified by the sentence arg. Returns the number of characters
	 *	read from input.
	 **/
	int getNextSentence(Sentence **sentence, int offset, const LocatedString *input);

	void attemptLongSentenceBreak(Sentence **results, int max_sentences);

	// Returns true if substring [start, end) is an acceptable sub-sentence for
	// use in attemptLongSentenceBreak
	bool isAcceptableSentenceString(const LocatedString *string, int start, int end);

	bool isEOSChar(const wchar_t ch) const;
	bool isSecondaryEOSChar(const wchar_t ch) const;
	bool isEOLChar(const wchar_t ch) const;
	bool isClosingPunctuation(const wchar_t ch) const;
};

#endif
