// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "ICEWS/ActorMentionFinder.h"
#include "ICEWS/ActorMentionSet.h"
#include "ICEWS/Identifiers.h"
#include "ICEWS/SentenceSpan.h"
#include "ICEWS/Stories.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/InputUtil.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PropositionSet.h"
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
	// This is the code for "Citizen" in the dict_agents table:
	Symbol CITIZEN_AGENT_CODE(L"309896b5-878d-49e0-962e-33ace57cf32b"); 
	// Special agent code used to indicate that a composite actor should be
	// treated as a direct mention of its paired actor:
	Symbol COMPOSITE_ACTOR_IS_PAIRED_ACTOR_SYM(L"COMPOSITE_ACTOR_IS_PAIRED_ACTOR");
	Symbol BLOCK_ACTOR_SYM(L"BLOCK_ACTOR");
	// These codes are from the dict_sectors table:
	Symbol MEDIA_SECTOR_CODE(L"MED");
	Symbol NEWS_SECTOR_CODE(L"133"); // should we also include "state media" here?
	// Proper noun phrase
	Symbol NPP_SYM(L"NPP");
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
	// Adjective POS
	Symbol JJ_POS_SYM(L"JJ");
	// Source note values:
	Symbol ACTOR_PATTERN_SYM(L"ACTOR_PATTERN");
	Symbol AGENT_PATTERN_SYM(L"AGENT_PATTERN");
	Symbol CITIZEN_OF_COUNTRY_SYM(L"CITIZEN_OF_COUNTRY:ACTOR_PATTERN");
	Symbol GAZETTEER_SYM(L"GAZETTEER");
	Symbol UNLABELED_PERSON_SYM(L"UNLABELED_PERSON");
	Symbol AGENT_OF_AGENT_PATTERN_SYM(L"AGENT-OF-AGENT:AGENT_PATTERN");
	Symbol COMPOSITE_ACTOR_PATTERN_SYM(L"COMPOSITE_ACTOR_PATTERN");
	Symbol PATTERN_END_IS_NOT_MENTION_END_SYM(L"PATTERN_END_IS_NOT_MENTION_END");
	Symbol COUNTRY_RESTRICTION_APPLIED_SYM(L"COUNTRY_RESTRICTION_APPLIED");
	Symbol TEMPORARY_ACTOR_MENTION_SYM(L"TEMPORARY_ACTOR_MENTION");
	Symbol LOCAL_COMPOSITE_ACRONYM_SYM(L"LOCAL_COMPOSITE_ACRONYM");
	Symbol LOCAL_PROPER_NOUN_ACRONYM_SYM(L"LOCAL_PROPER_NOUN_ACRONYM");
	Symbol US_CITY_SYM(L"US_CITY");
	// Symbol used as pattern id if a pattern is not labeled.
	Symbol UNLABELED_PATTERN_SYM(L"unlabeled-pattern");
	// Return value symbol for blocked mentions
	Symbol BLOCK_SYM(L"BLOCK");
	Symbol PERIOD_SYM(L".");
	Symbol USA_ACTOR_CODE(L"USA");
	// Score for actors found by findUSCities().  If you want to override one
	// of these with a custom pattern, then you'll should use a high weight
	// (eg 20-30).
	float US_CITY_SCORE = 45;
}

namespace ICEWS {

ActorMentionFinder::ActorMentionFinder(): 
		_actorTokenMatcher("actor"), _agentTokenMatcher("agent"), _compositeActorTokenMatcher("composite_actor"),
		_gazetteer(), _verbosity(1), _locationMentionResolver(&_gazetteer),
		_logSectorFreqs(0), _disable_coref(false), _discard_pronoun_actors(false), _discard_plural_actors(false),
		_encode_person_matching_country_as_citizen(false)
{
	bool encrypted_patterns = ParamReader::isParamTrue("icews_encrypt_patterns");
	_verbosity = ParamReader::getOptionalIntParamWithDefaultValue("icews_verbosity", 1);
	_logSectorFreqs = ParamReader::isParamTrue("icews_log_sector_frequencies");
	_icews_sentence_cutoff = ParamReader::getOptionalIntParamWithDefaultValue("icews_sentence_cutoff", INT_MAX);
	_disable_coref = ParamReader::isParamTrue("icews_disable_coref");
	_discard_pronoun_actors = ParamReader::isParamTrue("icews_discard_pronoun_actors");
	_discard_plural_actors = ParamReader::isParamTrue("icews_discard_plural_actors");
	_discard_plural_pronoun_actors = ParamReader::isParamTrue("icews_discard_plural_pronoun_actors");
	_encode_person_matching_country_as_citizen = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("icews_encode_person_matching_country_as_citizen", true);
	_block_default_country_if_another_country_in_same_sentence =
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("icews_block_default_country_if_another_country_in_same_sentence", true);
	_block_default_country_if_unknown_paired_actor_is_found =
		ParamReader::getOptionalTrueFalseParamWithDefaultVal("icews_block_default_country_if_unknown_paired_actor_is_found", true);
	_includeCitiesForDefaultCountrySelection = ParamReader::isParamTrue("icews_include_cities_for_default_country_selection");
	_maxAmbiguityForGazetteerActors = ParamReader::getOptionalIntParamWithDefaultValue("icews_max_ambiguity_for_gazetteer_actors", 3);
	_us_state_names = InputUtil::readFileIntoSymbolSet(ParamReader::getParam("icews_us_state_names"), true, true);
	_useDefaultLocationResolution = ParamReader::getOptionalTrueFalseParamWithDefaultVal("icews_use_default_location_resolution", false);

	// Get a list of "acceptable" country modifier words (eg western)
	_countryModifierWords = InputUtil::readFileIntoSet(ParamReader::getParam("icews_country_modifier_words"), false, true);
	_personModifierWords = InputUtil::readFileIntoSet(ParamReader::getParam("icews_person_modifier_words"), false, true);
	_organizationModifierWords = InputUtil::readFileIntoSet(ParamReader::getParam("icews_organization_modifier_words"), false, true);

	_perAgentNameWords = InputUtil::readFileIntoSet(ParamReader::getParam("person_agent_name_words"), false, true);	

	std::string agentPatternSetFilename = ParamReader::getRequiredParam("icews_agent_actor_patterns");
	_agentPatternSet = boost::make_shared<PatternSet>(agentPatternSetFilename.c_str(), encrypted_patterns);

	std::string blockDefaultCountryPatternSetFilename = ParamReader::getParam("icews_block_default_country_patterns");
	if (!blockDefaultCountryPatternSetFilename.empty())
		_blockDefaultCountryPatternSet = boost::make_shared<PatternSet>(blockDefaultCountryPatternSetFilename.c_str(), encrypted_patterns);

	// set ambiguity/verbosity parameters for location resolution
	if (!_useDefaultLocationResolution) {
		_locationMentionResolver.setMaxAmbiguity(_maxAmbiguityForGazetteerActors);
		_locationMentionResolver.setVerbosity(_verbosity);
	}

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
		msg << "Sector frequencies:" << std::endl;
		ActorInfo_ptr actorInfo = ActorInfo::getActorInfoSingleton();
		BOOST_FOREACH(CountSymbolPair p, pairs)
			msg << std::setw(6) << p.first << "  " << std::setw(36) << p.second 
			    << " " << actorInfo->getSectorName(p.second) << "\n";
		SessionLogger::info("ICEWS") << msg.str();
	}
}

void ActorMentionFinder::process(DocTheory *docTheory) {
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "=== Finding ICEWS Actor Mentions in " 
			<< docTheory->getDocument()->getName() << " ===";

	// Sanity check on the input
	if (docTheory->getEntitySet() == 0) {
		throw UnexpectedInputException("ActorMentionFinder::process",
			"This stage must be run after the doc-entities stage.");
	}

	std::string publicationDate = Stories::getStoryPublicationDate(docTheory->getDocument());
	if (publicationDate.empty())
		SessionLogger::warn("ICEWS") << "No publication date found for \"" 
			<< docTheory->getDocument()->getName() << "\"";
	else if (_verbosity > 1)
		SessionLogger::info("ICEWS") << "  Publication date: \"" << publicationDate << "\"";
	const char* publicationDateCStr = (publicationDate.empty()?0:publicationDate.c_str());

	// Construct an actor mention set to store our results.
	ActorMentionSet* actorMentionSet = _new ActorMentionSet();
	docTheory->takeSubtheory(actorMentionSet);

	// Build an ActorInfo (which connects to the databse) that we can use
	// to look up information about known actors.
	ActorInfo_ptr actorInfo_ptr = ActorInfo::getActorInfoSingleton();
	ActorInfo& actorInfo = *actorInfo_ptr;

	int sent_cutoff = getSentCutoff(docTheory);

	// Use patterns to find proper noun actors
	ActorMatchesBySentence unusedActorMatches = findProperNounActorMentions(docTheory, actorMentionSet, actorInfo, sent_cutoff, publicationDateCStr);

	// Choose default country
	ProperNounActorMention_ptr defaultCountryActorMention = getDefaultCountryActorMention(actorMentionSet, actorInfo);

	// Cautiously resolve any ambiguous locations to default country
	if (!_useDefaultLocationResolution && defaultCountryActorMention)
	{
		resolveAmbiguousLocationsToDefaultCountry(defaultCountryActorMention, docTheory, actorInfo, actorMentionSet, sent_cutoff);
	}

	// Find things that may block default country assignment.
	// These will block at the ENTITY level
	AgentActorPairs agentsOfUnknownActors = findAgentActorPairs(docTheory, actorMentionSet, actorInfo, PAIRED_UNKNOWN_ACTOR);
	PatternMatcher_ptr blockedMentionsPatternMatcher;
	if (_blockDefaultCountryPatternSet && defaultCountryActorMention) 
		blockedMentionsPatternMatcher = PatternMatcher::makePatternMatcher(docTheory, _blockDefaultCountryPatternSet);
	std::map<MentionUID, Symbol> blockedMentions = findMentionsThatBlockDefaultPairedActor(docTheory, blockedMentionsPatternMatcher);
	
	// Conservative coreference propagation; just proper noun actors at this point
	labelCoreferentMentions(docTheory, true, actorMentionSet, sent_cutoff);

	// Use patterns to find (but not add) composite actors, e.g. "Palestinian activists" --> Activist FOR Palestine
	// Will also find ICEWS agents without a paired actor, e.g. "activists" --> Activist FOR Unknown	
	AgentActorPairs agentsOfProperNounActors = findAgentActorPairs(docTheory, actorMentionSet, actorInfo, PAIRED_PROPER_NOUN_ACTOR, &unusedActorMatches);

	// Now actually do the assignment for those composite actors
	findCompositeActorMentions(docTheory, actorMentionSet, actorInfo, sent_cutoff, agentsOfProperNounActors);

	// Identify local acronym definitions and apply them.
	findLocalAcronymCompositeActorMentions(docTheory, actorMentionSet, actorInfo, sent_cutoff, publicationDateCStr);

	// Do conservative coreference propagation again, this time transferring those known ICEWS agents, 
	//  along with their paired actors (known or otherwise)
	labelCoreferentMentions(docTheory, true, actorMentionSet, sent_cutoff);

	// Use patterns to find (but not add) agents paired with composite actors (known or unknown)
	AgentActorPairs agentsOfCompositeActors = findAgentActorPairs(docTheory, actorMentionSet, actorInfo, PAIRED_COMPOSITE_ACTOR);
	
	// Label any people that we haven't tagged so far, if they are paired with a known actor
	labelPeople(docTheory, actorMentionSet, actorInfo, sent_cutoff, agentsOfProperNounActors, agentsOfCompositeActors, false);

	// Do complete coreference propagation
	labelCoreferentMentions(docTheory, false, actorMentionSet, sent_cutoff);

	// Apply the default country mention as the default paired actor (so we can handle agents paired with composites correctly)
	if (defaultCountryActorMention)
		assignDefaultCountryForUnknownPairedActors(actorMentionSet, defaultCountryActorMention, agentsOfUnknownActors, actorInfo, docTheory, blockedMentions);

	// Actually add agents of composite actors.
	findCompositeActorMentions(docTheory, actorMentionSet, actorInfo, sent_cutoff, agentsOfCompositeActors);

	// Label any people that we haven't tagged so far, even if they are paired with an unknown actor
	labelPeople(docTheory, actorMentionSet, actorInfo, sent_cutoff, agentsOfProperNounActors, agentsOfCompositeActors, true);
	
	// Label partitives of any mentions we've created so far (again)
	labelPartitiveMentions(docTheory, actorMentionSet, sent_cutoff);

	// Apply the default country mention as the default paired actor (again)
	if (defaultCountryActorMention)
		assignDefaultCountryForUnknownPairedActors(actorMentionSet, defaultCountryActorMention, agentsOfUnknownActors, actorInfo, docTheory, blockedMentions);
	
	// Label any unknown loc/fac as the default country.
	if (defaultCountryActorMention)
		labelLocationsAndFacilities(docTheory, actorMentionSet, actorInfo, sent_cutoff, defaultCountryActorMention, agentsOfProperNounActors, agentsOfCompositeActors, blockedMentions);

	// Discard any "bare" actor mentions we created
	discardBareActorMentions(actorMentionSet);

	if (_verbosity > 0) {
		SessionLogger::info("ICEWS") << "  Found " << actorMentionSet->size() << " actor mentions in "
			<< docTheory->getDocument()->getName() << std::endl;
		SessionLogger::info("ICEWS") << "  Cached geonames lookups: " << _gazetteer.cache_count << std::endl;
		SessionLogger::info("ICEWS") << "  Total geonames lookups: " << _gazetteer.geo_lookup_count << std::endl;
	}

	// Optionally record sector freqs.
	if (_logSectorFreqs) {
		BOOST_FOREACH(ActorMention_ptr actorMention, *actorMentionSet) {
			if (ProperNounActorMention_ptr pnActorMention = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention)) {
				ActorId actorId = pnActorMention->getActorId();
				std::vector<Symbol> sectors = actorInfo.getAssociatedSectorCodes(actorId);
				BOOST_FOREACH(const Symbol &sector, sectors)
					_sectorFreqs[sector] += 1;
			}
			if (CompositeActorMention_ptr cActorMention = boost::dynamic_pointer_cast<CompositeActorMention>(actorMention)) {
				AgentId agentId = cActorMention->getPairedAgentId();
				std::vector<Symbol> sectors = actorInfo.getAssociatedSectorCodes(agentId);
				BOOST_FOREACH(const Symbol &sector, sectors) 
					_sectorFreqs[sector] += 1;
			}
		}
	}
}

ActorMentionFinder::AgentActorPairs ActorMentionFinder::findAgentActorPairs(DocTheory *docTheory, const ActorMentionSet *actorMentionSet, ActorInfo &actorInfo, PairedActorKind actorKind, const ActorMatchesBySentence* unusedActorMatches) {
	if (_verbosity > 0) 
		SessionLogger::info("ICEWS") << "  Finding agent/actor pairs";
	AgentActorPairs agentActorPairs;
	PatternMatcher_ptr patternMatcher = PatternMatcher::makePatternMatcher(docTheory, _agentPatternSet);
	actorMentionSet->addEntityLabels(patternMatcher, actorInfo);
	for (int sentno=0; sentno<docTheory->getNSentences(); ++sentno) {
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
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
			SentenceTheory *sentTheory = docTheory->getSentenceTheory(i);
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
							ActorMention::ActorIdentifiers(match.id, match.code, match.patternId));
						agentActorPairs[ment->getUID()] = PairedActorMention(actorMention, NESTED_ACTOR_PATTERN_MATCH_SYM, true);
					}
				}
			}
		}
	}
	return agentActorPairs;
}

// Struct used to store information about how often various potential
// entities were mentioned in a given document.
//
// [xx] it's unclear whether overlap in associated sectors is a useful
// feature here, especially for high-level sectors such as "parties".
struct ActorMentionFinder::ActorMentionCounts {
	ActorId::HashMap<float> actorIdCount;
	CountryId::HashMap<float> countryIdCount;
	ActorId::HashMap<float> associatedActorIdCount;
	CountryId::HashMap<float> associatedCountryIdCount;
	SectorId::HashMap<float> associatedSectorIdCount;

	template<typename IdType>
	void addToTotal(const std::vector<IdType> &ids, typename IdType::template HashMap<float> &countMap) {
		BOOST_FOREACH(IdType id, ids) 
			countMap[id] += 1.0f/ids.size();
	}

	template<typename IdType>
	float getCount(const typename IdType::template HashMap<float> &countMap, IdType id) const {
		typename IdType::template HashMap<float>::const_iterator it = countMap.find(id);
		return (it == countMap.end()) ? 0.0f : ((*it).second);
	}

	template<typename IdType>
	float getOverlap(const std::vector<IdType> &ids, const typename IdType::template HashMap<float> &countMap) const {
		float result = 0.0f;
		BOOST_FOREACH(IdType id, ids) {
			result += getCount(countMap, id);
			//if (verbose)
			//	std::cout << "  * " << ActorInfo::getActorInfoSingleton()->getName(id) << ": " << getCount(countMap, id) << std::endl;
		}
		return result;
	}

};

namespace {
	Symbol LRB_SYM(L"-LRB-");
	Symbol RRB_SYM(L"-RRB-");
}
// For now, we only check for one-word acronyms, and they must be in all-caps, and at least two letters long.
ActorTokenMatcher_ptr ActorMentionFinder::findLocalProperNounAcronymDefinitions(const SortedActorMentions& sortedActorMentions) {
	ActorTokenMatcher_ptr result;
	ActorInfo_ptr actorInfo = ActorInfo::getActorInfoSingleton();
	BOOST_FOREACH(const ScoredActorMention& scoredActorMention, sortedActorMentions) {
		if (ProperNounActorMention_ptr actor = boost::dynamic_pointer_cast<ProperNounActorMention>(scoredActorMention.second)) {
			ActorId actorId = actor->getActorId();
			if (actorInfo->isAnIndividual(actorId))
				continue; // Eg: "John Smith (D)" does not imply "D=John Smith"
			if (actorInfo->isACountry(actorId))
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
	ActorInfo_ptr actorInfo = ActorInfo::getActorInfoSingleton();
	BOOST_FOREACH(ActorMention_ptr actorMention, (*actorMentions)) {
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

void ActorMentionFinder::findLocalAcronymCompositeActorMentions(DocTheory* docTheory, ActorMentionSet *result, ActorInfo &actorInfo, int sent_cutoff, const char *publicationDate) {
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Adding locally-defined composite acronym expansions";
	CompositeActorTokenMatcher_ptr acronymActorTokenMatcher = findLocalCompositeAcronymDefinitions(result);
	if (acronymActorTokenMatcher) {
		SortedActorMentions sortedActorMentions;
		CompositeActorMatchesBySentence acronymMatches = acronymActorTokenMatcher->findAllMatches(docTheory, sent_cutoff);
		for (size_t sentno=0; sentno<acronymMatches.size(); ++sentno) {
			SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
			BOOST_FOREACH(CompositeActorMatch match, acronymMatches[sentno]) {
				ScoredActorMention scoredActorMention = makePrecomposedCompositeActorMention(match, docTheory, sentTheory, actorInfo, LOCAL_COMPOSITE_ACRONYM_SYM, publicationDate);
				if (scoredActorMention.second)
					sortedActorMentions.insert(scoredActorMention);
			}
		}
		greedilyAddActorMentions(sortedActorMentions, result, actorInfo);
	}
}



ActorMentionFinder::ActorMatchesBySentence ActorMentionFinder::findProperNounActorMentions(DocTheory* docTheory, ActorMentionSet *result, ActorInfo &actorInfo, int sent_cutoff, const char *publicationDate) {
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Adding proper noun actor mentions";

	// Keep the unused matches -- they may be appropriate to use as paired actors.
	ActorMatchesBySentence unusedMatches;

	// Apply the Jabari actor patterns in _actorTokenMatcher to find all
	// possible pattern matches (including matches that are overlapping)
	ActorMatchesBySentence actorMatches = _actorTokenMatcher.findAllMatches(docTheory, sent_cutoff);

	// Calculate some document-level statistics that are used to determine the 
	// scores of individual matches.
	ActorMentionCounts actorMentionCounts = getActorMentionCounts(actorMatches, actorInfo, publicationDate);

	// Construct an ActorMention corresponding with each ActorMatch, and 
	// assign it a score.
	unusedMatches.resize(actorMatches.size());
	SortedActorMentions sortedActorMentions;
	for (size_t sentno=0; sentno<actorMatches.size(); ++sentno) {
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
		BOOST_FOREACH(ActorMatch match, actorMatches[sentno]) {
			ScoredActorMention scoredActorMention = makeProperNounActorMention(match, docTheory, sentTheory, actorMentionCounts, actorInfo, actorMatches, ACTOR_PATTERN_SYM, publicationDate);
			if (scoredActorMention.second)
				sortedActorMentions.insert(scoredActorMention);
			else 
				unusedMatches.at(sentno).push_back(match);
		}
	}

	// Add any precomposed composite actors.
	CompositeActorMatchesBySentence precomposedCompositeActorMatches = _compositeActorTokenMatcher.findAllMatches(docTheory, sent_cutoff);
	for (size_t sentno=0; sentno<actorMatches.size(); ++sentno) {
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
		BOOST_FOREACH(CompositeActorMatch match, precomposedCompositeActorMatches[sentno]) {
			ScoredActorMention scoredActorMention = makePrecomposedCompositeActorMention(match, docTheory, sentTheory, actorInfo, COMPOSITE_ACTOR_PATTERN_SYM, publicationDate, &actorMentionCounts, &actorMatches);
			if (scoredActorMention.second)
				sortedActorMentions.insert(scoredActorMention);
		}
	}


//// ED's resolution code 

	if (_useDefaultLocationResolution) {
		// Look up each location in the gazetteer, and add a corresponding
		// ActorMention for the country containing that location.
		for (size_t sentno=0; sentno<actorMatches.size(); ++sentno) {
			SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
			const MentionSet *ms = sentTheory->getMentionSet();
			for (int m = 0; m < ms->getNMentions(); m++) {
				const Mention *ment = ms->getMention(m);
				EntityType entityType = ment->getEntityType();
				if ((ment->getMentionType() == Mention::NAME) && (entityType.matchesLOC() || entityType.matchesGPE())) {
					std::wstring name_string = ment->getAtomicHead()->toTextString();
					boost::trim(name_string);					
					std::vector<Gazetteer::LocationInfo_ptr> resolutions = _gazetteer.lookup(name_string);
					SortedActorMentions localSortedActorMentions;
					BOOST_FOREACH(Gazetteer::LocationInfo_ptr location, resolutions) {
						ScoredActorMention scoredActorMention = makeProperNounActorMention(ment, location, docTheory, sentTheory, actorMentionCounts, actorInfo, publicationDate);
						if (scoredActorMention.second)
							localSortedActorMentions.insert(scoredActorMention);
					}
					// If the top two choices are tied, then refuse to make a choice, and add nothing.
					if (localSortedActorMentions.size() > 2) {
						SortedActorMentions::reverse_iterator it = localSortedActorMentions.rbegin();
						const ScoredActorMention& best = *it++;
						const ScoredActorMention& secondBest = *it;
						if (best.first - secondBest.first < 0.02) {
							SessionLogger::info("ICEWS") << "Blocked ambiguous gazetteer location for \""
														 << name_string << "\": two different countries (" 
														 << best.second << " and " << secondBest.second
														 << ") both have the similar scores (" << best.first 
														 << " and " << secondBest.first << ")";
							continue;
						}
					}
					if ((_maxAmbiguityForGazetteerActors>=0) && (static_cast<int>(resolutions.size()) > _maxAmbiguityForGazetteerActors)) {
						if (_verbosity > 3) {
							std::stringstream msg;
							msg << "Blocked ambiguous gazetteer location: \""
								<< name_string << "\" has " << resolutions.size() << " entries: ";
							BOOST_FOREACH(ScoredActorMention sam, localSortedActorMentions) {
								if (ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(sam.second))
									msg << sam.first << ":" << pnam->getActorCode() << " ";	
							}
							SessionLogger::info("ICEWS") << msg.str();
						}
						continue;
					} 
					BOOST_FOREACH(ScoredActorMention sam, localSortedActorMentions) {
						sortedActorMentions.insert(sam);
					}
				}
			}
		}
	}

//// Nick's resolution code: PHASE 1 -> conservative

	if (!_useDefaultLocationResolution) {
		// for each location mention, attempt to add a corresponding ActorMention
		// iff the location can be resolved to the database unambiguously
		SortedActorMentions localSortedActorMentions;
		for (size_t sentno=0; sentno<actorMatches.size(); ++sentno) {
			SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
			const MentionSet *ms = sentTheory->getMentionSet();
			for (int m = 0; m < ms->getNMentions(); m++) {
				const Mention *ment = ms->getMention(m);
				EntityType entityType = ment->getEntityType();
				if ((ment->getMentionType() == Mention::NAME) && (entityType.matchesLOC() || entityType.matchesGPE())) {
					std::wstring head_string = ment->getAtomicHead()->toTextString();
					std::wstring full_string = ment->toCasedTextString();
					boost::trim(head_string);
					boost::trim(full_string);
					ActorMention_ptr target = result->find(ment->getUID());
					// if we already have an actor for this mention that is a ProperNounActor and is geo-resolved, move on
					if (target) 
					{	
						if (ProperNounActorMention_ptr pnActor = boost::dynamic_pointer_cast<ProperNounActorMention>(target)) {
							if(pnActor->isResolvedGeo()) continue;
							else {
								Gazetteer::GeoResolution_ptr resolution = _locationMentionResolver.getUnambiguousResolution(ment);
								if (!resolution->isEmpty) {
									pnActor->setGeoResolution(resolution);
								}
								continue;
							}
						}
					} 
					else 
					{
						ScoredActorMention scoredLocationActorMention = getUnambiguousLocationActorMention(ment, docTheory, sentTheory, publicationDate);
						if(scoredLocationActorMention.second){
							localSortedActorMentions.insert(scoredLocationActorMention);
						}
					}
				}
			}
		}

		BOOST_FOREACH(ScoredActorMention sam, localSortedActorMentions) {
			sortedActorMentions.insert(sam);
		}
	}
	
	// Check if any new acronyms were defined.
	ActorTokenMatcher_ptr acronymActorTokenMatcher = findLocalProperNounAcronymDefinitions(sortedActorMentions);
	if (acronymActorTokenMatcher) {
		ActorMatchesBySentence acronymMatches = acronymActorTokenMatcher->findAllMatches(docTheory, sent_cutoff);
		for (size_t sentno=0; sentno<acronymMatches.size(); ++sentno) {
			SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
			BOOST_FOREACH(ActorMatch match, acronymMatches[sentno]) {
				ScoredActorMention scoredActorMention = makeProperNounActorMention(match, docTheory, sentTheory, actorMentionCounts, actorInfo, actorMatches, LOCAL_PROPER_NOUN_ACRONYM_SYM, publicationDate);
				if (scoredActorMention.second)
					sortedActorMentions.insert(scoredActorMention);
			}
		}
	}

	// Check for US-state locations such as "omaha, neb." in headlines.
	findUSCities(docTheory, sortedActorMentions, actorInfo, sent_cutoff);

	greedilyAddActorMentions(sortedActorMentions, result, actorInfo);

//// Nick's resolution code: PHASE 2 -> less conservative

	if (!_useDefaultLocationResolution) {
		// for each location mention that has not been resolved, attempt 
		// to add a corresponding ActorMention using a less conservative
		// approach than above

		_locationMentionResolver.setActorMentionSet(result);
		SortedActorMentions locationSortedActorMentions;
		for (size_t sentno=0; sentno<actorMatches.size(); ++sentno) {
			SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
			const MentionSet *ms = sentTheory->getMentionSet();
			for (int m = 0; m < ms->getNMentions(); m++) {
				const Mention *ment = ms->getMention(m);
				EntityType entityType = ment->getEntityType();
				if ((ment->getMentionType() == Mention::NAME) && (entityType.matchesLOC() || entityType.matchesGPE())) {
					std::wstring head_string = ment->getAtomicHead()->toTextString();
					std::wstring full_string = ment->toCasedTextString();
					boost::trim(head_string);
					boost::trim(full_string);
					ActorMention_ptr target = result->find(ment->getUID());
					// if we already have an actor for this mention that is a ProperNounActor and is geo-resolved, move one
					if (target) 
					{	
						if (ProperNounActorMention_ptr pnActor = boost::dynamic_pointer_cast<ProperNounActorMention>(target)) {
							if(pnActor->isResolvedGeo()) continue;
							else {
								Gazetteer::GeoResolution_ptr resolution = _locationMentionResolver.getLocationResolution(ment);
								if (!resolution->isEmpty) {
									pnActor->setGeoResolution(resolution);
								}
								continue;
							}
						}
					} 
					// otherwise try to make a new geo-resolved ProperNounActor for the mention
					ScoredActorMention scoredLocationActorMention = getUnresolvedLocationActorMention(ment, docTheory, sentTheory, publicationDate);
					if(scoredLocationActorMention.second){
						locationSortedActorMentions.insert(scoredLocationActorMention);
					}
				}
			}
		}

		greedilyAddActorMentions(locationSortedActorMentions, result, actorInfo);
		_locationMentionResolver.clear();
	}

	return unusedMatches;
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

void ActorMentionFinder::greedilyAddActorMentions(const ActorMentionFinder::SortedActorMentions& sortedActorMentions, ActorMentionSet *result, ActorInfo &actorInfo) {
	// Greedily assign ActorMentions to entity Mentions.
	for (SortedActorMentions::const_reverse_iterator it=sortedActorMentions.rbegin(); it!=sortedActorMentions.rend(); ++it) {
		double score = (*it).first;
		const ActorMention_ptr& actorMention = (*it).second;
		if (score > 0) {
			ActorMention_ptr target = result->find(actorMention->getEntityMention()->getUID());
			bool replace_old_value = (target && isCompatibleAndBetter(target, actorMention));
			logActorMention(actorMention, (*it).first, target, actorInfo, replace_old_value);
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

ProperNounActorMention_ptr ActorMentionFinder::defaultCountryAssignmentIsBlockedByOtherCountry(ActorMention_ptr actor, ProperNounActorMention_ptr pairedActorMention, ActorMentionSet *actorMentionSet, ActorInfo &actorInfo) {
	const MentionSet* mentionSet = actor->getSentence()->getMentionSet();
	for (int i=0; i<mentionSet->getNMentions(); i++) {
		const MentionUID m = mentionSet->getMention(i)->getUID();
		if (ProperNounActorMention_ptr a = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMentionSet->find(m))) {
			if ((actorInfo.isACountry(a->getActorId())) && (a->getActorId() != pairedActorMention->getActorId())) {
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
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
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
							"Unexpected return label from icews_block_default_country_patterns PatternSet", 
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
																	const AgentActorPairs &agentsOfUnknownActors, ActorInfo &actorInfo,
																	const DocTheory *docTheory, std::map<MentionUID, Symbol>& blockedMentions) 
{	
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Replacing unknown paired actors with default country: " << ActorMention_ptr(pairedActorMention);
	BOOST_FOREACH(ActorMention_ptr actor, *actorMentionSet) {
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
				ProperNounActorMention_ptr otherCountry = defaultCountryAssignmentIsBlockedByOtherCountry(actor, pairedActorMention, actorMentionSet, actorInfo);
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
				// Check to see whether this mention is co-referent with something that is paired
				//  with something other than the default country
				if (sisterMentionHasClashingActor(compositeActor->getEntityMention(), pairedActorMention, docTheory, actorMentionSet)) {
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

ProperNounActorMention_ptr ActorMentionFinder::getDefaultCountryActorMention(const ActorMentionSet *actorsFoundSoFar, ActorInfo &actorInfo) {
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Selecting default country";
	ProperNounActorMention_ptr result;

	// Check what countries are mentioned in this document so far.
	ActorId::HashMap<float> countryCount;
	ActorId::HashMap<ProperNounActorMention_ptr> countryActors;
	ActorId::HashMap<ProperNounActorMention_ptr> cityActors;
	BOOST_FOREACH(ActorMention_ptr actor, *actorsFoundSoFar) {
		if (ProperNounActorMention_ptr pn_actor = boost::dynamic_pointer_cast<ProperNounActorMention>(actor)) {
			ActorId actorId = pn_actor->getActorId();
			const Mention* mention = actor->getEntityMention();
			int sentno = mention->getSentenceNumber();
			if (actorInfo.isACountry(actorId)) {
				CountryId country_id = actorInfo.getCountryId(actorId);
				if (country_id.isNull())
					continue;
				// Country names that occurs as the very first mention in the first sentence are
				// usually part of a headline, and serve as strong candidates for the default 
				// country, so give them an extra score boost.
				if ((sentno == 0) && (mention->getNode()->getStartToken()==0)) {
					countryCount[actorId] += 3;
					if (_verbosity > 3)
						SessionLogger::info("ICEWS") << "    [  +3] for <" << actorInfo.getActorName(actorId) 
													 << ">: Occurs at the very beginning of the document";
				} else {
					countryCount[actorId] += 1;
					if (_verbosity > 3)
						SessionLogger::info("ICEWS") << "    [  +1] for <" << actorInfo.getActorName(actorId) 
							<< ">: Mentioned explicitly in sentence " << sentno;
				}
				if (!countryActors[actorId])
					countryActors[actorId] = pn_actor;
			} else {
				BOOST_FOREACH(ActorId country, actorInfo.getAssociatedCountryActorIds(actorId)) {
					if (_verbosity > 3)
						SessionLogger::info("ICEWS") << "    [+0.1] for <" << actorInfo.getActorName(country) 
							<< ">: Related actor <" << actorInfo.getActorName(actorId) 
							<< "> was mentioned in sentence " << sentno;
					countryCount[country] += 0.1f;
				}
				// Check if this is a city in a country, using the gazeteer.
				if (_includeCitiesForDefaultCountrySelection) {
					EntityType entityType = mention->getEntityType();
					if ((mention->getMentionType() == Mention::NAME) && (entityType.matchesLOC() || entityType.matchesGPE()) &&
						actorInfo.mightBeALocation(actorId)) {
						std::vector<Gazetteer::LocationInfo_ptr> locs = _gazetteer.lookup(actorInfo.getActorName(actorId));
						if (locs.size()) {
							BOOST_FOREACH(Gazetteer::LocationInfo_ptr locInfo, locs) {
								if (locInfo->countryInfo) {
									float score = 0.5f / locs.size();
									if (_verbosity > 3)
										SessionLogger::info("ICEWS") << "    [+" << score << "] for <" 
											<< actorInfo.getActorName(locInfo->countryInfo->actorId) 
											<< ">: \"" << actorInfo.getActorName(actorId) 
											<< "\" is a location in this country (from gazetteer)"
											<< " in sentence " << sentno;
									countryCount[locInfo->countryInfo->actorId] += score;
									if (!cityActors[locInfo->countryInfo->actorId])
										cityActors[locInfo->countryInfo->actorId] = pn_actor;
								}
							}
						}
					}
				}
			}
		}
	}

	// Get the most frequent & second-most frequent country mentions.
	typedef std::pair<ActorId, float> ActorIdCount;
	ActorIdCount mostFreq1(ActorId(), -1);
	ActorIdCount mostFreq2(ActorId(), -1);
	for (ActorId::HashMap<float>::const_iterator it=countryCount.begin(); it != countryCount.end(); ++it) {
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
		result = countryActors[mostFreq1.first]; // may be NULL.
		// If there's no direct mention of the country we selected, then
		// try creating a temporary actor mention based on some mention
		// of  acity inside the country.
		if ((!result) && (cityActors[mostFreq1.first])) {
			ProperNounActorMention_ptr cityActor = cityActors[mostFreq1.first];
			result = boost::make_shared<ProperNounActorMention>
				(cityActor->getSentence(), cityActor->getEntityMention(), TEMPORARY_ACTOR_MENTION_SYM, 
				 ActorMention::ActorIdentifiers(mostFreq1.first, actorInfo.getActorCode(mostFreq1.first),
												ActorPatternId()));
		}

	}
	
	// Log message.
	if (_verbosity > 1) {
		std::ostringstream logmsg;
		if (result)
			logmsg << "    Default country actor: \"" << actorInfo.getActorName(mostFreq1.first) << "\".";
		else
			logmsg << "    No default country actor.";
		if (mostFreq1.second > 0) {
			logmsg << "\n      The highest scoring country actor in this document <"
				<< actorInfo.getActorName(mostFreq1.first)
				<< "> (" << mostFreq1.first.getId() << ") has a score of "
				<< mostFreq1.second << ".";
		}
		if (mostFreq2.second > 0) {
			logmsg << "\n      The second highest scoring country actor <"
				<< actorInfo.getActorName(mostFreq2.first)
				<< "> (" << mostFreq2.first.getId() << ") has a score of "
				<< mostFreq2.second << ".";
		}
		SessionLogger::info("ICEWS") << logmsg.str();
	}

	return result;
}

void ActorMentionFinder::findCompositeActorMentions(DocTheory* docTheory, ActorMentionSet *result, ActorInfo &actorInfo, int sent_cutoff, const AgentActorPairs &agentActorPairs) {
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Adding composite actor mentions";

	// Apply the Jabari actor patterns in _agentTokenMatcher to find all
	// possible pattern matches (including matches that are overlapping)
	AgentMatchesBySentence agentMatches = _agentTokenMatcher.findAllMatches(docTheory, sent_cutoff);

	// For each AgentMatch, try to construct a composite ActorMention 
	// consisting of the matched agent and a paired actor; and assign
	// a score to that ActorMention.
	SortedActorMentions sortedActorMentions;
	for (size_t sentno=0; sentno<agentMatches.size(); ++sentno) {
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
		BOOST_FOREACH(AgentMatch match, agentMatches[sentno]) {
			ScoredActorMention scoredActorMention = makeCompositeActorMention(match, docTheory, sentTheory, 
				agentActorPairs, result, actorInfo);
			if (scoredActorMention.second) {
				sortedActorMentions.insert(scoredActorMention);
			}
		}
	}

	// Greedily assign ActorMentions to entity Mentions.
	greedilyAddActorMentions(sortedActorMentions, result, actorInfo);
}

void ActorMentionFinder::logActorMention(ActorMention_ptr actorMention, double score, ActorMention_ptr conflictingActor, ActorInfo &actorInfo, bool replace_old_value) {
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

ActorMentionFinder::ActorMentionCounts 
ActorMentionFinder::getActorMentionCounts(const ActorMatchesBySentence &actorMatches, ActorInfo &actorInfo, const char* publicationDate)
{
	ActorMentionCounts counts;

	BOOST_FOREACH(const std::vector<ActorMatch>& matchesForSentence, actorMatches) {
		BOOST_FOREACH(const ActorMatch &match, matchesForSentence) {
			// Frequency with which each actorid is mentioned.
			++counts.actorIdCount[match.id];
			// Frequency with which each country actor is mentioned.
			CountryId countryId = actorInfo.getCountryId(match.id);
			if (!countryId.isNull())
				++counts.countryIdCount[countryId];
			// Frequency of associated actors.
			counts.addToTotal(actorInfo.getAssociatedActorIds(match.id, publicationDate), counts.associatedActorIdCount);
			// Frequency of associated countries.
			counts.addToTotal(actorInfo.getAssociatedCountryIds(match.id, publicationDate), counts.associatedCountryIdCount);
			// Frequency of associated sectors.
			counts.addToTotal(actorInfo.getAssociatedSectorIds(match.id, publicationDate), counts.associatedSectorIdCount);
		}
	}

	return counts;
}

/** Return a number between zero and max_output_val, where the result stops
  * growing after we reach max_input_val. */
float ActorMentionFinder::countToScore(float count, float max_input_val, float max_output_val) 
{
	if (count > max_input_val) 
		return max_output_val;
	else 
		return (max_output_val - 
		        ((max_output_val/max_input_val/max_input_val) * 
		         ((count-max_input_val) * (count-max_input_val))));
}


ActorMentionFinder::ScoredActorMention ActorMentionFinder::makePrecomposedCompositeActorMention(const CompositeActorMatch &match, const DocTheory* docTheory, const SentenceTheory *sentTheory, ActorInfo &actorInfo, Symbol sourceNote, const char* publicationDate, const ActorMentionFinder::ActorMentionCounts *actorMentionCounts, const ActorMatchesBySentence *actorMatches) {
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
	Symbol pairedAgentCode = actorInfo.getAgentCode(pairedAgentId);
	Symbol pairedActorCode = actorInfo.getActorCode(pairedActorId);
	ActorMention_ptr actorMention = boost::make_shared<CompositeActorMention>(sentTheory, mention, sourceNote,
		ActorMention::AgentIdentifiers(pairedAgentId, pairedAgentCode, AgentPatternId()),
		ActorMention::ActorIdentifiers(pairedActorId, pairedActorCode, match.patternId), sourceNote);
	if (!same_end_tok)
		actorMention->addSourceNote(PATTERN_END_IS_NOT_MENTION_END_SYM);

	// Calculate a score for this new potential actor mention.
	double score = match.weight + match.pattern_strlen;
	if (match.isAcronymMatch && actorMatches) {
		ActorMatch pairedActorMatch(pairedActorId, match.patternId, pairedActorCode, match.start_token, match.end_token, match.pattern_strlen, match.weight, match.isAcronymMatch);
		score += scoreAcronymMatch(docTheory, sentTheory, pairedActorMatch, *actorMatches, actorInfo);
	}
	if (same_start_tok && same_end_tok)
		score += 15; // exact span match.
	else if (same_end_tok) 
		score += 5;

	// Give a higher score if more associated actors/ids/countries 
	// also appear in the same document.
	if (actorMentionCounts)
		score += getAssociationScore(pairedActorId, publicationDate, *actorMentionCounts, actorInfo);

	return ScoredActorMention(score, actorMention);
}

ActorMentionFinder::ScoredActorMention ActorMentionFinder::makeProperNounActorMention(const ActorMatch &match, const DocTheory* docTheory, const SentenceTheory *sentTheory, const ActorMentionFinder::ActorMentionCounts &actorMentionCounts, ActorInfo &actorInfo, const ActorMatchesBySentence &actorMatches, Symbol sourceNote, const char* publicationDate) {
	const Mention *mention = getCoveringNameDescMention(sentTheory, match.start_token, match.end_token);

	// If the matched phrase isn't the head of a mention, see if it's a non-head
	// child of one.  This can occur for mis-parses, and in a few other cases.
	bool head_match = (mention!=0);
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
		return ScoredActorMention(-1, ActorMention_ptr());

	const SynNode *node = mention->getAtomicHead();
	bool same_start_tok = (node->getStartToken() == match.start_token);
	bool same_end_tok = (node->getEndToken() == match.end_token);

	// When matching country names, require that the name be exact (modulo some 
	// acceptable modifiers such as "western")
	bool actor_is_a_country = actorInfo.isACountry(match.id);
	bool actor_is_an_individual = actorInfo.isAnIndividual(match.id);
	bool actor_is_an_org = !(actor_is_a_country || actor_is_an_individual);
	
	// Look for things like "The Americans"
	bool country_the_plural = false;
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
		const std::set<std::wstring> & modifierWords = (actor_is_a_country ? _countryModifierWords : 
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
				if (modifierWords.find(tokStr) == modifierWords.end()) {
					if (_verbosity > 3) {
						SessionLogger::info("ICEWS") << "    Skipping mention \"" << mention->toCasedTextString() 
							<< "\" which matches pattern \""
							<< sentTheory->getTokenSequence()->toString(match.start_token, match.end_token) << "\" "
							<< "(head=\"" << node->toCasedTextString(sentTheory->getTokenSequence()) << "\") "
							<< "because the actor is " << context << ", and the mention contains the unexpected word \""
							<< tokStr << "\" (" << postag << ")" << std::endl;
					}
					return ScoredActorMention(-1, ActorMention_ptr()); // supress this match.
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
			ActorMention::ActorIdentifiers(match.id, match.code, match.patternId));
	}
	if (!same_end_tok)
		actorMention->addSourceNote(PATTERN_END_IS_NOT_MENTION_END_SYM);

	// Calculate a score for this new potential actor mention.
	// Note: the "weights" that are hard-coded here should ideally
	// be replaced with (trained) feature weights at some point.
	double score = 0; // baseline score

	// Acronyms need extra justification.
	if (match.isAcronymMatch) 
		score += scoreAcronymMatch(docTheory, sentTheory, match, actorMatches, actorInfo);

	// Penalize non-head matches.
	if (!head_match)
		score -= 5;

	// How well does the mention span match up with the actor match?
	if (same_start_tok && same_end_tok)
		score += 15; // exact span match.
	else if (same_end_tok) 
		score += 5;

	// If the source pattern has extra weight, then add it to the score.
	score += match.weight;

	// Give a higher score if more associated actors/ids/countries 
	// also appear in the same document.
	score += getAssociationScore(match.id, publicationDate, actorMentionCounts, actorInfo);

	// Give extra weight to matches whose entity type matches the entity
	// type that we assigned to the mention.
	if (actorInfo.isACountry(match.id)) {
		if (mention->getEntityType().matchesGPE())
			score += 5;
		else if (mention->getEntityType().matchesLOC())
			score += 3;
		else if (mention->getEntityType().matchesPER()) {
			if (_encode_person_matching_country_as_citizen && !country_the_plural) {
				static AgentId citizenAgentId = actorInfo.getAgentByCode(CITIZEN_AGENT_CODE);										
				actorMention = boost::make_shared<CompositeActorMention>(sentTheory, mention, CITIZEN_OF_COUNTRY_SYM,
						ActorMention::AgentIdentifiers(citizenAgentId, CITIZEN_AGENT_CODE, AgentPatternId()), 
						ActorMention::ActorIdentifiers(match.id, match.code, match.patternId),
						PERSON_IS_CITIZEN_OF_COUNTRY_SYM);
				/*std::cerr << "citizen of country " << actorMention << " \"" << mention->toCasedTextString()
					<< "\" " << Mention::getTypeString(mention->getMentionType()) 
					<< "/" << mention->getEntityType().getName() << std::endl;*/
			}
		}
	}
	if (actorInfo.isAnIndividual(match.id)) {
		if (mention->getEntityType().matchesPER())
			score += 3;
	}

	// How long is the pattern that found the actor match?  Longer
	// patterns are usually more reliable.
	score += match.pattern_strlen;

	// We need a way to break ties when the patterns are of the same length;
	// This is a database problem but the database isn't ours, so that's the way it goes.
	// We want to be deterministic at least given a particular database.
	// There are currently 100,000+ agents, so multiplying by 1/1,000,000 and actor_id seems fair.
	score -= match.id.getId() / 1000000.0F;

	return ScoredActorMention(score, actorMention);
}

float ActorMentionFinder::getAssociationScore(ActorId actorId, const char* date, const ActorMentionFinder::ActorMentionCounts &actorMentionCounts, ActorInfo &actorInfo) {
	float num_direct_associated_actors = actorMentionCounts.getOverlap(actorInfo.getAssociatedActorIds(actorId, date), actorMentionCounts.actorIdCount);
	float num_indirect_associated_actors = actorMentionCounts.getOverlap(actorInfo.getAssociatedActorIds(actorId, date), actorMentionCounts.associatedActorIdCount);
	float num_associated_sectors = actorMentionCounts.getOverlap(actorInfo.getAssociatedSectorIds(actorId, date), actorMentionCounts.associatedSectorIdCount);
	float num_associated_countries = actorMentionCounts.getOverlap(actorInfo.getAssociatedCountryIds(actorId, date), actorMentionCounts.countryIdCount);
	return (
		countToScore(num_direct_associated_actors) +
		countToScore(num_indirect_associated_actors, 20, 3) +
		countToScore(num_associated_countries) +
		countToScore(num_associated_sectors, 10, 2));
}


// Try to find a good justification for using this acronym

float ActorMentionFinder::scoreAcronymMatch(const DocTheory *docTheory, const SentenceTheory *sentTheory, ActorMatch match, const ActorMatchesBySentence &actorMatches, ActorInfo &actorInfo) {
	float penalty = -50.0f; // default result if we find no justification for the acronym

	std::wstring reason = L"No justification found"; // for logging
	std::wstring actorName = actorInfo.getActorName(match.id);

	// First, check if the document contains any mentions of the same actor,
	// that use the actor's full name (and not just an acronym).  If so, then
	// using the acronym is fairly safe, so don't apply any penalty.
	for (size_t sent_no=0; sent_no<actorMatches.size(); ++sent_no) {
		BOOST_FOREACH(ActorMatch m2, actorMatches[sent_no]) {
			if ((m2.id == match.id) && (!m2.isAcronymMatch)) {
				penalty = 0;
				reason = std::wstring(L"Found full expansion (\"")+docTheory->getSentenceTheory(sent_no)->getTokenSequence()->toString(m2.start_token, m2.end_token)+L"\")";
			}
		}
	}

	// Next, get the expanded acronym name, and see if we can match that
	// expansion or something like it in the document.
	if (penalty<0) {
		for (size_t sent_no=0; sent_no<actorMatches.size(); ++sent_no) {
			const MentionSet *mentionSet = docTheory->getSentenceTheory(sent_no)->getMentionSet();
			for (int i=0; i<mentionSet->getNMentions(); ++i) {
				const Mention *m2 = mentionSet->getMention(i);
				std::wstring mentionText = m2->toCasedTextString();
				int editDistance = (actorName == mentionText) ? 0 : 10000;
				if ((actorName.size()<256) && (mentionText.size()<256))
					editDistance = CorefUtilities::editDistance(actorName, mentionText);
				if ((editDistance < 5) && (editDistance <= static_cast<int>(actorName.length())/4)) {
					// [XX] Should we also add an ActorMatch for the justifying mention that we found?
					float newPenalty = static_cast<float>(-editDistance);
					if (penalty < newPenalty) {
						penalty = newPenalty;
						reason = std::wstring(L"Found mention similar to expansion (\"")+mentionText+L"\")";
					}
				}
			}
		}
	}

	// If it's associated with a country, and that country is explicitly mentioned,
	// then we'll allow it.
	BOOST_FOREACH(ActorId associatedCountry, actorInfo.getAssociatedCountryActorIds(match.id)) {
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
				reason = std::wstring(L"Related country (\"")+actorInfo.getActorName(associatedCountry)+
					L"\") was mentioned "+boost::lexical_cast<std::wstring>(num_mentions)+L" times.";
			}
		}
	}

	// If it occurs within the first two sentences, and is the name of a media
	// organization, then we'll allow it.  This captures things like "(AP)".
	if (sentTheory->getSentNumber()<3) {
		BOOST_FOREACH(Symbol code, actorInfo.getAssociatedSectorCodes(match.id)) {
			if ((code == MEDIA_SECTOR_CODE) || (code == NEWS_SECTOR_CODE)) {
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
		msg << "      " << ((penalty<=-50.0f)?"Rejected":"Accpted") << " acronym expansion "
			<< sentTheory->getTokenSequence()->toString(match.start_token, match.end_token) 
			<< "=>\"" << actorName << "\"";
		if (penalty>-50.0f) 
			msg << "\n        [" << setw(5) << penalty << "] " << reason;
		SessionLogger::info("ICEWS") << msg.str();
	}

	return penalty;
}

ActorMentionFinder::ScoredActorMention ActorMentionFinder::getUnambiguousLocationActorMention(const Mention* mention, const DocTheory* docTheory, const SentenceTheory *sentTheory, const char* publicationDate) {
	Gazetteer::GeoResolution_ptr resolution = _locationMentionResolver.getUnambiguousResolution(mention);
	if (resolution->isEmpty || !resolution->countryInfo)
	{
		return ScoredActorMention(-1, ActorMention_ptr());
	}
	Gazetteer::CountryInfo_ptr countryInfo = resolution->countryInfo;

	ActorMention_ptr actorMention = boost::make_shared<ProperNounActorMention>(
		sentTheory, mention, GAZETTEER_SYM,
		ActorMention::ActorIdentifiers(countryInfo->actorId, countryInfo->actorCode, ActorPatternId()),
		resolution);

	float score = 100; 

	return ScoredActorMention(score, actorMention);
}

ActorMentionFinder::ScoredActorMention ActorMentionFinder::getUnresolvedLocationActorMention(const Mention* mention, const DocTheory* docTheory, const SentenceTheory *sentTheory, const char* publicationDate) {
	Gazetteer::GeoResolution_ptr resolution = _locationMentionResolver.getLocationResolution(mention);
	if (resolution->isEmpty || !resolution->countryInfo)
	{
		return ScoredActorMention(-1, ActorMention_ptr());
	}
	Gazetteer::CountryInfo_ptr countryInfo = resolution->countryInfo;

	ProperNounActorMention_ptr actorMention = boost::make_shared<ProperNounActorMention>(
		sentTheory, mention, GAZETTEER_SYM,
		ActorMention::ActorIdentifiers(countryInfo->actorId, countryInfo->actorCode, ActorPatternId()),
		resolution);

	
	float score = 100; 

	return ScoredActorMention(score, actorMention);
}

ActorMentionFinder::ScoredActorMention ActorMentionFinder::makeProperNounActorMention(const Mention* mention, Gazetteer::LocationInfo_ptr locationInfo, const DocTheory* docTheory, const SentenceTheory *sentTheory, const ActorMentionFinder::ActorMentionCounts &actorMentionCounts, ActorInfo &actorInfo, const char* publicationDate) {
	Gazetteer::CountryInfo_ptr countryInfo = locationInfo->countryInfo;
	if (!countryInfo) {
		//std::cout << "Location found, but no country: " << mention->toCasedTextString() << std::endl;
		return ScoredActorMention(-1, ActorMention_ptr());
	}
	ActorMention_ptr actorMention = boost::make_shared<ProperNounActorMention>(
		sentTheory, mention, GAZETTEER_SYM,
		ActorMention::ActorIdentifiers(countryInfo->actorId, countryInfo->actorCode, ActorPatternId()));

	double score = 10; // baseline score

	// higher population = higher score.
	if (locationInfo->population > 1000)
		score += log(static_cast<float>(locationInfo->population));

	// Give a higher score if more associated actors/ids/countries 
	// also appear in the same document.
	score += getAssociationScore(countryInfo->actorId, publicationDate, actorMentionCounts, actorInfo);

	return ScoredActorMention(score, actorMention);

}

bool ActorMentionFinder::sisterMentionHasClashingActor(const Mention *agentMention, ProperNounActorMention_ptr proposedActor, const DocTheory *docTheory, ActorMentionSet *actorMentionSet)
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
				if (_verbosity > 3) {
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
			MentionConfidence::MentionConfidence confidence = MentionConfidence::determineMentionConfidence(
				docTheory, sentTheory, agentMention2);
			Symbol headword = agentMention2->getNode()->getHeadWord();
			if (confidence != MentionConfidence::TITLE_DESC && confidence != MentionConfidence::APPOS_DESC &&
				confidence != MentionConfidence::WHQ_LINK_PRON && confidence != MentionConfidence::COPULA_DESC &&
				confidence != MentionConfidence::DOUBLE_SUBJECT_PERSON_PRON && 
				confidence != MentionConfidence::ONLY_ONE_CANDIDATE_PRON && confidence != MentionConfidence::ONLY_ONE_CANDIDATE_DESC &&
				confidence != MentionConfidence::NAME_AND_POSS_PRON && headword != Symbol(L"who")) {
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

ActorMentionFinder::ScoredActorMention ActorMentionFinder::makeCompositeActorMention(const AgentMatch &agentMatch, const DocTheory* docTheory, const SentenceTheory *sentTheory, const AgentActorPairs &agentActorPairs, const ActorMentionSet *actorMentions, ActorInfo &actorInfo) 
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
		ActorId pairedActorId;
		Symbol pairedActorCode;
		ActorPatternId pairedActorPatternId;
		Symbol sourceNote;
		if (ProperNounActorMention_ptr p = boost::dynamic_pointer_cast<ProperNounActorMention>(pairedActor.actorMention)) {
			pairedActorId = p->getActorId();
			pairedActorCode = p->getActorCode();
			pairedActorPatternId = p->getActorPatternId();
			sourceNote = AGENT_PATTERN_SYM;
		} else if (CompositeActorMention_ptr p = boost::dynamic_pointer_cast<CompositeActorMention>(pairedActor.actorMention)) {
			// <X for Y for Z> -> <X for Z>
			pairedActorId = p->getPairedActorId();
			pairedActorCode = p->getPairedActorCode();
			pairedActorPatternId = p->getPairedActorPatternId();
			sourceNote = AGENT_OF_AGENT_PATTERN_SYM;
		}
		if (!pairedActorId.isNull()) {
			score += 5;
			// Reduce score if the agent "requires" a country actor, but we don't have one.
			bool country_restriction_applies = false;
			if (actorInfo.isRestrictedToCountryActors(agentMatch.id)) {				
				if (!actorInfo.isACountry(pairedActorId)) {
					country_restriction_applies = true;
					if (actorInfo.isAnIndividual(pairedActorId))
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
					ActorMention::AgentIdentifiers(agentMatch.id, agentMatch.code, agentMatch.patternId), 
					ActorMention::ActorIdentifiers(pairedActorId, pairedActorCode, pairedActorPatternId), 
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
		ActorMention::AgentIdentifiers(agentMatch.id, agentMatch.code, agentMatch.patternId), 
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


bool ActorMentionFinder::isCompatibleAndBetter(ActorMention_ptr oldActorMention, ActorMention_ptr newActorMention) {
	if (CompositeActorMention_ptr old_m = boost::dynamic_pointer_cast<CompositeActorMention>(oldActorMention)) {
		if (CompositeActorMention_ptr new_m = boost::dynamic_pointer_cast<CompositeActorMention>(newActorMention)) {
			if ((old_m->getPairedAgentId() == new_m->getPairedAgentId()) &&
				(old_m->getPairedActorId().isNull()) && 
				(!new_m->getPairedActorId().isNull())) {
				return true;
			}
		}
	}
	return false;
}

void ActorMentionFinder::labelCoreferentMentions(DocTheory *docTheory, bool conservative, ActorMentionSet *result, int sent_cutoff) {
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
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
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

			// If we're being conservative, then we only want to copy the ActorMention
			// to mentions that are strongly linked w/ the entity.
			if (conservative) {
				MentionConfidence::MentionConfidence confidence = MentionConfidence::determineMentionConfidence(
					docTheory, sentTheory, mention);
				Symbol headword = mention->getNode()->getHeadWord();
				if (confidence != MentionConfidence::ANY_NAME &&
					confidence != MentionConfidence::TITLE_DESC && confidence != MentionConfidence::APPOS_DESC &&
					confidence != MentionConfidence::WHQ_LINK_PRON && confidence != MentionConfidence::COPULA_DESC &&
					confidence != MentionConfidence::DOUBLE_SUBJECT_PERSON_PRON && 
					confidence != MentionConfidence::ONLY_ONE_CANDIDATE_PRON && confidence != MentionConfidence::ONLY_ONE_CANDIDATE_DESC &&
					confidence != MentionConfidence::NAME_AND_POSS_PRON && headword != Symbol(L"who"))
					continue;
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
	BOOST_FOREACH(ActorMention_ptr actorMention, newActorMentions) {	
		addActorMention(actorMention, result);
	}
}

// Should this be restricted to PER.Individual, or should it apply to all people?
void ActorMentionFinder::labelPeople(const DocTheory* docTheory, ActorMentionSet *result, ActorInfo &actorInfo, 
									 int sent_cutoff, const AgentActorPairs &agentsOfProperNounActors, 
									 const AgentActorPairs &agentsOfCompositeActors, bool allow_unknowns) 
{
	if (_verbosity > 0)
		SessionLogger::info("ICEWS") << "  Adding default person actor mentions";

	static AgentId citizenAgentId = actorInfo.getAgentByCode(CITIZEN_AGENT_CODE);										
	EntitySet *entitySet = docTheory->getEntitySet();

	for (int i=0; i<entitySet->getNEntities(); ++i) {
		const Entity *ent = entitySet->getEntity(i);
		EntitySubtype entitySubtype = entitySet->guessEntitySubtype(ent);
		if (ent->getType().matchesPER())
		{
			//if ((entitySubtype.getName() != INDIVIDUAL_SYM) &&
			//	(entitySubtype.getName() != UNDET_SYM)) continue;
			for (int m=0; m<ent->getNMentions(); ++m) {
				MentionUID mentionUID = ent->getMention(m);
				int sentno = mentionUID.sentno();
				if (sentno >= sent_cutoff) continue;
				if (!result->find(mentionUID)) {
					ActorMention_ptr actor;
					const Mention *mention = entitySet->getMention(mentionUID);
					const SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
					// Check if there is any actor paired with this mention.
					PairedActorMention pairedActor = findActorForAgent(mention, agentsOfProperNounActors, docTheory);
					if (!pairedActor.actorMention)
						pairedActor = findActorForAgent(mention, agentsOfCompositeActors, docTheory);
					// Build the actor.  If the mention is paired with a country, then
					// it should be a citizen of that country; if the mention is paired
					// with a composite actor, then we just copy that actor; otherwise, 
					// make it a citizen of an unknown actor.
					if (ProperNounActorMention_ptr p = boost::dynamic_pointer_cast<ProperNounActorMention>(pairedActor.actorMention)) {
						if (actorInfo.isACountry(p->getActorId())) {
							actor = boost::make_shared<CompositeActorMention>(
								sentTheory, mention, UNLABELED_PERSON_SYM, 
								ActorMention::AgentIdentifiers(citizenAgentId, CITIZEN_AGENT_CODE, AgentPatternId()), 
								p, pairedActor.agentActorPatternName);
							actor->addSourceNote(L"AGENT-OF-COUNTRY");
						} else if (!actorInfo.isAnIndividual(p->getActorId())) {
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
							ActorMention::AgentIdentifiers(citizenAgentId, CITIZEN_AGENT_CODE, AgentPatternId()), 
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

void ActorMentionFinder::labelLocationsAndFacilities(const DocTheory* docTheory, ActorMentionSet *result, 
													 ActorInfo &actorInfo, int sent_cutoff, 
													 ProperNounActorMention_ptr defaultCountryActorMention,
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
						if (actorInfo.isACountry(p->getActorId()))
							actor = p->copyWithNewEntityMention(sentTheory, mention, L"AGENT-OF-COUNTRY:UNLABELED-LOC");
						else if (!actorInfo.isAnIndividual(p->getActorId()))
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

	BOOST_FOREACH(ActorMention_ptr srcActorMention, *result) {
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
	BOOST_FOREACH(ActorMention_ptr actorMention, newActorMentions) {	
		addActorMention(actorMention, result);
	}
}


int ActorMentionFinder::getSentCutoff(const DocTheory* docTheory) {
	if (_icews_sentence_cutoff < INT_MAX)
		return IcewsSentenceSpan::icewsSentenceNoToSerifSentenceNo(_icews_sentence_cutoff, docTheory);
	else
		return docTheory->getNSentences();
}

void ActorMentionFinder::discardBareActorMentions(ActorMentionSet *actorMentionSet) {
	std::vector<ActorMention_ptr> actorsToDiscard; // don't mutate container while we iterate over it
	BOOST_FOREACH(ActorMention_ptr actor, (*actorMentionSet)) {
		if (! (boost::dynamic_pointer_cast<ProperNounActorMention>(actor) ||
		       boost::dynamic_pointer_cast<CompositeActorMention>(actor)) ) {
			actorsToDiscard.push_back(actor);
		}
	}
	BOOST_FOREACH(ActorMention_ptr actor, actorsToDiscard) {
		SessionLogger::info("ICEWS") << "Discarding actor mention for \""
			<< actor->getEntityMention()->toCasedTextString()
			<< "\" because it has no actor/agent info.";
		actorMentionSet->discardActorMention(actor);
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

void ActorMentionFinder::findUSCities(DocTheory* docTheory, SortedActorMentions& result, ActorInfo &actorInfo, int sent_cutoff) {
	if (_us_state_names.empty()) return;
	static const Symbol::SymbolGroup okRootTags = Symbol::makeSymbolGroup(L"FRAGMENTS FRAG NPA NP");
	// For now, we just handle the special (but fairly common) case of a US city 
	// appearing in its own sentence (usually at the beginning of a document).
	// In particular, we handle the following cases:
    //        (FRAGMENTS^ (NP^ (NPP^ (NNP^ x)...) (, ,)) (NPP (NNP^ x)))
    //             (FRAG^ (NPA (NPP^ (NNP^ x)...) (, ,)  (NPP (NNP^ x))) (.^ x))
    //                   (NPA^ (NPP^ (NNP^ x)...) (, ,)  (NPP (NNP^ x))  (. x))
    //                    (NP^ (NPP^ (NNP^ x)...) (, ,)  (NPP (NNP^ x))  (. x))
	if (_useDefaultLocationResolution) {
		for (int sentno=0; sentno<std::min(docTheory->getNSentences(), sent_cutoff); ++sentno) {
			const SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
			const MentionSet *mset = sentTheory->getMentionSet();
			const SynNode *root = sentTheory->getPrimaryParse()->getRoot();
			// Do some checks to make sure that this looks like the right "type" of sentence;
			// if not, then move on.
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
			const Mention* stateMention = mset->getMention(2);
			static const ActorId usa_actorid = actorInfo.getActorByCode(USA_ACTOR_CODE);
			ActorMention::ActorIdentifiers usa(usa_actorid, USA_ACTOR_CODE);
			ActorMention_ptr cityActor = boost::make_shared<ProperNounActorMention>(
				sentTheory, cityMention, US_CITY_SYM, usa);
			ActorMention_ptr stateActor = boost::make_shared<ProperNounActorMention>(
				sentTheory, stateMention, US_CITY_SYM, usa);
			result.insert(ScoredActorMention(US_CITY_SCORE, cityActor));
			result.insert(ScoredActorMention(US_CITY_SCORE, stateActor));
			// Propagate to other mentions of this place
			const Entity *ent = docTheory->getEntitySet()->getEntityByMention(cityMention->getUID());
			if (ent != 0) {
				for (int m=0; m<ent->getNMentions(); ++m) {
					MentionUID propagatedMentionUID = ent->getMention(m);
					if (propagatedMentionUID == cityMention->getUID())
						continue;
					int new_sentno = propagatedMentionUID.sentno();
					if (new_sentno >= sent_cutoff) continue;
					const Mention *propagatedMention = docTheory->getEntitySet()->getMention(propagatedMentionUID);
					ActorMention_ptr propagatedActor = boost::make_shared<ProperNounActorMention>(docTheory->getSentenceTheory(new_sentno), 
						propagatedMention, US_CITY_SYM, usa);
					result.insert(ScoredActorMention(US_CITY_SCORE, propagatedActor));
				}
			}
		}
	}
	else {
		Gazetteer::GeoResolution_ptr usa_geo_resolution = _gazetteer.getCountryResolution(L"us");
		for (int sentno=0; sentno<std::min(docTheory->getNSentences(), sent_cutoff); ++sentno) {
			const SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
			const MentionSet *mset = sentTheory->getMentionSet();
			const SynNode *root = sentTheory->getPrimaryParse()->getRoot();
			// Do some checks to make sure that this looks like the right "type" of sentence;
			// if not, then move on.
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
			const Mention* stateMention = mset->getMention(2);
			Gazetteer::GeoResolution_ptr stateRes = _locationMentionResolver.getResolutionInCountry(stateMention, Symbol(L"US"));
			if (stateRes->isEmpty) stateRes = usa_geo_resolution;
			Gazetteer::GeoResolution_ptr cityRes = _locationMentionResolver.getResolutionInCountry(cityMention, Symbol(L"US"));
			if (cityRes->isEmpty) cityRes = stateRes;
			static const ActorId usa_actorid = actorInfo.getActorByCode(USA_ACTOR_CODE);
			ActorMention::ActorIdentifiers usa(usa_actorid, USA_ACTOR_CODE);
			ActorMention_ptr cityActor = boost::make_shared<ProperNounActorMention>(
				sentTheory, cityMention, US_CITY_SYM, usa, cityRes);
			ActorMention_ptr stateActor = boost::make_shared<ProperNounActorMention>(
				sentTheory, stateMention, US_CITY_SYM, usa, stateRes);
			// add actors to icews actor set
			result.insert(ScoredActorMention(US_CITY_SCORE, cityActor));
			result.insert(ScoredActorMention(US_CITY_SCORE, stateActor));
			// register resolutions so that they can be used later on
			std::wstring city_string = cityMention->getAtomicHead()->toTextString();
			city_string = _gazetteer.toCanonicalForm(city_string);
			std::wstring state_string = stateMention->getAtomicHead()->toTextString();
			state_string = _gazetteer.toCanonicalForm(state_string);
			_locationMentionResolver.registerResolution(city_string, cityRes);
			_locationMentionResolver.registerResolution(state_string, stateRes);
		}
	}
}

void ActorMentionFinder::resolveAmbiguousLocationsToDefaultCountry(ProperNounActorMention_ptr defaultCountryActorMention, const DocTheory* docTheory, ActorInfo &actorInfo, ActorMentionSet *result, int sent_cutoff) {
	if (_verbosity > 3)
		SessionLogger::info("ICEWS") << "  Resolving ambiguous locations to default country for doc.";

	Gazetteer::GeoResolution_ptr docResolution = defaultCountryActorMention->getGeoResolution();
	if (!docResolution->isEmpty)
	{
		const Gazetteer::CountryInfo_ptr countryInfo = docResolution->countryInfo;
		SortedActorMentions defaultActorMentions;
		for (size_t sentno=0; sentno<static_cast<size_t>(sent_cutoff); ++sentno) {
			const SentenceTheory *sentTheory = docTheory->getSentenceTheory(sentno);
			const MentionSet *ms = sentTheory->getMentionSet();
			for (int m = 0; m < ms->getNMentions(); m++) {
				const Mention *ment = ms->getMention(m);
				EntityType entityType = ment->getEntityType();
				if ((ment->getMentionType() == Mention::NAME) && (entityType.matchesLOC() || entityType.matchesGPE())) {
					std::wstring head_string = ment->getAtomicHead()->toTextString();
					std::wstring full_string = ment->toCasedTextString();
					boost::trim(head_string);
					boost::trim(full_string);
					if (_locationMentionResolver.isBlockedLocation(ment)) continue;
					ActorMention_ptr target = result->find(ment->getUID());
					if (target) 
					{	
						// Check if another country is explicitly mentioned in this sentence, if so block resolution
						ProperNounActorMention_ptr otherCountry = defaultCountryAssignmentIsBlockedByOtherCountry(target, defaultCountryActorMention, result, actorInfo);
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
							ActorMention::ActorIdentifiers(countryInfo->actorId, countryInfo->actorCode, ActorPatternId()),
							docResolution);
						defaultActorMentions.insert(ScoredActorMention(50, actorMention));
					}
				}
			}
		}
		greedilyAddActorMentions(defaultActorMentions, result, actorInfo);
	}
}

} // end of namespace ICEWS

