// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_EVENT_MENTION_FINDER_H
#define ICEWS_EVENT_MENTION_FINDER_H

#include "Generic/driver/DocumentDriver.h"
#include "Generic/common/Symbol.h"
#include "Generic/icews/ICEWSEventMention.h"
#include "Generic/actors/ActorInfo.h"
#include "Generic/actors/JabariTokenMatcher.h"
#include "Generic/actors/ActorMentionFinder.h"
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
class ActorMentionSet;
typedef boost::shared_ptr<PatternSet> PatternSet_ptr;
typedef boost::shared_ptr<PatternFeatureSet> PatternFeatureSet_ptr;
typedef boost::shared_ptr<PatternMatcher> PatternMatcher_ptr;

class ICEWSEventMentionSet;
class ICEWSEventMention;
class ICEWSEventType;
typedef boost::shared_ptr<ICEWSEventType> ICEWSEventType_ptr;
typedef boost::shared_ptr<ICEWSEventMention> ICEWSEventMention_ptr;

/** A document-level processing class used to find the ICEWSEventMentions 
  * in a given document.  A set of PatternMatchers (specified via the
  * "icews_event_models" parameter) are used to identify the event mentions
  * in each sentence.
  *
  * This processing class is run during the "icews-events" stage.
  */
class ICEWSEventMentionFinder: public DocumentDriver::DocTheoryStageHandler, private boost::noncopyable {
public:
	ICEWSEventMentionFinder();
	~ICEWSEventMentionFinder() { delete _actorMentionFinder; }
	void process(DocTheory *dt);
private:
	ActorMentionFinder* _actorMentionFinder; // for finding default country actor
	typedef std::pair<PatternSet_ptr, Symbol> PatternSetAndCode;
	typedef boost::unordered_map<Symbol, std::vector<const Mention*>, Symbol::Hash, Symbol::Eq> RoleMentions;
	typedef ICEWSEventMention::ParticipantList ParticipantList;
public:
	struct MatchData {
		Symbol eventCode;
		Symbol patternLabel;
		RoleMentions roleMentions;
		Symbol tense;
		//hang on to the pattern feature set that was used to populate this MatchData struct
		PatternFeatureSet_ptr matchedPatternFeatureSet;
		ValueMention* timeValueMention;
		int originalSentNo;
		std::vector<const Proposition*> propositions;
		MatchData(Symbol defaultEventCode): eventCode(defaultEventCode), patternLabel(), roleMentions(), tense(ICEWSEventMention::NEUTRAL_TENSE), originalSentNo(-1) {}
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
	void applyPatternSet(DocTheory *docTheory, ICEWSEventMentionSet* eventMentionSet, PatternMatcher_ptr matcher, Symbol defaultEventCode, 
		ActorMentionSet *actorMentions, int n_sentences, const std::map<MentionUID, Symbol> &blockedPairedActors, int& global_event_id_counter);
	MatchData extractMatchData(PatternFeatureSet_ptr match, Symbol defaultEventCode);
	bool checkMatchData(MatchData& matchData);
	void addEventMentions(ICEWSEventMentionSet* eventMentionSet, const DocTheory *docTheory, MatchData& matchData, ActorMentionSet *actorMentions, Symbol original_event_id);
	void addEventMentionsHelper(ICEWSEventMentionSet* eventMentionSet, const DocTheory *docTheory,
	                            ICEWSEventType_ptr eventType, MatchData& matchData, 
	                            ParticipantList &participantList, 
								std::vector<Symbol> &roles, ActorMentionSet *actorMentions, 
								Symbol original_event_id);
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
	void removeParticipantOverriddenEvents(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory);
	void removeDuplicateEvents(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory, size_t event_grouping);
	std::wstring getEventGroupingKey(ICEWSEventMention_ptr eventMention, const DocTheory *docTheory, size_t event_grouping, bool per_event_type);
	void markAllButOneAsDuplicate(const std::vector<ICEWSEventMention_ptr> &eventMentionGroup, std::set<ICEWSEventMention_ptr> &duplicateEvents, const std::wstring& duplicateRemovalKey);
	float getScoreForDuplicateEventMention(ICEWSEventMention_ptr eventMention);
	typedef std::vector<ICEWSEventMention_ptr> EventMentionGroup;
	std::map<std::wstring, EventMentionGroup> divideEventsIntoUniquenessGroups(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory, size_t event_grouping, bool per_event_type);
	void removeBlockedEvents(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory, PatternSet_ptr patternSet);
	void removeTemporaryEvents(ICEWSEventMentionSet *eventMentionSet);
	void removeSameEntityEvents(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory);


	// Tag event tense
	void tagEventTense(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory);

	// Helper
	std::vector<PatternFeatureSet_ptr> getICEWSEventMatches(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory, PatternSet_ptr patternSet);

	// Apply patterns that modify an event's type
	void applyReplaceEventTypePatterns(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory);

	// determineEventLocation is used to find paired actors for composite actor mentions
	ProperNounActorMention_ptr getPairedActorLocation(PatternFeatureSet_ptr match, const SentenceTheory *sentTheory, const ActorMentionSet *actorMentions);
	ProperNounActorMention_ptr getLocationForProp(const Proposition *prop, const SentenceTheory *sentTheory, const ActorMentionSet *actorMentions);
	void setDefaultLocations(DocTheory *docTheory, const MatchData& matchData, ProperNounActorMention_ptr eventLoc, ActorMentionSet *actorMentions, const SentenceTheory *sentTheory, const std::map<MentionUID, Symbol> &blockedPairedActors);

	void propagateActorLabels(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory);
	/** Literal event codes (eg "1413") or patterns that use "*" (eg "03*") */
	std::vector<boost::wregex> _enabledEventCodes;
	int _verbosity;	// default: 1
	int _actor_event_sentence_cutoff;
	bool _print_events_to_stdout;

	/* Event Location Resolution Functions and Parameters */

	/* _icews_add_locations_to_events --> determines if event mention finder should attempt to add LOCATION role to an icews event mention. 
	governed by par file parameter "icews_add_locations_to_events"; default: false */
	bool _icews_add_locations_to_events; 

	/* _blockEventLocationPatternSet --> patterns that prevent events from having a LOCATION role added to the matching event tuple
	governed by par file parameter "icews_block_event_location_patterns" */
	PatternSet_ptr _blockEventLocationPatternSet; 

	/* _eventLocationPatternSet --> patterns that indicate what geonames (DB) resolved locational mention should be used as the LOCATION role for an event (if any)
	governed by par file parameter "icews_event_location_patterns" (ordering of patterns matters as the first rule found to match is used to resolve event location*/
	PatternSet_ptr _eventLocationPatternSet;

	/* addLocationsToEvents --> ties locations to events whenever possible, adding a third role (LOCATION) to the event tuple. 
	Greedily tries to find pattern matches for patterns from _eventLocationPatternSet to deteremine event location.  First match found is used
	to for event location resolution, so ordering of patterns matters.  */
	void addLocationsToEvents(ICEWSEventMentionSet* eventMentionSet, const ActorMentionSet *actorMentions, const DocTheory* docTheory);
	
	/* blockEventLocationResolutions --> builds a set of events that should not have a LOCATION role added, based on the patterns found in the 
	_blockEventLocationPatternSet. */
	std::set<ICEWSEventMention_ptr> blockEventLocationResolutions(ICEWSEventMentionSet* eventMentionSet, const DocTheory* docTheory, PatternSet_ptr patternSet);

	/* Used to populate locational data for events when only an approximate location can be found for a given pattern.  Finds an ICEWS actor mention that has been 
	resolved to the geonames database and is within the given country iso code.*/
	ProperNounActorMention_ptr findCountryActor(const ActorMentionSet *actorMentions, Symbol isoCode);

	float countToScore(float count, float max_input_val, float max_output_val);

	ActorInfo_ptr _actorInfo;

	bool _do_not_binarize_icews_events;
	ICEWSEventMentionSet *unbinarizeEvents(ICEWSEventMentionSet *eventMentionSet);

};


#endif

