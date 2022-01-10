#ifndef KR_SENTENCE_BREAKER_H
#define KR_SENTENCE_BREAKER_H

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/LocatedString.h"
#include "Generic/common/limits.h"
#include "Generic/theories/Sentence.h"
#include "Generic/sentences/SentenceBreaker.h"

class Region;

class KoreanSentenceBreaker : public SentenceBreaker {
public:
	KoreanSentenceBreaker();
protected:
	class Metadata *_metadata;
	int _cur_sent_no;
private:

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

	/**
	 *  Reads the next complete sentence from input, beginning at 
	 *  offset and creates a new Sentence object in the location
	 *  specified by the sentence arg. Returns the number of characters
	 *	read from input.
	 **/
	int getNextSentence(Sentence **sentence, int offset, const LocatedString *input);

	void attemptSentenceBreak(Sentence **results);

	bool isEOSChar(const wchar_t ch) const;
	bool isSecondaryEOSChar(const wchar_t ch) const;
	bool isEOLChar(const wchar_t ch) const;
	bool isClosingPunctuation(const wchar_t ch) const;
	bool inRestrictedBreakSpan(int index, const LocatedString *input) const;

	bool matchFinalPeriod(const LocatedString *input, int index, int origin) const;
	bool matchPossibleFinalPeriod(const LocatedString *input, int index, int origin) const;
	int matchEllipsis(const LocatedString *input, int index) const;
	int getIndexAfterClosingPunctuation(const LocatedString *input, int index) const;
	bool matchLikelyAbbreviation(Symbol word) const;
	Symbol getWordEndingAt(int index, const LocatedString *input, int origin) const;
	bool matchDecimalPoint(const LocatedString *input, int index, int origin) const;
};

#endif
