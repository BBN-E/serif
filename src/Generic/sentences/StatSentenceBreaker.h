// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STAT_SENTENCE_BREAKER_H
#define STAT_SENTENCE_BREAKER_H

#include "Generic/common/LocatedString.h"
#include "Generic/theories/Sentence.h"
#include "Generic/sentences/StatSentBreakerFVecModel.h"
#include "Generic/sentences/SentenceBreaker.h"

class StatSentBreakerTokens;
class Metadata;
class Region;
class Zone;
class Document;

class StatSentenceBreaker: public SentenceBreaker  {
public:
	StatSentenceBreaker();
	~StatSentenceBreaker();

	/// Resets the sentence breaker to run on the next new document.
	void resetForNewDocument(const Document *doc);

	/// Breaks a LocatedString into individual Sentence objects.
	int getSentencesRaw(Sentence **results, int max_sentences,
						const Region* const* regions, int num_regions);
	int getSentencesRaw(Sentence **results, int max_sentences,
						const Zone* const* zones, int num_zones);

protected:
    const Document *_curDoc;
	const Region *_curRegion;
	const Zone *_curZone;
	int _cur_sent_no;

private:
	void breakLongSentence(Sentence **results, const LocatedString *string, 
						   StatSentBreakerTokens *tokens, int start, int end); 
	double getSTScore(Symbol word, Symbol word1, Symbol word2, int sent_len);

	const static Symbol START_SENTENCE;
	const static Symbol CONT_SENTENCE;


	StatSentBreakerFVecModel *_model;
	StatSentBreakerFVecModel *_defaultModel;
	StatSentBreakerFVecModel *_lowerCaseModel;
	StatSentBreakerFVecModel *_upperCaseModel;
};

#endif
