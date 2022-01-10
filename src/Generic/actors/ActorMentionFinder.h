// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACTOR_MENTION_FINDER_H
#define ACTOR_MENTION_FINDER_H

#include "Generic/actors/Gazetteer.h"
#include "Generic/actors/LocationMentionResolver.h"
#include "Generic/actors/ActorInfo.h"
#include "Generic/actors/JabariTokenMatcher.h"
#include "Generic/actors/ActorEditDistance.h"
#include "Generic/actors/ActorEntityScorer.h"
#include "Generic/actors/ActorTokenSubsetTrees.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/theories/SentenceItemIdentifier.h"
#include "Generic/theories/ActorMention.h"
#include "Generic/theories/ActorEntity.h"
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
class ActorMentionSet;
typedef boost::shared_ptr<PatternSet> PatternSet_ptr;
typedef boost::shared_ptr<PatternMatcher> PatternMatcher_ptr;

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
	enum Mode { ICEWS, ACTOR_MATCH, DOC_ACTORS };

	ActorMentionFinder(Mode mode = ICEWS);
	~ActorMentionFinder();

	/** Identify any actor mentions in the given document that can be tied
	  * to specific actors in the ICEWS database (either as "proper-noun
	  * actors" or as "composite actors" (aka agents of proper-noun actors).
	  * Create a new ActorMentionSet containing these actors, and add it to
	  * the given DocTheory. */
	void process(DocTheory *dt);

	/** Sentence level actor mention creation */
	ActorMentionSet* process(const SentenceTheory *sentTheory, const DocTheory *docTheory);

	ProperNounActorMention_ptr getDefaultCountryActorMention(const ActorMentionSet *actorsFoundSoFar);
	
	// This is used by EventMentionFinder when determining paired-actor values
	// for composite actors whose paired-actor is unknown.  It is also used to
	// determine which mentions should not get default country assignment.
	static std::map<MentionUID, Symbol> findMentionsThatBlockDefaultPairedActor(const DocTheory *docTheory, PatternMatcher_ptr patternSet);
	static bool sisterMentionHasClashingActor(const Mention *agentMention, ProperNounActorMention_ptr proposedActor, const DocTheory *docTheory, ActorMentionSet *actorMentionSet, int verbosity);

	void resetForNewSentence();
	void resetForNewDocument();

private:
	typedef enum { UNAMBIGUOUS, BEST_RESOLUTION_DESPITE_AMBIGUITY, ALL_RESOLUTIONS } resolution_ambiguity_t;

	typedef std::vector<std::vector<ActorMatch> > ActorMatchesBySentence;
	typedef std::vector<std::vector<AgentMatch> > AgentMatchesBySentence;
	typedef std::vector<std::vector<CompositeActorMatch> > CompositeActorMatchesBySentence;

	/** Class used to search for known composite actors (eg IOF=Israeli Occupation Forces) */
	CompositeActorTokenMatcher_ptr _compositeActorTokenMatcher;

	/** Class used to search for actors */
	ActorTokenMatcher_ptr _actorTokenMatcher;

	/** Class used to search for agents */
	AgentTokenMatcher_ptr _agentTokenMatcher;

	/** Classes used to resolve locations to database entries */
	Gazetteer_ptr _gazetteer;
	LocationMentionResolver_ptr _locationMentionResolver;

	/** Class used to calculate edit distance between Actors and Mentions */
	ActorEditDistance_ptr _actorEditDistance;

	/** Class used to do XDoc token subset trees on Actors and Mentions */
	ActorTokenSubsetTrees_ptr _actorTokenSubsetTrees;

	/** Class used to create and score ActorEntities in the doc-actors stage */
	ActorEntityScorer_ptr _actorEntityScorer;

	/** USA is treated specially in findUSCitites */
	ActorId _usa_actorid;

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

	/** Search for any ProperNounActors that occur in the given document, and add them
	  * to the ActorMentionSet "result".  Return a list of matches that were not used.
	  */
	ActorMatchesBySentence findProperNounActorMentions(DocTheory* docTheory, ActorMentionSet *result, int sent_cutoff, const char *publicationDate);

	/** Given a span in a sentence, return a NAME or DESC mention that corresponds
	  * with it, or NULL if none is found. */
	static const Mention* getCoveringNameDescMention(const SentenceTheory* sentTheory, int start_tok, int end_tok);
	static const Mention* getCoveringNameDescMention(const SentenceTheory* sentTheory, const SynNode* node);

	std::set<Symbol> _typesForForcedActorMentionCreation;
	void forceActorMentionCreation(const DocTheory* docTheory, ActorMentionSet *actorMentionSet, int sent_cutoff);

	void findUSCities(DocTheory* docTheory, SortedActorMentions& result, int sent_cutoff);
	bool isUSState(Mention* mention);
	bool isUSCity(Mention* mention);

	void findCompositeActorMentions(DocTheory* docTheory, ActorMentionSet *result, int sent_cutoff, const AgentActorPairs &agentActorPairs);

	/** Given an actor match, construct a corresponding actor mention,
	  * and assign it a score. */
	ScoredActorMention makeProperNounActorMention(const ActorMatch &actorMatch, const DocTheory* docTheory, const SentenceTheory *sentTheory, const ActorMatchesBySentence &actorMatches, Symbol sourceNote, const char* publicationDate);

	void resolveNamedLocations(const SentenceTheory *sentTheory, std::vector<ActorMatch> patternActorMatches, ActorMentionSet *currentActorMentionSet, 
		ActorMentionFinder::SortedActorMentions& result, const char *publicationDate, resolution_ambiguity_t ambiguity);

	ScoredActorMention makeProperNounActorMentionFromGazetteer(const Mention* mention, Gazetteer::ScoredGeoResolution resolution, const SentenceTheory *sentTheory, Symbol actorNoteSym);
	
	double scoreProperNounActorMention(ActorMention_ptr *actorMention, const ActorMatch &match, const DocTheory* docTheory, const SentenceTheory *sentTheory, const ActorMatchesBySentence &actorMatches, bool head_match, bool country_the_plural, bool add_weight_to_score);
	ActorMention_ptr createProperNounActorMention(const ActorMatch &match, const SentenceTheory *sentTheory, Symbol sourceNote, bool &head_match, bool &country_the_plural);
	ActorMention_ptr createSoftwareMention(const ActorMatch &match, const SentenceTheory *sentTheory, Symbol sourceNote, bool &head_match, bool &country_the_plural);
	ScoredActorMention makeCompositeActorMention(const AgentMatch &agentMatch, const DocTheory* docTheory, const SentenceTheory *sentTheory, 
		const AgentActorPairs &agentActorPairs, const ActorMentionSet* actorMentions);
	ScoredActorMention makePrecomposedCompositeActorMention(const CompositeActorMatch &match, const DocTheory* docTheory, const SentenceTheory *sentTheory, Symbol sourceNote, const char* publicationDate, const ActorMatchesBySentence *actorMatches=0);

	typedef enum { PAIRED_PROPER_NOUN_ACTOR, PAIRED_COMPOSITE_ACTOR, PAIRED_UNKNOWN_ACTOR} PairedActorKind;
	AgentActorPairs findAgentActorPairs(DocTheory *docTheory, const ActorMentionSet *actorMentionSet, PairedActorKind actorKind, const ActorMatchesBySentence* unusedActorMatches=0);
	void findPairedAgentActorMentions(PatternMatcher_ptr patternMatcher, SentenceTheory* sentTheory, const ActorMentionSet *actorMentionSet, AgentActorPairs &result, PairedActorKind actorKind);
	PairedActorMention findActorForAgent(const Mention *agentMention, const AgentActorPairs &agentActorPairs, const DocTheory* docTheory);
	void findAgentCodeForEntity(const Entity *ent, ActorMentionSet *currentActorMentionSet, int sent_cutoff, AgentId& id, Symbol& code);

	ProperNounActorMention_ptr getDefaultCountryActorMention(std::vector<ActorMention_ptr> &actorMentions);
	void fillDocumentCountryCounts(std::vector<ActorMention_ptr> &actorMentions); 
	void fillTentativeCountryCounts(const ActorMatchesBySentence& actorMatches); 
	void clearDocumentCountryCounts() {
		_countryCounts.clear();
		_countryActors.clear();
		_cityActors.clear();
	}
	double getAssociationScore(ActorMention_ptr actorMention, const char *publicationDate);

	void assignDefaultCountryForUnknownPairedActors(ActorMentionSet *actorMentionSet, ProperNounActorMention_ptr pairedActorMention, 
		const AgentActorPairs &agentsOfUnknownActors, AgentActorPairs& agentsOfProperNounActors, const DocTheory *docTheory, std::map<MentionUID, Symbol>& blockedMentions);
	ProperNounActorMention_ptr defaultCountryAssignmentIsBlockedByOtherCountry(ActorMention_ptr actor, ProperNounActorMention_ptr pairedActorMention, ActorMentionSet *actorMentionSet);
	void addExplicitLocations(DocTheory* docTheory, ActorMentionSet *result, int sent_cutoff, const AgentActorPairs &agentsOfProperNounActors);

	bool isCompatibleAndBetter(ActorMention_ptr oldActorMention, ActorMention_ptr newActorMention);

	void greedilyAddActorMentions(const SortedActorMentions& sortedActorMentions, ActorMentionSet *result);

	void labelPeople(const DocTheory* docTheory, ActorMentionSet *result, int sent_cutoff, 
		const AgentActorPairs &agentsOfProperNounActors, const AgentActorPairs &agentsOfCompositeActors, bool allow_unknowns);
	void labelOrganizations(const DocTheory* docTheory, ActorMentionSet *result, 
		int sent_cutoff, const AgentActorPairs &agentsOfProperNounActors, const AgentActorPairs &agentsOfCompositeActors);
	void labelLocationsAndFacilities(const DocTheory* docTheory, ActorMentionSet *result, int sent_cutoff, 
		ProperNounActorMention_ptr defaultCountryActorMention, const AgentActorPairs &agentsOfProperNounActors, const AgentActorPairs &agentsOfCompositeActors,
		std::map<MentionUID, Symbol>& blockedMentions);

	void discardBareActorMentions(ActorMentionSet *actorMentionSet);


	/** For each mention that is identified by an ActorMention in the given 
	  * ActorMentionSet, copy that ActorMention to any coreferent mentions
	  * (i.e., any mentions that belong to the same entity.  "result" is an
	  * input/output parameter. */
	void labelCoreferentMentions(DocTheory *docTheory, int aggressiveness, ActorMentionSet *result, AgentActorPairs& agentsOfProperNounActors, int sent_cutoff);

	/** For each mention that is identified by an ActorMention in the given
	  * ActorMentionSet, if it is the child of a partitive meniton, then
	  * copy the ActorMention information to its parent. */
	void labelPartitiveMentions(DocTheory *docTheory, ActorMentionSet *result, int sent_cutoff);

	// Display a log message showing that we added (or skipped) a given actor mention.
	void logActorMention(ActorMention_ptr actorMention, double score, ActorMention_ptr conflictingActor, bool replace_old_value);

	/** Patterns used to find the actors for agents */
	PatternSet_ptr _agentPatternSet;

	/** Patterns used to find AWAKE agents (since we don't use a dictionary) */
	PatternSet_ptr _awakeAgentPatternSet;

	// For debugging/logging -- maps pattern label to number of times we used it.
	Symbol::HashMap<size_t> _agentPatternMatchCounts;

	// For tracking document state
	ActorId::HashMap<float> _countryCounts;
	ActorId::HashMap<ProperNounActorMention_ptr> _countryActors;
	ActorId::HashMap<ProperNounActorMention_ptr> _cityActors;

	typedef std::pair<const SynNode*, const Mention*> SynNodeAndMention;

	std::set<std::wstring> _countryModifierWords;
	std::set<std::wstring> _personModifierWords;
	std::set<std::wstring> _organizationModifierWords;

	std::set<std::wstring> _perAgentNameWords;

	std::vector<ActorId> _actorsNotContributingToCountryCounts;

	/** Patterns that block default country assignment */
	PatternSet_ptr _blockDefaultCountryPatternSet;

	/** Add the given actor mention to the result set, and update _agentPatternMatchCounts. */
	void addActorMention(ActorMention_ptr actorMention, ActorMentionSet *result);

	std::pair<std::wstring, std::wstring> normalizeAcronymExpansion(std::wstring name);
	float scoreAcronymMatch(const DocTheory *docTheory, const SentenceTheory *sentTheory, ActorMatch match, const ActorMatchesBySentence &actorMatches);
	
	float scoreContextRequiredMatch(const DocTheory *docTheory, const SentenceTheory *sentTheory, ActorMatch match, const ActorMatchesBySentence &actorMatches);
	
	// Parameters from the parameter file that affect our behavior:
	size_t _verbosity;
	bool _disable_coref;                              // default: false
	bool _discard_pronoun_actors;                     // default: false
	bool _discard_plural_actors;                      // default: false
	bool _discard_plural_pronoun_actors;              // default: false
	bool _georesolve_facs;                            // default: false
	bool _encode_person_matching_country_as_citizen;  // default: true
	bool _block_default_country_if_another_country_in_same_sentence;  // default: true
	bool _block_default_country_if_unknown_paired_actor_is_found;     // default: true
	bool _require_entity_type_match;                                  // default: false, only in use in non-icews mode
	bool _allow_fac_org_matches;                                      // default: false, only in use when _require_entity_type_match is true
	bool _only_match_names;                                           // default: false, only in use in non-icews mode
	

	// Use this parameter to help tune precision/recall balance; default is 0.5
	float _icews_actor_match_aggressiveness;

	// If there are more than this number of distinct locations for a given 
	// name in the gazetteer, then don't use it to create an actor:
	int _max_ambiguity_for_gazetteer_actors;

	// Used to log how often each sector is used.
	bool _log_sector_freqs;
	Symbol::HashMap<size_t> _sectorFreqs;

	// These include both names and abbreviations:
	std::set<Symbol> _us_state_names;

	int _actor_event_sentence_cutoff;
	int getSentCutoff(const DocTheory* docTheory);

	//std::pair<ProperNounActorMention_ptr, Symbol> getPairedActor(const Mention *mention, const AgentActorPairs& agentActorPairs, ProperNounActorMention_ptr defaultCountryActorMention, ActorMentionSet *actorMentionSet);

	void findLocalAcronymCompositeActorMentions(DocTheory* docTheory, ActorMentionSet *result, int sent_cutoff, const char *publicationDate);
	void resolveAmbiguousLocationsToDefaultCountry(ProperNounActorMention_ptr defaultCountryActorMention, const DocTheory* docTheory, ActorMentionSet *result, int sent_cutoff);
	ActorTokenMatcher_ptr findLocalProperNounAcronymDefinitions(const SortedActorMentions& sortedActorMentions);
	CompositeActorTokenMatcher_ptr findLocalCompositeAcronymDefinitions(const ActorMentionSet *actorMentions);
	std::wstring getJabariPatternFromAcronymDefinition(ActorMention_ptr actor);

	bool entityTypeMatches(ProperNounActorMention_ptr pnActorMention, EntityType entityType);

	// resolve locations in the final ActorMentionSet to specific database entries
	void resolveLocations(ActorMentionSet *actorMentionSet);

	// Helper functions, look for ProperNounActorMention ActorId
	bool actorAlreadyFound(ActorId aid, std::vector<ActorMention_ptr> &actorMentions);
	bool actorAlreadyFound(ActorId aid, const Mention *mention, SortedActorMentions sortedActorMentions);

	bool isGeoresolvableFAC(const Mention *mention);

	// used in sentence-level actor matching to store previous matches
	ActorMatchesBySentence _sentenceActorMatches; 

	Mode _mode;

	ActorInfo_ptr _actorInfo;

	/********************
	* doc-actors stage *
	********************/
	bool _do_doc_actors;

	/** Take a vector of ActorMentions and returl a new vector
	  * containing ActorMentions with unique actor ids. ActorMentions
	  * with the higher pattern_match_score are kept. */
	std::vector<ProperNounActorMention_ptr> getUniqueActorMentions(std::vector<ActorMention_ptr> &actorMentions);

	/** A helper function for getUniqueActorMentions. Looks in 
	  * actorMentions for an ActorMention with actorMention's 
	  * actor id. Returns ProperNounActorMention_ptr() if not found. */
	ProperNounActorMention_ptr findActor(std::vector<ProperNounActorMention_ptr> &actorMentions, ProperNounActorMention_ptr actorMention);

	std::wstring getGeonamesId(ActorEntity_ptr ae);

	/** Scores the georesolved ActorMentions found in the 
	  * actor-mentions stage */
	void docActorsLocationResolution(DocTheory *docTheory, const char *publicationDate);
	void docActorsResolveNamedLocations(DocTheory *docTheory, const char *publicationDate, resolution_ambiguity_t ambiguity);

	/** We might have added some unlikely ActorMentions to the 
	  * ActorMentionSets, so we don't want to use those in counts. 
	  * This returns just the reliable ones. */
	std::vector<ActorMention_ptr> getCurrentReliableActorMentions(DocTheory *docTheory);

	/** Returns true if there is an ActorMention with georesolution
	  * score > 0 or a patter match score > 0 */
	bool hasGoodResolution(std::vector<ActorMention_ptr> &actorMentions);

	/** Return true if there is an ActorMention with any georesolution */
	bool hasGeoResolution(std::vector<ActorMention_ptr> &actorMentions);

	bool cityNameMatches(Gazetteer::GeoResolution_ptr geo1, Gazetteer::GeoResolution_ptr geo2);
	
public:
	/** doc-actors stage processing */
	void doDocActors(DocTheory *docTheory);

	


};


#endif

