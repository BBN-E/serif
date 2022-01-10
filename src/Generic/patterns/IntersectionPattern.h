// Copyright (c) 2006 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef INTERSECTION_PATTERN_H
#define INTERSECTION_PATTERN_H

#include "Generic/common/Symbol.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"

/** A sentence-matching pattern that will match a sentence iff all of its
  * subpatterns matches the sentence. 
  */
class IntersectionPattern : public LanguageVariantSwitchingPattern, public SentenceMatchingPattern
{
private:
	IntersectionPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(IntersectionPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);

public:	
	~IntersectionPattern() {}

	// Sentence-level matching
	PatternFeatureSet_ptr matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);	
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);

	// Overridden virtual methods:
	virtual std::string typeName() const { return "IntersectionPattern"; }
	virtual Pattern_ptr replaceShortcuts(const SymbolToPatternMap &refPatterns);
	virtual void getReturns(PatternReturnVecSeq & output) const;
	virtual void dump(std::ostream &out, int indent = 0) const;
	virtual Symbol getFirstValidID() const;
	size_t getNMembers() const { return _patternList.size(); }

private:
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);

	typedef std::pair<PatternFeatureSet_ptr, std::vector<float> > feature_score_pair_t;	
	std::vector<feature_score_pair_t> multiMatchesPatternGroup(PatternMatcher_ptr patternMatcher, std::vector<Pattern_ptr>& patternGroup,
																				SentenceTheory *sTheory, UTF8OutputStream *debug);

	std::vector<Pattern_ptr> _patternList;
};

#endif
