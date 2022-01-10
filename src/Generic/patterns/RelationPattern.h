// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_PATTERN_H
#define RELATION_PATTERN_H

#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/ExtractionPattern.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/BoostUtil.h"
#include <vector>

/** A pattern used to search for relation mentions that satisfy a given 
  * set of criteria.
  */
class RelationPattern: public ExtractionPattern,
	public SentenceMatchingPattern, public RelMentionMatchingPattern {
private:
	RelationPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(RelationPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);
public:
	~RelationPattern() {}

	// Sentence-level matchers
	PatternFeatureSet_ptr matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);

	// Sub-sentence-level matchers
	PatternFeatureSet_ptr matchesRelMention(PatternMatcher_ptr patternMatcher, int sent_no, const RelMention *vm);

	// Overridden virtual methods
	virtual std::string typeName() const { return "RelationPattern"; }
private:
	/*********************************************************************
	 * Construction Helper Methods
	 *********************************************************************/
	virtual Symbol getPatternTypeSym(); // returns Symbol(L"relation")
};

typedef boost::shared_ptr<RelationPattern> RelationPattern_ptr;
#endif
