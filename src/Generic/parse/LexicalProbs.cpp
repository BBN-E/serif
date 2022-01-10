// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/Cache.h"
#include "Generic/common/CacheStorage.h"

#include "Generic/parse/LexicalProbs.h"
#include "Generic/common/Symbol.h"

const char* LexicalProbs::CacheSuffix = "lex";

LexicalProbs::LexicalProbs(UTF8InputStream& stream, 
                           CacheType cacheType, 
                           long cacheMax, 
                           const char* cacheTag)
  :
    sevenGramLambda(_new NgramScoreTableGen<6>(stream)),
    sixGramLambda(_new NgramScoreTableGen<5>(stream)),
    triGramLambda(_new NgramScoreTableGen<2>(stream)),
    sevenGramProb(_new NgramScoreTableGen<7>(stream)),
    sixGramProb(_new NgramScoreTableGen<6>(stream)),
    triGramProb(_new NgramScoreTableGen<3>(stream)),
    biGramProb(_new NgramScoreTableGen<2>(stream)),
    cache_max(cacheMax),
    simpleCache(cacheMax),
#if !defined(_WIN32) && !defined(__APPLE_CC__)
    lruCache(cache_max),
#endif
    cache_type(cacheType),
    cache_tag(cacheTag)
{
}

LexicalProbs::~LexicalProbs() {
	if (sevenGramLambda != 0) { delete sevenGramLambda; }
	if (sixGramLambda != 0)   { delete sixGramLambda; }
	if (triGramLambda != 0)   { delete triGramLambda; }
	if (sevenGramProb != 0)   { delete sevenGramProb; }
	if (sixGramProb != 0)     { delete sixGramProb; }
	if (triGramProb != 0)     { delete triGramProb; }
	if (biGramProb != 0)      { delete biGramProb; }
}

float LexicalProbs::lookup(const LexicalProbs* altProbs, Symbol* ngram)
{
    if (cache_type == Simple) {
      // Simple cache
      Cache<N>::simple::iterator iter = simpleCache.find(ngram);
      if (iter != simpleCache.end()) {
        return (*iter).second;
      } else {
        float value = computeValue(altProbs, ngram);
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
        float value = computeValue(altProbs, ngram);
        lruCache.insert(std::make_pair(ngram, value));
        return value;
      }        
#endif
    } else {
      // no cache
      return computeValue(altProbs, ngram);
    }
}

inline
float LexicalProbs::computeValue(const LexicalProbs* altProbs, Symbol* ngram) const {
  float lambda1 = sevenGramLambda->lookup(ngram + 1);
  float prob1 = (lambda1 != 0? sevenGramProb->lookup(ngram): 0);
  if (lambda1 == 1)  // lambda should never = 1 though
    return prob1;
  
  // note that  ngram[5] = ngram[6];
  Symbol ngram_mod[N] = { ngram[0], ngram[1], ngram[2], ngram[3], ngram[4], ngram[6], ngram[6] };
  float lambda2 = sixGramLambda->lookup(ngram_mod + 1);
  float prob2 = (lambda2 != 0? sixGramProb->lookup(ngram_mod): 0);
  if (lambda2 == 1) // lambda should never = 1 though
    return ((lambda1 * prob1) + ((1 - lambda1) * (lambda2 * prob2)));
  
  float lambda3 = triGramLambda->lookup(ngram_mod + 1);
  float prob3 = (lambda3 != 0? triGramProb->lookup(ngram_mod): 0);
  if (lambda3 == 1) // lambda should never = 1 though
    return ((lambda1 * prob1) + ((1 - lambda1) * ((lambda2 * prob2) + ((1 - lambda2) * prob3))));
  
  ngram_mod[1] = ngram_mod[2];
  float prob4 = biGramProb->lookup(ngram_mod);
  float prob4Alt = altProbs->lookupML(ngram_mod[0], ngram_mod[2]);
  
  return ((lambda1 * prob1) + ((1 - lambda1) * ((lambda2 * prob2) +
                                                ((1 - lambda2) * ((lambda3 * prob3) + 
                                                                  ((1 - lambda3) * ((0.5F * prob4) + (0.5F * prob4Alt))))))));
}

float LexicalProbs::lookupML(const Symbol &mw, 
                             const Symbol &M, 
                             const Symbol &mt, 
                             const Symbol &P, 
                             const Symbol &H,
                             const Symbol &w, 
                             const Symbol &t) const
{
    Symbol ngram[7] = { mw, M, mt, P, H, w, t };
    return sevenGramProb->lookup(ngram);
}

float LexicalProbs::lookupML(const Symbol &mw, 
                             const Symbol &M, 
                             const Symbol &mt, 
                             const Symbol &P, 
                             const Symbol &H,
                             const Symbol &t) const
{
    Symbol ngram[6] = { mw, M, mt, P, H, t };
    return sixGramProb->lookup(ngram);
}

float LexicalProbs::lookupML(const Symbol &mw, const Symbol &M, const Symbol &mt) const
{
    Symbol ngram[3] = { mw, M, mt };
    return triGramProb->lookup(ngram);
}

float LexicalProbs::lookupML(const Symbol &mw, const Symbol &mt) const
{
    Symbol ngram[2] = { mw, mt };
    return biGramProb->lookup(ngram);
}

void LexicalProbs::readCache(const char* case_tag) {
  CacheStorage<N>::readCache(case_tag, CacheSuffix, cache_tag, simpleCache, cache_max);
}

void LexicalProbs::writeCache(const char* case_tag) {
  CacheStorage<N>::writeCache(case_tag, CacheSuffix, cache_tag, simpleCache);
}

void LexicalProbs::clearCache() {
	simpleCache.clear();
#if !defined(_WIN32) && !defined(__APPLE_CC__)
	lruCache.clear();
#endif
}
