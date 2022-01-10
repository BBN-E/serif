// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_EVENT_MENTION_PATTERN_H
#define ICEWS_EVENT_MENTION_PATTERN_H

#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/theories/DocTheory.h"
#include "ICEWS/EventMention.h"

namespace ICEWS {

/** Abstract interface for patterns that can match ICEWS event mentions. */
class ICEWSEventMentionMatchingPattern {
public:
	virtual PatternFeatureSet_ptr matchesICEWSEventMention(PatternMatcher_ptr patternMatcher, ICEWSEventMention_ptr em) = 0;
};

/** Pattern for matching ICEWS EventMentions.  Example:
  *
  * (icews-event (type 181 182) 
  *              (same-actor source target)
  *              (same-agent source target)
  *              (source (icews-actor ...))
  *              (target (icews-actor ...)))
  */
class ICEWSEventMentionPattern: public Pattern,  
	public ICEWSEventMentionMatchingPattern, public DocumentMatchingPattern {
private:
	ICEWSEventMentionPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(ICEWSEventMentionPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);
public:
	// Document-level matchers
	virtual std::vector<PatternFeatureSet_ptr> multiMatchesDocument(PatternMatcher_ptr patternMatcher, UTF8OutputStream *debug = 0);
	virtual PatternFeatureSet_ptr matchesDocument(PatternMatcher_ptr patternMatcher, UTF8OutputStream *debug = 0);

	// Theory-level matchers
	virtual PatternFeatureSet_ptr matchesICEWSEventMention(PatternMatcher_ptr patternMatcher, ICEWSEventMention_ptr em);
	
	// Overridden virtual methods:
	virtual std::string typeName() const { return "EventMentionPattern"; }
	virtual Pattern_ptr replaceShortcuts(const SymbolToPatternMap &refPatterns);
	virtual void getReturns(PatternReturnVecSeq & output) const;
	virtual void dump(std::ostream &out, int indent = 0) const;
private:
	virtual bool initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	virtual bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);

	/*********************************************************************
	 * Constraints
	 *********************************************************************/
	std::set<Symbol> _eventCode;
	std::set<Symbol> _blockEventCode;

	Symbol::HashMap<Pattern_ptr> _participants;
	Symbol::HashMap<Pattern_ptr> _blockParticipants;

	std::vector<std::set<Symbol> > _sameActorSets;
	std::vector<std::set<Symbol> > _blockSameActorSets;

	std::vector<std::set<Symbol> > _sameAgentSets;
	std::vector<std::set<Symbol> > _blockSameAgentSets;

	std::vector<Pattern_ptr> _sentenceMatches;
	std::vector<Pattern_ptr> _blockSentenceMatches;

	std::vector<Pattern_ptr> _documentMatches;
	std::vector<Pattern_ptr> _blockDocumentMatches;

	/*********************************************************************
	 * Helper methods
	 *********************************************************************/
	bool actorsAreTheSame(ICEWSEventMention_ptr em, const std::set<Symbol> &roles);
	bool agentsAreTheSame(ICEWSEventMention_ptr em, const std::set<Symbol> &roles);
	bool getAndCheckBoundVariables(const PatternFeatureSet_ptr match, const DocTheory *docTheory, std::map<std::wstring, Symbol>& existing_variables, 
		std::map<std::wstring, Symbol>& new_variables);

};

class ICEWSEventMentionReturnPFeature: public ReturnPatternFeature {
private:
	ICEWSEventMentionReturnPFeature(Pattern_ptr pattern, ICEWSEventMention_ptr eventMention)
		: ReturnPatternFeature(pattern), _eventMention(eventMention) {}
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(ICEWSEventMentionReturnPFeature, Pattern_ptr, ICEWS::ICEWSEventMention_ptr);
public:
	ICEWSEventMention_ptr getEventMention() const { return _eventMention; }
	virtual bool returnValueIsEqual(PatternFeature_ptr other_feature) const {
		boost::shared_ptr<ICEWSEventMentionReturnPFeature> other = boost::dynamic_pointer_cast<ICEWSEventMentionReturnPFeature>(other_feature);
		return (other && other->_eventMention==_eventMention); }
	virtual const wchar_t* getReturnTypeName() { return L"icews-event"; }
	virtual void setCoverage(const DocTheory * docTheory) {}
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher) {}
	virtual int getSentenceNumber() const { return -1; } // do NOT call default version of setCoverage, it will crash
	void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {}
private:
	ICEWSEventMention_ptr _eventMention;
};
typedef boost::shared_ptr<ICEWSEventMentionReturnPFeature> ICEWSEventMentionReturnPFeature_ptr;

class ICEWSEventMentionPFeature: public PatternFeature {
private:
	ICEWSEventMentionPFeature(Pattern_ptr pattern, ICEWSEventMention_ptr eventMention)
		: PatternFeature(pattern), _eventMention(eventMention) {}
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(ICEWSEventMentionPFeature, Pattern_ptr, ICEWS::ICEWSEventMention_ptr);
public:
	ICEWSEventMention_ptr getEventMention() const { return _eventMention; }
	bool isEventToBeDiscarded() const { return _eventMention->getEventType()->discardEventsWithThisType(); }
	virtual void setCoverage(const DocTheory * docTheory) {}
	virtual void setCoverage(const PatternMatcher_ptr patternMatcher) {}
	virtual void printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {}
	virtual bool equals(PatternFeature_ptr other) {
		boost::shared_ptr<ICEWSEventMentionPFeature> f = boost::dynamic_pointer_cast<ICEWSEventMentionPFeature>(other);
		return f && f->getEventMention() == getEventMention();
	}
	virtual int getSentenceNumber() const { return -1; }
	virtual int getStartToken() const { return -1; }
	virtual int getEndToken() const { return -1; }
private:
	ICEWSEventMention_ptr _eventMention;
};
typedef boost::shared_ptr<ICEWSEventMentionPFeature> ICEWSEventMentionPFeature_ptr;
}


#endif
