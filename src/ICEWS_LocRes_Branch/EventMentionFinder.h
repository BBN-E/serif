// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_EVENT_MENTION_FINDER_H
#define ICEWS_EVENT_MENTION_FINDER_H

#include "Generic/driver/DocumentDriver.h"
#include "Generic/common/Symbol.h"
#include "ICEWS/JabariTokenMatcher.h"
#include "ICEWS/EventMention.h"
#include "Generic/theories/SentenceItemIdentifier.h"
#include <boost/noncopyable.hpp>
#include <boost/regex.hpp> 
#include <vector>

// Forward declarations
class DocTheory;
class PatternSet;
class PatternFeatureSet;
class PatternMatcher;
class SentenceTheory;
class Mention;
class Proposition;
typedef boost::shared_ptr<PatternSet> PatternSet_ptr;
typedef boost::shared_ptr<PatternFeatureSet> PatternFeatureSet_ptr;
typedef boost::shared_ptr<PatternMatcher> PatternMatcher_ptr;
namespace ICEWS {
	class ICEWSEventMentionSet;
	class ICEWSEventMention;
	class ICEWSEventType;
	class ActorMention;
	class ActorMentionSet;
	class ProperNounActorMention;
	typedef boost::shared_ptr<ICEWSEventType> ICEWSEventType_ptr;
	typedef boost::shared_ptr<ActorMention> ActorMention_ptr;
	typedef boost::shared_ptr<ICEWSEventMention> ICEWSEventMention_ptr;
	typedef boost::shared_ptr<ProperNounActorMention> ProperNounActorMention_ptr;
}

namespace ICEWS {

/** A document-level processing class used to find the ICEWSEventMentions 
  * in a given document.  A set of PatternMatchers (specified via the
  * "icews_event_patterns" parameter) are used to identify the event mentions
  * in each sentence.
  *
  * If the "icews_create_events_with_generic_actors" parameter is false, 
  * then the ICEWSEventMentionFinder will only create events if all of the
  * event's participants are tied to actor mentions that were identified 
  * during the icews_actors stage (i.e., if the participants are all either
  * known actors or agents of known actors).  If this parameter is true
  * (the default), then the ICEWSEventMentionFinder will create events for 
  * any event mentions it finds, even if the participants are unknown.  In 
  * this case, the ICEWSEventMentionFinder will add new (generic) 
  * ActorMention objects to the document's ActorMentionSet.
  *
  * This processing class is run during the "icews_events" stage.
  */
class ICEWSEventMentionFinder: public DocumentDriver::DocTheoryStageHandler, private boost::noncopyable {
public:
	ICEWSEventMentionFinder();
	void process(DocTheory *dt);
private:
	bool _create_events_with_generic_actors;
	typedef std::pair<PatternSet_ptr, Symbol> PatternSetAndCode;
	typedef boost::unordered_map<Symbol, std::vector<const Mention*>, Symbol::Hash, Symbol::Eq> RoleMentions;
	typedef ICEWSEventMention::ParticipantMap ParticipantMap;
public:
	struct MatchData {
		Symbol eventCode;
		Symbol patternLabel;
		RoleMentions roleMentions;
		Symbol tense;
		MatchData(Symbol defaultEventCode): eventCode(defaultEventCode), patternLabel(), roleMentions(), tense(ICEWSEventMention::NEUTRAL_TENSE) {}
	};
private:
	/** A list of pattern sets that should be used to find event
	  * mentions, along with a default event code for each pattern
	  * set.  These default event codes can be overriden by 
	  * individaul patterns, using "event-code" return features. */
	std::vector<PatternSetAndCode> _patternSets;

	/** A list of patterns that can be used to block patterns that
	  * we would otherwise generate. */
	PatternSet_ptr _blockEventPatternSet;
	PatternSet_ptr _blockContingentEventPatternSet;

	/** Patterns that are used to block composite actors from getting 
	  * assigned paired actors based on the event location. */
	PatternSet_ptr _blockEventLocPairedActorPatternSet;

	/** A list of patterns that can be used to assign tense to
	  * ICEWS events */
	PatternSet_ptr _eventTensePatternSet;

	/** Patterns that are used to propagate actor labels from one 
	  * event participant to another. */
	PatternSet_ptr _propagageActorLabelPatternSet;

	/** Patterns that are used to modify an event's type. */
	PatternSet_ptr _replaceEventTypePatternSet;

	// Helper methods:
	void loadPatternSets(const std::string &patternSetFileList, std::vector<PatternSetAndCode> &patternSets);
	void applyPatternSet(ICEWSEventMentionSet* eventMentionSet, PatternMatcher_ptr matcher, Symbol defaultEventCode, ActorMentionSet *actorMentions, int n_sentences, const std::map<MentionUID, Symbol> &blockedPairedActors);
	MatchData extractMatchData(PatternFeatureSet_ptr match, Symbol defaultEventCode);
	bool checkMatchData(MatchData& matchData);
	void addEventMentions(ICEWSEventMentionSet* eventMentionSet, const DocTheory *docTheory, MatchData& matchData, ActorMentionSet *actorMentions);
	void addEventMentionsHelper(ICEWSEventMentionSet* eventMentionSet, const DocTheory *docTheory,
	                            ICEWSEventType_ptr eventType, MatchData& matchData, 
	                            ParticipantMap &participantMap, std::set<const Mention*> mentionsUsed,
								std::vector<Symbol> &roles, ActorMentionSet *actorMentions);
	ActorMention_ptr getActorForMention(const SentenceTheory *sentTheory, const Mention *mention, ActorMentionSet *actorMentions);
	bool eventCodeEnabled(Symbol eventCode);

	// A set of flags that indicates when we should consider two
	// events to be "different" for the purposes of eliminating 
	// duplicate events.
	std::vector<size_t> _event_uniqueness_groups;
	std::vector<size_t> _event_override_groups;
	static const size_t UNIQUE_PER_EVENT_TYPE             = 1 << 0;
	static const size_t UNIQUE_PER_SENTENCE               = 1 << 1;
	static const size_t UNIQUE_PER_MENTION_PAIR           = 1 << 2;
	static const size_t UNIQUE_PER_ENTITY_PAIR            = 1 << 3;
	static const size_t UNIQUE_PER_ACTOR_PAIR             = 1 << 4;
	static const size_t UNIQUE_PER_PROPER_NOUN_ACTOR_PAIR = 1 << 5;
	static const size_t UNIQUE_PER_EVENT_GROUP            = 1 << 6;
	static const size_t UNIQUE_PER_ICEWS_SENTENCE         = 1 << 7;
	std::vector<size_t> parseEventUniquenessGroupSpec(const char* paramName, const char* defaultValue);

	// Removal of duplicate and overridden events:
	void removeOverriddenEvents(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory, size_t event_grouping, bool temporary_events_only);
	void removeDuplicateEvents(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory, size_t event_grouping);
	std::wstring getEventGroupingKey(ICEWSEventMention_ptr eventMention, const DocTheory *docTheory, size_t event_grouping, bool per_event_type);
	void markAllButOneAsDuplicate(const std::vector<ICEWSEventMention_ptr> &eventMentionGroup, std::set<ICEWSEventMention_ptr> &duplicateEvents, const std::wstring& duplicateRemovalKey);
	float getScoreForDuplicateEventMention(ICEWSEventMention_ptr eventMention);
	typedef std::vector<ICEWSEventMention_ptr> EventMentionGroup;
	std::map<std::wstring, EventMentionGroup> divideEventsIntoUniquenessGroups(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory, size_t event_grouping, bool per_event_type);
	void removeBlockedEvents(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory, PatternSet_ptr patternSet);
	void removeTemporaryEvents(ICEWSEventMentionSet *eventMentionSet);

	// Tag event tense
	void tagEventTense(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory);

	// Helper
	std::vector<PatternFeatureSet_ptr> getICEWSEventMatches(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory, PatternSet_ptr patternSet);

	// Apply patterns that modify an event's type
	void applyReplaceEventTypePatterns(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory);

	ProperNounActorMention_ptr determineEventLocation(PatternFeatureSet_ptr match, const SentenceTheory *sentTheory, const ActorMentionSet *actorMentions);
	ProperNounActorMention_ptr getLocationForProp(const Proposition *prop, const SentenceTheory *sentTheory, const ActorMentionSet *actorMentions);
	void setDefaultLocations(const MatchData& matchData, ProperNounActorMention_ptr eventLoc, ActorMentionSet *actorMentions, const SentenceTheory *sentTheory, const std::map<MentionUID, Symbol> &blockedPairedActors);

	void propagateActorLabels(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory);

	/** Literal event codes (eg "1413") or patterns that use "*" (eg "03*") */
	std::vector<boost::wregex> _enabledEventCodes;

	int _icews_sentence_cutoff;
};

} // end of namespace ICEWS

#endif

