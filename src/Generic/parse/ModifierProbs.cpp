// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/Cache.h"
#include "Generic/common/CacheStorage.h"

#include "Generic/parse/ModifierProbs.h"
#include "Generic/common/Symbol.h"

const char* ModifierProbs::CacheSuffix = "mod";

ModifierProbs::ModifierProbs(UTF8InputStream& stream, 
                             CacheType cacheType, 
                             long cacheMax, 
                             const char* cacheTag)
  : sevenGramLambda(_new NgramScoreTableGen<5>(stream)),
    sixGramLambda(_new NgramScoreTableGen<4>(stream)),
    sevenGramProb(_new NgramScoreTableGen<7>(stream)),
    sixGramProb(_new NgramScoreTableGen<6>(stream)),
    fiveGramProb(_new NgramScoreTableGen<5>(stream)),
    cache_max(cacheMax),
    simpleCache(cacheMax),
#if !defined(_WIN32) && !defined(__APPLE_CC__)
    lruCache(cache_max),
#endif
    cache_type(cacheType),
    cache_tag(cacheTag)
{
}

ModifierProbs::~ModifierProbs() {
	if (sevenGramLambda != 0) { delete sevenGramLambda; }
	if (sixGramLambda != 0)   { delete sixGramLambda; }
	if (sevenGramProb != 0)   { delete sevenGramProb; }
	if (sixGramProb != 0)     { delete sixGramProb; }
	if (fiveGramProb != 0)    { delete fiveGramProb; }
}

float ModifierProbs::lookup(Symbol* ngram)
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
float ModifierProbs::computeValue(Symbol* ngram) const {
  float lambda1 = sevenGramLambda->lookup(ngram + 2);
  float prob1 = (lambda1 != 0? sevenGramProb->lookup(ngram): 0);
  if (lambda1 == 1) // lambda should never = 1 though
    return prob1;
  
  // note that  ngram[5] = ngram[6];
  Symbol ngram_mod[N] = { ngram[0], ngram[1], ngram[2], ngram[3], ngram[4], ngram[6], ngram[6] };
  float lambda2 = sixGramLambda->lookup(ngram_mod + 2);
  float prob2 = (lambda2 != 0? sixGramProb->lookup(ngram_mod): 0);
  
  if (lambda2 == 1) // lambda should never = 1 though
    return ((lambda1 * prob1) + ((1 - lambda1) * prob2));
  
  float prob3 = (lambda2 != 1? fiveGramProb->lookup(ngram_mod): 0);
  
  return ((lambda1 * prob1) + ((1 - lambda1) * ((lambda2 * prob2) +
                                                ((1 - lambda2) * prob3))));
}

float ModifierProbs::lookupML(const Symbol &M, 
                              const Symbol &mt, 
                              const Symbol &P, 
                              const Symbol &H,
                              const Symbol &PR, 
                              const Symbol &w, 
                              const Symbol &t) const
{
    Symbol ngram[7] = { M, mt, P, H, PR, w, t };
    return sevenGramProb->lookup(ngram);
}

float ModifierProbs::lookupML(const Symbol &M, 
                              const Symbol &mt, 
                              const Symbol &P, 
                              const Symbol &H,
                              const Symbol &PR, 
                              const Symbol &t) const
{
    Symbol ngram[6] = { M, mt, P, H, PR, t };
    return sixGramProb->lookup(ngram);
}

float ModifierProbs::lookupML(const Symbol &M, 
                              const Symbol &mt, 
                              const Symbol &P, 
                              const Symbol &H,
                              const Symbol &PR) const
{
    Symbol ngram[5] = { M, mt, P, H, PR };
    return fiveGramProb->lookup(ngram);
}


void ModifierProbs::readCache(const char* case_tag) {
  CacheStorage<N>::readCache(case_tag, CacheSuffix, cache_tag, simpleCache, cache_max);
}

void ModifierProbs::writeCache(const char* case_tag) {
  CacheStorage<N>::writeCache(case_tag, CacheSuffix, cache_tag, simpleCache);
}

void ModifierProbs::clearCache() {
	simpleCache.clear();
#if !defined(_WIN32) && !defined(__APPLE_CC__)
	lruCache.clear();
#endif
}
