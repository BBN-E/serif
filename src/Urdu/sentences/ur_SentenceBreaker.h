#ifndef UR_SENTENCE_BREAKER_H
#define UR_SENTENCE_BREAKER_H

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/theories/Metadata.h"
#include "Generic/theories/Sentence.h"
#include "Generic/sentences/DefaultSentenceBreaker.h"

class Metadata;
class Region;
class LocatedString;

class UrduSentenceBreaker : public DefaultSentenceBreaker {

public:
	~UrduSentenceBreaker();
	UrduSentenceBreaker();

private:

	/// Resets the sentence breaker to run on the next new document.
	void resetForNewDocument(const Document *doc);

	/// Breaks a LocatedString into individual Sentence objects.
	int getSentencesRaw(Sentence **results, int max_sentences,
						const Region* const* regions, int num_regions);

	/// Reads the next complete sentence from input.
	Sentence* getNextSentence(int *offset, const LocatedString *input);

	std::wstring _getNextToken(const LocatedString *input, int start, bool letters_only);
	std::wstring _getPrevToken(const LocatedString *input, int start, bool letters_only);

	bool _breakLongSentences;
	int _max_whitespace;
	void attemptLongSentenceBreak(Sentence **results, int max_sentences, Metadata *metadata);

	bool isSplitChar(const wchar_t c) const;
	bool isClosingPunctuation(const wchar_t c) const;
	bool isSecondaryEOSSymbol(const wchar_t c) const;
	bool isCarriageReturn(const wchar_t c) const;
	int getIndexAfterClosingPunctuation(const LocatedString *input, int index);
	int matchEllipsis(const LocatedString *input, int index);
	bool matchFinalPeriod(const LocatedString *input, int index, int origin);
	bool matchEmail(const LocatedString *input, int index);
	Sentence* createSubSentence(const Sentence *sentence, int sent_no, int start, int end, std::string justification);
	
	bool DEBUG;
	UTF8OutputStream _debugStream;

};

#endif
