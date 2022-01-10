// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef QUOTATION_PATTERN_H
#define QUOTATION_PATTERN_H

#include "common/Symbol.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/common/BoostUtil.h"
class Sexp;
class SentenceTheory;

/** A pattern that matches a quotation in a document.  A quotation consists
  * of a speaker (identified by a mention) and a quoted span of text, which
  * may span across multiple sentences and may contain partial sentences.
  * Each QuotationPattern can have two sub-patterns: one matches the speaker, 
  * and the other matches the quotation. */
class QuotationPattern: public Pattern, 
	public SpeakerQuotationMatchingPattern, public DocumentMatchingPattern {
private:
	QuotationPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(QuotationPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);
public:
	// Matcher methods
	std::vector<PatternFeatureSet_ptr> multiMatchesDocument(PatternMatcher_ptr patternMatcher, UTF8OutputStream *debug = 0);
	PatternFeatureSet_ptr matchesDocument(PatternMatcher_ptr patternMatcher, UTF8OutputStream *debug = 0);
	PatternFeatureSet_ptr matchesSpeakerQuotation(PatternMatcher_ptr patternMatcher, const SpeakerQuotation *quotation);

	// Overridden virtual methods:
	virtual std::string typeName() const { return "QuotationPattern"; }
	virtual Pattern_ptr replaceShortcuts(const SymbolToPatternMap &refPatterns);
	virtual void dump(std::ostream &out, int indent = 0) const;

private:
	bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);

	Pattern_ptr _speakerPattern;
	Pattern_ptr _quotePattern;
};

#endif
