// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTION_PATTERN_H
#define MENTION_PATTERN_H

#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/BoostUtil.h"
#include <set>
#include <vector>

// Forward declarations
class PatternMatcher;
class SentenceTheory;
class UTF8OutputStream;
class Argument;
class SynNode;
class EntityType;
class EntitySubtype;
class Entity;

/** A pattern used to search for mentions that satisfy a given set of
  * criteria.
  */
class MentionPattern: public LanguageVariantSwitchingPattern,
	public SentenceMatchingPattern, public MentionMatchingPattern, public ArgumentValueMatchingPattern, public ParseNodeMatchingPattern
{
private:
	MentionPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(MentionPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);

public:
	/** Sentence-level matchers */
	PatternFeatureSet_ptr matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);

	/** Theory object matchers */
	PatternFeatureSet_ptr matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides);
	PatternFeatureSet_ptr matchesMention(PatternMatcher_ptr patternMatcher, const Mention *mention, bool fall_through_children);
	PatternFeatureSet_ptr matchesParseNode(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node);

	/** Return true if this MentionPattern was marked as the "FOCUS".  This is
	  * used by entity-labeling patterns to mark the mention whose entity should
	  * recieve the label. */
	bool isFocusMentionPattern() const { return _is_focus; }
	
	// Overridden virtual methods:
	virtual std::string typeName() const { return "MentionPattern"; }
	virtual Pattern_ptr replaceShortcuts(const SymbolToPatternMap &refPatterns);
	virtual void getReturns(PatternReturnVecSeq & output) const;
	virtual void dump(std::ostream &out, int indent = 0) const;
	virtual bool allowFallThroughToChildren() const { return !_block_fall_through; }
	
private:
	virtual bool initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);

	// Is this a FOCUS mention for an entity-labeling pattern?
	bool _is_focus;

	/*********************************************************************
	 * Constraints
	 *********************************************************************/
	bool _requires_name;
	bool _requires_name_or_desc;
	bool _is_specific;
	bool _is_generic;
	bool _is_appositive;
	bool _is_appositive_child;
	bool _is_named_appositive_child;
	bool _block_fall_through;

	typedef struct {
		Symbol constraint_type;
		Symbol comparison_operator;
		int value;
	} ComparisonConstraint;

	struct ArgOfPropConstraint {
		std::set<Symbol> roles;
		Pattern_ptr propPattern;
	};
	typedef boost::shared_ptr<ArgOfPropConstraint> ArgOfPropConstraint_ptr;

	std::vector<ComparisonConstraint> _comparisonConstraints;
	std::vector<EntityType> _aceTypes;
	std::vector<EntitySubtype> _aceSubtypes;
	std::vector<EntityType> _blockedAceTypes;
	std::set<Mention::Type> _mentionTypes;
	std::set<Symbol> _headwords;
	std::set<Symbol> _headwordPrefixes;
	std::set<Symbol> _blockedHeadwords;
	std::set<Symbol> _blockedHeadwordPrefixes;
	std::vector<Symbol> _entityLabels;
	std::vector<Symbol> _blockingEntityLabels;
	Pattern_ptr _regexPattern;
	Pattern_ptr _propDefPattern;
	bool _head_regex;
	std::vector<ArgOfPropConstraint_ptr> _argOfPropConstraints;
	Symbol _brownClusterConstraint;
	int _brownCluster;
	int _brown_cluster_prefix;

	/** Create and return a feature set for the given mention.  matchSym is either   
	  * an entity type, an entity subtype, or a label.  If this pattern has
	  * a regex subpattern, then that pattern is checked during this method; and 
	  * if it fails, then a NULL feature set is returned. */
	PatternFeatureSet_ptr makePatternFeatureSet(const Mention* mention, PatternMatcher_ptr patternMatcher, const Symbol matchSym=Symbol(), float confidence=1.0);

	bool isGeneric(const Entity* ent, PatternMatcher_ptr patternMatcher) const;
};

typedef boost::shared_ptr<MentionPattern> MentionPattern_ptr;

#endif
