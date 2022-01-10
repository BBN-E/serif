// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef COMBINATION_PATTERN_H
#define COMBINATION_PATTERN_H

#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/common/BoostUtil.h"

/** A pattern that matches a theory object if some subset of its 
  * subpatterns match that theory object.  There are two kinds of 
  * combination pattern: ALL_OF, which matches a theory object if
  * all of its subpatterns match that theory object; and ANY_OF, 
  * which matches a theory object if any of its subpatterns match
  * that theory object.
  *
  * The class CombinationPattern actually serves as a base class
  * for various specialized subclasses that only match against a 
  * specific set of theory objects.  For example, the 
  * MentionCombinationPattern subclass only matches against theory
  * objects that are matched by MentionPattern.  A CombinationPattern
  * gets replaced by a pattern with the appropriate subclass during
  * shortcut expansion, since this is the earliest time that we know
  * what the pattern types of the subpatterns are (and therefore what
  * subclass should be used).
  */
class CombinationPattern: public Pattern, public SentenceMatchingPattern {
	CombinationPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(CombinationPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);
public:
	typedef enum {ANY_OF, ALL_OF, NONE_OF} CombinationType;

	// Sentence-level matching methods
	PatternFeatureSet_ptr matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out);
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out);

	CombinationType getCombinationType() const { return _combination_type; }
	bool isGreedy() const { return _is_greedy; }
	size_t getNMembers() const { return _patternList.size(); }
	Pattern_ptr getMember(size_t n) const { return _patternList[n]; }

	virtual void multiplyReturns(int patternCount, PatternReturnVecSeq & output) const;

	// Overridden virtual methods:
	virtual std::string typeName() const { return "CombinationPattern"; }
	virtual Pattern_ptr replaceShortcuts(const SymbolToPatternMap &refPatterns);
	virtual void getReturns(PatternReturnVecSeq & output) const;
	virtual void dump(std::ostream &out, int indent = 0) const;
	virtual Symbol getFirstValidID() const;
	
	bool hasFallThroughRoles() const;
	bool allowFallThrough() const;

private:
	virtual bool initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);

protected:
	CombinationPattern(boost::shared_ptr<CombinationPattern> pattern);

	template<typename MatchingType, typename ARG1_TYPE>
	PatternFeatureSet_ptr matchesWith(PatternMatcher_ptr patternMatcher, ARG1_TYPE);
	template<typename MatchingType, typename ARG1_TYPE, typename ARG2_TYPE>
	PatternFeatureSet_ptr matchesWith(PatternMatcher_ptr patternMatcher, ARG1_TYPE, ARG2_TYPE);
	template<typename MatchingType, typename ARG1_TYPE, typename ARG2_TYPE, typename ARG3_TYPE>
	PatternFeatureSet_ptr matchesWith(PatternMatcher_ptr patternMatcher, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE);

	std::vector<Pattern_ptr> _patternList;
	CombinationType _combination_type;
	PropStatusManager_ptr _patternStatusOverrides;
	bool _is_greedy; // Only relevant for ANY_OF patterns.
};
typedef boost::shared_ptr<CombinationPattern> CombinationPattern_ptr;






class PropCombinationPattern: public CombinationPattern,
	public PropMatchingPattern, public ArgumentValueMatchingPattern
{
	PropCombinationPattern(CombinationPattern_ptr pattern): CombinationPattern(pattern) {}
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(PropCombinationPattern, CombinationPattern_ptr);
public:
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out);
	PatternFeatureSet_ptr matchesProp(PatternMatcher_ptr patternMatcher, int sent, const Proposition *prop, bool fall_through_children, PropStatusManager_ptr statusOverrides);
	PatternFeatureSet_ptr matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides);
	virtual std::string typeName() const { return "PropCombinationPattern"; }
	virtual bool allowFallThroughToChildren() const;
};

class EventCombinationPattern: public CombinationPattern,
	public EventMentionMatchingPattern, public ArgumentValueMatchingPattern, public MentionMatchingPattern
{
	EventCombinationPattern(CombinationPattern_ptr pattern): CombinationPattern(pattern) {}
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(EventCombinationPattern, CombinationPattern_ptr);
public:
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out);
	virtual PatternFeatureSet_ptr matchesEventMention(PatternMatcher_ptr patternMatcher, int sent_no, const EventMention *em);
	virtual PatternFeatureSet_ptr matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides);
	virtual PatternFeatureSet_ptr matchesMention(PatternMatcher_ptr patternMatcher, const Mention *mention, bool fall_through_children);
	virtual std::string typeName() const { return "EventCombinationPattern"; }
	virtual bool allowFallThroughToChildren() const;
};

class RelationCombinationPattern: public CombinationPattern,
	public RelMentionMatchingPattern 
{
	RelationCombinationPattern(CombinationPattern_ptr pattern): CombinationPattern(pattern) {}
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(RelationCombinationPattern, CombinationPattern_ptr);
public:
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out);
	virtual PatternFeatureSet_ptr matchesRelMention(PatternMatcher_ptr patternMatcher, int sent_no, const RelMention *rm);
	virtual std::string typeName() const { return "RelationCombinationPattern"; }
};

class MentionCombinationPattern: public CombinationPattern,
	public MentionMatchingPattern, public ArgumentValueMatchingPattern, public ParseNodeMatchingPattern
{
	MentionCombinationPattern(CombinationPattern_ptr pattern): CombinationPattern(pattern) {}
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(MentionCombinationPattern, CombinationPattern_ptr);
public:
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out);
	virtual PatternFeatureSet_ptr matchesMention(PatternMatcher_ptr patternMatcher, const Mention *mention, bool fall_through_children);
	virtual PatternFeatureSet_ptr matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides);
	virtual PatternFeatureSet_ptr matchesParseNode(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node);
	virtual std::string typeName() const { return "MentionCombinationPattern"; }
	virtual bool allowFallThroughToChildren() const;

};

class ArgumentCombinationPattern: public CombinationPattern,
	public ArgumentMatchingPattern, public MentionAndRoleMatchingPattern, public ValueMentionAndRoleMatchingPattern 
{
	ArgumentCombinationPattern(CombinationPattern_ptr pattern): CombinationPattern(pattern) {}
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(ArgumentCombinationPattern, CombinationPattern_ptr);
public:
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out);
	virtual PatternFeatureSet_ptr matchesArgument(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children);
	virtual PatternFeatureSet_ptr matchesMentionAndRole(PatternMatcher_ptr patternMatcher, Symbol role, const Mention *mention, bool fall_through_children);
	virtual PatternFeatureSet_ptr matchesValueMentionAndRole(PatternMatcher_ptr patternMatcher, Symbol role, const ValueMention *valueMention);
	virtual std::string typeName() const { return "ArgumentCombinationPattern"; }
	virtual bool allowFallThroughToChildren() const;
};

class ArgumentValueCombinationPattern: public CombinationPattern, public ArgumentValueMatchingPattern
{
	ArgumentValueCombinationPattern(CombinationPattern_ptr pattern): CombinationPattern(pattern) {}
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(ArgumentValueCombinationPattern, CombinationPattern_ptr);
public:
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out);
	virtual PatternFeatureSet_ptr matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides);
	virtual std::string typeName() const { return "ArgumentValueCombinationPattern"; }
	virtual bool allowFallThroughToChildren() const;
private:
	PatternFeatureSet_ptr matchesChildren(PatternMatcher_ptr patternMatcher, int sent_no, const Proposition *compoundProp, bool fall_through_children, Symbol role, PropStatusManager_ptr statusOverridesToUse);
};

class ParseNodeCombinationPattern: public CombinationPattern,
	public ParseNodeMatchingPattern 
{
	ParseNodeCombinationPattern(CombinationPattern_ptr pattern): CombinationPattern(pattern) {}
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(ParseNodeCombinationPattern, CombinationPattern_ptr);
public:
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out);
	virtual PatternFeatureSet_ptr matchesParseNode(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node);
	virtual std::string typeName() const { return "ParseNodeCombinationPattern"; }
};

#endif
