// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LEXICAL_PROBS_H
#define LEXICAL_PROBS_H

#include <cstddef>
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/parse/LexicalProbDeriver.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/Cache.h"

class LexicalProbs {
private:
    static const size_t N = 7;
    NgramScoreTableGen<6>* sevenGramLambda;
    NgramScoreTableGen<5>* sixGramLambda;
    NgramScoreTableGen<2>* triGramLambda;
    NgramScoreTableGen<7>* sevenGramProb;
    NgramScoreTableGen<6>* sixGramProb;
    NgramScoreTableGen<3>* triGramProb;
    NgramScoreTableGen<2>* biGramProb;
    long cache_max;
    Cache<N>::simple simpleCache;
#if !defined(_WIN32) && !defined(__APPLE_CC__)
    Cache<N>::lru lruCache;
#endif
    CacheType cache_type;
    static const char* CacheSuffix;
    const char* cache_tag;
public:
    LexicalProbs(UTF8InputStream& stream, CacheType cacheType, long cacheMax, const char* cache_tag);
    LexicalProbs() : cache_max(0), cache_type(None), cache_tag("") {}
	~LexicalProbs();
    float lookup(const LexicalProbs* altProbs,
                 const Symbol &mw, const Symbol &M, const Symbol &mt, const Symbol &P, const Symbol &H,
                 const Symbol &w, const Symbol &t) {
		Symbol ngram[N] = { mw, M, mt, P, H, w, t };
		return lookup(altProbs, ngram);
	}
	float lookup(const LexicalProbs* altProbs, Symbol *ngram);
    float lookupML(const Symbol &mw, const Symbol &M, const Symbol &mt, const Symbol &P, const Symbol &H,
                   const Symbol &w, const Symbol &t) const;
    float lookupML(const Symbol &mw, const Symbol &M, const Symbol &mt, const Symbol &P, const Symbol &H,
                   const Symbol &t) const;
    float lookupML(const Symbol &mw, const Symbol &M, const Symbol &mt) const;
    float lookupML(const Symbol &mw, const Symbol &mt) const;
    
    float computeValue(const LexicalProbs* altProbs, Symbol* ngram) const;
 
    void set_table_access(LexicalProbDeriver *lpd) {
      sevenGramLambda = lpd->get_lexicalHistories_MtPHwt();
      sixGramLambda = lpd->get_lexicalHistories_MtPHt();
      triGramLambda = lpd->get_lexicalHistories_Mt();
      sevenGramProb = lpd->get_lexicalTransitions_MtPHwt();
      sixGramProb = lpd->get_lexicalTransitions_MtPHt();
      triGramProb = lpd->get_lexicalTransitions_Mt();
      biGramProb = lpd->get_lexicalTransitions_t();
    }
    
    void readCache(const char* case_tag);
    void writeCache(const char* case_tag);
	void clearCache();

};

#endif
