// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_SPLITTOKENSEQUENCE_H
#define ar_SPLITTOKENSEQUENCE_H

#include "Generic/common/limits.h" // SRS
#include "Generic/common/ParamReader.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Arabic/parse/ar_WordSegment.h"

class SplitTokenSequence{
private:
	Symbol _origTokens[MAX_SENTENCE_TOKENS];
	WordSegment* _possibleTokens[MAX_SENTENCE_TOKENS];
	Symbol _maximalSentence[MAX_SENTENCE_TOKENS];
	int _numTokens;
	int _maxSegs;
	int _currToken;
	WordSegment::Segmentation readBuckwalterPossibility(UTF8InputStream& stream, bool& is_analyzed);

public:
	SplitTokenSequence();
	~SplitTokenSequence();
	
	bool readBuckwalterSentence(UTF8InputStream& stream);
	WordSegment* readBuckwalterWord(UTF8InputStream& stream);
	WordSegment* makeWordSegmentFromBW(const WordSegment::Segmentation* possibilities, int num_poss);
	WordSegment* getWordSegment(int i){return _possibleTokens[i];};
	int getNTokens(){return _numTokens;};

};
#endif
