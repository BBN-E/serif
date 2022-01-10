// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_ACTOR_MENTION_FINDER_H
#define ICEWS_ACTOR_MENTION_FINDER_H

#include "ICEWS/ActorMention.h"
#include "ICEWS/JabariTokenMatcher.h"
#include "ICEWS/ActorInfo.h"
#include "ICEWS/Gazetteer.h"
#include "ICEWS/LocationMentionResolver.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/theories/SentenceItemIdentifier.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"
#include <vector>

// Forward declarations
class DocTheory;
class EntitySet;
class Mention;
class MentionSet;
class SynNode;
class PatternSet;
class PatternMatcher; 
typedef boost::shared_ptr<PatternSet> PatternSet_ptr;
typedef boost::shared_ptr<PatternMatcher> PatternMatcher_ptr;
namespace ICEWS { class ActorMentionSet; }


namespace ICEWS {


/** A document-level processing class used to find all of the ActorMentions 
  * in a given document.  JabariTokenMatchers (typically initialized based
  * on ICEWS databases) are used to identify the candidate actors and agents 
  * in the document.  A PatternMatcher (loaded from a file whose filename
  * is specified by the parameter "icews_agent_actor_patterns") is used to 
  * identify the actor/agent pairs that make up CompositeActorMentions.
  * A greedy algorithm is used to resolve conflicts between candidate actors.
  *
  * This processing class is run during the "icews_actors" stage.
  */
class ActorMentionFinder: public DocumentDriver::DocTheoryStageHandler, private boost::noncopyable {
public:
	ActorMentionFinder();
	~ActorMentionFinder();

	/** Identify any actor mentions in the given document that can be tied
	  * to specific actors in the ICEWS database (either as "proper-noun
	  * actors" or as "composite actors" (aka agents of proper-noun actors).
	  * Create a new ActorMentionSet containing these actors, and add it to
	  * the given DocTheory. */
	void process(DocTheory *dt);

	// Used by EventMentionFinder -- perhaps move this somewhere else?
	static float countToScore(float count, float max_input_val=10, float max_output_val=5);

	// This is used by EventMentionFinder when determining paired-actor values
	// for composite actors whose paired-actor is unknown.  It is also used to
	// determine which mentions should not get default country assignment.
	static std::map<MentionUID, Symbol> findMentionsThatBlockDefaultPairedActor(const DocTheory *docTheory, PatternMatcher_ptr patternSet);

private:
	typedef std::vector<std::vector<ActorMatch> > ActorMatchesBySentence;
	typedef std::vector<std::vector<AgentMatch> > AgentMatchesBySentence;
	typedef std::vector<std::vector<CompositeActorMatch> > CompositeActorMatchesBySentence;

	/** Class used to search for known composite actors (eg IOF=Israeli Occupation Forces) */
	CompositeActorTokenMatcher _compositeActorTokenMatcher;

	/** Class used to search for actors */
	ActorTokenMatcher _actorTokenMatcher;

	/** Class used to search for agents */
	AgentTokenMatcher _agentTokenMatcher;

	/** Classed used to resolve locations to database entries */
	Gazetteer _gazetteer;
	ComboLMR _locationMentionResolver;

	// We use these two types to associate a score with each actor mention, 
	// and then to sort them by score:
	typedef std::pair<double, ActorMention_ptr> ScoredActorMention;
	typedef std::set<ScoredActorMention> SortedActorMentions;

	struct PairedActorMention {
		PairedActorMention(): actorMention(), agentActorPatternName(), isTemporary() {}
		PairedActorMention(ActorMention_ptr actorMention, Symbol agentActorPatternName, bool isTemporary): 
		actorMention(actorMention), agentActorPatternName(agentActorPatternName), isTemporary(isTemporary) {}
		ActorMention_ptr actorMention;
		Symbol agentActorPatternName;
		bool isTemporary;
	};
	typedef std::map<MentionUID, PairedActorMention> AgentActorPairs;

	// The private ActorMentionCounts structure is used to record document-
	// wide frequency counts for actors and countries based on the 
	// preliminary set of matches.  This is then used to help resolve ambiguous
	// entity linking decisions.
	struct ActorMentionCounts;
	ActorMentionCounts getActorMentionCounts(const ActorMatchesBySentence &actorMatches, ActorInfo &actorInfo, const char* publicationDate);

	/** Search for any ProperNounActors that occur in the given document, and add them
	  * to the ActorMentionSet "result".  Return a list of matches that were not used.
	  */
	ActorMatchesBySentence findProperNounActorMentions(DocTheory* docTheory, ActorMentionSet *result, ActorInfo &actorInfo, int sent_cutoff, const char *publicationDate);

	void findUSCities(DocTheory* docTheory, SortedActorMentions& result, ActorInfo &actorInfo, int sent_cutoff);
	bool isUSState(Mention* mention);

	void findCompositeActorMentions(DocTheory* docTheory, ActorMentionSet *result, ActorInfo &actorInfo, int sent_cutoff, const AgentActorPairs &agentActorPairs);

	/** Given an actor match, construct a corresponding actor mention,
	  * and assign it a score. */
	ScoredActorMention makeProperNounActorMention(const ActorMatch &actorMatch, const DocTheory* docTheory, const SentenceTheory *sentTheory, const ActorMentionCounts &actorMentionCounts, ActorInfo &actorInfo, const ActorMatchesBySentence &actorMatches, Symbol sourceNote, const char* publicationDate);
	ScoredActorMention makeProperNounActorMention(const Mention* mention, Gazetteer::LocationInfo_ptr locationInfo, const DocTheory* docTheory, const SentenceTheory *sentTheory, const ActorMentionCounts &actorMentionCounts, ActorInfo &actorInfo, const char* publicationDate);
	ScoredActorMention getUnambiguousLocationActorMention(const Mention* mention, const DocTheory* docTheory, const SentenceTheory *sentTheory, const char* publicationDate);
	ScoredActorMention getUnresolvedLocationActorMention(const Mention* mention, const DocTheory* docTheory, const SentenceTheory *sentTheory, const char* publicationDate);
	ScoredActorMention makeCompositeActorMention(const AgentMatch &agentMatch, const DocTheory* docTheory, const SentenceTheory *sentTheory, 
		const AgentActorPairs &agentActorPairs, const ActorMentionSet* actorMentions, ActorInfo &actorInfo);
	ScoredActorMention makePrecomposedCompositeActorMention(const CompositeActorMatch &match, const DocTheory* docTheory, const SentenceTheory *sentTheory, ActorInfo &actorInfo, Symbol sourceNote, const char* publicationDate, const ActorMentionFinder::ActorMentionCounts *actorMentionCounts=0, const ActorMatchesBySentence *actorMatches=0);

	typedef enum { PAIRED_PROPER_NOUN_ACTOR, PAIRED_COMPOSITE_ACTOR, PAIRED_UNKNOWN_ACTOR} PairedActorKind;
	AgentActorPairs findAgentActorPairs(DocTheory *docTheory, const ActorMentionSet *actorMentionSet, ActorInfo &actorInfo, PairedActorKind actorKind, const ActorMatchesBySentence* unusedActorMatches=0);
	void findPairedAgentActorMentions(PatternMatcher_ptr patternMatcher, SentenceTheory* sentTheory, const ActorMentionSet *actorMentionSet, AgentActorPairs &result, PairedActorKind actorKind);
	PairedActorMention findActorForAgent(const Mention *agentMention, const AgentActorPairs &agentActorPairs, const DocTheory* docTheory);

	ProperNounActorMention_ptr getDefaultCountryActorMention(const ActorMentionSet *actorsFoundSoFar, ActorInfo &actorInfo);
	void assignDefaultCountryForUnknownPairedActors(ActorMentionSet *actorMentionSet, ProperNounActorMention_ptr pairedActorMention, 
		const AgentActorPairs &agentsOfUnknownActors, ActorInfo &actorInfo, const DocTheory *docTheory, std::map<MentionUID, Symbol>& blockedMentions);
	ProperNounActorMention_ptr defaultCountryAssignmentIsBlockedByOtherCountry(ActorMention_ptr actor, ProperNounActorMention_ptr pairedActorMention, ActorMentionSet *actorMentionSet, ActorInfo &actorInfo);

	bool isCompatibleAndBetter(ActorMention_ptr oldActorMention, ActorMention_ptr newActorMention);

	void greedilyAddActorMentions(const SortedActorMentions& sortedActorMentions, ActorMentionSet *result, ActorInfo &actorInfo);

	void labelPeople(const DocTheory* docTheory, ActorMentionSet *result, ActorInfo &actorInfo, int sent_cutoff, 
		const AgentActorPairs &agentsOfProperNounActors, const AgentActorPairs &agentsOfCompositeActors, bool allow_unknowns);
	void labelLocationsAndFacilities(const DocTheory* docTheory, ActorMentionSet *result, ActorInfo &actorInfo, int sent_cutoff, 
		ProperNounActorMention_ptr defaultCountryActorMention, const AgentActorPairs &agentsOfProperNounActors, const AgentActorPairs &agentsOfCompositeActors,
		std::map<MentionUID, Symbol>& blockedMentions);

	void discardBareActorMentions(ActorMentionSet *actorMentionSet);


	/** For each mention that is identified by an ActorMention in the given 
	  * ActorMentionSet, copy that ActorMention to any coreferent mentions
	  * (i.e., any mentions that belong to the same entity.  "result" is an
	  * input/output parameter. */
	void labelCoreferentMentions(DocTheory *docTheory, bool conservative, ActorMentionSet *result, int sent_cutoff);

	bool sisterMentionHasClashingActor(const Mention *agentMention, ProperNounActorMention_ptr proposedActor, const DocTheory *docTheory, ActorMentionSet *actorMentionSet);

	/** For each mention that is identified by an ActorMention in the given
	  * ActorMentionSet, if it is the child of a partitive meniton, then
	  * copy the ActorMention information to its parent. */
	void labelPartitiveMentions(DocTheory *docTheory, ActorMentionSet *result, int sent_cutoff);

	// Display a log message showing that we added (or skipped) a given actor mention.
	void logActorMention(ActorMention_ptr actorMention, double score, ActorMention_ptr conflictingActor, ActorInfo &actorInfo, bool replace_old_value);

	/** Patterns used to find the actors for agents */
	PatternSet_ptr _agentPatternSet;

	// For debugging/logging -- maps pattern label to number of times we used it.
	Symbol::HashMap<size_t> _agentPatternMatchCounts;


	typedef std::pair<const SynNode*, const Mention*> SynNodeAndMention;

	/** Given a span in a sentence, return a NAME or DESC mention that corresponds
	  * with it, or NULL if none is found. */
	const Mention* getCoveringNameDescMention(const SentenceTheory* sentTheory, int start_tok, int end_tok);
	const Mention* getCoveringNameDescMention(const SentenceTheory* sentTheory, const SynNode* node);

	std::set<std::wstring> _countryModifierWords;
	std::set<std::wstring> _personModifierWords;
	std::set<std::wstring> _organizationModifierWords;

	std::set<std::wstring> _perAgentNameWords;

	/** Patterns that block default country assignment */
	PatternSet_ptr _blockDefaultCountryPatternSet;

	/** Add the given actor mention to the result set, and update _agentPatternMatchCounts. */
	void addActorMention(ActorMention_ptr actorMention, ActorMentionSet *result);

	float scoreAcronymMatch(const DocTheory *docTheory, const SentenceTheory *sentTheory, ActorMatch match, const ActorMatchesBySentence &actorMatches, ActorInfo &actorInfo);
	float getAssociationScore(ActorId actorId, const char* date, const ActorMentionFinder::ActorMentionCounts &actorMentionCounts, ActorInfo &actorInfo);

	// Parameters from the parameter file that affect our behavior:
	size_t _verbosity;
	bool _disable_coref;                              // default: false
	bool _discard_pronoun_actors;                     // default: false
	bool _discard_plural_actors;                      // default: false
	bool _discard_plural_pronoun_actors;              // default: false
	bool _encode_person_matching_country_as_citizen;  // default: true
	bool _block_default_country_if_another_country_in_same_sentence;  // default: true
	bool _block_default_country_if_unknown_paired_actor_is_found;     // default: true
	bool _includeCitiesForDefaultCountrySelection;    // default: false
	bool _useDefaultLocationResolution;				  // default: false

	// If there are more than this number of distinct locations for a given 
	// name in the gazetteer, then don't use it to create an actor:
	int _maxAmbiguityForGazetteerActors;

	// Used to log how often each sector is used.
	bool _logSectorFreqs;
	Symbol::HashMap<size_t> _sectorFreqs;

	// This includes both names and abbreviations:
	std::set<Symbol> _us_state_names;

	int _icews_sentence_cutoff;
	int getSentCutoff(const DocTheory* docTheory);

	//std::pair<ProperNounActorMention_ptr, Symbol> getPairedActor(const Mention *mention, const AgentActorPairs& agentActorPairs, ProperNounActorMention_ptr defaultCountryActorMention, ActorMentionSet *actorMentionSet);

	void findLocalAcronymCompositeActorMentions(DocTheory* docTheory, ActorMentionSet *result, ActorInfo &actorInfo, int sent_cutoff, const char *publicationDate);
	void resolveAmbiguousLocationsToDefaultCountry(ProperNounActorMention_ptr defaultCountryActorMention, const DocTheory* docTheory, ActorInfo &actorInfo, ActorMentionSet *result, int sent_cutoff);
	ActorTokenMatcher_ptr findLocalProperNounAcronymDefinitions(const SortedActorMentions& sortedActorMentions);
	CompositeActorTokenMatcher_ptr findLocalCompositeAcronymDefinitions(const ActorMentionSet *actorMentions);
	std::wstring getJabariPatternFromAcronymDefinition(ActorMention_ptr actor);

	// resolve locations in the final ActorMentionSet to specific database entries
	void resolveLocations(ActorMentionSet *actorMentionSet);

};

} // end namespace ICEWS

#endif

