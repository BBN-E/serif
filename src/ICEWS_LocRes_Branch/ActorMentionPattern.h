// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_ACTOR_MENTION_PATTERN_H
#define ICEWS_ACTOR_MENTION_PATTERN_H

#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "ICEWS/ActorMention.h"

namespace ICEWS {

/** Abstract interface for patterns that can match ICEWS actor mentions. */
class ICEWSActorMentionMatchingPattern {
public:
	virtual PatternFeatureSet_ptr matchesICEWSActorMention(PatternMatcher_ptr patternMatcher, ActorMention_ptr m) = 0;
};

/** Pattern for matching ICEWS ActorMentions.  Example:
  *
  * (icews-actor (paired-agent CVL))
  */
class ICEWSActorMentionPattern: public Pattern, public ICEWSActorMentionMatchingPattern {
private:
	ICEWSActorMentionPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(ICEWSActorMentionPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);
public:
	// Theory-level matchers
	virtual PatternFeatureSet_ptr matchesICEWSActorMention(PatternMatcher_ptr patternMatcher, ActorMention_ptr m);
	
	// Overridden virtual methods:
	virtual std::string typeName() const { return "ActorMentionPattern"; }
	virtual Pattern_ptr replaceShortcuts(const SymbolToPatternMap &refPatterns);
	virtual void getReturns(PatternReturnVecSeq & output) const;
	virtual void dump(std::ostream &out, int indent = 0) const;
private:
	virtual bool initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);

	/*********************************************************************
	 * Constraints
	 *********************************************************************/
	typedef enum {PROPER_NOUN_ACTOR, COMPOSITE_ACTOR, ANY_ACTOR} ActorType;
	ActorType _actorType;
	std::set<Symbol> _agentCodes; // must be one of these
	std::set<Symbol> _actorCodes; // must be one of these
	std::set<Symbol> _sectorCodes; // must be one of these
	std::set<Symbol> _blockAgentCodes; // may not be one of these
	std::set<Symbol> _blockActorCodes; // may not be one of these
	std::set<Symbol> _blockSectorCodes; // may not be one of these
	bool _mustBeCountry;
	bool _mayNotBeCountry;
	Pattern_ptr _mention;
	Pattern_ptr _blockMention;

	/*********************************************************************
	 * Helper Methods
	 *********************************************************************/
};

class ActorMentionReturnPFeature: public ReturnPatternFeature {
private:
	ActorMentionReturnPFeature(Pattern_ptr pattern, ActorMention_ptr actorMention)
		: ReturnPatternFeature(pattern), _actorMention(actorMention) {}
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(ActorMentionReturnPFeature, Pattern_ptr, ICEWS::ActorMention_ptr);
public:
	ActorMention_ptr getActorMention() const { return _actorMention; }
	virtual bool returnValueIsEqual(PatternFeature_ptr other_feature) const {
		boost::shared_ptr<ActorMentionReturnPFeature> other = boost::dynamic_pointer_cast<ActorMentionReturnPFeature>(other_feature);
		return (other && other->_actorMention==_actorMention); }
	virtual const wchar_t* getReturnTypeName() { return L"icews-actor"; }
	virtual int getSentenceNumber() const { return _actorMention->getEntityMention()->getSentenceNumber(); }
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {}
private:
	ActorMention_ptr _actorMention;
};
typedef boost::shared_ptr<ActorMentionReturnPFeature> ActorMentionReturnPFeature_ptr;


}

#endif
