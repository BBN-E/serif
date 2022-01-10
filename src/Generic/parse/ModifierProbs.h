// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MODIFIER_PROBS_H
#define MODIFIER_PROBS_H

#include <cstddef>
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/parse/ModifierProbDeriver.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/Cache.h"

class ModifierProbs {
private:
    static const size_t N = 7;
    NgramScoreTableGen<5>* sevenGramLambda;
    NgramScoreTableGen<4>* sixGramLambda;
    NgramScoreTableGen<7>* sevenGramProb;
    NgramScoreTableGen<6>* sixGramProb;
    NgramScoreTableGen<5>* fiveGramProb;
    long cache_max;
    Cache<N>::simple simpleCache;
#if !defined(_WIN32) && !defined(__APPLE_CC__)
    Cache<N>::lru lruCache;
#endif
    CacheType cache_type;
    static const char* CacheSuffix;
    const char* cache_tag;
public:
    ModifierProbs(UTF8InputStream& stream, CacheType cacheType, long cacheMax, const char* cache_tag);
    ModifierProbs() : cache_max(0), cache_type(None), cache_tag("") {}
	~ModifierProbs();
    float lookup(const Symbol &M, const Symbol &mt, const Symbol &P, const Symbol &H, const Symbol &PR,
	             const Symbol &w, const Symbol &t) {
		Symbol ngram[N] = { M, mt, P, H, PR, w, t };
		return lookup(ngram);
	}
	float lookup(Symbol *ngram);
    float lookupML(const Symbol &M, const Symbol &mt, const Symbol &P, const Symbol &H, const Symbol &PR,
                   const Symbol &w, const Symbol &t) const;
    float lookupML(const Symbol &M, const Symbol &mt, const Symbol &P, const Symbol &H, const Symbol &PR,
                   const Symbol &t) const;
    float lookupML(const Symbol &M, const Symbol &mt, const Symbol &P, const Symbol &H, const Symbol &PR) const;

    float computeValue(Symbol* ngram) const;

    void set_table_access(ModifierProbDeriver *mpd) {
      sevenGramLambda = mpd->get_modifierHistories_PHpwt();
      sixGramLambda = mpd->get_modifierHistories_PHpt();
      sevenGramProb = mpd->get_modifierTransitions_PHpwt();
      sixGramProb = mpd->get_modifierTransitions_PHpt();
      fiveGramProb = mpd->get_modifierTransitions_PHp();
    }

    void readCache(const char* case_tag);
    void writeCache(const char* case_tag);
	void clearCache();

};

#endif
