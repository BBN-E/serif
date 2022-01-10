// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ARGUMENT_PATTERN_H
#define ARGUMENT_PATTERN_H

#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/BoostUtil.h"
#include <boost/shared_ptr.hpp>
#include <vector>

class PatternMatcher;
class Argument;
class Mention;
class ValueMention;
class PatternFeatureSet;
typedef boost::shared_ptr<PatternFeatureSet> PatternFeatureSet_ptr;
typedef boost::shared_ptr<PatternMatcher> PatternMatcher_ptr;

/** Pattern used to match a single argument for a proposition, event mention,
  * or relation mention.  This pattern defines three pattern-matching methods,
  * which can be used to match against Arguments, Mentions, and ValueMentions
  * respectively.
  */
class ArgumentPattern: public Pattern,
	public ArgumentMatchingPattern, public MentionAndRoleMatchingPattern, public ValueMentionAndRoleMatchingPattern 
{
private:
	ArgumentPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(ArgumentPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);

public:
	~ArgumentPattern() {}
	
	// Pattern matching methods.
	PatternFeatureSet_ptr matchesArgument(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children);
	PatternFeatureSet_ptr matchesMentionAndRole(PatternMatcher_ptr patternMatcher, Symbol role, const Mention *mention, bool fall_through_children);
	PatternFeatureSet_ptr matchesValueMentionAndRole(PatternMatcher_ptr patternMatcher, Symbol role, const ValueMention *valueMention);

	/** Return true if this argument pattern is marked as optional.  If so, then
	  * parent patterns should not require it to match in order to be successful.
	  * The ArgumentPattern itself does not make any use of this value in its
	  * matchXyz() methods -- i.e., these methods can fail to match even if 
	  * isOptional() is true. */
	bool isOptional() const { return _is_optional; }

	// Overridden virtual methods:
	virtual std::string typeName() const { return "ArgumentPattern"; }
	virtual Pattern_ptr replaceShortcuts(const SymbolToPatternMap &refPatterns);
	virtual void getReturns(PatternReturnVecSeq & output) const;
	virtual void dump(std::ostream &out, int indent = 0) const;
	virtual bool allowFallThroughToChildren() const;

	/** Check whether any of this pattern's roles contains at least one
	  * capitalized letter.  If so, then raise an UnexpectedInputException. */
	void disallowCapitalizedRoles() const;

	bool hasFallThroughRoles() const { return _fallThroughRoles.size() != 0; }
	

private:
	virtual bool initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);

	// Constraints
	std::vector<Symbol> _roles;  // role must be in this set (unless the fall-through roles apply, below)
	Pattern_ptr _pattern;        // arg value must match this pattern

	std::vector<Symbol> _fallThroughRoles; // used to allow fall-through (for preposition attachment issues, mostly)
	PatternFeatureSet_ptr matchesNestedArgument(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children);

	// Other data
	bool _is_optional;           // checked by parent pattern

	// Helper method: return true if role is in _roles.
	bool matchesRole(Symbol role) const;
};

typedef boost::shared_ptr<ArgumentPattern> ArgumentPattern_ptr;

#endif
