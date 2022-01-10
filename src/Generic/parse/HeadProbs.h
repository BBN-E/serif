// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEAD_PROBS_H
#define HEAD_PROBS_H

#include <cstddef>
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/parse/HeadProbDeriver.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/Cache.h"

class HeadProbs {
private:
    static const size_t N = 4;
    NgramScoreTableGen<3>* fourGramLambda;
    NgramScoreTableGen<2>* triGramLambda;
    NgramScoreTableGen<4>* fourGramProb;
    NgramScoreTableGen<3>* triGramProb;
    NgramScoreTableGen<2>* biGramProb;
    long cache_max;
    Cache<N>::simple simpleCache;
#if !defined(_WIN32) && !defined(__APPLE_CC__)
    Cache<N>::lru lruCache;
#endif
    CacheType cache_type;
    static const char* CacheSuffix;
public:
    HeadProbs(UTF8InputStream& stream, CacheType cacheType, long cacheMax);
    HeadProbs() : cache_max(0), cache_type(None) {}
	~HeadProbs();
	float lookup(const Symbol &H, const Symbol &P, const Symbol &w, const Symbol &t) {
		Symbol ngram[N] = { H, P, w, t };
		return lookup(ngram);
	}
    float lookup(Symbol *ngram);
    float lookupML(const Symbol &H, const Symbol &P, const Symbol &w, const Symbol &t) const;
    float lookupML(const Symbol &H, const Symbol &P, const Symbol &t) const;
    float lookupML(const Symbol &H, const Symbol &P) const;

    float computeValue(Symbol* ngram) const;
      
    void set_table_access(HeadProbDeriver *hpd) {
      fourGramLambda = hpd->get_headHistories_pwt();
      triGramLambda = hpd->get_headHistories_pt();
      fourGramProb = hpd->get_headTransitions_pwt();
      triGramProb = hpd->get_headTransitions_pt();
      biGramProb = hpd->get_headTransitions_p();
    }

    void readCache(const char* case_tag);
    void writeCache(const char* case_tag);
	void clearCache();
};
    
#endif
    
