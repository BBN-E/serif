// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef VALUE_MENTION_PATTERN_H
#define VALUE_MENTION_PATTERN_H

#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/patterns/QueryDate.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/common/BoostUtil.h"
#include <vector>

// Forward declarations
class PatternMatcher;
class SentenceTheory;
class UTF8OutputStream;
class Argument;
class SynNode;
class EntityType;
class EntitySubtype;

/** A pattern used to search for value mentions that satisfy a given set of
  * criteria.
  */
class ValueMentionPattern: public LanguageVariantSwitchingPattern,
	public SentenceMatchingPattern, public ValueMentionMatchingPattern, public ArgumentValueMatchingPattern, public ParseNodeMatchingPattern
{
private:
	ValueMentionPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(ValueMentionPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);

public:
	/** Sentence-level matchers */
	PatternFeatureSet_ptr matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);

	/** Theory object matchers */
	PatternFeatureSet_ptr matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides);
	PatternFeatureSet_ptr matchesValueMention(PatternMatcher_ptr patternMatcher, const ValueMention *valueMention);
	PatternFeatureSet_ptr matchesParseNode(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node);

	// Overridden virtual methods:
	virtual std::string typeName() const { return "ValueMentionPattern"; }
	virtual Pattern_ptr replaceShortcuts(const SymbolToPatternMap &refPatterns);
	virtual void getReturns(PatternReturnVecSeq & output) const;
	virtual void dump(std::ostream &out, int indent = 0) const;
	virtual bool allowFallThroughToChildren() const { return true; }

private:
	virtual bool initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);

	/*********************************************************************
	 * Constraints
	 *********************************************************************/
	QueryDate::DateStatus _activity_date_status;
	bool _must_be_specific_date;
	bool _must_be_recent_date;
	bool _must_be_future_date;
	static const int RECENT_DAYS_CONSTRAINT = 30;
	std::vector<ValueType> _valueTypes;
	Pattern_ptr _regexPattern;
	
	typedef struct {
		Symbol constraint_type;
		Symbol comparison_operator;
		int value;
	} ComparisonConstraint;
	std::vector<ComparisonConstraint> _comparisonConstraints;
	
	/** Create and return a feature set for the given valueMention.  If this 
	  * pattern has a regex subpattern, then that pattern is checked during  
	  * this method; and if it fails, then a NULL feature set is returned. */
	PatternFeatureSet_ptr makePatternFeatureSet(const ValueMention* valueMention, PatternMatcher_ptr patternMatcher, const Symbol matchSym=Symbol(), float confidence=1.0);
};

typedef boost::shared_ptr<ValueMentionPattern> ValueMentionPattern_ptr;

#endif
