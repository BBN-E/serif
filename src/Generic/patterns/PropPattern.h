// Copyright (c) 2006 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef PROP_PATTERN_H
#define PROP_PATTERN_H

#include "common/Symbol.h"
#include "Generic/theories/Proposition.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/patterns/PropStatusManager.h"

/** A pattern used to search for propositions that satisfy a given set of
  * criteria.
  */
class PropPattern: public LanguageVariantSwitchingPattern,
	public SentenceMatchingPattern, public PropMatchingPattern, public ArgumentValueMatchingPattern {
private:
	PropPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(PropPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);

public:
	// Sentence-level matching methods
	std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out);
	PatternFeatureSet_ptr matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out);

	// Theory-level matching methods
	PatternFeatureSet_ptr matchesProp(PatternMatcher_ptr patternMatcher, int sent, const Proposition *prop, bool fall_through_children, PropStatusManager_ptr statusOverrides);
	PatternFeatureSet_ptr matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides);
	
	// Overridden virtual methods:
	virtual std::string typeName() const { return "PropPattern"; }
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
	std::set<Symbol> _predicates;
	std::set<Symbol> _alignedPredicates;
	std::set<Symbol> _predicatePrefixes;
	std::set<Symbol> _blockedPredicates;	
	std::set<Symbol> _blockedPredicatePrefixes;
	std::set<Symbol> _particles;
	std::set<Symbol> _adjectives;
	std::set<Symbol> _blockedAdjectives;
	std::set<Symbol> _adverbOrParticles;
	std::set<Symbol> _blockedAdverbsOrParticles;
	std::set<Symbol> _propModifierRoles;
	std::set<Symbol> _negations;
	std::set<Symbol> _negationPrefixes;
	std::set<Symbol> _blockedNegations;
	std::set<Symbol> _blockedNegationPrefixes;
	std::set<Symbol> _modals;
	std::set<Symbol> _modalPrefixes;
	std::set<Symbol> _blockedModals;
	std::set<Symbol> _blockedModalPrefixes;
	PropStatusManager_ptr _propStatusManager;
	bool _psm_manually_initialized;
	Pattern_ptr _propModifierPattern;
	Pattern_ptr _regexPattern;
	Proposition::PredType _predicateType;
	std::vector<Pattern_ptr> _args;
	std::vector<Pattern_ptr> _optArgs;
	std::vector<Pattern_ptr> _blockedArgs;
	bool _require_one_to_one_argument_mapping;
	bool _allow_many_to_many_mapping;
	bool _require_all_arguments_to_match_some_pattern;
	bool _stem_predicate;
	bool _block_fall_through;

	/*********************************************************************
	 * Helper Methods
	 *********************************************************************/
	bool validPredicate(Symbol predicate);
	bool blockedPredicate(Symbol predicate);
	bool matchesAdjective(const std::set<Symbol>& adjectives, PatternMatcher_ptr patternMatcher, int sent, const Proposition *prop);
	PatternFeatureSet_ptr matchesPropModifier(PatternMatcher_ptr patternMatcher, int sent_no, const Proposition *prop);
	bool matchesNegation(const std::set<Symbol>& negations, const Proposition *prop);

};

typedef boost::shared_ptr<PropPattern> PropPattern_ptr;

#endif

