// Copyright (c) 2006 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef UNION_PATTERN_H
#define UNION_PATTERN_H

#include "Generic/common/Symbol.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"

/** A sentence-matching pattern that will match a sentence iff any of its
  * subpatterns matches the sentence.  A UnionPattern can either be greedy,
  * in which case the returned feature set will only contain the features
  * from the first match; or non-greedy, in which case the returned feature
  * set will contain features from all matches.  (Note that greed does not
  * affect *which* sentences get matched, but instead affect what gets 
  * returned.  Greedy matching is faster, since it can return immediately
  * as soon as one subpattern matches.)
  *
  * The multiMatchesSentence() method can currently only be used with greedy 
  * union patterns, and will always return either 0 or 1 feature sets.
  */
class UnionPattern : public LanguageVariantSwitchingPattern, public SentenceMatchingPattern
{
private:
	UnionPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(UnionPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);

public:	
	~UnionPattern() {}

	bool isGreedy() const { return _is_greedy; }

	// Sentence-level matching
	PatternFeatureSet_ptr matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);	
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);

	// Overridden virtual methods:
	virtual std::string typeName() const { return "UnionPattern"; }
	virtual Pattern_ptr replaceShortcuts(const SymbolToPatternMap &refPatterns);
	virtual void getReturns(PatternReturnVecSeq & output) const;
	virtual void dump(std::ostream &out, int indent = 0) const;
	virtual Symbol getFirstValidID() const;

private:
	virtual bool initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	std::vector<Pattern_ptr> _patternList;
	bool _is_greedy;
};

#endif
