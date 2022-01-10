// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_PATTERN_H
#define EVENT_PATTERN_H

#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/ExtractionPattern.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/BoostUtil.h"
#include <vector>

/** A pattern used to search for event mentions that satisfy a given 
  * set of criteria.
  */
class EventPattern: public ExtractionPattern,
	public SentenceMatchingPattern, public EventMentionMatchingPattern, public ArgumentValueMatchingPattern, public MentionMatchingPattern {
private:
	EventPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(EventPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);
public:
	~EventPattern() {}

	// Sentence-level matchers
	PatternFeatureSet_ptr matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);

	// Sub-sentence-level matchers
	PatternFeatureSet_ptr matchesEventMention(PatternMatcher_ptr patternMatcher, int sent_no, const EventMention *vm);
	PatternFeatureSet_ptr matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides);
	PatternFeatureSet_ptr matchesMention(PatternMatcher_ptr patternMatcher, const Mention *mention, bool fall_through_children);

	// Overridden virtual methods
	virtual std::string typeName() const { return "EventPattern"; }
	virtual Pattern_ptr replaceShortcuts(const SymbolToPatternMap &refPatterns);
	virtual bool allowFallThroughToChildren() const { return true; }

private:
	/*********************************************************************
	 * Data
	 *********************************************************************/
	Pattern_ptr _anchorPattern;

	/*********************************************************************
	 * Construction Helper Methods
	 *********************************************************************/
	virtual Symbol getPatternTypeSym(); // returns Symbol(L"event")
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);

	/*********************************************************************
	 * Helper Methods
	 *********************************************************************/
	EventMention *getVMFromNode(PatternMatcher_ptr patternMatcher, const SynNode *node, int sent_no);
};

typedef boost::shared_ptr<EventPattern> EventPattern_ptr;

#endif
