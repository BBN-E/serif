// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TOPIC_PATTERN_H
#define TOPIC_PATTERN_H

#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/PropTree/PropMatch.h"
#include "Generic/common/BoostUtil.h"

class TopicPattern: public Pattern, 
	public SentenceMatchingPattern, public MentionMatchingPattern,
	public ArgumentValueMatchingPattern, public PropMatchingPattern {
private:
	TopicPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(TopicPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);
public:
	// Sentence-level matchers
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);
	PatternFeatureSet_ptr matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);

	// Theory-level matchers
	PatternFeatureSet_ptr matchesMention(PatternMatcher_ptr patternMatcher, const Mention *ment, bool fall_through_children);
	PatternFeatureSet_ptr matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides);
	PatternFeatureSet_ptr matchesProp(PatternMatcher_ptr patternMatcher, int sent_no, const Proposition* prop, bool fall_through_children, PropStatusManager_ptr statusOverrides);

	// Accessors
	typedef enum { FULL, EDGE, NODE, DECISION_TREE } MatchStrategy;
	MatchStrategy getMatchStrategy() const { return _match_strategy; }
	bool useRealScores() const { return _use_real_scores; }
	Symbol getQuerySlot() const { return _querySlot; }
	int getContextSize() const { return _context_size; }
	int getForwardRange() const { return _forward_range; }
	int getBackwardRange() const { return _backward_range; }

	// Overridden virtual methods:
	virtual std::string typeName() const { return "PropPattern"; }
	virtual void dump(std::ostream &out, int indent = 0) const;
	virtual bool allowFallThroughToChildren() const { return true; }
private:
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	PatternFeatureSet_ptr makeCorePatternFeatureSet(int sentno, const LanguageVariant_ptr& languageVariant, float confidence, const Mention* mention=0);
	PatternFeatureSet_ptr makePatternFeatureSet(boost::shared_ptr<PropMatch> propMatch, int sentno, const LanguageVariant_ptr& languageVariant, float confidence, const Mention* mention=0);
	PatternFeatureSet_ptr makePatternFeatureSet(std::vector<PatternFeatureSet_ptr> matches, int sentno, const LanguageVariant_ptr& languageVariant, float confidence);
	
	bool satisfiesQueryConstraints(boost::shared_ptr<PropMatch> propMatch);

	Symbol _querySlot;
	float _threshold;
	int _context_size;
	int _forward_range;
	int _backward_range;
	int _min_node_count;
	int _max_node_count;
	MatchStrategy _match_strategy;
	bool _use_real_scores;
};

typedef boost::shared_ptr<TopicPattern> TopicPattern_ptr;

#endif
