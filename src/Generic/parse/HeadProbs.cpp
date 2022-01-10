// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/Cache.h"
#include "Generic/common/CacheStorage.h"

#include "Generic/parse/HeadProbs.h"
#include "Generic/common/Symbol.h"

const char* HeadProbs::CacheSuffix = "head";

HeadProbs::HeadProbs(UTF8InputStream& stream, 
                     CacheType cacheType, 
                     long cacheMax)
  : fourGramLambda(_new NgramScoreTableGen<3>(stream)),
    triGramLambda(_new NgramScoreTableGen<2>(stream)),
    fourGramProb(_new NgramScoreTableGen<4>(stream)),
    triGramProb(_new NgramScoreTableGen<3>(stream)),
    biGramProb(_new NgramScoreTableGen<2>(stream)),
    cache_max(cacheMax),
    simpleCache(cacheMax),
#if !defined(_WIN32) && !defined(__APPLE_CC__)
    lruCache(cache_max),
#endif
    cache_type(cacheType)
{
}

HeadProbs::~HeadProbs() {
	if (fourGramLambda != 0)  { delete fourGramLambda; }
	if (triGramLambda != 0)   { delete triGramLambda; }
	if (fourGramProb != 0)    { delete fourGramProb; }
	if (triGramProb != 0)     { delete triGramProb; }
	if (biGramProb != 0)      { delete biGramProb; }
}

float HeadProbs::lookup(Symbol *ngram)
{
    if (cache_type == Simple) {
      // Simple cache
      Cache<N>::simple::iterator iter = simpleCache.find(ngram);
      if (iter != simpleCache.end()) {
        return (*iter).second;
      } else {
        float value = computeValue(ngram);
        simpleCache.insert(std::make_pair(ngram, value));
        return value;
      }
#if !defined(_WIN32) && !defined(__APPLE_CC__)
    } else if (cache_type == Lru) {
      // LRU cache
      Cache<N>::lru::iterator iter = lruCache.find(ngram);
      if (iter != lruCache.end()) {
        return (*iter).second;
      } else {
        float value = computeValue(ngram);
        lruCache.insert(std::make_pair(ngram, value));
        return value;
      }        
#endif
    } else {
      // no cache
      return computeValue(ngram);
    }
}

inline
float HeadProbs::computeValue(Symbol* ngram) const {
  float lambda1 = fourGramLambda->lookup(ngram + 1);
  float prob1 = (lambda1 != 0? fourGramProb->lookup(ngram): 0);
  if (lambda1 == 1) // lambda should never = 1 though
    return prob1;
  
  // note that ngram[2] = ngram[3];
  Symbol ngram_mod[N] = { ngram[0], ngram[1], ngram[3], ngram[3] };
  float lambda2 = triGramLambda->lookup(ngram_mod + 1);
  float prob2 = (lambda2 != 0? triGramProb->lookup(ngram_mod): 0);
  if (lambda2 == 1) // lambda should never = 1 though
    return ((lambda1 * prob1) + ((1 - lambda1) * prob2));
  
  float prob3 = (lambda2 != 1? biGramProb->lookup(ngram_mod): 0);
  
  return ((lambda1 * prob1) + ((1 - lambda1) * ((lambda2 * prob2) +
                                                ((1 - lambda2) * prob3))));
  
  
}

float HeadProbs::lookupML(const Symbol &H, 
                          const Symbol &P, 
                          const Symbol &w, 
                          const Symbol &t) const
{
    Symbol ngram[4] = { H, P, w, t };
    return fourGramProb->lookup(ngram);
}

float HeadProbs::lookupML(const Symbol &H, 
                          const Symbol &P, 
                          const Symbol &t) const
{
    Symbol ngram[3] = { H, P, t };
    return triGramProb->lookup(ngram);
}

float HeadProbs::lookupML(const Symbol &H, 
                          const Symbol &P) const
{
    Symbol ngram[2] = { H, P };
    return biGramProb->lookup(ngram);
}

void HeadProbs::readCache(const char* case_tag) {
  CacheStorage<N>::readCache(case_tag, CacheSuffix, simpleCache, cache_max);
}

void HeadProbs::writeCache(const char* case_tag) {
  CacheStorage<N>::writeCache(case_tag, CacheSuffix, simpleCache);
}

void HeadProbs::clearCache() {
	simpleCache.clear();
#if !defined(_WIN32) && !defined(__APPLE_CC__)
	lruCache.clear();
#endif
}

