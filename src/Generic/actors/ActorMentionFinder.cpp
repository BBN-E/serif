// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/icews/SentenceSpan.h"
#include "Generic/icews/Stories.h"
#include "Generic/icews/ICEWSActorInfo.h"
#include "Generic/icews/ICEWSGazetteer.h"
#include "Generic/actors/Identifiers.h"
#include "Generic/actors/ActorMentionFinder.h"
#include "Generic/actors/AWAKEGazetteer.h"
#include "Generic/actors/AWAKEActorInfo.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/InputUtil.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/NationalityRecognizer.h"
#include "Generic/theories/ActorMentionSet.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/MentionConfidence.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/ActorEntitySet.h"
#include "Generic/theories/DocumentActorInfo.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/patterns/PatternSet.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/TopLevelPFeature.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/edt/CorefUtilities.h"
#include "Generic/edt/Guesser.h"
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <math.h>
#include <limits.h>
#include <iomanip>

namespace {
	// Special agent code used to indicate that a composite actor should be
	// treated as a direct mention of its paired actor:
	Symbol COMPOSITE_ACTOR_IS_PAIRED_ACTOR_SYM(L"COMPOSITE_ACTOR_IS_PAIRED_ACTOR");
	// These codes are from the dict_sectors table:
	Symbol MEDIA_SECTOR_CODE(L"MED");
	Symbol NEWS_SECTOR_CODE(L"133"); // should we also include "state media" here?
	// These names come from the sectors table of the BBN actor DB
	Symbol MEDIA_SECTOR_NAME(L"Media");
	Symbol NEWS_SECTOR_NAME(L"News");
	// These are entity subtypes of PER:
	Symbol INDIVIDUAL_SYM(L"Individual");
	Symbol UNDET_SYM(L"UNDET");
	// These are values for CompositeActorMention::agentActorPatternName:
	Symbol UNKNOWN_ACTOR_SYM(L"UNKNOWN-ACTOR");
	Symbol DEFAULT_COUNTRY_ACTOR_SYM(L"DEFAULT-COUNTRY-ACTOR");
	Symbol PERSON_IS_CITIZEN_OF_DEFAULT_CONTRY_SYM(L"PERSON-IS-CITIZEN-OF-DEFAULT-COUNTRY");
	Symbol PERSON_IS_CITIZEN_OF_UNKNOWN_ACTOR_SYM(L"PERSON-IS-CITIZEN-OF-UNKNOWN-ACTOR");
	Symbol PERSON_IS_CITIZEN_OF_COUNTRY_SYM(L"PER-COUNTRY-IS-CITIZEN-OF-COUNTRY");
	Symbol NESTED_ACTOR_PATTERN_MATCH_SYM(L"NESTED-ACTOR-PATTERN-MATCH");
	// Return value symbols for agent/actor patterns
	Symbol AGENT_SYM(L"AGENT");
	Symbol ACTOR_SYM(L"ACTOR");
	// Source note values:
	Symbol ACTOR_PATTERN_SYM(L"ACTOR_PATTERN");
	Symbol AGENT_PATTERN_SYM(L"AGENT_PATTERN");
	Symbol CITIZEN_OF_COUNTRY_SYM(L"CITIZEN_OF_COUNTRY:ACTOR_PATTERN");
	Symbol GAZETTEER_SYM(L"GAZETTEER");
	Symbol UNAMBIGUOUS_GAZETTEER_SYM(L"UNAMBIGUOUS_GAZETTEER");
	Symbol UNLABELED_PERSON_SYM(L"UNLABELED_PERSON");
	Symbol AGENT_OF_AGENT_PATTERN_SYM(L"AGENT-OF-AGENT:AGENT_PATTERN");
	Symbol COMPOSITE_ACTOR_PATTERN_SYM(L"COMPOSITE_ACTOR_PATTERN");
	Symbol COUNTRY_RESTRICTION_APPLIED_SYM(L"COUNTRY_RESTRICTION_APPLIED");
	Symbol TEMPORARY_ACTOR_MENTION_SYM(L"TEMPORARY_ACTOR_MENTION");
	Symbol LOCAL_COMPOSITE_ACRONYM_SYM(L"LOCAL_COMPOSITE_ACRONYM");
	Symbol LOCAL_PROPER_NOUN_ACRONYM_SYM(L"LOCAL_PROPER_NOUN_ACRONYM");
	Symbol PATTERN_END_IS_NOT_MENTION_END_SYM = Symbol(L"PATTERN_END_IS_NOT_MENTION_END");
	Symbol EDIT_DISTANCE = Symbol(L"EDIT_DISTANCE");
	Symbol TOKEN_SUBSET_TREE = Symbol(L"TOKEN_SUBSET_TREE");
	Symbol US_CITY_SYM(L"US_CITY");
	// Symbol used as pattern id if a pattern is not labeled.
	Symbol UNLABELED_PATTERN_SYM(L"unlabeled-pattern");
	// Return value symbol for blocked mentions
	Symbol BLOCK_SYM(L"BLOCK");
	Symbol PERIOD_SYM(L".");
	Symbol USA_ACTOR_CODE(L"USA");
	Symbol VATICAN_ACTOR_CODE(L"VAT");
	// Score for actors found by findUSCities().  If you want to override one
	// of these with a custom pattern, then you'll should use a high weight
	// (eg 20-30).
	float US_CITY_SCORE = 45;
	Symbol HIS(L"his");
	Symbol HE(L"he");
	Symbol HIM(L"him");
	Symbol SHE(L"she");
	Symbol HER(L"her");

	Symbol BLOCK_ACTOR_SYM = Symbol(L"BLOCK_ACTOR");

	Symbol NPP_SYM = Symbol(L"NPP");
	Symbol JJ_POS_SYM = Symbol(L"JJ");
	
	Symbol GEORGIA_SYM = Symbol(L"georgia");
	Symbol GEORGIAN_SYM = Symbol(L"georgian");

	// These are default thresholds used for now to toggle conservative-ness for actor matching for ICEWS only
	float ICEWS_ACTOR_MATCH_CONSERVATIVE_VALUE = 0.1F; // -- doesn't code low-confidence pronouns
	float ICEWS_ACTOR_MATCH_BALANCED_VALUE = 0.5F;
	float ICEWS_ACTOR_MATCH_AGGRESSIVE_VALUE = 0.9F; // -- is more aggressive about default countries

}

ActorMentionFinder::ActorMentionFinder(Mode mode):
	_verbosity(1), _log_sector_freqs(false), _disable_coref(false), 
	_do_doc_actors(false), _require_entity_type_match(false), 
	_only_match_names(false), _mode(mode), _discard_plural_actors(false),
	_discard_pronoun_actors(false), _discard_plural_pronoun_actors(false), 
	_block_default_country_if_another_country_in_same_sentence(false),
	_block_default_country_if_unknown_paired_actor_is_found(false),
	_allow_fac_org_matches(false),
	_icews_actor_match_aggressiveness(0.5F), _awakeAgentPatternSet(PatternSet_ptr())
{
	bool encrypted_patterns = ParamReader::isParamTrue("icews_encrypt_patterns");	
	bool use_awake_db_for_icews = ParamReader::isParamTrue("use_awake_db_for_icews");

	if (_mode == ICEWS) {
		// Could be either ICEWS or AWAKE, depending on parameter
		_actorInfo = ActorInfo::getAppropriateActorInfoForICEWS();

		if (use_awake_db_for_icews) {
			_actorTokenMatcher = boost::make_shared<ActorTokenMatcher>("bbn_actor", true, _actorInfo);
			_gazetteer = boost::make_shared<AWAKEGazetteer>();
			_require_entity_type_match = ParamReader::getRequiredTrueFalseParam("actor_match_require_exact_entity_type_match");
			_allow_fac_org_matches = ParamReader::getRequiredTrueFalseParam("allow_fac_org_matches"); // overrides actor_match_require_exact_entity_type_match params for FAC->ORG
			_only_match_names = ParamReader::getRequiredTrueFalseParam("actor_match_only_match_names");		
			if (ParamReader::hasParam("awake_agent_patterns")) {
				std::string awakeAgentPatternSetFilename = ParamReader::getRequiredParam("awake_agent_patterns");
				_awakeAgentPatternSet = boost::make_shared<PatternSet>(awakeAgentPatternSetFilename.c_str(), encrypted_patterns);			
			} else {
				_awakeAgentPatternSet = PatternSet_ptr();
			}
			_agentTokenMatcher = boost::make_shared<AgentTokenMatcher>("bbn_agent"); 
			_compositeActorTokenMatcher = CompositeActorTokenMatcher_ptr();
		} else {
			_actorTokenMatcher = boost::make_shared<ActorTokenMatcher>("actor");
			_gazetteer = boost::make_shared<ICEWSGazetteer>();
			_agentTokenMatcher = boost::make_shared<AgentTokenMatcher>("agent"); 
			_compositeActorTokenMatcher = boost::make_shared<CompositeActorTokenMatcher>("composite_actor"); 
		}
		_locationMentionResolver = boost::make_shared<LocationMentionResolver>(_gazetteer.get());
		_actorEditDistance = ActorEditDistance_ptr();
		_actorEntityScorer = ActorEntityScorer_ptr();
		_actorTokenSubsetTrees = ActorTokenSubsetTrees_ptr();
		_icews_actor_match_aggressiveness = static_cast<float>(ParamReader::getOptionalFloatParamWithDefaultValue("icews_actor_match_aggressiveness", 0.5F));

		std::string types_param = ParamReader::getParam("types_to_force_as_actor_mentions");
		if (types_param != "") {
			std::wstring types_wparam = UnicodeUtil::toUTF16StdString(types_param);
			std::vector<std::wstring> types_vec;
			boost::split(types_vec, types_wparam, boost::is_any_of(L","));
			BOOST_FOREACH(std::wstring t, types_vec) {
				Symbol sym(t);
				_typesForForcedActorMentionCreation.insert(sym);
			}
		}

		std::string agentPatternSetFilename = ParamReader::getRequiredParam("icews_agent_actor_patterns");
		_agentPatternSet = boost::make_shared<PatternSet>(agentPatternSetFilename.c_str(), encrypted_patterns);

		std::vector<std::wstring> temp = InputUtil::readFileIntoVector(ParamReader::getParam("actors_not_contributing_to_country_counts"), true, false);				
		BOOST_FOREACH(std::wstring actorStr, temp) {
			ActorId actor;
			if (use_awake_db_for_icews)
				actor = _actorInfo->getActorByName(actorStr);
			else
				actor = _actorInfo->getActorByCode(Symbol(actorStr));
			if (!actor.isNull())
				_actorsNotContributingToCountryCounts.push_back(actor);
		}

		std::string blockDefaultCountryPatternSetFilename = ParamReader::getParam("icews_block_default_country_patterns");
		if (!blockDefaultCountryPatternSetFilename.empty())
			_blockDefaultCountryPatternSet = boost::make_shared<PatternSet>(blockDefaultCountryPatternSetFilename.c_str(), encrypted_patterns);

		_discard_pronoun_actors = ParamReader::isParamTrue("icews_discard_pronoun_actors");
		_discard_plural_actors = ParamReader::isParamTrue("icews_discard_plural_actors");
		_discard_plural_pronoun_actors = ParamReader::isParamTrue("icews_discard_plural_pronoun_actors");
		_log_sector_freqs = ParamReader::isParamTrue("icews_log_sector_frequencies");

		_block_default_country_if_another_country_in_same_sentence =
			ParamReader::getOptionalTrueFalseParamWithDefaultVal("icews_block_default_country_if_another_country_in_same_sentence", true);
		_block_default_country_if_unknown_paired_actor_is_found =
			ParamReader::getOptionalTrueFalseParamWithDefaultVal("icews_block_default_country_if_unknown_paired_actor_is_found", true);

		_perAgentNameWords = InputUtil::readFileIntoSet(ParamReader::getParam("person_agent_name_words"), false, true);	
	}

	if (_mode == ACTOR_MATCH) {
		_actorInfo = AWAKEActorInfo::getAWAKEActorInfo();
		_actorTokenMatcher = boost::make_shared<ActorTokenMatcher>("bbn_actor", true, _actorInfo);
		_agentTokenMatcher = AgentTokenMatcher_ptr();
		_compositeActorTokenMatcher = CompositeActorTokenMatcher_ptr();
		_gazetteer = boost::make_shared<AWAKEGazetteer>();
		_locationMentionResolver = boost::make_shared<LocationMentionResolver>(_gazetteer.get());
		_actorEditDistance = boost::make_shared<ActorEditDistance>(_actorInfo);
		_actorEntityScorer = boost::make_shared<ActorEntityScorer>();
		_actorTokenSubsetTrees = boost::make_shared<ActorTokenSubsetTrees>(_actorInfo, _actorEntityScorer);

		_require_entity_type_match = ParamReader::getRequiredTrueFalseParam("actor_match_require_exact_entity_type_match");
		_allow_fac_org_matches = ParamReader::getRequiredTrueFalseParam("allow_fac_org_matches"); // overrides actor_match_require_exact_entity_type_match params for FAC->ORG
		_only_match_names = ParamReader::getRequiredTrueFalseParam("actor_match_only_match_names");
		_disable_coref = ParamReader::isParamTrue("icews_disable_coref");

		_agentPatternSet = PatternSet_ptr();
	}

	if (_mode == DOC_ACTORS) {
		_do_doc_actors = ParamReader::isParamTrue("do_actor_match");

		if (!_do_doc_actors) 
			return;

		_actorInfo = AWAKEActorInfo::getAWAKEActorInfo();
		_actorTokenMatcher = ActorTokenMatcher_ptr();
		_agentTokenMatcher = AgentTokenMatcher_ptr();
		_compositeActorTokenMatcher = CompositeActorTokenMatcher_ptr();
		_gazetteer = boost::make_shared<AWAKEGazetteer>();
		_locationMentionResolver = boost::make_shared<LocationMentionResolver>(_gazetteer.get());
		_actorEditDistance = ActorEditDistance_ptr();
		_actorEntityScorer = boost::make_shared<ActorEntityScorer>();
		_actorTokenSubsetTrees = ActorTokenSubsetTrees_ptr();

		_agentPatternSet = PatternSet_ptr();
	}

	// Get a list of "acceptable" country modifier words (eg western)
	// Required for now to remind me to update ICEWS parameter file
	_countryModifierWords = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("country_modifier_words"), false, true);
	_personModifierWords = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("person_modifier_words"), false, true);
	_organizationModifierWords = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("organization_modifier_words"), false, true);

	_verbosity = ParamReader::getOptionalIntParamWithDefaultValue("actor_event_verbosity", 1);
	#ifdef BLOCK_FULL_SERIF_OUTPUT
	_verbosity = 0;
	#endif
	_actor_event_sentence_cutoff = ParamReader::getOptionalIntParamWithDefaultValue("actor_event_sentence_cutoff", INT_MAX);

	_max_ambiguity_for_gazetteer_actors = ParamReader::getOptionalIntParamWithDefaultValue("max_ambiguity_for_gazetteer_actors", 3);
	_us_state_names = InputUtil::readFileIntoSymbolSet(ParamReader::getParam("us_state_names"), true, true);

	_encode_person_matching_country_as_citizen = 
			ParamReader::getOptionalTrueFalseParamWithDefaultVal("encode_person_matching_country_as_citizen", true);
	_georesolve_facs = ParamReader::isParamTrue("allow_georesolution_of_reliable_facs");

	// set ambiguity/verbosity parameters for location resolution
	if (_locationMentionResolver) {
		_locationMentionResolver->setMaxAmbiguity(_max_ambiguity_for_gazetteer_actors);
		_locationMentionResolver->setVerbosity(_verbosity);
	}

	if (mode != ICEWS || use_awake_db_for_icews){
		_usa_actorid = _actorInfo->getActorByName(L"United States");}
	else 
		_usa_actorid = _actorInfo->getActorByCode(USA_ACTOR_CODE);

	// Make sure that the guesser is initialized (we use this to determine whether
	// a mention is plural or not).
	Guesser::initialize();
}

ActorMentionFinder::~ActorMentionFinder() {
	typedef std::pair<size_t, Symbol> CountSymbolPair;
	// Display the actor-agent pattern match frequencies, sorted by frequency
	if (_agentPatternMatchCounts.size()>0) {
		std::vector<CountSymbolPair> pairs;
		for (Symbol::HashMap<size_t>::const_iterator it=_agentPatternMatchCounts.begin(); it != _agentPatternMatchCounts.end(); ++it)
			pairs.push_back(CountSymbolPair((*it).second, (*it).first));
		std::sort(pairs.begin(), pairs.end());
		std::ostringstream msg;
		/*msg << "Actor/Agent Pattern match frequencies:" << std::endl;
		BOOST_FOREACH(CountSymbolPair p, pairs)
			msg << std::setw(10) << p.first << "  " << p.second << "\n";
		SessionLogger::info("ICEWS") << msg.str();*/
	}
	// Display sector freqs, sorted
	if (_sectorFreqs.size()>0) {
		std::vector<CountSymbolPair> pairs;
		for (Symbol::HashMap<size_t>::const_iterator it=_sectorFreqs.begin(); it != _sectorFreqs.end(); ++it)
			pairs.push_back(CountSymbolPair((*it).second, (*it).first));
		std::sort(pairs.begin(), pairs.end());
		std::ostringstream msg;
		BOOST_FOREACH(CountSymbolPair p, pairs) 
			msg << std::setw(6) << p.first << "  " << std::setw(36) << p.second 
				<< " " << _actorInfo->getSectorName(p.second) << "\n";
		SessionLogger::info("ICEWS") << msg.str();
	}
}

void ActorMentionFinder::process(DocTheory *docTheory) {

	//
	// This is the ICEWS-specific processing function. It works over the document
	//   as a whole.
	//

	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "=== Finding ICEWS Actor Mentions in " 
			<< docTheory->getDocument()->getName() << " ===";

	// Sanity check on the input
	if (docTheory->getEntitySet() == 0) {
		throw UnexpectedInputException("ActorMentionFinder::process",
			"Document has not been run through doc-entities Serif stage");
	}

	if (_mode != ICEWS) {
		throw UnexpectedInputException("ActorMentionFinder::process",
			"This stage can only be run in ICEWS mode");
	}

	std::string publicationDate = Stories::getStoryPublicationDate(docTheory->getDocument());
	// SessionLogger TODO -- Liz check this out
	if (publicationDate.empty())
		SessionLogger::warn("ICEWS") << "No publication date found for \"" 
			<< docTheory->getDocument()->getName() << "\"";
	else if (_verbosity > 1)
		SessionLogger::info("ICEWS") << "  Publication date: \"" << publicationDate << "\"";
	const char* publicationDateCStr = (publicationDate.empty()?0:publicationDate.c_str());

	// Construct an actor mention set to store our results.
	ActorMentionSet* actorMentionSet = _new ActorMentionSet();
	_locationMentionResolver->clear();
	docTheory->takeActorMentionSet(actorMentionSet);

	int sent_cutoff = getSentCutoff(docTheory);

	// Use patterns to find proper noun actors
	ActorMatchesBySentence unusedActorMatches = findProperNounActorMentions(docTheory, actorMentionSet, sent_cutoff, publicationDateCStr);

	// Choose default country
	ProperNounActorMention_ptr defaultCountryActorMention = getDefaultCountryActorMention(actorMentionSet);

	// Find things that may block default country assignment.
	// These will block at the ENTITY level
	AgentActorPairs agentsOfUnknownActors = findAgentActorPairs(docTheory, actorMentionSet, PAIRED_UNKNOWN_ACTOR);
	PatternMatcher_ptr blockedMentionsPatternMatcher;
	if (_blockDefaultCountryPatternSet && defaultCountryActorMention) 
		blockedMentionsPatternMatcher = PatternMatcher::makePatternMatcher(docTheory, _blockDefaultCountryPatternSet);
	std::map<MentionUID, Symbol> blockedMentions = findMentionsThatBlockDefaultPairedActor(docTheory, blockedMentionsPatternMatcher);
	
	// Conservative coreference propagation; just proper noun actors at this point
	AgentActorPairs nullSet;
	labelCoreferentMentions(docTheory, 0, actorMentionSet, nullSet, sent_cutoff);

	// Use patterns to find (but not add) composite actors, e.g. "Palestinian activists" --> Activist FOR Palestine
	// Will also find ICEWS agents without a paired actor, e.g. "activists" --> Activist FOR Unknown	
	AgentActorPairs agentsOfProperNounActors = findAgentActorPairs(docTheory, actorMentionSet, PAIRED_PROPER_NOUN_ACTOR, &unusedActorMatches);

	// Now actually do the assignment for those composite actors
	findCompositeActorMentions(docTheory, actorMentionSet, sent_cutoff, agentsOfProperNounActors);

	// Add explicit location information, e.g. "the village in Pakistan"
	addExplicitLocations(docTheory, actorMentionSet, sent_cutoff, agentsOfProperNounActors);	

	// Identify local acronym definitions and apply them.
	findLocalAcronymCompositeActorMentions(docTheory, actorMentionSet, sent_cutoff, publicationDateCStr);

	// Do conservative coreference propagation again, this time transferring those known ICEWS agents, 
	//  along with their paired actors (known or otherwise)
	// Also transfer to pronouns at this point too
	labelCoreferentMentions(docTheory, 1, actorMentionSet, agentsOfProperNounActors, sent_cutoff);

	// Use patterns to find (but not add) agents paired with composite actors (known or unknown)
	AgentActorPairs agentsOfCompositeActors = findAgentActorPairs(docTheory, actorMentionSet, PAIRED_COMPOSITE_ACTOR);
	
	// Label any people/orgs that we haven't tagged so far, if they are paired with a known actor
	labelPeople(docTheory, actorMentionSet, sent_cutoff, agentsOfProperNounActors, agentsOfCompositeActors, false);
	//labelOrganizations(docTheory, actorMentionSet, sent_cutoff, agentsOfProperNounActors, agentsOfCompositeActors);

	// Do complete coreference propagation, unless we are being conservative
	if (_icews_actor_match_aggressiveness > ICEWS_ACTOR_MATCH_CONSERVATIVE_VALUE)
		labelCoreferentMentions(docTheory, 2, actorMentionSet, agentsOfProperNounActors, sent_cutoff);

	// Apply the default country mention as the default paired actor (so we can handle agents paired with composites correctly)
	if (defaultCountryActorMention)
		assignDefaultCountryForUnknownPairedActors(actorMentionSet, defaultCountryActorMention, agentsOfUnknownActors, agentsOfProperNounActors, docTheory, blockedMentions);

	// Actually add agents of composite actors.
	findCompositeActorMentions(docTheory, actorMentionSet, sent_cutoff, agentsOfCompositeActors);

	// Label any people that we haven't tagged so far, even if they are paired with an unknown actor
	labelPeople(docTheory, actorMentionSet, sent_cutoff, agentsOfProperNounActors, agentsOfCompositeActors, true);
	
	// Label partitives of any mentions we've created so far (again)
	labelPartitiveMentions(docTheory, actorMentionSet, sent_cutoff);

	// Apply the default country mention as the default paired actor (again)
	if (defaultCountryActorMention)
		assignDefaultCountryForUnknownPairedActors(actorMentionSet, defaultCountryActorMention, agentsOfUnknownActors, agentsOfProperNounActors, docTheory, blockedMentions);
	
	// Label any unknown loc/fac as the default country.
	if (defaultCountryActorMention)
		labelLocationsAndFacilities(docTheory, actorMentionSet, sent_cutoff, defaultCountryActorMention, agentsOfProperNounActors, agentsOfCompositeActors, blockedMentions);

	// Discard any "bare" actor mentions we created
	discardBareActorMentions(actorMentionSet);

	// Force ActorMention creation for all mentions of particular types
	forceActorMentionCreation(docTheory, actorMentionSet, sent_cutoff);


	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Found " << actorMentionSet->size() << " actor mentions in "
			<< docTheory->getDocument()->getName() << std::endl;

	// Optionally record sector freqs.
	if (_log_sector_freqs) {
		BOOST_FOREACH(ActorMention_ptr actorMention, actorMentionSet->getAll()) {
			if (ProperNounActorMention_ptr pnActorMention = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention)) {
				ActorId actorId = pnActorMention->getActorId();
				std::vector<Symbol> sectors = _actorInfo->getAssociatedSectorCodes(actorId);
				BOOST_FOREACH(const Symbol &sector, sectors)
					_sectorFreqs[sector] += 1;
			}
			if (CompositeActorMention_ptr cActorMention = boost::dynamic_pointer_cast<CompositeActorMention>(actorMention)) {
				AgentId agentId = cActorMention->getPairedAgentId();
				std::vector<Symbol> sectors = _actorInfo->getAssociatedSectorCodes(agentId);
				BOOST_FOREACH(const Symbol &sector, sectors) 
					_sectorFreqs[sector] += 1;
			}
		}
	}
}

ActorMentionFinder::AgentActorPairs ActorMentionFinder::findAgentActorPairs(DocTheory *docTheory, const ActorMentionSet *actorMentionSet, PairedActorKind actorKind, const ActorMatchesBySentence* unusedActorMatches) {
	if (_verbosity > 0) 
		SessionLogger::info("ICEWS") << "  Finding agent/actor pairs";
	AgentActorPairs agentActorPairs;
	PatternMatcher_ptr patternMatcher = PatternMatcher::makePatternMatcher(docTheory, _agentPatternSet);
	actorMentionSet->addEntityLabels(patternMatcher, _actorInfo);
	for (int sentno=0; sentno<docTheory->getNSentences(); ++sentno) {
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
		findPairedAgentActorMentions(patternMatcher, sentTheory, actorMentionSet, agentActorPairs, actorKind);
	}
	// "Unused actor matches" are places where the named actor patterns matched,
	// but we couldn't find any corresponding name/desc mention.  This often
	// occurs when patterns match adjectives (eg "British"), since we don't 
	// create mentions for these.  In this case, find the closest containing 
	// name/desc node, and propose this as a possible paired actor for that 
	// mention (if it doesn't already have a paired actor).
	if (unusedActorMatches) {
		for (size_t i=0; i<unusedActorMatches->size(); ++i) {
			SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(i));
			BOOST_FOREACH(const ActorMatch &match, unusedActorMatches->at(i)) {
				const SynNode *parseRoot = sentTheory->getPrimaryParse()->getRoot();
				const SynNode *node = parseRoot->getCoveringNodeFromTokenSpan(match.start_token, match.end_token);
				while (node && !node->hasMention())
					node = node->getParent();
				if (node) {
					const Mention *ment = sentTheory->getMentionSet()->getMentionByNode(node);
					if (ment->getMentionType() == Mention::NONE && ment->getParent() != 0)
						ment = ment->getParent();
					// Certainly don't do this for persons-- you'll get things like "Chad Murray" being marked as the country Chad
					// It seems like non-ORG types in general aren't a good idea
					// Thankfully metonyms like "U.S. Embassy" are tagged as ORGS when they show up as names
					if (!ment->getEntityType().matchesORG())
						continue;					
					if ((agentActorPairs.find(ment->getUID()) == agentActorPairs.end()) &&
						(ment->getMentionType() == Mention::NAME)) 
					{
						ActorMention_ptr actorMention = boost::make_shared<ProperNounActorMention>(
							sentTheory, ment, TEMPORARY_ACTOR_MENTION_SYM,
							ActorMention::ActorIdentifiers(match, _actorInfo->getActorName(match.id), _actorTokenMatcher->patternRequiresContext(match.patternId)));
						agentActorPairs[ment->getUID()] = PairedActorMention(actorMention, NESTED_ACTOR_PATTERN_MATCH_SYM, true);
					}
				}
			}
		}
	}
	return agentActorPairs;
}

namespace {
	Symbol LRB_SYM(L"-LRB-");
	Symbol RRB_SYM(L"-RRB-");
}
// For now, we only check for one-word acronyms, and they must be in all-caps, and at least two letters long.
ActorTokenMatcher_ptr ActorMentionFinder::findLocalProperNounAcronymDefinitions(const SortedActorMentions& sortedActorMentions) {
	ActorTokenMatcher_ptr result;
	BOOST_FOREACH(const ScoredActorMention& scoredActorMention, sortedActorMentions) {
		if (ProperNounActorMention_ptr actor = boost::dynamic_pointer_cast<ProperNounActorMention>(scoredActorMention.second)) {
			ActorId actorId = actor->getActorId();
			if (_actorInfo->isAnIndividual(actorId))
				continue; // Eg: "John Smith (D)" does not imply "D=John Smith"
			if (_actorInfo->isACountry(actorId))
				continue; // Eg: "Bank of Kenya (BoK)" does not imply "BoK=Kenya"
			std::wstring jabariPattern = getJabariPatternFromAcronymDefinition(actor);
			if (!jabariPattern.empty()) {
				if (!result) 
					result = boost::make_shared<ActorTokenMatcher>("actor", false);
				// weight of -0.01f means this won't win if we have an existing pattern for the acronym
				result->addPattern(jabariPattern, actor->getActorPatternId(),
					actor->getActorId(), actor->getActorCode(), -0.01f);
			}
		}
	}
	return result;
}

CompositeActorTokenMatcher_ptr ActorMentionFinder::findLocalCompositeAcronymDefinitions(const ActorMentionSet *actorMentions) {
	CompositeActorTokenMatcher_ptr result;

	BOOST_FOREACH(ActorMention_ptr actorMention, actorMentions->getAll()) {
		if (CompositeActorMention_ptr actor = boost::dynamic_pointer_cast<CompositeActorMention>(actorMention)) {
			AgentId agentId = actor->getPairedAgentId();
			ActorId actorId = actor->getPairedActorId();
			if (agentId.isNull() || actorId.isNull()) continue;
			std::wstring jabariPattern = getJabariPatternFromAcronymDefinition(actor);
			if (!jabariPattern.empty()) {
				std::wostringstream compositeCode;
				compositeCode << actor->getPairedAgentCode() << ":" << actor->getPairedAgentCode();
				Symbol compositeCodeSym(compositeCode.str().c_str());
				if (!result) 
					result = boost::make_shared<CompositeActorTokenMatcher>("composite_actor", false);
				result->addPattern(jabariPattern, actor->getPairedActorPatternId(),
					CompositeActorId(agentId, actorId), compositeCodeSym, 0);
			}
		}
	}
	return result;
}

std::wstring ActorMentionFinder::getJabariPatternFromAcronymDefinition(ActorMention_ptr actor) {
	static const boost::wregex acronymWordRegex(L"[A-Z]\\.?([A-Z]\\.?)+");
	const SynNode *actorNode = actor->getEntityMention()->getNode();
	const TokenSequence* tokSeq = actor->getSentence()->getTokenSequence();
	int end_tok = actorNode->getEndToken();
	int start_tok = actorNode->getStartToken();
	if (((end_tok-start_tok) > 3) && 
		(tokSeq->getToken(end_tok)->getSymbol() == RRB_SYM) &&
		(tokSeq->getToken(end_tok-2)->getSymbol() == LRB_SYM)) {
		// Check if the there's a subsumed mention that includes the parenthetical; if
		// so, then the acronym should be assigned to it, not us.
		const Mention *parentheticalMention = getCoveringNameDescMention(actor->getSentence(), end_tok-3, end_tok);
		if (parentheticalMention == actor->getEntityMention()) {
			Symbol acronym = tokSeq->getToken(end_tok-1)->getSymbol();
			std::wstring acronymString(acronym.to_string());
			if (boost::regex_match(acronym.to_string(), acronymWordRegex)) {
				if (_verbosity > 2)
					SessionLogger::info("ICEWS") << "    Adding local acronym definition:" 
						<< acronym << " => " << actor << "\n      based on the phrase \"" 
						<< actorNode->toCasedTextString(tokSeq) << "\"";
				return acronymString+L"=";
			}
		}
	}
	return L"";
}

void ActorMentionFinder::findLocalAcronymCompositeActorMentions(DocTheory* docTheory, ActorMentionSet *result, int sent_cutoff, const char *publicationDate) {
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Adding locally-defined composite acronym expansions";
	CompositeActorTokenMatcher_ptr acronymActorTokenMatcher = findLocalCompositeAcronymDefinitions(result);
	if (acronymActorTokenMatcher) {
		SortedActorMentions sortedActorMentions;
		CompositeActorMatchesBySentence acronymMatches = acronymActorTokenMatcher->findAllMatches(docTheory, sent_cutoff);
		for (size_t sentno=0; sentno<acronymMatches.size(); ++sentno) {
			SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
			BOOST_FOREACH(CompositeActorMatch match, acronymMatches[sentno]) {
				ScoredActorMention scoredActorMention = makePrecomposedCompositeActorMention(match, docTheory, sentTheory, LOCAL_COMPOSITE_ACRONYM_SYM, publicationDate);
				if (scoredActorMention.second)
					sortedActorMentions.insert(scoredActorMention);
			}
		}
		greedilyAddActorMentions(sortedActorMentions, result);
	}
}



ActorMentionFinder::ActorMatchesBySentence ActorMentionFinder::findProperNounActorMentions(DocTheory* docTheory, ActorMentionSet *result, 
																						   int sent_cutoff, const char *publicationDate) 
{
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Adding proper noun actor mentions";

	// Keep the unused matches -- they may be appropriate to use as paired actors.
	ActorMatchesBySentence unusedMatches;

	// Apply the Jabari actor patterns in _actorTokenMatcher to find all
	// possible pattern matches (including matches that are overlapping)
	ActorMatchesBySentence actorMatches = _actorTokenMatcher->findAllMatches(docTheory, sent_cutoff);
	fillTentativeCountryCounts(actorMatches);

	// Construct an ActorMention corresponding with each ActorMatch, and 
	// assign it a score.
	unusedMatches.resize(actorMatches.size());
	SortedActorMentions sortedActorMentions;
	for (size_t sentno=0; sentno<actorMatches.size(); ++sentno) {
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
		BOOST_FOREACH(ActorMatch match, actorMatches[sentno]) {
			ScoredActorMention scoredActorMention = makeProperNounActorMention(match, docTheory, sentTheory, actorMatches, ACTOR_PATTERN_SYM, publicationDate);
			if (scoredActorMention.second)
				sortedActorMentions.insert(scoredActorMention);
			else 
				unusedMatches.at(sentno).push_back(match);
		}
	}

	// Add any precomposed composite actors.
	if (_compositeActorTokenMatcher) {
		CompositeActorMatchesBySentence precomposedCompositeActorMatches = _compositeActorTokenMatcher->findAllMatches(docTheory, sent_cutoff);
		for (size_t sentno=0; sentno<actorMatches.size(); ++sentno) {
			SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
			BOOST_FOREACH(CompositeActorMatch match, precomposedCompositeActorMatches[sentno]) {
				ScoredActorMention scoredActorMention = makePrecomposedCompositeActorMention(match, docTheory, sentTheory, COMPOSITE_ACTOR_PATTERN_SYM, publicationDate, &actorMatches);
				if (scoredActorMention.second)
					sortedActorMentions.insert(scoredActorMention);
			}
		}
	}

	// Check if any new acronyms were defined.
	ActorTokenMatcher_ptr acronymActorTokenMatcher = findLocalProperNounAcronymDefinitions(sortedActorMentions);
	if (acronymActorTokenMatcher) {
		ActorMatchesBySentence acronymMatches = acronymActorTokenMatcher->findAllMatches(docTheory, sent_cutoff);
		for (size_t sentno=0; sentno<acronymMatches.size(); ++sentno) {
			SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
			BOOST_FOREACH(ActorMatch match, acronymMatches[sentno]) {
				ScoredActorMention scoredActorMention = makeProperNounActorMention(match, docTheory, sentTheory, actorMatches, LOCAL_PROPER_NOUN_ACRONYM_SYM, publicationDate);
				if (scoredActorMention.second)
					sortedActorMentions.insert(scoredActorMention);
			}
		}
	}
	
	//// Georesolution code: PHASE 1 -> conservative (allow_ambiguity = false)
	//// Typically these will score HIGHER than the actor patterns but they won't
	////   fire if an exact pattern match exists. This allows us to trump a bad
	////   match for 'West Bengal' (which should be India, not 'Bengal'/Bangladesh)
	////   but still keep the actor pattern for, e.g., 'Dubai'.

	// Clear the tentative document counts that we derived from pattern matches; we don't want anything messing with this 'unambiguous step'
	clearDocumentCountryCounts();

	SortedActorMentions conservativeLocationActorMentions;
	for (int sentno=0; sentno<docTheory->getNSentences(); ++sentno) {
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
		resolveNamedLocations(sentTheory, actorMatches[sentno], result, conservativeLocationActorMentions, publicationDate, UNAMBIGUOUS);
	}
	BOOST_FOREACH(ScoredActorMention sam, conservativeLocationActorMentions) {
		sortedActorMentions.insert(sam);
	}

	// Check for US-state locations such as "omaha, neb." in headlines.
	findUSCities(docTheory, sortedActorMentions, sent_cutoff);

	// This is a horrible thing, but we can't afford to have "Georgia" match here-- it's NOT unambiguous,
	//  but its possession of an actor pattern makes it seem that way. We'll add these back in later
	//  at the 'less conservative' stage.
	SortedActorMentions georgiaActorMentions;
	BOOST_FOREACH(ScoredActorMention sam, sortedActorMentions) {
		Symbol headword = sam.second->getEntityMention()->getNode()->getHeadWord();
		if (headword == GEORGIA_SYM || headword == GEORGIAN_SYM)
			georgiaActorMentions.insert(sam);
	}
	BOOST_FOREACH(ScoredActorMention sam, georgiaActorMentions) {
		sortedActorMentions.erase(sam);
	}

	greedilyAddActorMentions(sortedActorMentions, result);
	
	// OK, now re-fill up the "real" document counts and pass these into the resolver for help resolving ambiguity
	std::vector<ActorMention_ptr> temp = result->getAll();
	fillDocumentCountryCounts(temp);

	//// Georesolution code: PHASE 2 -> less conservative (allow_ambiguity = true)
	SortedActorMentions aggressiveLocationActorMentions;
	for (int sentno=0; sentno<docTheory->getNSentences(); ++sentno) {
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
		resolveNamedLocations(sentTheory, actorMatches[sentno], result, aggressiveLocationActorMentions, publicationDate, BEST_RESOLUTION_DESPITE_AMBIGUITY);
	}

	// Add Georgia back in...
	BOOST_FOREACH(ScoredActorMention sam, georgiaActorMentions) {
		aggressiveLocationActorMentions.insert(sam);
	}

	greedilyAddActorMentions(aggressiveLocationActorMentions, result);	

	// Now do a last pass on the document counts so they can continue to be used if necessary.
	std::vector<ActorMention_ptr> tempFinal = result->getAll();
	fillDocumentCountryCounts(tempFinal);

	return unusedMatches;
}

// This function will fill in the vector 'result' with sorted actor mentions with georesolutions
// It does NOT add things to the currentActorMentionSet; this needs to be done by the calling function
// However, in the case where we geo-resolve to something that already had a pattern-matched actor,
//   we do remove the pattern-matched actor from currentActorMentionSet (on the--hopefully solid--
//   assumption that the georesolved actor will get plucked from 'result' later to be added back in
//   as a superior choice)
void ActorMentionFinder::resolveNamedLocations(const SentenceTheory *sentTheory, 
											   std::vector<ActorMatch> patternActorMatches, 
											   ActorMentionSet *currentActorMentionSet, 
											   ActorMentionFinder::SortedActorMentions& result, 
											   const char *publicationDate, resolution_ambiguity_t ambiguity) 
{								
	
	if (_mode != ICEWS && _mode != ACTOR_MATCH) {
		throw UnexpectedInputException("ActorMentionFinder::resolveNamedLocations", "This function can only be run in ICEWS or ACTOR_MATCH mode.");
	}

	const MentionSet *ms = sentTheory->getMentionSet();
	for (int m = 0; m < ms->getNMentions(); m++) {
		const Mention *ment = ms->getMention(m);
		EntityType entityType = ment->getEntityType();
		if ((ment->getMentionType() == Mention::NAME) && 
			(entityType.matchesLOC() || entityType.matchesGPE() || (_georesolve_facs && isGeoresolvableFAC(ment))))
		{
			// See if there are any actor mentions already in currentActorMentionSet
			// For ICEWS, this will happen only if we have already run the conservative round, 
			//   added the results to the currentActorMentionSet and are now running
			//   the second more-aggressive round.
			// For AWAKE, this function only gets called once, but the pattern-matched actors have
			//   always already been added, so this will happen for locations that have patterns.

			std::vector<ActorMention_ptr> actorMentions = currentActorMentionSet->findAll(ment->getUID());
			Gazetteer::ScoredGeoResolution resolution;
			bool allow_ambiguity = true;
			if (ambiguity == UNAMBIGUOUS)
				allow_ambiguity = false;

			// Check all ActorMentions for this Mention; assign resolutions where possible
			std::vector<ProperNounActorMention_ptr> actorMentionsToRemove;
			BOOST_FOREACH(ActorMention_ptr target, actorMentions) {
				if (ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(target)) {

					// Get the countries associated with this actor, so we can resolve it appropriately
					std::vector<CountryId> associatedCountries;
					if (_actorInfo->isACountry(pnam->getActorId())) {
						associatedCountries.push_back(_actorInfo->getCountryId(pnam->getActorId()));
					} else {
						associatedCountries = _actorInfo->getAssociatedCountryIds(pnam->getActorId(), publicationDate);
					}

					// Get all candidate resolutions
					Gazetteer::SortedGeoResolutions sortedCandidateResolutions;
					_locationMentionResolver->getCandidateResolutions(sentTheory, ment, sortedCandidateResolutions, false, associatedCountries);

					// If we're not ready to allow ambiguity, then don't resolve this
					if (sortedCandidateResolutions.size() > 0 && !allow_ambiguity)
						continue;

					if (sortedCandidateResolutions.size() != 0) {
						// Just take the candidate with the highest score (they come pre-sorted)
						Gazetteer::ScoredGeoResolution resolution = *(sortedCandidateResolutions.rbegin());
						pnam->setGeoResolution(resolution.second);
						// Check whether we resolved against a geonameid that has an equivalent actor. If so, discard the pattern matched
						// actor and use the geoname's actor instead, which has more information. See comment above on 
						// modifications to currentActorMentionSet-- we remove the incomplete one from currentActorMentionSet
						// but do not add the new one; it merely gets returned as a candidate to be added later
						ActorId geonameActorId = _actorInfo->getActorIdForGeonameId(resolution.second->geonameid);
						if (!geonameActorId.isNull()) {
							ScoredActorMention scoredLocationActorMention = makeProperNounActorMentionFromGazetteer(ment, resolution, sentTheory, GAZETTEER_SYM);
							if (scoredLocationActorMention.second) {
								actorMentionsToRemove.push_back(pnam);
								result.insert(scoredLocationActorMention);
							}
						}
					}
				} 
			}

			// This can't be removed in the loop above, because it messes with what's being iterated over
			BOOST_FOREACH(ProperNounActorMention_ptr am, actorMentionsToRemove) {
				currentActorMentionSet->discardActorMention(am);
			}

			if (_mode == ACTOR_MATCH) {

				// This is AWAKE sentence-level mode; we just want to get all possible resolutions
				Gazetteer::SortedGeoResolutions sortedCandidateResolutions;
				_locationMentionResolver->getCandidateResolutions(sentTheory, ment, sortedCandidateResolutions, false);

				// Add a new actor mention for each resolution that gives us a new actor ID
				//  (sometimes we will have already gotten the right actor via an actor pattern)
				BOOST_FOREACH(Gazetteer::ScoredGeoResolution sgr, sortedCandidateResolutions) {
					ScoredActorMention scoredLocationActorMention = makeProperNounActorMentionFromGazetteer(ment, sgr, sentTheory, GAZETTEER_SYM);
					if (scoredLocationActorMention.second) {
						ProperNounActorMention_ptr newPnam = boost::dynamic_pointer_cast<ProperNounActorMention>(scoredLocationActorMention.second);
						// Look both in the result (so far) as well as the existing actorMentions for this actor
						if (!actorAlreadyFound(newPnam->getActorId(), ment, result) && !actorAlreadyFound(newPnam->getActorId(), actorMentions))
							result.insert(scoredLocationActorMention);
					}
				}

			} else if (_mode == ICEWS) {

				// This is ICEWS mode, so we use the ICEWS resolver
				// If the mode is UNAMBIGUOUS, it simply won't return an answer if it can't find an unambiguous one
				Gazetteer::ScoredGeoResolution resolution = _locationMentionResolver->getICEWSLocationResolution(currentActorMentionSet, 
					patternActorMatches, _countryCounts, _usa_actorid, sentTheory, ment, allow_ambiguity, std::vector<CountryId>(), _actorInfo);
				if (resolution.first != 0) {
					Symbol actorNoteSym = GAZETTEER_SYM;
					if (!allow_ambiguity)
						actorNoteSym = UNAMBIGUOUS_GAZETTEER_SYM;
					ScoredActorMention scoredLocationActorMention = makeProperNounActorMentionFromGazetteer(ment, resolution, sentTheory, actorNoteSym);
					if (scoredLocationActorMention.second) {
						result.insert(scoredLocationActorMention);
					}
				}
			}  
		}

		// Special case -- check per names without matches to see if we can 
		// match to country. This is how we get Iraqi -> Citizen FROM Iraq
		// when we don't have actor strings for country in the AWAKE DB 
		if (_mode == ICEWS && entityType.matchesPER() &&
			(ment->getMentionType() == Mention::NAME || ment->getMentionType() == Mention::DESC) &&
			NationalityRecognizer::isNationalityWord(ment->getNode()->getHeadWord())) 
		{
			std::vector<ActorMention_ptr> actorMentions = currentActorMentionSet->findAll(ment->getUID());
			if (actorMentions.size() > 0) {
				continue;
			}

			Gazetteer::ScoredGeoResolution resolution =_locationMentionResolver->getUnambiguousCountryResolution(sentTheory, ment, _actorInfo);
			if (resolution.first != 0) {
				ActorId geonameActorId = _actorInfo->getActorIdForGeonameId(resolution.second->geonameid);
				if (!geonameActorId.isNull()) {
					ActorMention_ptr actorMention = boost::make_shared<CompositeActorMention>(
						sentTheory, 
						ment, 
						CITIZEN_OF_COUNTRY_SYM,
						ActorMention::AgentIdentifiers(_actorInfo->getDefaultPersonAgentId(), _actorInfo->getAgentName(_actorInfo->getDefaultPersonAgentId()), _actorInfo->getDefaultPersonAgentCode(), AgentPatternId()),
						ActorMention::ActorIdentifiers(geonameActorId, _actorInfo->getActorName(geonameActorId)), 
						PERSON_IS_CITIZEN_OF_COUNTRY_SYM);
					result.insert(ScoredActorMention(resolution.first, actorMention));
				} 
			}
		}
	}
}

void ActorMentionFinder::addActorMention(ActorMention_ptr actorMention, ActorMentionSet *result) {
	const Mention *mention = actorMention->getEntityMention();
	// If pronoun coref is disabled, then never copy an actor mention to a pronoun.
	if (_discard_pronoun_actors && actorMention->getEntityMention()->getMentionType() == Mention::PRON) {
		if (_verbosity > 4)
			SessionLogger::info("ICEWS") << "Discarding actor mention because it is a pronoun: "
				<< actorMention << " for \"" 
				<< actorMention->getEntityMention()->toCasedTextString() << "\"";
	}
	if (Guesser::guessNumber(mention->getAtomicHead(), mention) == Guesser::PLURAL) {
		if (_discard_plural_actors) {
			if (_verbosity > 4)
				SessionLogger::info("ICEWS") << "Discarding actor mention because it is plural: "
					<< actorMention << " for \"" 
					<< actorMention->getEntityMention()->toCasedTextString() << "\"";
			return;
		} else if (_discard_plural_pronoun_actors && actorMention->getEntityMention()->getMentionType() == Mention::PRON) {
			if (_verbosity > 4)
				SessionLogger::info("ICEWS") << "Discarding actor mention because it is a plural pronoun: "
					<< actorMention << " for \"" 
					<< actorMention->getEntityMention()->toCasedTextString() << "\"";
			return;
		} else {
			actorMention->addSourceNote(L"PLURAL");
			if (_verbosity > 5)
				SessionLogger::info("ICEWS") << "Marking actor mention as plural: "
					<< actorMention << " for \"" 
					<< actorMention->getEntityMention()->toCasedTextString() << "\"";
		}
	}
	result->addActorMention(actorMention);
	if (CompositeActorMention_ptr c = boost::dynamic_pointer_cast<CompositeActorMention>(actorMention))
		++_agentPatternMatchCounts[c->getAgentActorPatternName()];
}

void ActorMentionFinder::greedilyAddActorMentions(const ActorMentionFinder::SortedActorMentions& sortedActorMentions, ActorMentionSet *result) {
	// Greedily assign ActorMentions to entity Mentions.
	for (SortedActorMentions::const_reverse_iterator it=sortedActorMentions.rbegin(); it!=sortedActorMentions.rend(); ++it) {
		double score = (*it).first;
		const ActorMention_ptr& actorMention = (*it).second;
		if (score > 0) {
			ActorMention_ptr target = result->find(actorMention->getEntityMention()->getUID());
			bool replace_old_value = (target && isCompatibleAndBetter(target, actorMention));
			logActorMention(actorMention, (*it).first, target, replace_old_value);
			if ((!target) || replace_old_value) {
				addActorMention(actorMention, result);
			}
		} else if (_verbosity > 3) {
			SessionLogger::info("ICEWS") << "    Discarding match "
				<< actorMention << " for \"" 
				<< actorMention->getEntityMention()->toCasedTextString() 
				<< "\" (sent " << actorMention->getSentence()->getSentNumber()
				<< ") because its score (" << score << ") was negative";
		}
	}
}

ProperNounActorMention_ptr ActorMentionFinder::defaultCountryAssignmentIsBlockedByOtherCountry(ActorMention_ptr actor, ProperNounActorMention_ptr pairedActorMention, ActorMentionSet *actorMentionSet) {
	const MentionSet* mentionSet = actor->getSentence()->getMentionSet();
	for (int i=0; i<mentionSet->getNMentions(); i++) {
		const MentionUID m = mentionSet->getMention(i)->getUID();
		if (ProperNounActorMention_ptr a = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMentionSet->find(m))) {
			if ((_actorInfo->isACountry(a->getActorId())) && (a->getActorId() != pairedActorMention->getActorId())) {
				return a;
			}
		}
	}
	return ProperNounActorMention_ptr();
}

std::map<MentionUID, Symbol> ActorMentionFinder::findMentionsThatBlockDefaultPairedActor(const DocTheory *docTheory,
																						 PatternMatcher_ptr patternMatcher) {
	std::map<MentionUID, Symbol> result;
	if (!patternMatcher) return result;
	for (int sentno=0; sentno<docTheory->getNSentences(); ++sentno) {
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
		std::vector<PatternFeatureSet_ptr> matches = patternMatcher->getSentenceSnippets(sentTheory, 0, true);
		BOOST_FOREACH(const PatternFeatureSet_ptr &match, matches) {
			Symbol patternLabel = UNLABELED_PATTERN_SYM;

			for (size_t f = 0; f < match->getNFeatures(); f++) {
				if (TopLevelPFeature_ptr tsf = boost::dynamic_pointer_cast<TopLevelPFeature>(match->getFeature(f)))
					patternLabel = tsf->getPatternLabel();
			}
			for (size_t f = 0; f < match->getNFeatures(); f++) {
				if (MentionReturnPFeature_ptr mrpf = boost::dynamic_pointer_cast<MentionReturnPFeature>(match->getFeature(f))) {
					if (mrpf->getReturnLabel() == BLOCK_SYM) {
						result[mrpf->getMention()->getUID()] = patternLabel;
					} else {
						throw UnexpectedInputException("ActorMentionFinder::findMentionsThatBlockDefaultPairedActor",
							"Unexpected return label in file referred to by parameter 'icews_block_default_country_patterns'", 
							mrpf->getReturnLabel().to_debug_string());
					}
				}
			}
		}
	}

	// If one mention in an entity is blocked, so should they all be
	// So, "the foreign doctor" blocks default country assignment for itself as well as 
	//   the "who" and "the doctor" to which it co-refers
	typedef std::pair<MentionUID, Symbol> block_pair_t;
	BOOST_FOREACH(block_pair_t blocked, result) {
		Entity *ent = docTheory->getEntitySet()->getEntityByMention(blocked.first);
		if (ent != 0) {
			for (int m=0; m<ent->getNMentions(); ++m) {
				Mention *ment2 = docTheory->getEntitySet()->getMention(ent->getMention(m));
				if (result.find(ment2->getUID()) == result.end())
					result[ment2->getUID()] = blocked.second;
			}
		}
	}
	return result;
}


void ActorMentionFinder::assignDefaultCountryForUnknownPairedActors(ActorMentionSet *actorMentionSet, ProperNounActorMention_ptr pairedActorMention, 
																	const AgentActorPairs &agentsOfUnknownActors, AgentActorPairs& agentsOfProperNounActors,
																    const DocTheory *docTheory, std::map<MentionUID, Symbol>& blockedMentions) 
{	
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Replacing unknown paired actors with default country: " << ActorMention_ptr(pairedActorMention);

	ActorId defaultCountryID;
	if (ProperNounActorMention_ptr pn_actor = boost::dynamic_pointer_cast<ProperNounActorMention>(pairedActorMention)) {
		defaultCountryID = pn_actor->getActorId();
	} else {
		return;
	}

	BOOST_FOREACH(ActorMention_ptr actor, actorMentionSet->getAll()) {
		if (CompositeActorMention_ptr compositeActor = boost::dynamic_pointer_cast<CompositeActorMention>(actor)) {
			if (compositeActor->getPairedActorId().isNull()) {
				// Check if any pattern blocks this mention from being assigned a default country.
				if (blockedMentions.find(actor->getEntityMention()->getUID()) != blockedMentions.end()) {
					if (_verbosity > 3)
						SessionLogger::info("ICEWS") << "    * Default country blocked: \"" 
							<< compositeActor->getEntityMention()->toCasedTextString() << "\" "
							<< ActorMention_ptr(compositeActor) << "\n        Blocked by pattern: \""
							<< blockedMentions[actor->getEntityMention()->getUID()] << "\"";
					continue;
				}
				// Check if another country is explicitly mentioned in this sentence.
				ProperNounActorMention_ptr otherCountry = defaultCountryAssignmentIsBlockedByOtherCountry(actor, pairedActorMention, actorMentionSet);
				if (otherCountry) {
					if (_block_default_country_if_another_country_in_same_sentence) {
						if (_verbosity > 3)
							SessionLogger::info("ICEWS") << "    * Default country blocked: \"" 
								<< compositeActor->getEntityMention()->toCasedTextString() << "\" "
								<< ActorMention_ptr(compositeActor) << "\n        Blocked by country: \""
								<< otherCountry->getEntityMention()->toCasedTextString() << "\" "
								<< ActorMention_ptr(otherCountry);
						continue;
					} else {
						compositeActor->addSourceNote(L"NON_DEFAULT_COUNTRY_IN_SENTENCE");
					}
				}
				// Check if this composite actor has an explicit (but unknown) paired actor.
				AgentActorPairs::const_iterator pairedActor = agentsOfUnknownActors.find(compositeActor->getEntityMention()->getUID());
				if (pairedActor != agentsOfUnknownActors.end()) {
					if (_block_default_country_if_unknown_paired_actor_is_found) {
						const Mention *pairedMention = (*pairedActor).second.actorMention->getEntityMention();
						if (_verbosity > 3)
							SessionLogger::info("ICEWS") << "    * Default country blocked: \""
								<< compositeActor->getEntityMention()->toCasedTextString() << "\" "
								<< ActorMention_ptr(compositeActor)
								<< "\n        Blocked by paired actor:\"" << pairedMention->toCasedTextString() << "\"";
						continue;
					} else {
						compositeActor->addSourceNote(L"HAS_UNKNOWN_ACTOR");
					}
				}
				
				// Check if this composite actor has an explicit (known) paired actor that is also a location/GPE.
				pairedActor = agentsOfProperNounActors.find(compositeActor->getEntityMention()->getUID());
				if (pairedActor != agentsOfProperNounActors.end()) {
					ActorMention_ptr explicitActor = (*pairedActor).second.actorMention;
					if (ProperNounActorMention_ptr pn_actor = boost::dynamic_pointer_cast<ProperNounActorMention>(explicitActor)) {
						if (pn_actor->getActorId() != defaultCountryID) {
							const Mention *pairedMention = (*pairedActor).second.actorMention->getEntityMention();
							if (pairedMention->getEntityType().matchesGPE() || pairedMention->getEntityType().matchesLOC()) {
								if (_verbosity > 3)
									SessionLogger::info("ICEWS") << "    * Default country blocked: \""
									<< compositeActor->getEntityMention()->toCasedTextString() << "\" "
									<< ActorMention_ptr(compositeActor)
									<< "\n        Blocked by paired explicit location actor:\"" << pn_actor->getEntityMention()->toCasedTextString() << "\"";
								continue;
							}
						}
					}
				}

				// Check to see whether this mention is co-referent with something that is paired
				//  with something other than the default country
				if (sisterMentionHasClashingActor(compositeActor->getEntityMention(), pairedActorMention, docTheory, actorMentionSet, static_cast<int>(_verbosity))) {
					// Message printed in sub-function
					continue;
				}
				// Assign a default country to this composite actor.
				compositeActor->setPairedActorMention(pairedActorMention, L"DEFAULT-COUNTRY:");
				if (_verbosity > 3)
					SessionLogger::info("ICEWS") << "    * Default Country Assigned: \""
						<< compositeActor->getEntityMention()->toCasedTextString() << "\" "
						<< ActorMention_ptr(compositeActor);
			}
		}
	}
}

ProperNounActorMention_ptr ActorMentionFinder::getDefaultCountryActorMention(const ActorMentionSet *actorsFoundSoFar) {
	std::vector<ActorMention_ptr> actorMentions = actorsFoundSoFar->getAll();
	return getDefaultCountryActorMention(actorMentions);
}

void ActorMentionFinder::fillTentativeCountryCounts(const ActorMatchesBySentence& actorMatches) {

	clearDocumentCountryCounts();
	BOOST_FOREACH(const std::vector<ActorMatch>& matchesForSentence, actorMatches) {
		BOOST_FOREACH(const ActorMatch &match, matchesForSentence) {
			ActorId actorId = match.id;
			if (_actorInfo->isACountry(actorId)) {
				_countryCounts[actorId] += 1.0f;
			} else {
				BOOST_FOREACH(ActorId country, _actorInfo->getAssociatedCountryActorIds(actorId)) {
					_countryCounts[country] += 0.1f;
				}
			}
		}
	}
}
	

void ActorMentionFinder::fillDocumentCountryCounts(std::vector<ActorMention_ptr> &actorMentions) 
{	
	clearDocumentCountryCounts();
	// Check what countries are mentioned in this document so far.
	BOOST_FOREACH(ActorMention_ptr actor, actorMentions) {
		if (ProperNounActorMention_ptr pn_actor = boost::dynamic_pointer_cast<ProperNounActorMention>(actor)) {
			ActorId actorId = pn_actor->getActorId();

			// Get rid of things like "AP" which shouldn't really weight the decision to the United States
			if (std::find(_actorsNotContributingToCountryCounts.begin(), _actorsNotContributingToCountryCounts.end(), actorId) != _actorsNotContributingToCountryCounts.end())
				continue;
			const Mention* mention = actor->getEntityMention();			
			int sentno = mention->getSentenceNumber();

			CountryId country_id;
			ActorId country_actor_id;
			if (_actorInfo->isACountry(actorId)) {
				country_id = _actorInfo->getCountryId(actorId);
				country_actor_id = actorId;
				if (!_countryActors[actorId])
					_countryActors[actorId] = pn_actor;
				else {
					// This eliminates SOME of the non-meaningful but still annoying differences cross-platforms
					// Prefers proper nouns with an actor pattern match
					ProperNounActorMention_ptr existing = _countryActors[actorId];
					if (existing->getActorPatternId() == ActorPatternId())
						_countryActors[actorId] = pn_actor;
					else if (pn_actor->getActorPatternId() != ActorPatternId() && existing->getActorPatternId() < pn_actor->getActorPatternId())
						_countryActors[actorId] = pn_actor;
				}
			} else if (pn_actor->getGeoResolution() && pn_actor->getGeoResolution()->countryInfo) {
				country_id = pn_actor->getGeoResolution()->countryInfo->countryId;
				country_actor_id = pn_actor->getGeoResolution()->countryInfo->actorId;
				if (!_cityActors[country_actor_id])
					_cityActors[country_actor_id] = pn_actor;
			}

			if (!country_id.isNull()) {
				// Country names that occurs as the very first mention in the first sentence are
				// usually part of a headline, and serve as strong candidates for the default 
				// country, so give them an extra score boost.
				if ((sentno == 0) && (mention->getNode()->getStartToken()==0)) {
					_countryCounts[country_actor_id] += 3;
					if (_verbosity > 3)
						SessionLogger::info("ICEWS") << "    [  +3] for <" << _actorInfo->getActorName(country_actor_id) 
													 << ">: Occurs at the very beginning of the document (\"" << _actorInfo->getActorName(actorId) << "\")";
				} else {
					_countryCounts[country_actor_id] += 1;
					if (_verbosity > 3)
						SessionLogger::info("ICEWS") << "    [  +1] for <" << _actorInfo->getActorName(country_actor_id) 
							<< ">: Mentioned explicitly in sentence " << sentno <<  " (\"" << _actorInfo->getActorName(actorId) << "\")";
				}				
			} else {
				BOOST_FOREACH(ActorId country, _actorInfo->getAssociatedCountryActorIds(actorId)) {
					if (_verbosity > 3)
						SessionLogger::info("ICEWS") << "    [+0.1] for <" << _actorInfo->getActorName(country) 
							<< ">: Related actor <" << _actorInfo->getActorName(actorId) 
							<< "> was mentioned in sentence " << sentno;
					_countryCounts[country] += 0.1f;
				}
			}
		}
	}
}


ProperNounActorMention_ptr ActorMentionFinder::getDefaultCountryActorMention(std::vector<ActorMention_ptr> &actorMentions) {
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Selecting default country";
	ProperNounActorMention_ptr result;

	fillDocumentCountryCounts(actorMentions);

	// Get the most frequent & second-most frequent country mentions.
	typedef std::pair<ActorId, float> ActorIdCount;
	ActorIdCount mostFreq1(ActorId(), -1);
	ActorIdCount mostFreq2(ActorId(), -1);
	for (ActorId::HashMap<float>::const_iterator it=_countryCounts.begin(); it != _countryCounts.end(); ++it) {
		if ((*it).second > mostFreq1.second) {
			mostFreq2 = mostFreq1; 
			mostFreq1 = (*it);
		} else if ((*it).second > mostFreq2.second) {
			mostFreq2 = (*it);
		}
	}

	// If the most frequently mentioned country is (a) mentioned at least
	// once, (b) mentioned at least twice as often as the second most
	// frequently mentioned country, and (c) is mentioned explicitly at 
	// least once, then that's good enough to use it as
	// a default country actor.
	if ((mostFreq1.second >= 1) && (mostFreq1.second >= (mostFreq2.second*2))) {
		result = _countryActors[mostFreq1.first]; // may be NULL.
		// If there's no direct mention of the country we selected, then
		// try creating a temporary actor mention based on some mention
		// of a city inside the country.
		if ((!result) && (_cityActors[mostFreq1.first])) {
			ProperNounActorMention_ptr cityActor = _cityActors[mostFreq1.first];
			result = boost::make_shared<ProperNounActorMention>
				(cityActor->getSentence(), cityActor->getEntityMention(), TEMPORARY_ACTOR_MENTION_SYM, 
				 ActorMention::ActorIdentifiers(mostFreq1.first, _actorInfo->getActorName(mostFreq1.first), 
											   _actorInfo->getActorCode(mostFreq1.first), ActorPatternId()));
		}

	}

	// Don't require the double-threshold if we are in aggressive-mode
	if (_icews_actor_match_aggressiveness > ICEWS_ACTOR_MATCH_AGGRESSIVE_VALUE) {
		if (!result) {
			if (mostFreq1.second >= 1 && mostFreq1.second >= mostFreq2.second) {
				result = _countryActors[mostFreq1.first];
			}
		}
	}
	
	// Log message.
	if (_verbosity > 1) {
		std::ostringstream logmsg;
		if (result)
			logmsg << "    Default country actor: \"" << _actorInfo->getActorName(mostFreq1.first) << "\".";
		else
			logmsg << "    No default country actor.";
		if (mostFreq1.second > 0) {
			logmsg << "\n      The highest scoring country actor in this document <"
				<< _actorInfo->getActorName(mostFreq1.first)
				<< "> (" << mostFreq1.first.getId() << ") has a score of "
				<< mostFreq1.second << ".";
		}
		if (mostFreq2.second > 0) {
			logmsg << "\n      The second highest scoring country actor <"
				<< _actorInfo->getActorName(mostFreq2.first)
				<< "> (" << mostFreq2.first.getId() << ") has a score of "
				<< mostFreq2.second << ".";
		}
		SessionLogger::info("ICEWS") << logmsg.str();
	}

	return result;
}

void ActorMentionFinder::findCompositeActorMentions(DocTheory* docTheory, ActorMentionSet *result, int sent_cutoff, const AgentActorPairs &agentActorPairs) {
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Adding composite actor mentions";

	AgentMatchesBySentence tokenMatcherAgentMatches;
	if (_agentTokenMatcher) {

		// Apply the Jabari actor patterns in _agentTokenMatcher to find all
		// possible pattern matches (including matches that are overlapping)
		tokenMatcherAgentMatches = _agentTokenMatcher->findAllMatches(docTheory, sent_cutoff);

	} 
	
	AgentMatchesBySentence patternAgentMatches;
	if (_awakeAgentPatternSet) {

		PatternMatcher_ptr patternMatcher = PatternMatcher::makePatternMatcher(docTheory, _awakeAgentPatternSet);
		int num_sents = std::min(docTheory->getNSentences(), sent_cutoff);
		for (int sentno=0; sentno<num_sents; ++sentno) {
			SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
			std::vector<PatternFeatureSet_ptr> matches = patternMatcher->getSentenceSnippets(sentTheory, 0, true);
			std::vector<AgentMatch> localAgentMatches;
			BOOST_FOREACH(PatternFeatureSet_ptr patternMatch, matches) {
				Symbol agentName;
				const Mention *agentMention = 0;
				Symbol patternLabel;
				for (size_t f = 0; f < patternMatch->getNFeatures(); f++) {
					PatternFeature_ptr sf = patternMatch->getFeature(f);
					if (TopLevelPFeature_ptr tsf = boost::dynamic_pointer_cast<TopLevelPFeature>(sf)) {
						patternLabel = tsf->getPatternLabel();
					} else if (MentionReturnPFeature_ptr mrpf = boost::dynamic_pointer_cast<MentionReturnPFeature>(sf)) {
						agentName = mrpf->getReturnLabel();
						agentMention = mrpf->getMention();
					}
				}
				if (patternLabel.is_null()) {
					SessionLogger::warn("ICEWS") << "AWAKE ICEWS agent pattern has no label";
				} else if (!agentMention) {
					SessionLogger::warn("ICEWS") << "AWAKE ICEWS agent pattern " << patternLabel << "has no agent";
				} else {
					int st = agentMention->getNode()->getStartToken();
					int et = agentMention->getNode()->getEndToken();	
					AgentId agentId = _actorInfo->getAgentByName(agentName);
					if (agentId.isNull()) {
						SessionLogger::warn("ICEWS") << "AWAKE ICEWS agent name from pattern (" << agentName << ") not in dictionary";
						continue;
					}						
					AgentMatch match = AgentMatch(agentId, AgentPatternId(), agentName, st, et, et-st, patternMatch->getScore(), false);
					localAgentMatches.push_back(match);
				}
			}
			patternAgentMatches.push_back(localAgentMatches);
		}

	} 

	// For each AgentMatch, try to construct a composite ActorMention 
	// consisting of the matched agent and a paired actor; and assign
	// a score to that ActorMention.
	SortedActorMentions sortedActorMentions;

	// Token matcher matches
	for (size_t sentno=0; sentno < tokenMatcherAgentMatches.size(); ++sentno) {
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
		BOOST_FOREACH(AgentMatch match, tokenMatcherAgentMatches[sentno]) {
			ScoredActorMention scoredActorMention = makeCompositeActorMention(match, docTheory, sentTheory, 
				agentActorPairs, result);
			if (scoredActorMention.second) {
				sortedActorMentions.insert(scoredActorMention);		
			}
		}
	}

	// Pattern matches
	for (size_t sentno=0; sentno < patternAgentMatches.size(); ++sentno) {
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
		BOOST_FOREACH(AgentMatch match, patternAgentMatches[sentno]) {
			ScoredActorMention scoredActorMention = makeCompositeActorMention(match, docTheory, sentTheory, 
				agentActorPairs, result);
			if (scoredActorMention.second) {
				sortedActorMentions.insert(scoredActorMention);		
			}
		}
	}

	// Greedily assign ActorMentions to entity Mentions.
	greedilyAddActorMentions(sortedActorMentions, result);
}

void ActorMentionFinder::addExplicitLocations(DocTheory* docTheory, ActorMentionSet *result, int sent_cutoff, const AgentActorPairs &agentsOfProperNounActors) {
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Adding explicit location information";

	for (int sent_no=0; sent_no<docTheory->getNSentences(); ++sent_no) {
		const SentenceTheory *sentTheory = docTheory->getSentenceTheory(sent_no);
		const MentionSet *mentionSet = sentTheory->getMentionSet();
		if (sent_no > sent_cutoff)
			break;
		for (int i=0; i<mentionSet->getNMentions(); ++i) {
			const Mention *m = mentionSet->getMention(i);

			// Location-type places only
			if (!m->getEntityType().matchesGPE() && !m->getEntityType().matchesLOC() && !m->getEntityType().matchesFAC())
				continue;

			MentionUID uid = m->getUID();
			if (!result->find(uid)) {
				ActorMention_ptr actor;
				AgentActorPairs::const_iterator iter = agentsOfProperNounActors.find(uid);
				if (iter != agentsOfProperNounActors.end()) {
					PairedActorMention pairedActor = (*iter).second;
					if ((!pairedActor.actorMention) || pairedActor.isTemporary)
						continue;
					if (ProperNounActorMention_ptr p = boost::dynamic_pointer_cast<ProperNounActorMention>(pairedActor.actorMention)) {
						if (_actorInfo->isACountry(p->getActorId())) {
							actor = p->copyWithNewEntityMention(sentTheory, m, L"EXPLICIT-LINK-TO-COUNTRY");
						}
					}
					if (actor) {
						addActorMention(actor, result);
						if (_verbosity > 1) {
							std::wostringstream msg;
							msg << L"    Adding ActorMention: " << actor << L" for \""
								<< m->toCasedTextString() << L"\", because it's explicitly linked to a known mention";
							SessionLogger::info("ICEWS") << msg.str();
						}
					}
				}
			}
		}
	}			
}

void ActorMentionFinder::logActorMention(ActorMention_ptr actorMention, double score, ActorMention_ptr conflictingActor, bool replace_old_value) {
	if (_verbosity <= 1) return;
	if (conflictingActor && (_verbosity <= 2)) return;

	// If we've already given it this label, then don't bother to mention that we're skipping it.
	if (conflictingActor) {
		if (ProperNounActorMention_ptr p1 = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention)) {
			if (ProperNounActorMention_ptr p2 = boost::dynamic_pointer_cast<ProperNounActorMention>(conflictingActor))
				if (p1->getActorId() == p2->getActorId()) return;
		}
		if (CompositeActorMention_ptr p1 = boost::dynamic_pointer_cast<CompositeActorMention>(actorMention)) {
			if (CompositeActorMention_ptr p2 = boost::dynamic_pointer_cast<CompositeActorMention>(conflictingActor))
				if ((p1->getPairedActorId() == p2->getPairedActorId()) &&
					(p1->getPairedAgentId() == p2->getPairedAgentId())) return;
		}
	}

	std::wostringstream msg;
	msg << "    " << ((conflictingActor&&!replace_old_value)?L"Discarding candidate: ":L"Adding ActorMention: ")
		<< actorMention << L" for "
		<< L"\"" << actorMention->getEntityMention()->toCasedTextString() << L"\""
		<< L" (sent " << actorMention->getSentence()->getSentNumber() << ")"
		<< L", score=" << score;
	if (ProperNounActorMention_ptr p = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention)) {
		msg << L", pattern=" << p->getActorPatternId().getId();
	}
	if (CompositeActorMention_ptr p = boost::dynamic_pointer_cast<CompositeActorMention>(actorMention)) {
		msg << L", pattern=" << p->getAgentActorPatternName().to_debug_string();
	}
	if (_verbosity > 3)
		msg << ", source_note=" << actorMention->getSourceNote();
	if (conflictingActor && (_verbosity > 3)) {
		if (replace_old_value)
			msg << L"\n    -> Replacing old value " << conflictingActor;
		else
			msg << L"\n    -> Already tagged as " << conflictingActor;
	}
	SessionLogger::info("ICEWS") << msg;
}

ActorMentionFinder::ScoredActorMention ActorMentionFinder::makePrecomposedCompositeActorMention(const CompositeActorMatch &match, const DocTheory* docTheory, const SentenceTheory *sentTheory, Symbol sourceNote, const char* publicationDate, const ActorMatchesBySentence *actorMatches) {
	const Mention *mention = getCoveringNameDescMention(sentTheory, match.start_token, match.end_token);
	if (!mention)
		return ScoredActorMention(-1, ActorMention_ptr());

	const SynNode *node = mention->getAtomicHead();
	bool same_start_tok = (node->getStartToken() == match.start_token);
	bool same_end_tok = (node->getEndToken() == match.end_token);


	// Construct a proper noun actor mention that links the 
	// actor mention from match with the entity mention.
	AgentId pairedAgentId = match.id.getPairedAgentId();
	ActorId pairedActorId = match.id.getPairedActorId();
	Symbol pairedAgentCode = _actorInfo->getAgentCode(pairedAgentId);
	Symbol pairedActorCode = _actorInfo->getActorCode(pairedActorId);
	bool paired_actor_requires_context = _actorTokenMatcher->patternRequiresContext(match.patternId);
	ActorMention_ptr actorMention = boost::make_shared<CompositeActorMention>(sentTheory, mention, sourceNote,
		ActorMention::AgentIdentifiers(pairedAgentId, _actorInfo->getAgentName(pairedAgentId), pairedAgentCode, AgentPatternId()),
		ActorMention::ActorIdentifiers(pairedActorId, _actorInfo->getActorName(pairedActorId), pairedActorCode, match.patternId, match.isAcronymMatch, paired_actor_requires_context), sourceNote);
	if (!same_end_tok)
		actorMention->addSourceNote(PATTERN_END_IS_NOT_MENTION_END_SYM);

	// Calculate a score for this new potential actor mention.
	double score = match.weight + match.pattern_strlen;
	if (match.isAcronymMatch && actorMatches) {
		ActorMatch pairedActorMatch(pairedActorId, match.patternId, pairedActorCode, match.start_token, match.end_token, match.pattern_strlen, match.weight, match.isAcronymMatch);
		score += scoreAcronymMatch(docTheory, sentTheory, pairedActorMatch, *actorMatches);
	}
	if (same_start_tok && same_end_tok)
		score += 15; // exact span match.
	else if (same_end_tok) 
		score += 5;

	// Give a higher score if more associated actors/ids/countries 
	// also appear in the same document.
	score += getAssociationScore(actorMention, publicationDate);

	return ScoredActorMention(score, actorMention);
}

double ActorMentionFinder::scoreProperNounActorMention(
	ActorMention_ptr *actorMention, const ActorMatch &match, 
	const DocTheory* docTheory, const SentenceTheory *sentTheory, 
	const ActorMatchesBySentence &actorMatches, 
	bool head_match, bool country_the_plural, bool add_weight_to_score)
{
	const Mention *mention = (*actorMention)->getEntityMention();

	// Calculate a score for this new potential actor mention.
	// Note: the "weights" that are hard-coded here should ideally
	// be replaced with (trained) feature weights at some point.
	double score = 0; // baseline score

	const SynNode *node = mention->getAtomicHead();
	bool same_start_tok = (node->getStartToken() == match.start_token);
	bool same_end_tok = (node->getEndToken() == match.end_token);

	// Acronyms need extra justification.
	if (match.isAcronymMatch) 
		score += scoreAcronymMatch(docTheory, sentTheory, match, actorMatches);

	if (_actorTokenMatcher->patternRequiresContext(match.patternId))
		score += scoreContextRequiredMatch(docTheory, sentTheory, match, actorMatches);

	// Penalize non-head matches.
	if (!head_match)
		score -= 5;

	// How well does the mention span match up with the actor match?
	if (same_start_tok && same_end_tok)
		score += 15; // exact span match.
	else if (same_end_tok) 
		score += 5;

	// If the source pattern has extra weight, then add it to the score.
	if (add_weight_to_score)
		score += match.weight;

	// Give extra weight to matches whose entity type matches the entity
	// type that we assigned to the mention.
	if (_actorInfo->isACountry(match.id) && _actorInfo->getActorCode(match.id) != VATICAN_ACTOR_CODE) {
		if (mention->getEntityType().matchesGPE())
			score += 5;
		else if (mention->getEntityType().matchesLOC())
			score += 3;
		else if (mention->getEntityType().matchesPER()) {
			if (_encode_person_matching_country_as_citizen && !country_the_plural) {
				if (_mode == ICEWS) {
					*actorMention = boost::make_shared<CompositeActorMention>(sentTheory, mention, CITIZEN_OF_COUNTRY_SYM,
						ActorMention::AgentIdentifiers(_actorInfo->getDefaultPersonAgentId(), _actorInfo->getAgentName(_actorInfo->getDefaultPersonAgentId()), 
						_actorInfo->getDefaultPersonAgentCode(), AgentPatternId()), 
						ActorMention::ActorIdentifiers(match, _actorInfo->getActorName(match.id), _actorTokenMatcher->patternRequiresContext(match.patternId)),
						PERSON_IS_CITIZEN_OF_COUNTRY_SYM);
					/*std::cerr << "citizen of country " << actorMention << " \"" << mention->toCasedTextString()
					<< "\" " << Mention::getTypeString(mention->getMentionType()) 
					<< "/" << mention->getEntityType().getName() << std::endl;*/
				}
				else if (_mode == ACTOR_MATCH) {
					*actorMention = ActorMention_ptr();
				}
			}
		}
	}
	if (_actorInfo->isAnIndividual(match.id)) {
		if (mention->getEntityType().matchesPER())
			score += 3;
	}

	// How long is the pattern that found the actor match?  Longer
	// patterns are usually more reliable.
	score += match.pattern_strlen;

	// Don't trust single token PER matches
	if (_mode == ACTOR_MATCH && _actorInfo->isAPerson(match.id) && match.start_token == match.end_token)
		score -= 50;

	// We need a way to break ties when the patterns are of the same length;
	// This is a database problem but the database isn't ours, so that's the way it goes.
	// We want to be deterministic at least given a particular database.
	// At the moment (2016), there are ~1,000,000 actors, so multiplying by 1/10,000,000 and actor_id seems fair.	
	score -= match.id.getId() / 10000000.0F;

	return score;
}

ActorMentionFinder::ScoredActorMention ActorMentionFinder::makeProperNounActorMention(
	const ActorMatch &match, const DocTheory* docTheory, const SentenceTheory *sentTheory, 
	const ActorMatchesBySentence &actorMatches, Symbol sourceNote, const char* publicationDate) 
{	
	bool head_match = false;
	bool country_the_plural = false;

	// Parallel function for software
	ActorMention_ptr  actorMention;
	if (match.id.getDbName() == Symbol(L"software")) {
		actorMention = createSoftwareMention(match, sentTheory, sourceNote, head_match, country_the_plural);
	} else {
		actorMention = createProperNounActorMention(match, sentTheory, sourceNote, head_match, country_the_plural);
	}

	// Check if actorMention is null
	if (!actorMention) {
		return ScoredActorMention(-1, ActorMention_ptr());
	}

	double score = scoreProperNounActorMention(
		&actorMention, match, docTheory, sentTheory, 
		actorMatches, head_match, country_the_plural, true);

	// Give a higher score if more associated actors/ids/countries 
	// also appear in the same document.
	score += getAssociationScore(actorMention, publicationDate);

	return ScoredActorMention(score, actorMention);
}

// Version of createProperNounActorMention specifically for software
ActorMention_ptr ActorMentionFinder::createSoftwareMention(
	const ActorMatch &match, const SentenceTheory *sentTheory, Symbol sourceNote,
	bool &head_match, bool &country_the_plural) 
{
	const Mention *mention = getCoveringNameDescMention(sentTheory, match.start_token, match.end_token);

	// If the matched phrase isn't the head of a mention, see if it's a non-head
	// child of one.  This can occur for mis-parses, and in a few other cases.
	head_match = (mention != 0);
	if (!head_match) {
		const SynNode *parseRoot = sentTheory->getPrimaryParse()->getRoot();
		const SynNode *coveringNode = parseRoot->getCoveringNodeFromTokenSpan(match.start_token, match.end_token);
		if (coveringNode->getParent() && coveringNode->getParent()->getParent() && 
			(coveringNode->getParent()->getParent()->getTag() == NPP_SYM)) {
			coveringNode = coveringNode->getParent()->getParent();
			mention = getCoveringNameDescMention(sentTheory, coveringNode);
			if (mention && (_verbosity > 4)) {
				SessionLogger::info("ICEWS") << "    Non-head match: mention = ["
					<< mention->toCasedTextString() << "].  pattern match = ["
					<< sentTheory->getTokenSequence()->toString(match.start_token, match.end_token) 
					<< "], head = [" << coveringNode->getHead()->toCasedTextString(sentTheory->getTokenSequence()) << "]";
			}
		}
	}

	// If no such mention is found, then give up.
	if (!mention) {
		//std::cout << "returning--couldn't find a mention\n";
		return ActorMention_ptr();
	}

	// This is currently only set when matching using an AWAKE database
	if (_only_match_names && mention->getMentionType() != Mention::NAME) {
		//std::cout << "returning--not a name\n";
		return ActorMention_ptr();
	}

	const SynNode *node = mention->getAtomicHead();
	bool same_start_tok = (node->getStartToken() == match.start_token);
	bool same_end_tok = (node->getEndToken() == match.end_token);

	if (!(same_start_tok && same_end_tok)) {
		const char* context = "a piece of software";
		int start_tok = node->getStartToken();
		int end_tok = node->getEndToken();
		//return an empty mention if the match is not fully contained within the mention
		if (!(match.start_token >= start_tok) && (match.end_token <= end_tok)) {
			//std::cout << "match is not contained within mention...returning\n";
			return ActorMention_ptr();
		}
	}

	// Construct a proper noun actor mention that links the 
	// actor mention from match with the entity mention.
	ActorMention_ptr actorMention;
	if (match.code == BLOCK_ACTOR_SYM) {
		// If we're explicitly blocking this agent, then return an empty 
		// ActorMention; this will be discarded at the end of processing.
		actorMention = boost::make_shared<ActorMention>(sentTheory, mention, BLOCK_ACTOR_SYM);
	} else {
		actorMention = boost::make_shared<ProperNounActorMention>(sentTheory, mention, sourceNote,
			ActorMention::ActorIdentifiers(match, _actorInfo->getActorName(match.id), _actorTokenMatcher->patternRequiresContext(match.patternId)));
	}
	
	
	if (!same_end_tok)
		actorMention->addSourceNote(PATTERN_END_IS_NOT_MENTION_END_SYM);

	// This is currently only set when matching using an AWAKE database
	if (_require_entity_type_match) {
		if (ProperNounActorMention_ptr pn_actor = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention)) {
			//LT: changed this line to avoid having to edit entityTypeMatches
			//if (!entityTypeMatches(pn_actor, mention->getEntityType())) {
			if (!(mention->getEntityType().getName() == _actorInfo->getEntityTypeForActor(pn_actor->getActorId()))) {
				return ActorMention_ptr();
			}
		}
	}

	return actorMention;
}

/* Does not include software, that has a separate function createSoftwareMention */
ActorMention_ptr ActorMentionFinder::createProperNounActorMention(
	const ActorMatch &match, const SentenceTheory *sentTheory, Symbol sourceNote,
	bool &head_match, bool &country_the_plural) 
{
	const Mention *mention = getCoveringNameDescMention(sentTheory, match.start_token, match.end_token);

	// If the matched phrase isn't the head of a mention, see if it's a non-head
	// child of one.  This can occur for mis-parses, and in a few other cases.
	head_match = (mention != 0);
	if (!head_match) {
		const SynNode *parseRoot = sentTheory->getPrimaryParse()->getRoot();
		const SynNode *coveringNode = parseRoot->getCoveringNodeFromTokenSpan(match.start_token, match.end_token);
		if (coveringNode->getParent() && coveringNode->getParent()->getParent() && 
			(coveringNode->getParent()->getParent()->getTag() == NPP_SYM)) {
			coveringNode = coveringNode->getParent()->getParent();
			mention = getCoveringNameDescMention(sentTheory, coveringNode);
			if (mention && (_verbosity > 4)) {
				SessionLogger::info("ICEWS") << "    Non-head match: mention = ["
					<< mention->toCasedTextString() << "].  pattern match = ["
					<< sentTheory->getTokenSequence()->toString(match.start_token, match.end_token) 
					<< "], head = [" << coveringNode->getHead()->toCasedTextString(sentTheory->getTokenSequence()) << "]";
			}
		}
	}

	// If no such mention is found, then give up.
	if (!mention)
		return ActorMention_ptr();

	// This is currently only set when matching using an AWAKE database
	if (_only_match_names && mention->getMentionType() != Mention::NAME) {
		return ActorMention_ptr();
	}

	const SynNode *node = mention->getAtomicHead();
	bool same_start_tok = (node->getStartToken() == match.start_token);
	bool same_end_tok = (node->getEndToken() == match.end_token);

	// When matching country names, require that the name be exact (modulo some 
	// acceptable modifiers such as "western")
	bool actor_is_a_country = _actorInfo->isACountry(match.id);
	bool actor_is_an_individual = _actorInfo->isAnIndividual(match.id);
	bool actor_is_an_org = !(actor_is_a_country || actor_is_an_individual);
	
	// Look for things like "The Americans"
	country_the_plural = false;
	if (actor_is_a_country && mention->getMentionType() == Mention::DESC &&
		node->getTag() == Symbol(L"NNPS") && node->getParent() != 0 && node->getParent()->getTag() == Symbol(L"NPP"))
	{
		const SynNode* parent = node->getParent()->getParent();
		if (parent != 0 && parent->getParent() != 0 && parent->getParent()->hasMention() &&
			sentTheory->getMentionSet()->getMention(parent->getParent()->getMentionIndex())->getMentionType() == Mention::DESC) 
		{
			// this is part of a bigger mention, let's bail
		} else if (parent != 0 && parent->getStartToken() + 1 == parent->getEndToken() && parent->getNthTerminal(0)->getHeadWord() == Symbol(L"the"))	{
			country_the_plural = true;
		}
	}

	if (!(same_start_tok && same_end_tok) && !country_the_plural) {
		const std::set<std::wstring> & modifierWords = (actor_is_a_country ?     _countryModifierWords : 
		                                                actor_is_an_individual ? _personModifierWords : 
		                                                                         _organizationModifierWords);

		const char* context = actor_is_a_country ? "a country" : (actor_is_an_individual ? "a person" : "an organization");
		int start_tok = node->getStartToken();
		int end_tok = node->getEndToken();
		for (int tok=start_tok; tok<=end_tok; ++tok) {
			if ((tok<match.start_token) || (tok>match.end_token)) {
				const SynNode *preterminal = node->getNthTerminal(tok-start_tok)->getParent();
				Symbol postag = preterminal ? preterminal->getTag() : Symbol(L"???");
				// For people and orgs (but not countries), adjectives are acceptable.
				if ((!actor_is_a_country) && (postag == JJ_POS_SYM)) continue;
				// Otherwise, the word must appear in our modifier words list.
				std::wstring tokStr(sentTheory->getTokenSequence()->getToken(tok)->getSymbol().to_string());
				boost::to_lower(tokStr);

				// Special case required to not allow "NORTH KOREA" to match "KOREA"
				if ((tokStr == L"north" && node->getHeadWord() == Symbol(L"korea")) ||
					 modifierWords.find(tokStr) == modifierWords.end())
				{
					if (_verbosity > 3) {
						SessionLogger::info("ICEWS") << "    Skipping mention \"" << mention->toCasedTextString() 
							<< "\" which matches pattern \""
							<< sentTheory->getTokenSequence()->toString(match.start_token, match.end_token) << "\" "
							<< "(head=\"" << node->toCasedTextString(sentTheory->getTokenSequence()) << "\") "
							<< "because the actor is " << context << ", and the mention contains the unexpected word \""
							<< tokStr << "\" (" << postag << ")" << std::endl;
					}
					return ActorMention_ptr(); // supress this match.
				}
			}
		}
	}

	// Construct a proper noun actor mention that links the 
	// actor mention from match with the entity mention.
	ActorMention_ptr actorMention;
	if (match.code == BLOCK_ACTOR_SYM) {
		// If we're explicitly blocking this agent, then return an empty 
		// ActorMention; this will be discarded at the end of processing.
		actorMention = boost::make_shared<ActorMention>(sentTheory, mention, BLOCK_ACTOR_SYM);
	} else {
		actorMention = boost::make_shared<ProperNounActorMention>(sentTheory, mention, sourceNote,
			ActorMention::ActorIdentifiers(match, _actorInfo->getActorName(match.id), _actorTokenMatcher->patternRequiresContext(match.patternId)));
	}
	if (!same_end_tok)
		actorMention->addSourceNote(PATTERN_END_IS_NOT_MENTION_END_SYM);

	// This is currently only set when matching using an AWAKE database
	if (_require_entity_type_match) {
		if (ProperNounActorMention_ptr pn_actor = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention)) {
			if (!entityTypeMatches(pn_actor, mention->getEntityType())) {
				return ActorMention_ptr();
			}
		}
	}

	return actorMention;
}

std::pair<std::wstring, std::wstring> ActorMentionFinder::normalizeAcronymExpansion(std::wstring name) {
	boost::replace_all(name, " 's", "'s");
	std::vector<std::wstring> tokens;
	boost::split(tokens, name, boost::is_any_of(L" "));
	std::wstring result = L"";
	std::wstring inits = L"";
	// Organization on Stuff --> (organizationstuff, OS)

	int n_caps = 0;
	// Remove spaces and lowercase words
	BOOST_FOREACH(std::wstring tok, tokens) {
		if (tok.size() > 0 && iswupper(tok.at(0))) {
			inits += tok.at(0);
			boost::to_lower(tok);
			result += tok;
			n_caps++;
		}
	}

	// If there weren't at least two capitalized words, don't rely on casing, use all words
	if (n_caps < 2) {
		result = L"";
		inits = L"";
		BOOST_FOREACH(std::wstring tok, tokens) {
			if (tok.size() > 0) {
				boost::to_lower(tok);
				result += tok;
				inits += tok.at(0);
			}
		}
	}

	return std::make_pair(result, inits);
}

// Try to find a good justification for using this acronym
float ActorMentionFinder::scoreAcronymMatch(const DocTheory *docTheory, const SentenceTheory *sentTheory, ActorMatch match, const ActorMatchesBySentence &actorMatches) {
	float penalty = -50.0f; // default result if we find no justification for the acronym

	std::wstring reason = L"No justification found"; // for logging
	std::wstring actorName = _actorInfo->getActorName(match.id);

	// First, check if the document contains any mentions of the same actor,
	// that use the actor's full name (and not just an acronym).  If so, then
	// using the acronym is fairly safe, so don't apply any penalty.
	for (size_t sent_no=0; sent_no<actorMatches.size(); ++sent_no) {
		BOOST_FOREACH(ActorMatch m2, actorMatches[sent_no]) {
			if ((m2.id == match.id) && (!m2.isAcronymMatch) && !_actorTokenMatcher->patternRequiresContext(m2.patternId)) {
				penalty = 0;
				reason = std::wstring(L"Found full expansion (\"")+docTheory->getSentenceTheory(static_cast<int>(sent_no))->getTokenSequence()->toString(m2.start_token, m2.end_token)+L"\")";
			}
		}
	}

	// Next, get the expanded acronym name, and see if we can match that
	// expansion or something similar to it in the document.
	bool found_conflict = false;
	if (penalty<0) {
		std::pair<std::wstring, std::wstring> actorNameNorm = normalizeAcronymExpansion(actorName);				
		for (size_t sent_no=0; sent_no<actorMatches.size(); ++sent_no) {
			const MentionSet *mentionSet = docTheory->getSentenceTheory(static_cast<int>(sent_no))->getMentionSet();
			for (int i=0; i<mentionSet->getNMentions(); ++i) {
				const Mention *m2 = mentionSet->getMention(i);
				std::wstring mentionText = m2->toCasedTextString();

				// Former approach used edit distance, but this lets through things like
				//   Asian Development Bank ~ African Development Bank
				// We need to be more conservative. Most of the correct edit distances were from
				//   differences in spacing and prepositions. So, we remove those and just require
				//   exact match. This occasionally misses things like Cambodian Party ~ Cambodia Party
				//   but we will survive-- if SERIF correctly corefs these, edit distance should get this
				//   at the full mention.

				std::pair<std::wstring, std::wstring> mentionTextNorm = normalizeAcronymExpansion(mentionText);
				if (actorNameNorm.first == mentionTextNorm.first) {
					// [XX] Should we also add an ActorMatch for the justifying mention that we found?
					penalty = 0;
					reason = std::wstring(L"Found mention similar to expansion (\"")+mentionText+L"\")";
				} else if (penalty != 0 && actorNameNorm.second.size() > 1 && actorNameNorm.second == mentionTextNorm.second) {
					penalty = -100;
					found_conflict = true;
					reason = std::wstring(L"Found mention with same initials but different expansion (\"")+mentionText+L"\")";
				}
			}
		}
	}

	// If it's associated with a country, and that country is explicitly mentioned,
	// then we'll allow it.
	BOOST_FOREACH(ActorId associatedCountry, _actorInfo->getAssociatedCountryActorIds(match.id)) {
		if (found_conflict)
			continue;
		int num_mentions = 0;
		for (size_t sent_no=0; sent_no<actorMatches.size(); ++sent_no) {
			BOOST_FOREACH(ActorMatch m2, actorMatches[sent_no]) {
				if ((m2.id == associatedCountry) && (!m2.isAcronymMatch)) {
					++num_mentions;
				}
			}
		}
		if (num_mentions > 0) {
			float newPenalty = static_cast<float>(std::min(-5, -10+num_mentions));
			if (penalty < newPenalty) {
				penalty = newPenalty;
				reason = std::wstring(L"Related country (\"")+_actorInfo->getActorName(associatedCountry)+
					L"\") was mentioned "+boost::lexical_cast<std::wstring>(num_mentions)+L" times.";
			}
		}
	}

	// If it occurs within the first two sentences, and is the name of a media
	// organization, then we'll allow it.  This captures things like "(AP)".
	if (!found_conflict && sentTheory->getSentNumber()<3) {
		BOOST_FOREACH(Symbol code, _actorInfo->getAssociatedSectorCodes(match.id)) {
			if ((code == MEDIA_SECTOR_CODE) || (code == NEWS_SECTOR_CODE) ||
				(code == MEDIA_SECTOR_NAME) || (code == NEWS_SECTOR_NAME)) 
			{
				float newPenalty = -5.0f * (sentTheory->getSentNumber()+1);
				if (penalty < newPenalty) {
					penalty = newPenalty;
					reason = L"Media org occuring in the first three sentences.";
				}
			}
		}
	}

	if (_verbosity > 3) {
		std::ostringstream msg;
		msg << "      " << ((penalty<=-50.0f)?"Rejected":"Accepted") << " acronym expansion "
			<< sentTheory->getTokenSequence()->toString(match.start_token, match.end_token) 
			<< "=>\"" << actorName << "\"";
		msg << "\n        [" << std::setw(5) << penalty << "] " << reason;
		SessionLogger::info("ICEWS") << msg.str();
	}

	return penalty;
}


// Try to find a good justification for using this match
float ActorMentionFinder::scoreContextRequiredMatch(const DocTheory *docTheory, const SentenceTheory *sentTheory, ActorMatch match, const ActorMatchesBySentence &actorMatches) {
	float penalty = -50.0f; // default result if we find no justification for the pattern

	std::wstring reason = L"No justification found"; // for logging

	// First, check if the document [to this point] contains any mentions of the same actor,
	// that use a version of the actor that doesn't require context. If so, then
	// using the shorter form is fairly safe, so don't apply any penalty.
	for (size_t sent_no=0; sent_no<actorMatches.size(); ++sent_no) {
		BOOST_FOREACH(ActorMatch m2, actorMatches[sent_no]) {
			if (m2.id == match.id && !m2.isAcronymMatch && !_actorTokenMatcher->patternRequiresContext(m2.patternId)) {
				penalty = 0;
				reason = std::wstring(L"Found full expansion (\"")+docTheory->getSentenceTheory(static_cast<int>(sent_no))->getTokenSequence()->toString(m2.start_token, m2.end_token)+L"\")";
			}
		}
	}

	// If it's associated with a location, and that location is explicitly mentioned already,
	// then we'll allow it.
	BOOST_FOREACH(ActorId associatedLocation, _actorInfo->getAssociatedLocationActorIds(match.id)) {
		int num_mentions = 0;
		for (size_t sent_no=0; sent_no<actorMatches.size(); ++sent_no) {
			BOOST_FOREACH(ActorMatch m2, actorMatches[sent_no]) {
				if (m2.id == associatedLocation) {
					++num_mentions;
				}
			}
		}
		if (num_mentions > 0) {
			penalty = 0;
			reason = std::wstring(L"Context (\"")+_actorInfo->getActorName(associatedLocation)+
				L"\") was mentioned "+boost::lexical_cast<std::wstring>(num_mentions)+L" times.";
		}
	}

	if (_verbosity > 3) {
		std::ostringstream msg;
		msg << "      " << ((penalty<=-50.0f)?"Rejected":"Accepted") << " pattern requiring context "
			<< sentTheory->getTokenSequence()->toString(match.start_token, match.end_token) 
			<< "=>\"" << _actorInfo->getActorName(match.id) << "\"";
		msg << "\n        [" << std::setw(5) << penalty << "] " << reason;
		SessionLogger::info("ICEWS") << msg.str();
	}

	return penalty;
}



ActorMentionFinder::ScoredActorMention ActorMentionFinder::makeProperNounActorMentionFromGazetteer(const Mention* mention, Gazetteer::ScoredGeoResolution resolution, 
																								   const SentenceTheory *sentTheory, Symbol actorNoteSym) 
{
	if (resolution.first == 0 || !resolution.second->countryInfo)
	{
		return ScoredActorMention(-1, ActorMention_ptr());
	}

	ActorId geonameActorId = _actorInfo->getActorIdForGeonameId(resolution.second->geonameid);
	if (!geonameActorId.isNull()) {
		ActorMention_ptr actorMention = boost::make_shared<ProperNounActorMention>(
			sentTheory, mention, actorNoteSym, ActorMention::ActorIdentifiers(geonameActorId, _actorInfo->getActorName(geonameActorId), Symbol(), ActorPatternId()),
			resolution.second);
		return ScoredActorMention(resolution.first, actorMention);
	}

	Gazetteer::CountryInfo_ptr countryInfo = resolution.second->countryInfo;
	
	ActorMention_ptr actorMention = boost::make_shared<ProperNounActorMention>(
		sentTheory, mention, actorNoteSym,
		ActorMention::ActorIdentifiers(countryInfo->actorId, _actorInfo->getActorName(countryInfo->actorId), countryInfo->actorCode, ActorPatternId()),
		resolution.second);

	return ScoredActorMention(resolution.first, actorMention);
}

bool ActorMentionFinder::sisterMentionHasClashingActor(const Mention *agentMention, ProperNounActorMention_ptr proposedActor, 
													   const DocTheory *docTheory, ActorMentionSet *actorMentionSet, int verbosity)
{
	EntitySet *entitySet = docTheory->getEntitySet();

	// Get the entity that corresponds with this mention.
	const Entity *ent = docTheory->getEntityByMention(agentMention);
	if (!ent) return false; 

	// Iterate through the mentions that are coreferent with this entity.
	for (int m=0; m<ent->getNMentions(); ++m) {
		MentionUID mentionUID = ent->getMention(m);
		Mention *corefMention = entitySet->getMention(mentionUID);
		SentenceTheory *corefSentTheory = docTheory->getSentenceTheory(mentionUID.sentno());

		bool clash = false;
		if (ActorMention_ptr existing = actorMentionSet->find(mentionUID)) {
			if (ProperNounActorMention_ptr pn_actor = boost::dynamic_pointer_cast<ProperNounActorMention>(existing)) {
				if (pn_actor->getActorId() != proposedActor->getActorId())
					clash = true;
			} else if (CompositeActorMention_ptr c_actor = boost::dynamic_pointer_cast<CompositeActorMention>(existing)) {
				if (c_actor->getPairedActorId().isNull())
					continue;
				if (c_actor->getPairedActorId() != proposedActor->getActorId())
					clash = true;
			}
			if (clash) {
				if (verbosity > 3) {
					SessionLogger::info("ICEWS") << "    * Default country blocked: \"" 
						<< agentMention->toCasedTextString() << "\"\n "
						<< "        Blocked by coref mention with tag: \""
						<< corefMention->toCasedTextString() << L"\"";
				}
				return true;
			}
		}
	}
	return false;
}

ActorMentionFinder::PairedActorMention ActorMentionFinder::findActorForAgent(
	const Mention *agentMention, const AgentActorPairs &agentActorPairs, const DocTheory* docTheory) 
{
	bool conservative = true;
	AgentActorPairs::const_iterator actorMentionIt;

	// Is there a direct connection from the agent to some actor?
	actorMentionIt = agentActorPairs.find(agentMention->getUID());
	if (actorMentionIt != agentActorPairs.end())
		return (*actorMentionIt).second;
	
	// What about mentions that are coreferent with the agent? 
	const Entity *ent = docTheory->getEntitySet()->getEntityByMention(agentMention->getUID());
	if (ent) {
		for (int m=0; m<ent->getNMentions(); ++m) {
			Mention *agentMention2 = docTheory->getEntitySet()->getMention(ent->getMention(m));
			if (agentMention == agentMention2) continue;
			// We don't trust coref that crosses sentence boundaries (and besides, agentActorPairs
			// will currently only include pairs from the current sentence)
			if (agentMention2->getSentenceNumber() != agentMention->getSentenceNumber())
				continue;
			// We only trust coreference links with high conficence:
			const SentenceTheory* sentTheory = docTheory->getSentenceTheory(agentMention2->getSentenceNumber());
			MentionConfidenceAttribute confidence = MentionConfidence::determineMentionConfidence(
				docTheory, sentTheory, agentMention2);
			Symbol headword = agentMention2->getNode()->getHeadWord();
			if (confidence != MentionConfidenceStatus::TITLE_DESC && confidence != MentionConfidenceStatus::APPOS_DESC &&
				confidence != MentionConfidenceStatus::WHQ_LINK_PRON && confidence != MentionConfidenceStatus::COPULA_DESC &&
				confidence != MentionConfidenceStatus::DOUBLE_SUBJECT_PERSON_PRON && 
				confidence != MentionConfidenceStatus::ONLY_ONE_CANDIDATE_PRON && confidence != MentionConfidenceStatus::ONLY_ONE_CANDIDATE_DESC &&
				confidence != MentionConfidenceStatus::NAME_AND_POSS_PRON && headword != Symbol(L"who")) {
				continue;
			}
			actorMentionIt = agentActorPairs.find(agentMention2->getUID());
			if (actorMentionIt != agentActorPairs.end())
				return (*actorMentionIt).second;
		}
	}

	// No paired actor found!
	return PairedActorMention();
}

ActorMentionFinder::ScoredActorMention ActorMentionFinder::makeCompositeActorMention(const AgentMatch &agentMatch, const DocTheory* docTheory, const SentenceTheory *sentTheory, const AgentActorPairs &agentActorPairs, const ActorMentionSet *actorMentions) 
{
	const Mention *agentMention = getCoveringNameDescMention(sentTheory, agentMatch.start_token, agentMatch.end_token);
	if (!agentMention)
		return ScoredActorMention(-1, ActorMention_ptr());
	const SynNode *agentNode = agentMention->getAtomicHead();

	// Calculate a score for the composite actor mention.
	double score = 0;

	// How well does the mention span match up with the actor match?
	bool same_start_tok = (agentNode->getStartToken() == agentMatch.start_token);
	bool same_end_tok = (agentNode->getEndToken() == agentMatch.end_token);
	if (same_start_tok && same_end_tok)
		score += 5; // exact span match.
	else if (same_end_tok) 
		score += 2;
	score += agentMatch.pattern_strlen; // longer patterns are better!

	// We need a way to break ties when the patterns are of the same length;
	// This is a database problem but the database isn't ours, so that's the way it goes.
	// We want to be deterministic at least given a particular database.
	// There are currently 691 agents, so multiplying by 1/10000 and agent_id seems fair.
	score -= agentMatch.id.getId() / 10000.0F;

	if (agentMention->getEntityType().matchesPER() &&
		agentMention->getMentionType() == Mention::NAME &&
		agentMatch.start_token >= agentNode->getStartToken())
	{
		std::wstring headword = agentNode->getHeadWord().to_string();
		if (_perAgentNameWords.find(headword) == _perAgentNameWords.end()) {
			if (_verbosity > 3) {
				SessionLogger::info("ICEWS") << "  Agent match blocked for person name: " << agentMention->toCasedTextString();
			}
			return ScoredActorMention(-1, ActorMention_ptr());
		}
	}
	
	// Check if there's an actor paired with this agent.  If it's paired with a 
	// composite actor, then we take the paired actor of that composite actor.
	PairedActorMention pairedActor = findActorForAgent(agentMention, agentActorPairs, docTheory);
	if (pairedActor.actorMention) {
		Symbol sourceNote;
		ActorMention::ActorIdentifiers pairedActorIdentifiers;
		if (ProperNounActorMention_ptr p = boost::dynamic_pointer_cast<ProperNounActorMention>(pairedActor.actorMention)) {
			pairedActorIdentifiers = p->getActorIdentifiers();
			sourceNote = AGENT_PATTERN_SYM;
		} else if (CompositeActorMention_ptr p = boost::dynamic_pointer_cast<CompositeActorMention>(pairedActor.actorMention)) {
			// <X for Y for Z> -> <X for Z>
			pairedActorIdentifiers = p->getPairedActorIdentifiers();
			sourceNote = AGENT_OF_AGENT_PATTERN_SYM;
		}
		if (!pairedActorIdentifiers.id.isNull()) {
			score += 5;
			// Reduce score if the agent "requires" a country actor, but we don't have one.
			bool country_restriction_applies = false;
			if (_actorInfo->isRestrictedToCountryActors(agentMatch.id)) {				
				if (!_actorInfo->isACountry(pairedActorIdentifiers.id)) {
					country_restriction_applies = true;
					if (_actorInfo->isAnIndividual(pairedActorIdentifiers.id))
						score -= 8;
					else
						score -= 4;
				} else {
					score += 1;
				}
			}
			ActorMention_ptr compoundActorMention;
			if (country_restriction_applies) {
				compoundActorMention = pairedActor.actorMention->copyWithNewEntityMention(
					sentTheory, agentMention, COUNTRY_RESTRICTION_APPLIED_SYM.to_string());
			} else if (agentMatch.code == COMPOSITE_ACTOR_IS_PAIRED_ACTOR_SYM) {
				// This special agent code is used to indicate that we should treat
				// this agent as if it were a direct mention of the paired actor.
				compoundActorMention = pairedActor.actorMention->copyWithNewEntityMention(
					sentTheory, agentMention, COMPOSITE_ACTOR_IS_PAIRED_ACTOR_SYM.to_string());
			} else {
				compoundActorMention = boost::make_shared<CompositeActorMention>(
					sentTheory, agentMention, sourceNote,
					ActorMention::AgentIdentifiers(agentMatch.id, _actorInfo->getAgentName(agentMatch.id), agentMatch.code, agentMatch.patternId), 
					ActorMention::ActorIdentifiers(pairedActorIdentifiers), 
					pairedActor.agentActorPatternName);
			}
			if (!same_end_tok)
				compoundActorMention->addSourceNote(PATTERN_END_IS_NOT_MENTION_END_SYM);
			return ScoredActorMention(score, compoundActorMention);
		}
	}
	// For this pseudo-agent code (which does not appear in the database), 
	// we don't return anything if we can't find a paired actor.  The mention 
	// still may get labeled by later stages (such as labelPeople()).
	if (agentMatch.code == COMPOSITE_ACTOR_IS_PAIRED_ACTOR_SYM)
		return ScoredActorMention(-1, ActorMention_ptr());

	// If we're explicitly blocking this agent, then return an empty ActorMention; this will be
	// discarded at the end of processing.
	if (agentMatch.code == BLOCK_ACTOR_SYM) {
		ActorMention_ptr actorMention = boost::make_shared<ActorMention>(sentTheory, agentMention, BLOCK_ACTOR_SYM);
		return ScoredActorMention(score, actorMention);
	}

	// If there's no actor paired with the agent, then create an actor-less
	// composite mention.
	ActorMention_ptr compoundActorMention = boost::make_shared<CompositeActorMention>(
		sentTheory, agentMention, AGENT_PATTERN_SYM,
		ActorMention::AgentIdentifiers(agentMatch.id, _actorInfo->getAgentName(agentMatch.id), agentMatch.code, agentMatch.patternId), 
		ActorMention::ActorIdentifiers(), UNKNOWN_ACTOR_SYM);
	if (!same_end_tok)
		compoundActorMention->addSourceNote(PATTERN_END_IS_NOT_MENTION_END_SYM);
	return ScoredActorMention(score, compoundActorMention);
}

void ActorMentionFinder::findPairedAgentActorMentions(PatternMatcher_ptr patternMatcher, SentenceTheory* sentTheory, const ActorMentionSet *actorMentionSet, AgentActorPairs &result, PairedActorKind actorKind) 
{
	// Apply the agent/actor pattern set to find agents of actors:
	std::vector<PatternFeatureSet_ptr> matches = patternMatcher->getSentenceSnippets(sentTheory, 0, true);

	BOOST_FOREACH(PatternFeatureSet_ptr match, matches) {
		const Mention *agentMention = 0;
		const Mention *actorMention = 0;
		Symbol patternLabel;
		for (size_t f = 0; f < match->getNFeatures(); f++) {
			PatternFeature_ptr sf = match->getFeature(f);
			if (TopLevelPFeature_ptr tsf = boost::dynamic_pointer_cast<TopLevelPFeature>(sf)) {
				patternLabel = tsf->getPatternLabel();
			} else if (MentionReturnPFeature_ptr mrpf = boost::dynamic_pointer_cast<MentionReturnPFeature>(sf)) {
				//std::vector<std::wstring> roles;
				if (mrpf->getReturnLabel() == AGENT_SYM) {
					agentMention = mrpf->getMention();
				} else if (mrpf->getReturnLabel() == ACTOR_SYM) {
					actorMention = mrpf->getMention();
				}
			}
		}
		if (patternLabel.is_null()) {
			SessionLogger::warn("ICEWS") << "ICEWS agent/actor pattern has no label";
		} else if (!agentMention) {
			SessionLogger::warn("ICEWS") << "ICEWS agent/actor pattern " << patternLabel << "has no AGENT";
		} else if (!actorMention) {
			SessionLogger::warn("ICEWS") << "ICEWS agent/actor pattern " << patternLabel << "has no ACTOR";
		} else {
			// In case of conflict, the first pattern wins.
			if (result.find(agentMention->getUID()) == result.end()) {
				ActorMention_ptr actor = actorMentionSet->find(actorMention->getUID());
				ActorMention_ptr pn_actor;
				bool is_temporary = false;
				if (ProperNounActorMention_ptr a=boost::dynamic_pointer_cast<ProperNounActorMention>(actor)) {
					if (actorKind == PAIRED_PROPER_NOUN_ACTOR)
						pn_actor = a;
				} else if (CompositeActorMention_ptr a=boost::dynamic_pointer_cast<CompositeActorMention>(actor)) {
					if (actorKind == PAIRED_COMPOSITE_ACTOR)
						pn_actor = a;
				} else {
					// note: the paired unknown actors are just used by assignDefaultCountryForUnknownPairedActors()
					// to decide when we should block assignment of default countries, so we don't need/want to add
					// them to the document's ActorMentionSet.
					if (actorKind == PAIRED_UNKNOWN_ACTOR) {
						pn_actor = boost::make_shared<ActorMention>(const_cast<const SentenceTheory*>(sentTheory), actorMention, TEMPORARY_ACTOR_MENTION_SYM);
						is_temporary = true;
					}
				}
				if (pn_actor) {
					result[agentMention->getUID()] = PairedActorMention(pn_actor, patternLabel, is_temporary);
					if (_verbosity > 2) {
						std::ostringstream msg;
						if (!patternLabel.is_null())
							msg << "    Pattern " << patternLabel << ": ";
						msg << "\"" << agentMention->toCasedTextString() << "\" is a potential agent for \""
							<< actorMention->toCasedTextString() << "\" (" << ActorMention_ptr(pn_actor) << ")";
						SessionLogger::info("ICEWS") << msg.str();
					}
				}
			}
		}
	}
}

bool ActorMentionFinder::isCompatibleAndBetter(ActorMention_ptr oldActorMention, ActorMention_ptr newActorMention) {
	if (CompositeActorMention_ptr old_m = boost::dynamic_pointer_cast<CompositeActorMention>(oldActorMention)) {
		if (ProperNounActorMention_ptr new_p = boost::dynamic_pointer_cast<ProperNounActorMention>(newActorMention)) {
			// If the old one has no paired actor AND is a pronoun, and the new one is a proper noun, prefer the new one
			if (old_m->getPairedActorId().isNull()) {
				if (old_m->getEntityMention()->getMentionType() == Mention::PRON)
					return true;
				std::vector<Symbol> properNounSectors = _actorInfo->getAssociatedSectorCodes(new_p->getActorId());
				std::vector<Symbol> agentSectors = _actorInfo->getAssociatedSectorCodes(old_m->getPairedAgentId());
				for (std::vector<Symbol>::const_iterator s1 = properNounSectors.begin(); s1 != properNounSectors.end(); ++s1) {
					for (std::vector<Symbol>::const_iterator s2 = agentSectors.begin(); s2 != agentSectors.end(); ++s2) {
						if ((*s1) == (*s2))
							return true;
					}
				}
			}
		}
		if (CompositeActorMention_ptr new_m = boost::dynamic_pointer_cast<CompositeActorMention>(newActorMention)) {
			// If both have the same agent, but the old one has no paired actor and the new one has one, prefer the new one
			if ((old_m->getPairedAgentId() == new_m->getPairedAgentId()) &&
				(old_m->getPairedActorId().isNull()) && 
				(!new_m->getPairedActorId().isNull())) {
				return true;
			}
		}
	}
	return false;
}

void ActorMentionFinder::labelCoreferentMentions(DocTheory *docTheory, int aggressiveness, ActorMentionSet *result, AgentActorPairs& agentsOfProperNounActors, int sent_cutoff) {
	if (_disable_coref) {
		SessionLogger::warn("ICEWS") << "coref disabled (icews_disable_coref=true); not labeling coreferent mentions";
		return;
	}
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Adding coreferent actor mentions";
	EntitySet *entitySet = docTheory->getEntitySet();

	// The 'newActorMentions' set is used to collect any new actor mentions
	// that we add -- we shouldn't write them directly to result, because
	//  it's dangerous to modify a set that you're currently iterating over.
	ActorMentionSet newActorMentions;

	for (int sentno = 0; sentno < docTheory->getNSentences(); sentno++) {
		if (sentno > sent_cutoff)
			continue;
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
		const MentionSet *mSet = sentTheory->getMentionSet();
		for (int m1 = 0; m1 < mSet->getNMentions(); m1++) {
			const Mention *mention = mSet->getMention(m1);
			// Skip mentions that have proper noun actors already
			ActorMention_ptr existingActor = result->find(mention->getUID());
			if (ProperNounActorMention_ptr p = boost::dynamic_pointer_cast<ProperNounActorMention>(existingActor))
				continue;
			const Entity *ent = entitySet->getEntityByMention(mention->getUID());
			if (ent == 0)
				continue;

			PairedActorMention knownPairedActor = findActorForAgent(mention, agentsOfProperNounActors, docTheory);	
			
			// If we're being conservative, then we only want to copy the ActorMention
			// to mentions that are strongly linked w/ the entity.
			bool is_highly_confident = false;
			MentionConfidenceAttribute confidence = MentionConfidenceStatus::UNKNOWN_CONFIDENCE;
			if (aggressiveness == 0 || aggressiveness == 1) {
				MentionConfidenceAttribute confidence = MentionConfidence::determineMentionConfidence(
					docTheory, sentTheory, mention);
				Symbol headword = mention->getNode()->getHeadWord();
				if (confidence == MentionConfidenceStatus::ANY_NAME ||
					confidence == MentionConfidenceStatus::TITLE_DESC || confidence == MentionConfidenceStatus::APPOS_DESC ||
					confidence == MentionConfidenceStatus::WHQ_LINK_PRON || confidence == MentionConfidenceStatus::COPULA_DESC ||
					confidence == MentionConfidenceStatus::DOUBLE_SUBJECT_PERSON_PRON || 
					confidence == MentionConfidenceStatus::ONLY_ONE_CANDIDATE_PRON || confidence == MentionConfidenceStatus::ONLY_ONE_CANDIDATE_DESC ||
					confidence == MentionConfidenceStatus::NAME_AND_POSS_PRON || headword == Symbol(L"who"))
				{
					// OK
					is_highly_confident = true;
				} else if (aggressiveness == 1 && (confidence == MentionConfidenceStatus::OTHER_PRON || confidence == MentionConfidenceStatus::PREV_SENT_DOUBLE_SUBJECT_PRON)) {
					Symbol headWord = mention->getNode()->getHeadWord();
					if (headWord == HE || headWord == HIM || headWord == HIS || headWord == SHE || headWord == HER) {
						// OK
					} else continue;
				} else {
					continue;
				}
			}

			ActorMention_ptr bestMatch = ActorMention_ptr();
			const Mention *bestMention = 0; // debug only
			int score = 0;

			for (int m2=0; m2<ent->getNMentions(); ++m2) {
				MentionUID corefMentionUID = ent->getMention(m2);
				if (corefMentionUID.sentno() >= sent_cutoff) continue;
				Mention *corefMention = entitySet->getMention(corefMentionUID);
				ActorMention_ptr corefActor = result->find(corefMentionUID);

				// If we already have an actor and this one isn't compatible+better, don't allow it
				if (existingActor && !isCompatibleAndBetter(existingActor, corefActor))
					continue;

				// Deal with cases where we know the paired actor for this thing but it hasn't yet
				//  been made into a composite actor because it doesn't know what kind of agent it is
				// Don't allow it to be turned an UNKNOWN paired actor via coref
				if (CompositeActorMention_ptr cam = boost::dynamic_pointer_cast<CompositeActorMention>(corefActor)) {
					if (cam->getPairedActorId().isNull() && knownPairedActor.actorMention) {
						if (ProperNounActorMention_ptr p = boost::dynamic_pointer_cast<ProperNounActorMention>(knownPairedActor.actorMention)) {
							if (!p->getActorId().isNull()) {
								continue;
							}
						}
					}
				}

				if (corefActor == ActorMention_ptr())
					continue;
				int coref_score = 0;

				if (corefMention->getMentionType() == Mention::NAME)
					coref_score += 10000;
				
				if (ProperNounActorMention_ptr p = boost::dynamic_pointer_cast<ProperNounActorMention>(corefActor))
					coref_score += 5000;

				if (corefMentionUID.sentno() == sentno)
					coref_score += 2000;
				else if (corefMentionUID.sentno() < sentno)
					coref_score += 1000;

				if (corefMention->getMentionType() == Mention::DESC)
					coref_score += 100;		

				if (corefMentionUID.sentno() < sentno) {
					int diff = sentno - corefMentionUID.sentno();
					if (diff > 8)
						diff = 8;
					coref_score += (10 - diff);
				}
								
				if (coref_score > score) {
					bestMatch = corefActor;
					bestMention = corefMention;
					score = coref_score;
				} 
			}

			if (bestMatch) {
				for (int m2=0; m2<ent->getNMentions(); ++m2) {
					MentionUID corefMentionUID = ent->getMention(m2);
					if (corefMentionUID.sentno() >= sent_cutoff) continue;
					Mention *corefMention = entitySet->getMention(corefMentionUID);
					ActorMention_ptr corefActor = result->find(corefMentionUID);

					// If we can find a compatible but better match than our otherwise best match, use that instead
					// Otherwise sometimes we lose some stuff
					if (corefActor && isCompatibleAndBetter(bestMatch, corefActor)) {
						bestMatch = corefActor;
						bestMention = corefMention;
					} 						
				}
			}

			if (bestMatch != ActorMention_ptr()) {
				
				// Copy the actor mention to its coreferent mention.
				ActorMention_ptr newActorMention = bestMatch->copyWithNewEntityMention(sentTheory, mention, L"COREF");
				newActorMentions.addActorMention(newActorMention);

				if (_verbosity > 1) {
					std::wostringstream msg;
					msg << L"    Adding ActorMention: " << newActorMention << L" for \"" 
						<< mention->toCasedTextString() << L"\" because it is coreferent with \""
						<< bestMention->toCasedTextString() << L"\"";
					SessionLogger::info("ICEWS") << msg.str();
				}
			}
		}
	}
	BOOST_FOREACH(ActorMention_ptr actorMention, newActorMentions.getAll()) {	
		addActorMention(actorMention, result);
	}
}

void ActorMentionFinder::findAgentCodeForEntity(const Entity *ent, ActorMentionSet *currentActorMentionSet, int sent_cutoff, AgentId& id, Symbol& code) {

	if (ent == 0)
		return;

	for (int m=0; m<ent->getNMentions(); ++m) {
		MentionUID mentionUID = ent->getMention(m);
		int sentno = mentionUID.sentno();
		if (sentno >= sent_cutoff) continue;
		ActorMention_ptr existingActor = currentActorMentionSet->find(mentionUID);
		if (existingActor == ActorMention_ptr())
			continue;
		if (CompositeActorMention_ptr p = boost::dynamic_pointer_cast<CompositeActorMention>(existingActor)) {
			id = p->getPairedAgentId();
			code = p->getPairedAgentCode();
		}
	}

}

// Should this be restricted to PER.Individual, or should it apply to all people?
void ActorMentionFinder::labelPeople(const DocTheory* docTheory, ActorMentionSet *result, 
									 int sent_cutoff, const AgentActorPairs &agentsOfProperNounActors, 
									 const AgentActorPairs &agentsOfCompositeActors, bool allow_unknowns) 
{
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Adding default person actor mentions";

	EntitySet *entitySet = docTheory->getEntitySet();

	for (int i=0; i<entitySet->getNEntities(); ++i) {
		const Entity *ent = entitySet->getEntity(i);
		EntitySubtype entitySubtype = entitySet->guessEntitySubtype(ent);

		AgentId newAgentID = _actorInfo->getDefaultPersonAgentId();
		Symbol newAgentCode = _actorInfo->getDefaultPersonAgentCode();
		findAgentCodeForEntity(ent, result, sent_cutoff, newAgentID, newAgentCode);		

		if (ent->getType().matchesPER())
		{
			//if ((entitySubtype.getName() != INDIVIDUAL_SYM) &&
			//	(entitySubtype.getName() != UNDET_SYM)) continue;
			for (int m=0; m<ent->getNMentions(); ++m) {
				MentionUID mentionUID = ent->getMention(m);
				int sentno = mentionUID.sentno();
				if (sentno >= sent_cutoff) continue;
				if (result->find(mentionUID) == ActorMention_ptr()) {
					ActorMention_ptr actor;
					const Mention *mention = entitySet->getMention(mentionUID);
					const SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
					// Check if there is any actor paired with this mention.
					PairedActorMention pairedActor = findActorForAgent(mention, agentsOfProperNounActors, docTheory);
					if (!pairedActor.actorMention)
						pairedActor = findActorForAgent(mention, agentsOfCompositeActors, docTheory);
					// Build the actor.  If the mention is paired with a country, then
					// it should be a citizen of that country; if the mention is paired
					// with a composite actor, then we just copy that actor; otherwise, 
					// make it a citizen of an unknown actor.
					if (ProperNounActorMention_ptr p = boost::dynamic_pointer_cast<ProperNounActorMention>(pairedActor.actorMention)) {
						if (_actorInfo->isACountry(p->getActorId())) {
							actor = boost::make_shared<CompositeActorMention>(
								sentTheory, mention, UNLABELED_PERSON_SYM, 
								ActorMention::AgentIdentifiers(newAgentID, _actorInfo->getAgentName(newAgentID), newAgentCode, AgentPatternId()), 
								p, pairedActor.agentActorPatternName);
							actor->addSourceNote(L"AGENT-OF-COUNTRY");
						} else if (!_actorInfo->isAnIndividual(p->getActorId())) {
							if (!pairedActor.isTemporary)
								actor = p->copyWithNewEntityMention(sentTheory, mention, L"AGENT-OF-ORG:UNLABELED_PERSON");
						}
					} else if (CompositeActorMention_ptr p = boost::dynamic_pointer_cast<CompositeActorMention>(pairedActor.actorMention)) {
						// <X for Y for Z> -> <X for Z>
						if (!pairedActor.isTemporary)
							actor = p->copyWithNewEntityMention(sentTheory, mention, L"AGENT-OF-AGENT:UNLABELED_PERSON");
					}
					// Don't assign default citizenship to partitives
					if (!actor) {
						if (!allow_unknowns)
							continue;
						if (mention->getMentionType() == Mention::PART) {
							if (_verbosity > 1) {
								std::wostringstream msg;
								msg << L"    Not adding default citizen ActorMention for \""
									<< mention->toCasedTextString() << L"\", because it's a partitive";
								SessionLogger::info("ICEWS") << msg.str();
							}
							continue;
						}
						pairedActor.agentActorPatternName = PERSON_IS_CITIZEN_OF_UNKNOWN_ACTOR_SYM;
						actor = boost::make_shared<CompositeActorMention>(
							sentTheory, mention, UNLABELED_PERSON_SYM,
							ActorMention::AgentIdentifiers(newAgentID, _actorInfo->getAgentName(newAgentID), newAgentCode, AgentPatternId()), 
							ProperNounActorMention_ptr(), pairedActor.agentActorPatternName);
					}
					addActorMention(actor, result);
					if (_verbosity > 1) {
						std::wostringstream msg;
						msg << L"    Adding ActorMention: " << actor << L" for \""
							<< mention->toCasedTextString() << L"\", because it's an unlabeled person matching "
							<< pairedActor.agentActorPatternName.to_string();
						if (pairedActor.agentActorPatternName != PERSON_IS_CITIZEN_OF_UNKNOWN_ACTOR_SYM) 
							msg << " with paired actor " << ActorMention_ptr(pairedActor.actorMention);
						SessionLogger::info("ICEWS") << msg.str();
					}
				}
			}
		}
	}
}

// THIS FUNCTION IS NOT CURRENTLY CALLED
void ActorMentionFinder::labelOrganizations(const DocTheory* docTheory, ActorMentionSet *result, 
									 int sent_cutoff, const AgentActorPairs &agentsOfProperNounActors, 
									 const AgentActorPairs &agentsOfCompositeActors) 
{
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Adding default person actor mentions";

	EntitySet *entitySet = docTheory->getEntitySet();

	for (int i=0; i<entitySet->getNEntities(); ++i) {
		const Entity *ent = entitySet->getEntity(i);
		EntitySubtype entitySubtype = entitySet->guessEntitySubtype(ent);

		if (!ent->getType().matchesORG())
			continue;

		AgentId newAgentID = AgentId();
		Symbol newAgentCode = Symbol();
		findAgentCodeForEntity(ent, result, sent_cutoff, newAgentID, newAgentCode);	

		for (int m=0; m<ent->getNMentions(); ++m) {
			MentionUID mentionUID = ent->getMention(m);
			int sentno = mentionUID.sentno();
			if (sentno >= sent_cutoff) continue;
			if (result->find(mentionUID) == ActorMention_ptr()) {
				ActorMention_ptr actor;
				const Mention *mention = entitySet->getMention(mentionUID);
				const SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
				// Check if there is any actor paired with this mention.
				PairedActorMention pairedActor = findActorForAgent(mention, agentsOfProperNounActors, docTheory);
				if (!pairedActor.actorMention)
					pairedActor = findActorForAgent(mention, agentsOfCompositeActors, docTheory);				
				if (ProperNounActorMention_ptr p = boost::dynamic_pointer_cast<ProperNounActorMention>(pairedActor.actorMention)) {
					if (_actorInfo->isACountry(p->getActorId())) {
						// If this is a country, we need an agent type
						if (newAgentID.isNull())
							continue;
						else {
							actor = boost::make_shared<CompositeActorMention>(
								sentTheory, mention, Symbol(L"UNLABELED_ORG"), 
								ActorMention::AgentIdentifiers(newAgentID, _actorInfo->getAgentName(newAgentID), newAgentCode, AgentPatternId()), 
								p, pairedActor.agentActorPatternName);
							actor->addSourceNote(L"AGENT-OF-COUNTRY");
						}
					} else if (!pairedActor.isTemporary) {
						actor = p->copyWithNewEntityMention(sentTheory, mention, L"AGENT-OF-ORG:UNLABELED_ORG");
					}
				} else if (CompositeActorMention_ptr p = boost::dynamic_pointer_cast<CompositeActorMention>(pairedActor.actorMention)) {
					// <X for Y for Z> -> <X for Z>
					if (!pairedActor.isTemporary)
						actor = p->copyWithNewEntityMention(sentTheory, mention, L"AGENT-OF-AGENT:UNLABELED_ORG");
				}
				if (actor) {
					addActorMention(actor, result);
					if (_verbosity > 1) {
						SessionLogger::info("ICEWS") << L"    Adding ActorMention: " << actor << L" for \""
						<< mention->toCasedTextString() << L"\", because it's an unlabeled org with paired actor "
						<< ActorMention_ptr(pairedActor.actorMention);
					}
				}
			}
		}
	}
}

void ActorMentionFinder::labelLocationsAndFacilities(const DocTheory* docTheory, ActorMentionSet *result, 
													 int sent_cutoff, ProperNounActorMention_ptr defaultCountryActorMention,
													 const AgentActorPairs &agentsOfProperNounActors, 
													 const AgentActorPairs &agentsOfCompositeActors,
													 std::map<MentionUID, Symbol>& blockedMentions) 
{
	if (!defaultCountryActorMention) 
		return; // Nothing to do!
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Adding default location & facility actor mentions";
	EntitySet *entitySet = docTheory->getEntitySet();
	for (int i=0; i<entitySet->getNEntities(); ++i) {
		const Entity *ent = entitySet->getEntity(i);

		AgentId newAgentID = AgentId();
		Symbol newAgentCode = Symbol();
		findAgentCodeForEntity(ent, result, sent_cutoff, newAgentID, newAgentCode);		

		if (ent->getType().matchesLOC() || ent->getType().matchesFAC())
		{
			for (int m=0; m<ent->getNMentions(); ++m) {
				MentionUID mentionUID = ent->getMention(m);
				if (mentionUID.sentno() >= sent_cutoff) continue;

				if (blockedMentions.find(mentionUID) != blockedMentions.end()) {
					if (_verbosity > 3) {
						const Mention *mention = entitySet->getMention(mentionUID);
						SessionLogger::info("ICEWS") << "    * Default country blocked for location/facility: \"" 
							<< entitySet->getMention(mentionUID)->toCasedTextString()
							<< "\n        Blocked by pattern: \""
							<< blockedMentions[mentionUID] << "\"";
					}
					continue;
				}
				if (!result->find(mentionUID)) {
					ActorMention_ptr actor;
					const Mention *mention = entitySet->getMention(mentionUID);

					// NOT CURRENTLY NEEDED SINCE WE AREN'T DOING GPEs... Non-named GPEs are too dicey
					//if (ent->getType().matchesGPE() && mention->getMentionType() != Mention::NAME)
					//	continue;

					const SentenceTheory *sentTheory = docTheory->getSentenceTheory(mentionUID.sentno());
					// Check if there is any actor paired with this mention.
					PairedActorMention pairedActor = findActorForAgent(mention, agentsOfProperNounActors, docTheory);
					if ((!pairedActor.actorMention) || pairedActor.isTemporary)
						pairedActor = findActorForAgent(mention, agentsOfCompositeActors, docTheory);
					if (pairedActor.isTemporary)
						pairedActor = PairedActorMention();
					// If the mention has a non-country paired actor, then use that as
					// our result.
					if (ProperNounActorMention_ptr p = boost::dynamic_pointer_cast<ProperNounActorMention>(pairedActor.actorMention)) {
						if (_actorInfo->isACountry(p->getActorId())) {
							if (!newAgentID.isNull()) {
								// We know the right agent for this from another sister mention
								actor = boost::make_shared<CompositeActorMention>(
									sentTheory, mention, Symbol(L"AGENT-OF-COUNTRY:UNLABELED-LOC-BUT-KNOWN-AGENT"), 
									ActorMention::AgentIdentifiers(newAgentID, _actorInfo->getAgentName(newAgentID), newAgentCode, AgentPatternId()), 
									p, pairedActor.agentActorPatternName);
							} else {
								actor = p->copyWithNewEntityMention(sentTheory, mention, L"AGENT-OF-COUNTRY:UNLABELED-LOC");
							}
						}
						else if (!_actorInfo->isAnIndividual(p->getActorId()))
							actor = p->copyWithNewEntityMention(sentTheory, mention, L"AGENT-OF-ORG:UNLABELED-LOC");
					} else if (CompositeActorMention_ptr p = boost::dynamic_pointer_cast<CompositeActorMention>(pairedActor.actorMention)) {
						// <X for Y for Z> -> <X for Z>
						actor = p->copyWithNewEntityMention(sentTheory, mention, L"AGENT-OF-AGENT:UNLABELED-LOC");
					}
					if (!actor)
						actor = defaultCountryActorMention->copyWithNewEntityMention(sentTheory, mention, L"UNLABELED-LOC");
					addActorMention(actor, result);
					if (_verbosity > 1) {
						std::wostringstream msg;
						msg << L"    Adding ActorMention: " << actor << L" for \""
							<< mention->toCasedTextString() << L"\", because it's an unlabeled location or facility";
						SessionLogger::info("ICEWS") << msg.str();
					}
				}
			}
		}
	}
}

void ActorMentionFinder::labelPartitiveMentions(DocTheory *docTheory, ActorMentionSet *result, int sent_cutoff) {
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Adding partitive actor mentions";

	// The 'newActorMentions' set is used to collect any new actor mentions
	// that we add -- we shouldn't write them directly to result, because
	//  it's dangerous to modify a set that you're currently iterating over.
	ActorMentionSet newActorMentions;

	BOOST_FOREACH(ActorMention_ptr srcActorMention, result->getAll()) {
		const Mention *srcMention = srcActorMention->getEntityMention();
		const Mention *dstMention = srcMention->getParent();
		if (dstMention && (dstMention->getMentionType() == Mention::PART)) {
			if (dstMention->getSentenceNumber() >= sent_cutoff) continue;
			// If the mention already has an ActorMention, then do nothing.
			if (ActorMention_ptr old = result->find(dstMention->getUID()))
				if (!isCompatibleAndBetter(old, srcActorMention)) continue;
			// Copy the actor mention to its coreferent mention.
			SentenceTheory *dstSentTheory = docTheory->getSentenceTheory(dstMention->getSentenceNumber());
			ActorMention_ptr dstActorMention = srcActorMention->copyWithNewEntityMention(dstSentTheory, dstMention, L"PARTITIVE");
			newActorMentions.addActorMention(dstActorMention);

			if (_verbosity > 1) {
				std::wostringstream msg;
				msg << L"    Adding ActorMention: " << dstActorMention << L" for \"" 
					<< dstMention->toCasedTextString() << L"\" because it is a partitive of \""
					<< srcMention->toCasedTextString() << L"\"";
				SessionLogger::info("ICEWS") << msg.str();
			}
		}
	}
	BOOST_FOREACH(ActorMention_ptr actorMention, newActorMentions.getAll()) {	
		addActorMention(actorMention, result);
	}
}


int ActorMentionFinder::getSentCutoff(const DocTheory* docTheory) {
	if (_actor_event_sentence_cutoff < INT_MAX)
		return IcewsSentenceSpan::icewsSentenceNoToSerifSentenceNo(_actor_event_sentence_cutoff, docTheory);
	else
		return docTheory->getNSentences();
}

void ActorMentionFinder::discardBareActorMentions(ActorMentionSet *actorMentionSet) {
	std::vector<ActorMention_ptr> actorsToDiscard; // don't mutate container while we iterate over it
	BOOST_FOREACH(ActorMention_ptr actor, actorMentionSet->getAll()) {
		if (! (boost::dynamic_pointer_cast<ProperNounActorMention>(actor) ||
		       boost::dynamic_pointer_cast<CompositeActorMention>(actor)) ) {
			actorsToDiscard.push_back(actor);
		}
	}
	BOOST_FOREACH(ActorMention_ptr actor, actorsToDiscard) {
		SessionLogger::info("ICEWS") << "Discarding actor mentions for \""
			<< actor->getEntityMention()->toCasedTextString()
			<< "\" because it has no actor/agent info.";
		actorMentionSet->discardActorMention(actor);
	}
}

void ActorMentionFinder::forceActorMentionCreation(const DocTheory* docTheory, ActorMentionSet *actorMentionSet, int sent_cutoff) {

	if (_typesForForcedActorMentionCreation.size() == 0)
		return;

	// Create (empty) ActorMentions for all mentions of specific types
	for (int sentno=0; sentno<std::min(docTheory->getNSentences(), sent_cutoff); ++sentno) {
		const SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
		const MentionSet *mentSet = sentTheory->getMentionSet();
		for (int m = 0; m < mentSet->getNMentions(); m++) {
			const Mention *ment = mentSet->getMention(m);
			if (ment->getMentionType() == Mention::NAME ||
				ment->getMentionType() == Mention::DESC ||
				ment->getMentionType() == Mention::PRON)
			{
				if (_typesForForcedActorMentionCreation.find(ment->getEntityType().getName()) != _typesForForcedActorMentionCreation.end()) {
					ActorMention_ptr actor = actorMentionSet->find(ment->getUID());
					if (actor == ActorMention_ptr()) {
						actor = boost::make_shared<ActorMention>(sentTheory, ment, UNKNOWN_ACTOR_SYM);
						actorMentionSet->addActorMention(actor);
					}
				}
			}
		}
	}
}

namespace {
}

bool ActorMentionFinder::isUSState(Mention* mention) {
	std::wstring mention_text = mention->node->toTextString();
	boost::replace_all(mention_text, L".", "");
	boost::trim(mention_text);
	return (_us_state_names.find(mention_text) != _us_state_names.end());
}

void ActorMentionFinder::findUSCities(DocTheory* docTheory, SortedActorMentions& result, int sent_cutoff) {
	if (_us_state_names.empty()) return;
	if (_usa_actorid.isNull()) return;

	static const Symbol::SymbolGroup okRootTags = Symbol::makeSymbolGroup(L"FRAGMENTS FRAG NPA NP NPP");

	// For now, we just handle the special (but fairly common) case of a US city 
	// appearing in its own sentence (usually at the beginning of a document).
	// In particular, we handle the following cases:
    //        (FRAGMENTS^ (NP^ (NPP^ (NNP^ x)...) (, ,)) (NPP (NNP^ x)))
    //             (FRAG^ (NPA (NPP^ (NNP^ x)...) (, ,)  (NPP (NNP^ x))) (.^ x))
    //                   (NPA^ (NPP^ (NNP^ x)...) (, ,)  (NPP (NNP^ x))  (. x))
    //                    (NP^ (NPP^ (NNP^ x)...) (, ,)  (NPP (NNP^ x))  (. x))	
	Gazetteer::GeoResolution_ptr usa_geo_resolution = _gazetteer->getCountryResolution(L"us");
	for (int sentno=0; sentno<std::min(docTheory->getNSentences(), sent_cutoff); ++sentno) {
		const SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
		const MentionSet *mset = sentTheory->getMentionSet();
		const SynNode *root = sentTheory->getPrimaryParse()->getRoot();

		if (mset->getNMentions() != 3) continue;
		if (okRootTags.find(root->getTag()) == okRootTags.end()) continue;
		if (mset->getMention(0)->getMentionType() != Mention::NAME) continue;
		if (mset->getMention(1)->getMentionType() != Mention::NONE) continue;
		if (mset->getMention(2)->getMentionType() != Mention::NAME) continue;
		if (mset->getMention(0)->getChild() != mset->getMention(1)) continue;
		// Now make sure that the final mention is a state name.
		if (!isUSState(mset->getMention(2))) continue;
		// Ok, everything looks good!
		const Mention* cityMention = mset->getMention(0);
		std::vector<std::wstring> canonicalCityNames = _gazetteer->toCanonicalForms(sentTheory, cityMention);
		const Mention* stateMention = mset->getMention(2);
		std::vector<std::wstring> canonicalStateNames = _gazetteer->toCanonicalForms(sentTheory, stateMention);
	
		Symbol stateCode;
		Gazetteer::GeoResolution_ptr stateRes = _locationMentionResolver->getResolutionInRegion(sentTheory, stateMention, Symbol(L"US"));
		if (stateRes->isEmpty) {
			// Strangely, Colorado is not in the gazetteer :(			
			stateRes = usa_geo_resolution;
			if (std::find(canonicalStateNames.begin(), canonicalStateNames.end(), L"colorado") != canonicalStateNames.end() ||
				std::find(canonicalStateNames.begin(), canonicalStateNames.end(), L"colo .") != canonicalStateNames.end()) 
			{
				stateCode = Symbol(L"CO");
			} else continue; // we have no idea what's going on here
		} else {
			stateCode = _gazetteer->getGeoRegion(stateRes->geonameid);
		}
			
		Gazetteer::GeoResolution_ptr cityRes = _locationMentionResolver->getResolutionInRegion(sentTheory, cityMention, Symbol(L"US"), stateCode);
		if (cityRes->isEmpty) {
			// special case for georgia, because the state and country often get confused -> if no city is found in the US state of Georgia, 
			//    don't resolve either city or state mentions
			if (stateMention->getNode()->getHeadWord() == GEORGIA_SYM)
				continue;
			// If we can't find a resolution for the city itself, just use the state resolution				
			cityRes = stateRes;
		}

		ActorMention::ActorIdentifiers usa(_usa_actorid, _actorInfo->getActorName(_usa_actorid), USA_ACTOR_CODE);

		ActorMention_ptr cityActor;
		ActorId cityGeonameActorId = _actorInfo->getActorIdForGeonameId(cityRes->geonameid);
		if (cityGeonameActorId.isNull())
			cityActor = boost::make_shared<ProperNounActorMention>(
				sentTheory, cityMention, US_CITY_SYM, usa, cityRes);
		else
			cityActor = boost::make_shared<ProperNounActorMention>(
				sentTheory, cityMention, US_CITY_SYM, 
				ActorMention::ActorIdentifiers(cityGeonameActorId, _actorInfo->getActorName(cityGeonameActorId), Symbol(), ActorPatternId()), 
				cityRes);

		ActorMention_ptr stateActor;
		ActorId stateGeonameActorId = _actorInfo->getActorIdForGeonameId(stateRes->geonameid);
		if (stateGeonameActorId.isNull())
			stateActor = boost::make_shared<ProperNounActorMention>(
				sentTheory, stateMention, US_CITY_SYM, usa, stateRes);
		else
			stateActor = boost::make_shared<ProperNounActorMention>(
				sentTheory, stateMention, US_CITY_SYM, 
				ActorMention::ActorIdentifiers(stateGeonameActorId, _actorInfo->getActorName(stateGeonameActorId), Symbol(), ActorPatternId()), 
				stateRes);

		// add actors to icews actor set
		result.insert(ScoredActorMention(US_CITY_SCORE, cityActor));
		result.insert(ScoredActorMention(US_CITY_SCORE, stateActor));
		// register resolutions so that they can be used later on
		_locationMentionResolver->registerResolutions(canonicalCityNames, std::make_pair(US_CITY_SCORE, cityRes));
		_locationMentionResolver->registerResolutions(canonicalStateNames, std::make_pair(US_CITY_SCORE, stateRes));

		// Propagate to other mentions of this place; this should always take precedence
		const Entity *ent = docTheory->getEntitySet()->getEntityByMention(cityMention->getUID());
		if (ent != 0) {
			for (int m=0; m<ent->getNMentions(); ++m) {
				MentionUID propagatedMentionUID = ent->getMention(m);
				if (propagatedMentionUID == cityMention->getUID())
					continue;
				int new_sentno = propagatedMentionUID.sentno();
				if (new_sentno >= sent_cutoff) continue;
				const Mention *propagatedMention = docTheory->getEntitySet()->getMention(propagatedMentionUID);
				const SentenceTheory *st = docTheory->getSentenceTheory(new_sentno);
				
				ActorMention_ptr propagatedActor;

				if (cityGeonameActorId.isNull())
					propagatedActor = boost::make_shared<ProperNounActorMention>(
						st, propagatedMention, US_CITY_SYM, usa, cityRes);
				else
					propagatedActor = boost::make_shared<ProperNounActorMention>(
						st, propagatedMention, US_CITY_SYM, 
						ActorMention::ActorIdentifiers(cityGeonameActorId, _actorInfo->getActorName(cityGeonameActorId), Symbol(), ActorPatternId()), 
						cityRes);

				result.insert(ScoredActorMention(US_CITY_SCORE, propagatedActor));
			}
		}
	}
}

void ActorMentionFinder::resolveAmbiguousLocationsToDefaultCountry(ProperNounActorMention_ptr defaultCountryActorMention, const DocTheory* docTheory, ActorMentionSet *result, int sent_cutoff) {
	if (_verbosity > 3)
		SessionLogger::info("ICEWS") << "  Resolving ambiguous locations to default country for doc.";

	Gazetteer::GeoResolution_ptr docResolution = defaultCountryActorMention->getGeoResolution();
	if (!docResolution->isEmpty)
	{
		const Gazetteer::CountryInfo_ptr countryInfo = docResolution->countryInfo;
		SortedActorMentions defaultActorMentions;
		for (size_t sentno=0; sentno<static_cast<size_t>(sent_cutoff); ++sentno) {
			const SentenceTheory *sentTheory = docTheory->getSentenceTheory(static_cast<int>(sentno));
			const MentionSet *ms = sentTheory->getMentionSet();
			for (int m = 0; m < ms->getNMentions(); m++) {
				const Mention *ment = ms->getMention(m);
				EntityType entityType = ment->getEntityType();
				if ((ment->getMentionType() == Mention::NAME) && (entityType.matchesLOC() || entityType.matchesGPE())) {
					std::wstring head_string = ment->getAtomicHead()->toTextString();
					std::wstring full_string = ment->toCasedTextString();
					boost::trim(head_string);
					boost::trim(full_string);
					if (_locationMentionResolver->isBlockedLocation(sentTheory, ment)) continue;
					ActorMention_ptr target = result->find(ment->getUID());
					if (target) 
					{	
						// Check if another country is explicitly mentioned in this sentence, if so block resolution
						ProperNounActorMention_ptr otherCountry = defaultCountryAssignmentIsBlockedByOtherCountry(target, defaultCountryActorMention, result);
						if (otherCountry) continue;
						if (ProperNounActorMention_ptr pnActor = boost::dynamic_pointer_cast<ProperNounActorMention>(target)) {
							if(pnActor->isResolvedGeo()) continue;
							else {
								pnActor->setGeoResolution(docResolution);
								if (_verbosity > 3)
									SessionLogger::info("ICEWS") << "  Default country assigned to ...";
							}
						}
					} 
					else {
						ProperNounActorMention_ptr actorMention = boost::make_shared<ProperNounActorMention>(
							sentTheory, ment, GAZETTEER_SYM,
							ActorMention::ActorIdentifiers(countryInfo->actorId, _actorInfo->getActorName(countryInfo->actorId), countryInfo->actorCode, ActorPatternId()),
							docResolution);
						defaultActorMentions.insert(ScoredActorMention(50, actorMention));
					}
				}
			}
		}
		greedilyAddActorMentions(defaultActorMentions, result);
	}
}

const Mention* ActorMentionFinder::getCoveringNameDescMention(const SentenceTheory* sentTheory, int start_tok, int end_tok) {
	const SynNode *parseRoot = sentTheory->getPrimaryParse()->getRoot();
	const SynNode *node = parseRoot->getCoveringNodeFromTokenSpan(start_tok, end_tok);
	return getCoveringNameDescMention(sentTheory, node);
}

const Mention* ActorMentionFinder::getCoveringNameDescMention(const SentenceTheory* sentTheory, const SynNode* node) {
	const MentionSet *ms = sentTheory->getMentionSet();
	// try to find the lowest mention of type DESC/NAME, while walking up the head chain
	while (node) {		
		if (node->hasMention()) {
			const Mention *ment = ms->getMentionByNode(node);
			if (ment->getMentionType() == Mention::DESC || ment->getMentionType() == Mention::NAME)
				return ment;
		}
		// try to go up the head chain; if not, break
		node = node->isHeadChild() ? node->getParent() : 0;
	}
	return 0;
}

void ActorMentionFinder::resetForNewDocument() {
	_sentenceActorMatches.clear();
	_locationMentionResolver->clear();
}

ActorMentionSet* ActorMentionFinder::process(const SentenceTheory *sentTheory, const DocTheory *docTheory) {

	if (_mode != ACTOR_MATCH) {
		throw UnexpectedInputException("ActorMentionFinder::process",
			"This stage can only be run in ACTOR_MATCH mode.");
	}

	const MentionSet *mentionSet = sentTheory->getMentionSet();
	ActorMentionSet *ams = new ActorMentionSet();

	if (_only_match_names && sentTheory->getNameTheory()->getNNameSpans() == 0) {
		assert((int)_sentenceActorMatches.size() == sentTheory->getSentNumber());
		std::vector<ActorMatch> actorMatches;
		_sentenceActorMatches.push_back(actorMatches);
		return ams;
	}

	std::vector<ActorMatch> actorMatches = _actorTokenMatcher->findAllMatches(sentTheory);

	assert((int)_sentenceActorMatches.size() == sentTheory->getSentNumber());
	_sentenceActorMatches.push_back(actorMatches);

	BOOST_FOREACH(ActorMatch match, actorMatches) {
		bool head_match = false;
		bool country_the_plural = false;

		ActorMention_ptr actorMention = createProperNounActorMention(
			match, sentTheory, ACTOR_PATTERN_SYM, head_match, country_the_plural);

		ProperNounActorMention_ptr pnActorMention = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention);
		if (!pnActorMention)
			continue;

		const Mention *mention = pnActorMention->getEntityMention();

		if (mention->getEntityType() == EntityType::getUndetType() || mention->getEntityType() == EntityType::getOtherType())
			continue;

		if (match.isAcronymMatch && mention->getEntityType().matchesPER() && _actorInfo->isAnOrganization(pnActorMention->getActorId()))
			continue;

		if (_require_entity_type_match && !entityTypeMatches(pnActorMention, mention->getEntityType()))
			continue;

		if (_only_match_names && mention->getMentionType() != Mention::NAME)
			continue;

		// match.weight is the pattern confidence in this context
		if (match.weight < 1 && !entityTypeMatches(pnActorMention, mention->getEntityType()))
			continue;

		const SynNode *node = mention->getAtomicHead();
		bool same_start_tok = (node->getStartToken() == match.start_token);
		bool same_end_tok = (node->getEndToken() == match.end_token);

		// Calculate score indicating how closely this pattern matches, can nullify actorMention in ACTOR_MATCH mode (which we are in)
		double score = scoreProperNounActorMention(&actorMention, match, docTheory, sentTheory, _sentenceActorMatches, head_match, country_the_plural, false);
		if (!actorMention)
			continue;

		pnActorMention->setPatternMatchScore(score);
		pnActorMention->setPatternConfidenceScore(match.weight);
		pnActorMention->setImportanceScore(_actorInfo->getImportanceScoreForActor(pnActorMention->getActorId()));

		ams->appendActorMention(pnActorMention);
	}

	// Edit Distance mapping
	for (int m = 0; m < mentionSet->getNMentions(); m++) {
		const Mention *ment = mentionSet->getMention(m);
		EntityType entityType = ment->getEntityType();
		if (ment->getMentionType() == Mention::NAME) {
			EntityType entityType = ment->getEntityType();
			std::vector<ActorMention_ptr> existingActors = ams->findAll(ment->getUID());
			
			double threshold = _actorEntityScorer->getLowEditDistanceThreshold(entityType);
			if (hasGoodResolution(existingActors) || entityType.matchesGPE() || entityType.matchesLOC())
				threshold = 0.9; // If we already have a good match, we want to be more careful about proposing more matches, regardless of scoring thresholds 

			if (threshold >= 1.0)
				continue;

			std::map<ActorId, double> closeActors = _actorEditDistance->findCloseActors(ment, threshold, sentTheory, docTheory);
			if (closeActors.size() == 0)
				continue;

			std::map<ActorId, double>::const_iterator iter;
			for (iter = closeActors.begin(); iter != closeActors.end(); ++iter) {
				ActorId aid = iter->first;
				double edit_distance = iter->second;

				if (!actorAlreadyFound(aid, existingActors)) {
					ProperNounActorMention_ptr actorMention = 
						boost::make_shared<ProperNounActorMention>(sentTheory, ment, EDIT_DISTANCE, 
						                                           ActorMention::ActorIdentifiers(aid, _actorInfo->getActorName(aid), Symbol(), ActorPatternId()));
					actorMention->setEditDistanceScore(edit_distance);
					actorMention->setImportanceScore(_actorInfo->getImportanceScoreForActor(actorMention->getActorId()));
					ams->appendActorMention(actorMention);
				}
			}
		}
	} 

	// Get all possible location resolutions
	SortedActorMentions newLocationActorMentions;
	resolveNamedLocations(sentTheory, actorMatches, ams, newLocationActorMentions, 0, ALL_RESOLUTIONS);
	BOOST_FOREACH(ScoredActorMention sam, newLocationActorMentions) {
		ActorMention_ptr actorMention = sam.second;
		ProperNounActorMention_ptr pnActorMention = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention);
		if (!pnActorMention)
			continue;
		pnActorMention->setGeoresolutionScore(sam.first); 
		pnActorMention->setImportanceScore(_actorInfo->getImportanceScoreForActor(pnActorMention->getActorId()));
		ams->appendActorMention(pnActorMention);
	}

	// Token Subset Tree mapping, based on unambiguous leafs
	for (int m = 0; m < mentionSet->getNMentions(); m++) {
		const Mention *ment = mentionSet->getMention(m);
		EntityType entityType = ment->getEntityType();
		if (ment->getMentionType() == Mention::NAME && 
			(ment->getEntityType().matchesPER() || ment->getEntityType().matchesORG()))
		{
			std::vector<ActorMention_ptr> existingActors = ams->findAll(ment->getUID());

			if (hasGoodResolution(existingActors))
				continue;

			std::map<ActorId, double> equivalentActors = _actorTokenSubsetTrees->getTSTEqNames(ment);
			if (equivalentActors.size() == 0) 
				continue;

			std::map<ActorId, double>::const_iterator iter;
			for (iter = equivalentActors.begin(); iter != equivalentActors.end(); ++iter) {
				ActorId aid = iter->first;
				double score = iter->second;

				ProperNounActorMention_ptr actorMention = boost::make_shared<ProperNounActorMention>(sentTheory, ment, TOKEN_SUBSET_TREE, ActorMention::ActorIdentifiers(aid, _actorInfo->getActorName(aid), Symbol(), ActorPatternId()));
				actorMention->setEditDistanceScore(score);
				actorMention->setImportanceScore(_actorInfo->getImportanceScoreForActor(actorMention->getActorId()));
				ams->appendActorMention(actorMention);
			}
		}
	}

	return ams;
}

bool ActorMentionFinder::actorAlreadyFound(ActorId aid, std::vector<ActorMention_ptr> &actorMentions) {
	BOOST_FOREACH(ActorMention_ptr actorMention, actorMentions) {
		ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention);
		if (!pnam)
			continue;
		if (pnam->getActorId() == aid)
			return true;
	}
	return false;
}

bool ActorMentionFinder::actorAlreadyFound(ActorId aid, const Mention *mention, ActorMentionFinder::SortedActorMentions sortedActorMentions) {
	for (SortedActorMentions::const_iterator it = sortedActorMentions.begin(); it != sortedActorMentions.end(); ++it) {
		const ActorMention_ptr& actorMention = (*it).second;
		if (actorMention->getEntityMentionUID() != mention->getUID())
			continue;
		ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention);
		if (!pnam)
			continue;
		if (aid == pnam->getActorId())
			return true;
	}
	return false;
}

void ActorMentionFinder::resetForNewSentence() { }

bool ActorMentionFinder::entityTypeMatches(ProperNounActorMention_ptr pnActorMention, EntityType entityType) {

	if (entityType.matchesGPE() || entityType.matchesLOC()) {
		if (_actorInfo->mightBeALocation(pnActorMention->getActorId()))
			return true;
		else
			return false;
    }

	if (entityType.matchesORG()) {
		if (_actorInfo->isAnOrganization(pnActorMention->getActorId()))
			return true;
		else
			return false;
    }

	if (entityType.matchesPER()) {
		if (_actorInfo->isAPerson(pnActorMention->getActorId()))
			return true;
		else
			return false;
    }

	if (entityType.matchesFAC()) {
		if (_actorInfo->isAFacility(pnActorMention->getActorId()) || (_allow_fac_org_matches && _actorInfo->isAnOrganization(pnActorMention->getActorId())))
			return true;
		else
			return false;
    }

	return false;
}

/********************
 * doc-actors stage *
 ********************/
void ActorMentionFinder::doDocActors(DocTheory *docTheory) {
	if (!_do_doc_actors)
		return;
	
	if (_mode != DOC_ACTORS) {
		throw UnexpectedInputException("ActorMentionFinder::doDocActors",
			"This stage can only be run in DOC_ACTORS mode.");
	}

	// Collect all ActorMentions, set sentenceTheories
	std::vector<ProperNounActorMention_ptr> allActorMentions;
	for (int i = 0; i < docTheory->getNSentences(); i++) {
		SentenceTheory *st = docTheory->getSentenceTheory(i);
		ActorMentionSet *ams = st->getActorMentionSet();

		// necessary because ActorMentions point to their sentence, 
		// but SentenceTheory objects may have been deleted since 
		// the ActorMentions were created
		ams->setSentenceTheories(st); 

		std::vector<ActorMention_ptr> amList = ams->getAll();
		BOOST_FOREACH(ActorMention_ptr am, amList) {
			if (ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(am))
				allActorMentions.push_back(pnam);
		}
	}

	// Calculate some document-level statistics that are used to determine the 
	// scores of individual matches.
	std::string publicationDate = Stories::getStoryPublicationDate(docTheory->getDocument());
	const char* publicationDateCStr = (publicationDate.empty()?0:publicationDate.c_str());

	// Set georesolution scores
	docActorsLocationResolution(docTheory, publicationDateCStr);

	// Choose default country
	std::vector<ActorMention_ptr> reliableActorMentions = getCurrentReliableActorMentions(docTheory);
	std::vector<ActorMention_ptr> temp(reliableActorMentions.begin(), reliableActorMentions.end());
	ProperNounActorMention_ptr defaultCountryActorMention = getDefaultCountryActorMention(temp);
	if (defaultCountryActorMention) {
		DocumentCountryActor *documentCountryActor = _new DocumentCountryActor(defaultCountryActorMention->getActorId());
		DocumentActorInfo *documentActorInfo = _new DocumentActorInfo();

		documentActorInfo->takeDocumentCountryActor(documentCountryActor);
		docTheory->takeDocumentActorInfo(documentActorInfo);
	}

	// Set association scores
	BOOST_FOREACH(ProperNounActorMention_ptr am, allActorMentions)
		am->setAssociationScore(getAssociationScore(am, publicationDateCStr));

	// Turn ActorMentions into ActorEntitys and score ActorEntitys
	ActorEntitySet *aes = _actorEntityScorer->createActorEntitySet(docTheory, allActorMentions, defaultCountryActorMention);

	docTheory->takeActorEntitySet(aes);
}

void ActorMentionFinder::docActorsLocationResolution(DocTheory *docTheory, const char *publicationDate) 
{
	clearDocumentCountryCounts();

	// We'll redo the geo scores now that we have more info
	for (int i = 0; i < docTheory->getNSentences(); i++) {
		const SentenceTheory *st = docTheory->getSentenceTheory(i);
		ActorMentionSet *ams = st->getActorMentionSet();
		BOOST_FOREACH(ActorMention_ptr am, ams->getAll()) {
			ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(am);
			if (pnam)
				pnam->setGeoresolutionScore(0.0); 
		}
	}

	docActorsResolveNamedLocations(docTheory, publicationDate, UNAMBIGUOUS);

	// US Cities 
	SortedActorMentions sortedActorMentions;
	findUSCities(docTheory, sortedActorMentions, getSentCutoff(docTheory));
	for (SortedActorMentions::const_iterator it = sortedActorMentions.begin(); it != sortedActorMentions.end(); ++it) {
		double score = (*it).first;
		const ActorMention_ptr& actorMention = (*it).second;
		ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention);
		if (!pnam)
			continue;

		const Mention *mention = actorMention->getEntityMention();

		// These are very reliable, update score of found actor match, and set all others to 0
		bool found_actor_mention = false;
		const SentenceTheory *st = docTheory->getSentenceTheory(mention->getSentenceNumber());
		ActorMentionSet *ams = st->getActorMentionSet();
		std::vector<ActorMention_ptr> actorMentions = ams->findAll(mention->getUID());
		for (size_t i = 0; i < actorMentions.size(); i++) {
			ActorMention_ptr existingActorMention = actorMentions[i];
			ProperNounActorMention_ptr existingPnam = boost::dynamic_pointer_cast<ProperNounActorMention>(existingActorMention);
			assert(existingPnam);
			if (existingPnam->getActorId() == pnam->getActorId()) {
				found_actor_mention = true;
				existingPnam->setGeoResolution(pnam->getGeoResolution());
				existingPnam->setGeoresolutionScore(score);
			} else {
				existingPnam->setPatternMatchScore(0.0);
				existingPnam->setGeoresolutionScore(0.0);
			}
		}

		if (!found_actor_mention) {
			// add it to the existing set
			pnam->setGeoresolutionScore(score);
			ams->appendActorMention(pnam);
		}
	}

	std::vector<ActorMention_ptr> reliableActorMentions = getCurrentReliableActorMentions(docTheory);

	fillDocumentCountryCounts(reliableActorMentions); 
	docActorsResolveNamedLocations(docTheory, publicationDate, BEST_RESOLUTION_DESPITE_AMBIGUITY);
}

std::vector<ActorMention_ptr> ActorMentionFinder::getCurrentReliableActorMentions(DocTheory *docTheory) {
	
	std::vector<ActorMention_ptr> result;
	for (int i = 0; i < docTheory->getNSentences(); i++) {
		const SentenceTheory *st = docTheory->getSentenceTheory(i);
		ActorMentionSet *ams = st->getActorMentionSet();
		BOOST_FOREACH(ActorMention_ptr am, ams->getAll()) {
			ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(am);
			if (!pnam)
				continue;
			if (pnam->getPatternMatchScore() > 0 || pnam->getGeoresolutionScore() > 0)
				result.push_back(pnam);
		}
	}
	return result;
}

bool ActorMentionFinder::isGeoresolvableFAC(const Mention *mention) {
	return mention->getEntityType().matchesFAC() && mention->getEntitySubtype().getName() == Symbol(L"Airport");
}

void ActorMentionFinder::docActorsResolveNamedLocations(DocTheory *docTheory, const char *publicationDate, resolution_ambiguity_t ambiguity) {

	// Make temporary new ActorMentionSet with all ActorMentions in it, as the resolver depends on it
	ActorMentionSet fullActorMentionSet;
	for (int i = 0; i < docTheory->getNSentences(); i++) {
		const SentenceTheory *st = docTheory->getSentenceTheory(i);
		ActorMentionSet *ams = st->getActorMentionSet();
		BOOST_FOREACH(ActorMention_ptr am, ams->getAll()) {
			ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(am);
			fullActorMentionSet.appendActorMention(am);
		}
	}

	// Cycle over mentions, resolving each one
	for (int i = 0; i < docTheory->getNSentences(); i++) {
		const SentenceTheory *st = docTheory->getSentenceTheory(i);
		const MentionSet *ms = st->getMentionSet();
		ActorMentionSet *ams = st->getActorMentionSet();
		for (int j = 0; j < ms->getNMentions(); j++) {
			const Mention *mention = ms->getMention(j);
			EntityType entityType = mention->getEntityType();

			if (mention->getMentionType() != Mention::NAME || 
				(!entityType.matchesLOC() && !entityType.matchesGPE() && (!_georesolve_facs || !isGeoresolvableFAC(mention))))
			{
				continue;
			}

			Gazetteer::ScoredGeoResolution resolution;

			std::vector<ActorMention_ptr> actorMentions = ams->findAll(mention->getUID());
			if (actorMentions.size() == 0) 
				continue;

			bool is_georgia = (mention->getNode()->getHeadWord() == GEORGIA_SYM || mention->getNode()->getHeadWord() == GEORGIAN_SYM);

			// Special case for Georgia, the ambiguous country name, as always
			if (!is_georgia && hasGoodResolution(actorMentions))
				continue;

			// gather associated countries
			std::vector<CountryId> associatedCountries;
			if (ambiguity == UNAMBIGUOUS) {
				BOOST_FOREACH(ActorMention_ptr am, actorMentions) {
					ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(am);
					assert(pnam);
					if (_actorInfo->isACountry(pnam->getActorId())) {
						associatedCountries.push_back(_actorInfo->getCountryId(pnam->getActorId()));
					} else {
						associatedCountries = _actorInfo->getAssociatedCountryIds(pnam->getActorId(), publicationDate);
					}
				}
			}

			// resolve!
			if (ambiguity == UNAMBIGUOUS) 
				resolution = _locationMentionResolver->getICEWSLocationResolution(&fullActorMentionSet, 
					std::vector<ActorMatch>(), _countryCounts, _usa_actorid, st, mention, false, associatedCountries, _actorInfo);
			else if (ambiguity == BEST_RESOLUTION_DESPITE_AMBIGUITY)
				resolution = _locationMentionResolver->getICEWSLocationResolution(&fullActorMentionSet,
					std::vector<ActorMatch>(), _countryCounts, _usa_actorid, st, mention, true, std::vector<CountryId>(), _actorInfo);
		
			if (resolution.first == 0)
				continue;

			// locate resolved ActorMention, and assign it the resolved score
			bool found_existing_resolution = false;
			for (size_t k = 0; k < actorMentions.size(); k++) {
				ActorMention_ptr am = actorMentions[k];
				ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(am);
				assert(pnam);
				Gazetteer::GeoResolution_ptr existingGeoresolution = pnam->getGeoResolution();

				if (existingGeoresolution && 
					cityNameMatches(existingGeoresolution, resolution.second) &&
					existingGeoresolution->geonameid == resolution.second->geonameid &&
					existingGeoresolution->latitude == resolution.second->latitude &&
					existingGeoresolution->longitude == resolution.second->longitude) 
				{
					pnam->setGeoresolutionScore(resolution.first);
					found_existing_resolution = true;
					break;
				}
			}
			if (!found_existing_resolution) {
				std::wstring cityNameStr(L"Unknown city name");
				if (resolution.second->cityname != Symbol())
					cityNameStr = resolution.second->cityname.to_string();
				SessionLogger::info("GEORESOLUTION") 
					<< L"Mention: " << mention->getUID().toInt() << L" "
					<< L"Could not find georesolution match for " 
					<< cityNameStr << " "
					<< resolution.second->geonameid << " " 
					<< resolution.second->latitude << " "
					<< resolution.second->longitude;
			}
		}
	}
}

bool ActorMentionFinder::cityNameMatches(Gazetteer::GeoResolution_ptr geo1, Gazetteer::GeoResolution_ptr geo2) {
	// geo cities can be either empty strings or null depending 
	// on if they were loaded from serifxml or if they were
	// created earlier in the run.

	bool geo1_city_empty = false;
	if (geo1->cityname.is_null() || geo1->cityname == Symbol(L""))
		geo1_city_empty = true;

	bool geo2_city_empty = false;
	if (geo2->cityname.is_null() || geo2->cityname == Symbol(L""))
		geo2_city_empty = true;

	if (geo1_city_empty && geo2_city_empty)
		return true;

	return geo1->cityname == geo2->cityname;
}

bool ActorMentionFinder::hasGoodResolution(std::vector<ActorMention_ptr> &actorMentions) {
	BOOST_FOREACH(ActorMention_ptr am, actorMentions) {
		ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(am);
		if (!pnam) continue;
		if (pnam->getGeoresolutionScore() > 0.0 || pnam->getPatternMatchScore() > 0.0)
			return true;
	}
	return false;
}

bool ActorMentionFinder::hasGeoResolution(std::vector<ActorMention_ptr> &actorMentions) {
	BOOST_FOREACH(ActorMention_ptr am, actorMentions) {
		ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(am);
		if (!pnam) continue;
		if (pnam->getGeoResolution())
			return true;
	}
	return false;
}

double ActorMentionFinder::getAssociationScore(ActorMention_ptr actorMention, const char *publicationDate) {
	double score = 0;
	if (ProperNounActorMention_ptr pn_actor = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention)) {
		std::vector<ActorId> associatedCountries = _actorInfo->getAssociatedCountryActorIds(pn_actor->getActorId(), publicationDate);
		BOOST_FOREACH(ActorId ac, associatedCountries) {
			score += _countryCounts[ac];
		}
	}
	return score;
}

std::wstring ActorMentionFinder::getGeonamesId(ActorEntity_ptr ae) {
	std::vector<ProperNounActorMention_ptr> actorMentions = ae->getActorMentions();
	for (size_t i = 0; i < actorMentions.size(); i++) {
		ProperNounActorMention_ptr pnam = actorMentions[i];
		if (pnam->isNamedLocation() && pnam->isResolvedGeo()) {
			
			Gazetteer::GeoResolution_ptr geo = pnam->getGeoResolution();
			std::wstring geonameid = geo->geonameid;
			if (geo->cityname.is_null())
				continue;
			std::wstring cityname = geo->cityname.to_string();
			std::wstring actorName = _actorInfo->getActorName(pnam->getActorId());

			if (geonameid.size() != 0 && cityname.size() != 0) {
				std::transform(actorName.begin(), actorName.end(), actorName.begin(), towlower);
				std::transform(cityname.begin(), cityname.end(), cityname.begin(), towlower);

				if (actorName != cityname)
					return geonameid;
			}

		}
	}
	return L"-1";
}
