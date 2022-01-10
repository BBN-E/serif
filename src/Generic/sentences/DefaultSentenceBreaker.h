// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DEFAULT_SENTENCE_BREAKER_H
#define DEFAULT_SENTENCE_BREAKER_H

#include "Generic/sentences/SentenceBreaker.h"
#include "Generic/common/LocatedString.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Region.h"
//#include "Generic/theories/Zone.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif


/**
 * Note: This class has subclasses (eg EnglishSentenceBreaker)
 */
class SERIF_EXPORTED DefaultSentenceBreaker: public SentenceBreaker {
public:

	DefaultSentenceBreaker();
	~DefaultSentenceBreaker();

	/// Resets the sentence breaker to run on the next new document.
	void resetForNewDocument(const Document *doc);

	/// Breaks a LocatedString into individual Sentence objects.
	int getSentencesRaw(Sentence **results, int max_sentences,
			const Region* const* regions, int num_regions);
	//int getSentencesRaw(Sentence **results, int max_sentences,
	//		const Zone* const* zones, int num_zones);

protected:
	int getNextSentence(Sentence **results, int sent_no, const LocatedString* region);

	int findEllipsis(int start_index, std::wstring input);
	int findPeriodPlus(int start_index, std::wstring input);
	int findWhiteSpaceXTimes(int start_index, std::wstring input);
	int findClosingQuotation(int start_index, std::wstring input);

	std::set<std::wstring> _nonFinalAbbrevs;
	std::wstring getWordEndingAt(int index, std::wstring input_string, int origin);	

	void simpleAttemptLongSentenceBreak(Sentence **results, int max_sentences);


	// Attempts to break sentence into ~n_split_sents smaller sentences
	std::vector<Sentence*> attemptLongSentenceBreak(const Sentence *sentence, int n_split_sents);
	bool attemptLongSentenceBreak(Sentence **results, int max_sentences);

	int _max_token_breaks;

	// Experimental attempt to detect and break up tables into one sentence per row
	bool _breakTableSentences;
	bool _skipTableSentences;
	size_t _minTableRows;
	bool attemptTableSentenceBreak(Sentence **results, int max_sentences);

	// Returns a new Sentence created from a substring of the sentence passed in
	Sentence* createSubSentence(const Sentence *sentence, int sent_no, int start, int end, std::string justification);
	Sentence* createFinalSubSentence(const Sentence *sentence, int sent_no, int start);
	
	// Returns true if a sentence break is NOT permitted between index and index-1 
	bool inRestrictedBreakSpan(Metadata *metadata, const LocatedString *input, int index) const;

	// Returns a count of the number of whitespace/punctuation spans
	// between index start and index end in string; multiple 
	// contiguous chars count as one span. Estimate of number of tokens.
	int countPossibleTokenBreaks(const LocatedString* sentence, int start, int end);
	int countPossibleTokenBreaks(const LocatedString* sentence);
	
	// Returns true if substring [start, end) is an acceptable sub-sentence for
	// use in attemptLongSentenceBreak
	virtual bool isAcceptableSentenceString(const LocatedString *string, int start, int end);// { return true; }

	virtual bool isSecondaryEOSChar(const wchar_t ch) const { return false; }


	const Document* _curDoc;
	const Region* _curRegion;
	//const Zone* _curZone;
	int _cur_sent_no;
};

#endif
