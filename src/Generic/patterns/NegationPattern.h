// Copyright (c) 2006 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef NEGATION_PATTERN_H
#define NEGATION_PATTERN_H

#include "Generic/common/Symbol.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"

/** A sentence-matching pattern that will match a sentence iff all of its
  * subpatterns matches the sentence. 
  */
class NegationPattern : public Pattern,
	public SentenceMatchingPattern
{
private:
	NegationPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(NegationPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);

public:	
	~NegationPattern() {}

	// Sentence-level matching
	PatternFeatureSet_ptr matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);	
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);

	// Overridden virtual methods:
	virtual std::string typeName() const { return "NegationPattern"; }
	virtual Pattern_ptr replaceShortcuts(const SymbolToPatternMap &refPatterns);
	virtual void dump(std::ostream &out, int indent = 0) const;

private:
	virtual bool initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	PatternFeatureSet_ptr matchForSentence(int sent_no, const LanguageVariant_ptr& languageVariant);
	Pattern_ptr _pattern;
};

#endif
