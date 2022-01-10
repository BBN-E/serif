// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "ICEWS/EventMentionFinder.h"
#include "ICEWS/EventType.h"
#include "ICEWS/EventMention.h"
#include "ICEWS/EventMentionSet.h"
#include "ICEWS/ActorMentionSet.h"
#include "ICEWS/ActorMentionFinder.h"
#include "ICEWS/ActorMentionPattern.h"
#include "ICEWS/EventMentionPattern.h"
#include "ICEWS/ActorInfo.h"
#include "ICEWS/SentenceSpan.h"
#include "ICEWS/TenseDetection.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/patterns/PatternSet.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/TopLevelPFeature.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/features/PropPFeature.h"
#include "Generic/patterns/features/MentionPFeature.h"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <limits.h>
#include <boost/scoped_ptr.hpp>

namespace {
	Symbol UNLABELED_PATTERN_SYM(L"UNLABELED_PATTERN");
	Symbol COPY_ACTOR_SRC_SYM(L"COPY_ACTOR_SRC");
	Symbol COPY_ACTOR_DST_SYM(L"COPY_ACTOR_DST");
	Symbol BLOCK_SYM(L"BLOCK");
	Symbol EVENT_TENSE_SYM(L"event-tense");

}

namespace ICEWS {

ICEWSEventMentionFinder::ICEWSEventMentionFinder() {
	_create_events_with_generic_actors = ParamReader::getOptionalTrueFalseParamWithDefaultVal("icews_create_events_with_generic_actors", true);
	
	// Determine what domain we should be unique over.
	_event_uniqueness_groups = parseEventUniquenessGroupSpec("icews_event_uniqueness", 
		"ONE_GROUP_PER_MENTION_PAIR_PER_EVENT_GROUP_PER_ICEWS_SENTENCE"); // <-- default value
	_event_override_groups = parseEventUniquenessGroupSpec("icews_event_overrides",
		"ONE_GROUP_PER_MENTION_PAIR"); // <-- default value
	BOOST_FOREACH(size_t event_overrid_group, _event_override_groups) {
		if (event_overrid_group & UNIQUE_PER_EVENT_TYPE)
			throw UnexpectedInputException("ICEWSEventMentionFinder::ICEWSEventMentionFinder", 
				"icews_event_overrides should not include _PER_EVENT_TYPE");
	}

	_icews_sentence_cutoff = ParamReader::getOptionalIntParamWithDefaultValue("icews_sentence_cutoff", INT_MAX);
	// Load event types.  Event types may be read from either a file or a database.
	ICEWSEventType::loadEventTypes();

	std::vector<std::wstring> enabledEventCodes = ParamReader::getWStringVectorParam("icews_event_codes");
	BOOST_FOREACH(const std::wstring& patternStr, enabledEventCodes) {
		// Escape everything except '*', then replace '*' with '.*'
		static const boost::wregex escape(L"([\\.\\[\\{\\}\\(\\)\\+\\?\\|\\^\\$\\\\])");
		static const std::wstring rep(L"\\\\\\1");
		std::wstring escapedPatternStr = boost::regex_replace(patternStr, escape, rep);
		boost::algorithm::replace_all(escapedPatternStr, L"*", L".*");
		try {
			_enabledEventCodes.push_back(boost::wregex(escapedPatternStr));
		} catch (boost::regex_error &) {
			throw UnexpectedInputException("ICEWSEventMentionFinder::ICEWSEventMentionFinder",
				"Bad value for icews_event_codes parameter: ", ParamReader::getParam("icews_event_codes").c_str());
		}
	}
	if (_enabledEventCodes.empty()) 
		_enabledEventCodes.push_back(boost::wregex(L".*"));

	// Load our pattern sets.
	bool encrypted_patterns = ParamReader::isParamTrue("icews_encrypt_patterns");
	loadPatternSets(ParamReader::getParam("icews_event_patterns"), _patternSets);
	SessionLogger::info("ICEWS") << "Loaded " << _patternSets.size() << " pattern sets" << std::endl;

	std::string blockedEventFilename = ParamReader::getParam("icews_block_event_patterns");
	if (!blockedEventFilename.empty())
		_blockEventPatternSet = boost::make_shared<PatternSet>(blockedEventFilename.c_str(), encrypted_patterns);

	std::string blockedContingentEventFilename = ParamReader::getParam("icews_block_contingent_event_patterns");
	if (!blockedContingentEventFilename.empty())
		_blockContingentEventPatternSet = boost::make_shared<PatternSet>(blockedContingentEventFilename.c_str(), encrypted_patterns);
	
	std::string eventTenseFilename = ParamReader::getParam("icews_event_tense_patterns");
	if (!eventTenseFilename.empty())
		_eventTensePatternSet = boost::make_shared<PatternSet>(eventTenseFilename.c_str(), encrypted_patterns);

	std::string blockEventLocPairedActorPatternsFilename = ParamReader::getParam("icews_block_event_loc_paired_actor_patterns");
	if (!blockEventLocPairedActorPatternsFilename.empty())
		_blockEventLocPairedActorPatternSet = boost::make_shared<PatternSet>(blockEventLocPairedActorPatternsFilename.c_str(), encrypted_patterns);

	std::string propagateActorLabelPatternsFilename = ParamReader::getParam("icews_propagate_event_actor_label_patterns");
	if (!propagateActorLabelPatternsFilename.empty())
		_propagageActorLabelPatternSet = boost::make_shared<PatternSet>(propagateActorLabelPatternsFilename.c_str(), encrypted_patterns);

	std::string replaceEventTypePatternsFilename = ParamReader::getParam("icews_replace_event_type_patterns");
	if (!replaceEventTypePatternsFilename.empty())
		_replaceEventTypePatternSet = boost::make_shared<PatternSet>(replaceEventTypePatternsFilename.c_str(), encrypted_patterns);
	
}

std::vector<size_t> ICEWSEventMentionFinder::parseEventUniquenessGroupSpec(const char* paramName, const char *defaultValue) {
	std::vector<size_t> result;
	std::vector<std::string> descriptions = ParamReader::getStringVectorParam(paramName);
	if (descriptions.empty())
		descriptions.push_back(defaultValue);
	BOOST_FOREACH(std::string description, descriptions) {
		boost::trim(description);
		size_t uniqueness = 0;
		// Input should have the form: "ONE_GROUP_PER_X_PER_Y_PER_Z"
		static const boost::regex descrCheckRegex("^ONE_GROUP(_PER_[A-Z_]+)+$");
		if (!boost::regex_match(description, descrCheckRegex)) {
			throw UnexpectedInputException("ICEWSEventMentionFidner::determineUniqueness",
				"icews_event_uniqueness parameter must have the form ONE_GROUP_PER_XXX_PER_YYY...");
		}
		description = description.substr(9); // strip of the "ONE_GROUP" prefix.
		typedef boost::split_iterator<string::iterator> SplitIter;
		for(SplitIter it=boost::make_split_iterator(description, boost::first_finder("_PER_", boost::is_iequal())); it!=SplitIter(); ++it) {
			std::string domain = boost::copy_range<std::string>(*it);
			if (domain.empty()) continue;
			if (boost::iequals(domain, "event_type"))
				uniqueness |= UNIQUE_PER_EVENT_TYPE;
			else if (boost::iequals(domain, "event_group"))
				uniqueness |= UNIQUE_PER_EVENT_GROUP;
			else if (boost::iequals(domain, "sentence"))
				uniqueness |= UNIQUE_PER_SENTENCE;
			else if (boost::iequals(domain, "icews_sentence"))
				uniqueness |= UNIQUE_PER_ICEWS_SENTENCE;
			else if (boost::iequals(domain, "mention_pair"))
				uniqueness |= UNIQUE_PER_MENTION_PAIR;
			else if (boost::iequals(domain, "entity_pair"))
				uniqueness |= UNIQUE_PER_ENTITY_PAIR;
			else if (boost::iequals(domain, "actor_pair"))
				uniqueness |= UNIQUE_PER_ACTOR_PAIR;
			else if (boost::iequals(domain, "proper_noun_actor_pair"))
				uniqueness |= UNIQUE_PER_PROPER_NOUN_ACTOR_PAIR;
			else
				throw UnexpectedInputException("ICEWSEventMentionFidner::determineUniqueness",
				    "Unexpected domain for icews_event_uniqueness parameter: ", domain.c_str());
		}
		result.push_back(uniqueness);
	}
	return result;
}


bool ICEWSEventMentionFinder::eventCodeEnabled(Symbol eventCode) {
	BOOST_FOREACH(const boost::wregex &pattern, _enabledEventCodes) {
		if (boost::regex_match(eventCode.to_string(), pattern))
			return true;
	}
	return false;
}


void ICEWSEventMentionFinder::loadPatternSets(const std::string &patternSetFileList, std::vector<PatternSetAndCode> &patternSets) {
	// Load the pattern sets that we'll use to find each event type
	if (patternSetFileList.empty()) return;
	bool encrypted_patterns = ParamReader::isParamTrue("icews_encrypt_patterns");

	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(patternSetFileList.c_str()));
	UTF8InputStream& stream(*stream_scoped_ptr);
	if( (!stream.is_open()) || (stream.fail()) ){
		std::stringstream errmsg;
		errmsg << "Failed to open icews_event_patterns file \"" << patternSetFileList << "\"";
		throw UnexpectedInputException( "ICEWSEventMentionFinder::loadPatternSets()", errmsg.str().c_str() );
	}

	while (!(stream.eof() || stream.fail())) {
		std::wstring wline;
		std::getline(stream, wline);
		wline = wline.substr(0, wline.find_first_of(L'#')); // Remove comments.
		boost::trim(wline);
		std::string line = UnicodeUtil::toUTF8StdString(wline);

		if (!line.empty()) {
			// Expand parameter variables.
			try {
				line = ParamReader::expand(line);
			} catch (UnexpectedInputException &e) {
				std::ostringstream prefix;
				prefix << "while processing icews_events_pattern file " << patternSetFileList << ": ";
				e.prependToMessage(prefix.str().c_str());
				throw;
			}

			// Parse the line
			std::string::size_type splitpos = line.find_first_of(":");
			if (splitpos == std::string::npos)
				throw UnexpectedInputException("ICEWSEventMentionFinder::loadPatternSets",
					"Parse error: expected each line to contain 'code:filename'");
			std::string eventCode = line.substr(0, splitpos);
			std::string filename = line.substr(splitpos+1);
			boost::trim(eventCode);
			boost::trim(filename);
			if (eventCode.empty() || filename.empty())
				throw UnexpectedInputException("ICEWSEventMentionFinder::loadPatternSets",
					"Parse error: expected each line to contain 'code:filename'");
			std::wstring eventCodeWstr(eventCode.begin(), eventCode.end());
			Symbol eventCodeSym(eventCodeWstr); // default event code for this pattern set.
			// Check that the event code exists.
			if (!ICEWSEventType::getEventTypeForCode(eventCodeSym))
				throw UnexpectedInputException("ICEWSEventMentionFinder::loadPatternSets",
					"Event type not found: ", eventCode.c_str());
			// 
			if (!eventCodeEnabled(eventCodeSym.to_string()))
				continue;
			// Normalize filename
			boost::algorithm::replace_all(filename, "/", SERIF_PATH_SEP);
			boost::algorithm::replace_all(filename, "\\", SERIF_PATH_SEP);
			// Load the pattern set, and add it to our list of pattern sets.
			try {
				PatternSet_ptr patternSet = boost::make_shared<PatternSet>(filename.c_str(), encrypted_patterns);
				patternSets.push_back(PatternSetAndCode(patternSet, eventCodeSym));
			} catch (UnrecoverableException &exc) {
				std::ostringstream prefix;
				prefix << "While processing \"" << filename << "\": ";
				exc.prependToMessage(prefix.str().c_str());
				throw;
			}
		}
	}
}

void ICEWSEventMentionFinder::process(DocTheory *docTheory) {
	if (_patternSets.empty()) return; // Nothing to do!

	ActorInfo_ptr actorInfo = ActorInfo::getActorInfoSingleton();

	// Find actor mentions.
	ActorMentionSet *actorMentions = docTheory->getSubtheory<ActorMentionSet>();
	if (!actorMentions) {
		SessionLogger::err("ICEWS") << "No ICEWS ActorMentionSet found; creating an empty one!";
		actorMentions = new ActorMentionSet();
		docTheory->takeSubtheory(actorMentions);
	}

	// Construct an event mention set to store our results.
	ICEWSEventMentionSet* eventMentionSet = _new ICEWSEventMentionSet();
	docTheory->takeSubtheory(eventMentionSet);

	int n_sentences = std::min(IcewsSentenceSpan::icewsSentenceNoToSerifSentenceNo(_icews_sentence_cutoff, docTheory), 
		docTheory->getNSentences());		

	// Check for mentions that should not be assigned paired actors.
	PatternMatcher_ptr blockPairedActorsPatternMatcher;
	if (_blockEventLocPairedActorPatternSet)
		blockPairedActorsPatternMatcher = PatternMatcher::makePatternMatcher(docTheory, _blockEventLocPairedActorPatternSet);
	std::map<MentionUID, Symbol> blockedPairedActors = ActorMentionFinder::findMentionsThatBlockDefaultPairedActor(docTheory, blockPairedActorsPatternMatcher);

	// Run each pattern set on the document.
	BOOST_FOREACH(PatternSetAndCode pair, _patternSets) {
		if (pair.first->getNTopLevelPatterns() == 0) 
			continue; // this is just a stub file; it contains no real patterns yet.
		PatternMatcher_ptr patternMatcher = PatternMatcher::makePatternMatcher(docTheory, pair.first);
		actorMentions->addEntityLabels(patternMatcher, *actorInfo);
		applyPatternSet(eventMentionSet, patternMatcher, pair.second, actorMentions, n_sentences, blockedPairedActors);
	}
	// Propagate actor labels between event participants
	propagateActorLabels(eventMentionSet, docTheory);

	// Run the replace_event_type patterns, and replace any event types that
	// are indicated by those patterns.
	applyReplaceEventTypePatterns(eventMentionSet, docTheory);

	// Run the block_event patterns, and discard any events that they match.
	removeBlockedEvents(eventMentionSet, docTheory, _blockEventPatternSet);

	// Remove duplicate/overridden events
	if (eventMentionSet->size() > 0) {
		BOOST_FOREACH(size_t event_grouping, _event_override_groups)
			removeOverriddenEvents(eventMentionSet, docTheory, event_grouping, true);		
		BOOST_FOREACH(size_t event_grouping, _event_override_groups)
			removeOverriddenEvents(eventMentionSet, docTheory, event_grouping, false);
	}
	
	// Run the "contingent" block_event patterns and discard any events that they match
	// These are meant to run after all "bad" events are gone and only good ones remain;
	// These are essentially meant to be sophisticated duplicate removal
	removeBlockedEvents(eventMentionSet, docTheory, _blockContingentEventPatternSet);

	if (eventMentionSet->size() > 0) {
		BOOST_FOREACH(size_t event_grouping, _event_uniqueness_groups)
			removeDuplicateEvents(eventMentionSet, docTheory, event_grouping);
	}

	// Discard any events whose event_type has a NULL id -- these are just used to
	// block the events that we care about.
	removeTemporaryEvents(eventMentionSet);

	// Tag events with tense
	tagEventTense(eventMentionSet, docTheory);

	std::wostringstream msg;
	msg << L"Found " << eventMentionSet->size() << L" events in " 
		<< docTheory->getDocument()->getName() << ":" << std::endl;
	BOOST_FOREACH(ICEWSEventMention_ptr eventMention, *eventMentionSet) {
		eventMention->dump(msg, "  ");
		msg << L"\n\n";
	}
	SessionLogger::info("ICEWS") << msg.str();
	//std::wcout << msg.str();
}

void ICEWSEventMentionFinder::applyPatternSet(ICEWSEventMentionSet* eventMentionSet, PatternMatcher_ptr patternMatcher, Symbol defaultEventCode, ActorMentionSet *actorMentions, int n_sentences, const std::map<MentionUID, Symbol> &blockedPairedActors) {
	ActorInfo_ptr actorInfo = ActorInfo::getActorInfoSingleton();
	for (int sentno=0; sentno<n_sentences; ++sentno) {
		SentenceTheory *sentTheory = patternMatcher->getDocTheory()->getSentenceTheory(sentno);
		std::vector<PatternFeatureSet_ptr> matches = patternMatcher->getSentenceSnippets(sentTheory, 0, true);
		BOOST_FOREACH(PatternFeatureSet_ptr match, matches) {
			MatchData matchData = extractMatchData(match, defaultEventCode);
			if (!checkMatchData(matchData))
				continue;
			ProperNounActorMention_ptr eventLoc = determineEventLocation(match, sentTheory, actorMentions);
			if (eventLoc)
				setDefaultLocations(matchData, eventLoc, actorMentions, sentTheory, blockedPairedActors);
			TenseDetection::setTense(match, matchData, patternMatcher->getDocTheory(), sentTheory);
			ICEWSEventType_ptr eventType = ICEWSEventType::getEventTypeForCode(matchData.eventCode);
			addEventMentions(eventMentionSet, patternMatcher->getDocTheory(), matchData, actorMentions);
		}
	}
}

void ICEWSEventMentionFinder::setDefaultLocations(const MatchData& matchData, ProperNounActorMention_ptr eventLoc, ActorMentionSet *actorMentions, const SentenceTheory *sentTheory, const std::map<MentionUID, Symbol> &blockedPairedActors) {
	typedef std::pair<Symbol, std::vector<const Mention*> > RoleAndMentions;
	BOOST_FOREACH(const RoleAndMentions &roleAndMentions, matchData.roleMentions) {
		BOOST_FOREACH(const Mention* mention, roleAndMentions.second) {

			ActorMention_ptr actorMention = actorMentions->find(mention->getUID());
			if (actorMention) {
				if (CompositeActorMention_ptr ca = boost::dynamic_pointer_cast<CompositeActorMention>(actorMention)) {
					if (ca->getPairedActorId().isNull()) {
						if (blockedPairedActors.find(ca->getEntityMention()->getUID()) != blockedPairedActors.end()) {
							SessionLogger::info("ICEWS") << "  Blocked from setting paired actor for \"" 
								<< actorMention->getEntityMention()->toCasedTextString() << "\" "
								<< actorMention << " to " << ActorMention_ptr(eventLoc) << "\n        Blocked by pattern: \""
								<< blockedPairedActors.find(actorMention->getEntityMention()->getUID())->second << "\"";
						} else {
							SessionLogger::info("ICEWS") << "  Setting paired actor for " << ActorMention_ptr(ca)
								<< " based on event location to " << ActorMention_ptr(eventLoc);
							ca->setPairedActorMention(eventLoc, L"EVENT-BASED-LOC:");
						}
					}
				}
			}
		}
	}
}

ProperNounActorMention_ptr ICEWSEventMentionFinder::determineEventLocation(PatternFeatureSet_ptr match, const SentenceTheory *sentTheory, const ActorMentionSet *actorMentions) {
	MentionSet *mentionSet = sentTheory->getMentionSet();
	PropositionSet *propSet = sentTheory->getPropositionSet();

	for (size_t f = 0; f < match->getNFeatures(); f++) {
		if (PropPFeature_ptr sf = boost::dynamic_pointer_cast<PropPFeature>(match->getFeature(f))) {
			ProperNounActorMention_ptr result = getLocationForProp(sf->getProp(), sentTheory, actorMentions);
			if (result) return result;
		} else if (MentionPFeature_ptr sf = boost::dynamic_pointer_cast<MentionPFeature>(match->getFeature(f))) {
			ProperNounActorMention_ptr result = getLocationForProp(propSet->getDefinition(sf->getMention()->getIndex()), sentTheory, actorMentions);
			if (result) return result;
		}
	}
	return ProperNounActorMention_ptr();
}

ProperNounActorMention_ptr ICEWSEventMentionFinder::getLocationForProp(const Proposition *prop, const SentenceTheory *sentTheory, const ActorMentionSet *actorMentions) {
	if (!prop) return ProperNounActorMention_ptr();

	MentionSet *ms = sentTheory->getMentionSet();
	for (int a = 0; a < prop->getNArgs(); a++) {
		const Argument *arg = prop->getArg(a);
		if ((arg->getRoleSym() == Symbol(L"in") || arg->getRoleSym() == Symbol(L"near") || arg->getRoleSym() == Symbol(L"outside") || arg->getRoleSym() == Symbol(L"inside")) 
			&& arg->getType() == Argument::MENTION_ARG) 
		{
			const Mention *ment = ms->getMention(arg->getMentionIndex());
			Symbol headword = ment->getNode()->getHeadWord();
			if (headword == Symbol(L"embassy") || headword == Symbol(L"consulate"))
				continue;
			if (ment->getEntityType().matchesGPE() || ment->getEntityType().matchesLOC() || ment->getEntityType().matchesFAC()) {
				ProperNounActorMention_ptr actor = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMentions->find(ment->getUID()));
				if (actor) return actor;
			}
		}
	}
	return ProperNounActorMention_ptr();
}

ICEWSEventMentionFinder::MatchData ICEWSEventMentionFinder::extractMatchData(PatternFeatureSet_ptr match, Symbol defaultEventCode) {
	MatchData matchData(defaultEventCode);
	for (size_t f = 0; f < match->getNFeatures(); f++) {
		PatternFeature_ptr sf = match->getFeature(f);
		if (TopLevelPFeature_ptr tsf = boost::dynamic_pointer_cast<TopLevelPFeature>(sf)) {
			matchData.patternLabel = tsf->getPatternLabel();
		} else if (MentionReturnPFeature_ptr mrpf = boost::dynamic_pointer_cast<MentionReturnPFeature>(sf)) {
			std::vector<std::wstring> roles;
			std::wstring returnLabel = mrpf->getReturnLabel().to_string();
			boost::split(roles, returnLabel, boost::is_any_of("+"));
			BOOST_FOREACH(std::wstring role, roles)
				matchData.roleMentions[Symbol(role)].push_back(mrpf->getMention());
		} else if (ReturnPatternFeature_ptr rsf = boost::dynamic_pointer_cast<ReturnPatternFeature>(sf)) {
			if (rsf->hasReturnValue(L"event-code"))  {
				Symbol newEventCode = Symbol(rsf->getReturnValue(L"event-code"));
				if (matchData.eventCode == defaultEventCode) {
					// If we only have the default so far, then always override
					matchData.eventCode = newEventCode;
				} else {
					// But if we've got more than one override, try to figure out which one is better
					// This could still be random based on the order of features if an override is not
					// specified one way or the other. So, be sure to specify overrides if you are going 
					// to have more than one possible type in a given match!
					ICEWSEventType_ptr existingEventType = ICEWSEventType::getEventTypeForCode(matchData.eventCode);
					ICEWSEventType_ptr newEventType = ICEWSEventType::getEventTypeForCode(newEventCode);
					if (newEventType->overridesEventType(existingEventType))
						matchData.eventCode = newEventCode;
				}
			}
			if (rsf->hasReturnValue(EVENT_TENSE_SYM.to_string()))
				matchData.tense = Symbol(rsf->getReturnValue(EVENT_TENSE_SYM.to_string()));			
		}
	}
	return matchData;
}

// Todo: should this generate exceptions rather than session log messages?
bool ICEWSEventMentionFinder::checkMatchData(MatchData& matchData) {
	// Check the pattern label.
	if (matchData.patternLabel.is_null()) {
		SessionLogger::warn("ICEWS") << "Pattern without top-level ID; setting to null\n";
		matchData.patternLabel = Symbol(L"NO_PATTERN_ID");
	}
	ICEWSEventType_ptr eventType = ICEWSEventType::getEventTypeForCode(matchData.eventCode);

	if (!eventType) {
		SessionLogger::err("ICEWS") << "Unexpected event type: " << matchData.eventCode;
		return false;
	}

	if (!ICEWSEventMention::isValidTense(matchData.tense))
	{
		SessionLogger::err("ICEWS") << "Unexpected tense (resetting to NEUTRAL): " << matchData.tense;
		matchData.tense = ICEWSEventMention::NEUTRAL_TENSE;
	}

	// Check the event participants.
	typedef std::pair<Symbol, std::vector<const Mention*> > RoleMentionsPair;
	BOOST_FOREACH(RoleMentionsPair roleMentionsPair, matchData.roleMentions) {
		const Symbol &role = roleMentionsPair.first;
		if (!eventType->hasRole(role)) {
			SessionLogger::err("ICEWS") << "Unexpected role \"" << role
				<< "\" for event type \"" << eventType << "\"";
			return false;
		}
	}
	BOOST_FOREACH(Symbol role, eventType->getRequiredRoles()) {
		if (matchData.roleMentions.find(role) == matchData.roleMentions.end()) {
			SessionLogger::err("ICEWS") << "Required role \"" << role
				<< "\" for event type \"" << eventType << "\" not found; pattern = " << matchData.patternLabel;
			return false;
		}
	}
	return true;
}

void ICEWSEventMentionFinder::addEventMentions(ICEWSEventMentionSet* eventMentionSet, const DocTheory *docTheory, 
											   MatchData& matchData, ActorMentionSet *actorMentions) 
{
	// need to take all combinations of participants, and add an event for each one.  E.g., 
	// if we have two values for TARGET and two for SOURCE, then add 4 events.  What do
	// I want to do here w/ reflexive events (which really only have one participant type,
	// even though it's labeled as two)?  Or events that imply other events?

	ICEWSEventType_ptr eventType = ICEWSEventType::getEventTypeForCode(matchData.eventCode);
	std::vector<Symbol> roles;
	for (RoleMentions::iterator it=matchData.roleMentions.begin(); it!=matchData.roleMentions.end(); ++it)
		roles.push_back((*it).first);

	ParticipantMap participantMap;
	std::set<const Mention*> mentionsUsed;
	addEventMentionsHelper(eventMentionSet, docTheory, eventType, matchData, participantMap, mentionsUsed, roles, actorMentions);
}

void ICEWSEventMentionFinder::addEventMentionsHelper(ICEWSEventMentionSet* eventMentionSet, // <- result goes here
													 const DocTheory *docTheory,
													 ICEWSEventType_ptr eventType,
													 MatchData& matchData,
													 ParticipantMap &participantMap, 
													 std::set<const Mention*> mentionsUsed,
													 std::vector<Symbol> &roles, 
													 ActorMentionSet *actorMentions)
{
	if (roles.empty()) {
		ICEWSEventMention_ptr eventMention = boost::make_shared<ICEWSEventMention>(eventType, participantMap, matchData.patternLabel, matchData.tense);
		eventMentionSet->addEventMention(eventMention);
		std::wostringstream msg;
		msg << "[" << matchData.patternLabel << "] found candidate:\n";
		eventMention->dump(msg, "    ");
		SessionLogger::info("ICEWS") << msg;
	} else {
		// Remove the role
		Symbol role = roles.back();
		roles.pop_back();
		// Recurse for each role value
		BOOST_FOREACH(const Mention* mention, matchData.roleMentions[role]) {
			// Check if this role conflicts with any role we already have.
			bool conflict = false;
			if (mentionsUsed.find(mention) != mentionsUsed.end())
				conflict = true;
			const Entity *entity = docTheory->getEntitySet()->getEntityByMention(mention->getUID());
			typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
			BOOST_FOREACH(const ParticipantPair& roleAndParticipant, participantMap) {
				if (!eventType->rolesCanBeFilledByTheSameEntity(role, roleAndParticipant.first)) {
					const Mention* usedMention = roleAndParticipant.second->getEntityMention();
					const Entity *usedEntity = docTheory->getEntitySet()->getEntityByMention(usedMention->getUID());
					if (usedEntity == entity)
						conflict = true;
				}
			}
			if (!conflict) {
				const SentenceTheory* sentTheory = docTheory->getSentenceTheory(mention->getSentenceNumber());
				participantMap[role] = getActorForMention(sentTheory, mention, actorMentions);
				mentionsUsed.insert(mention);
				addEventMentionsHelper(eventMentionSet, docTheory, eventType, matchData,
					participantMap, mentionsUsed, roles, actorMentions);
				mentionsUsed.erase(mention);
			}
		}
		// Restore "roles" and "participantMap" to their original values.
		roles.push_back(role);
		participantMap.erase(role);
	}
}

namespace {
	Symbol UNKNOWN_ACTOR_SYM(L"UNKNOWN_ACTOR");
}

ActorMention_ptr ICEWSEventMentionFinder::getActorForMention(const SentenceTheory* sentTheory, const Mention *mention, ActorMentionSet *actorMentions) {
	// [XX] Currently, we only check the mention itself; however, we could 
	// potentially follow coref chains to find a coreferent mention that's 
	// in the actorMentions map.
	ActorMention_ptr actorMention = actorMentions->find(mention->getUID());

	// If no actor mention is found, then create a "generic" one (not connected
	// to any specific actor/agent)
	if ((!actorMention) && _create_events_with_generic_actors) {
		actorMention = boost::make_shared<ActorMention>(sentTheory, mention, UNKNOWN_ACTOR_SYM);
		actorMentions->addActorMention(actorMention);
	}
	return actorMention;
}

std::map<std::wstring, ICEWSEventMentionFinder::EventMentionGroup> ICEWSEventMentionFinder::divideEventsIntoUniquenessGroups(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory, size_t event_grouping, bool per_event_type) {
	std::map<std::wstring, EventMentionGroup> eventMentionGroups;
	BOOST_FOREACH(ICEWSEventMention_ptr eventMention, *eventMentionSet) {
		std::wstring key = getEventGroupingKey(eventMention, docTheory, event_grouping, per_event_type);
		eventMentionGroups[key].push_back(eventMention);
	}
	return eventMentionGroups;
}

std::wstring ICEWSEventMentionFinder::getEventGroupingKey(ICEWSEventMention_ptr eventMention, const DocTheory *docTheory, size_t event_grouping, bool per_event_type) {
	std::wostringstream key;
	// we are relying on the fact that this is a std::map, and thus sorted;
	// if we were using a hashmap, then we would need to do our own sort to
	// ensure that the parameters always appear in the same orders.
	const ParticipantMap &participants = eventMention->getParticipantMap();
	typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
	bool first = true;
	int max_sentnum = 0;
	int max_icews_sentnum = 0;
	BOOST_FOREACH(const ParticipantPair &participantPair, participants) {
		if (!first) key << ",";
		first = false;
		key << participantPair.first << L"=";
		ActorMention_ptr actor = participantPair.second;
		MentionUID actorMentionId = actor->getEntityMention()->getUID();
		const SynNode *actorNode = actor->getEntityMention()->getNode();
		EDTOffset actorPos = actor->getSentence()->getTokenSequence()->getToken(actorNode->getStartToken())->getStartEDTOffset();
		max_sentnum = std::max(max_sentnum, actorMentionId.sentno());
		max_icews_sentnum = std::max(max_icews_sentnum,
			IcewsSentenceSpan::edtOffsetToIcewsSentenceNo(actorPos, docTheory));
		if (event_grouping & UNIQUE_PER_MENTION_PAIR) {
			key << L"M(" << actorMentionId.toInt() << L")";
		}
		if (event_grouping & UNIQUE_PER_ENTITY_PAIR) {
			const Entity *ent = docTheory->getEntitySet()->getEntityByMention(actorMentionId);
			key << L"E(" << ent->getID() << L")";
		}
		if (event_grouping & UNIQUE_PER_ACTOR_PAIR) {
			if (ProperNounActorMention_ptr actor = boost::dynamic_pointer_cast<ProperNounActorMention>(participantPair.second)) {
				key << L"A(" << actor->getActorId().getId() << L")";
			} else if (CompositeActorMention_ptr actor = boost::dynamic_pointer_cast<CompositeActorMention>(participantPair.second)) {
				key << L"A(" << actor->getPairedActorId().getId()
					<< L"." << actor->getPairedAgentId().getId() << L")";
			} else {
				key << L"A(0)"; // What do we want to do in this case?
			}
		} 
		if (event_grouping & UNIQUE_PER_PROPER_NOUN_ACTOR_PAIR) {
			if (ProperNounActorMention_ptr actor = boost::dynamic_pointer_cast<ProperNounActorMention>(participantPair.second)) {
				key << L"A(" << actor->getActorId().getId() << L")";
			} else if (CompositeActorMention_ptr actor = boost::dynamic_pointer_cast<CompositeActorMention>(participantPair.second)) {
				key << L"A(" << actor->getPairedActorId().getId() << L")";
			} else {
				key << L"A(0)"; // What do we want to do in this case?
			}
		}
	}
	if (event_grouping & UNIQUE_PER_SENTENCE) {
		key << L",SENTNO=" << max_sentnum;
	}
	if (event_grouping & UNIQUE_PER_ICEWS_SENTENCE) {
		key << L",SENTNO=" << max_icews_sentnum;
	}
	if (per_event_type && (event_grouping & UNIQUE_PER_EVENT_TYPE)) {
		key << L",EVENT_TYPE=" << eventMention->getEventType()->getEventCode().to_string();
	}
	if (per_event_type && (event_grouping & UNIQUE_PER_EVENT_GROUP)) {
		key << L",EVENT_GROUP=" << eventMention->getEventType()->getEventGroup().to_string();
	}
	return key.str();
}

void ICEWSEventMentionFinder::removeOverriddenEvents(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory, size_t event_grouping, bool temporary_events_only) {
	std::set<ICEWSEventMention_ptr> overriddenEvents; // Events we will delete.
	std::map<std::wstring, EventMentionGroup> eventMentionGroups = divideEventsIntoUniquenessGroups(eventMentionSet, docTheory, event_grouping, false);

	typedef std::pair<std::wstring, EventMentionGroup> KeyAndEventMentionGroup;
	BOOST_FOREACH(const KeyAndEventMentionGroup &keyAndGroup, eventMentionGroups) {
		const EventMentionGroup& group = keyAndGroup.second;
		if (group.size() == 1) continue; // no chance for duplicates.
		for (size_t i=0; i<group.size(); ++i) {
			for (size_t j=i+1; j<group.size(); ++j) {
				ICEWSEventMention_ptr eventToDiscard;
				ICEWSEventMention_ptr eventToKeep;
				if (group[i]->getEventType()->overridesEventType(group[j]->getEventType())) {
					eventToDiscard = group[j];
					eventToKeep = group[i];
				} else if (group[j]->getEventType()->overridesEventType(group[i]->getEventType())) {
					eventToDiscard = group[i];
					eventToKeep = group[j];
				}
				if (temporary_events_only && eventToKeep && !eventToKeep->getEventType()->discardEventsWithThisType())
					continue;
				if (eventToDiscard) {
					overriddenEvents.insert(eventToDiscard);
					std::wostringstream msg;
					msg << L"Discarding event:\n";
					eventToDiscard->dump(msg, "      ");
					msg << L"\n    ...because it is overridden by...\n";
					eventToKeep->dump(msg, "      ");
					SessionLogger::info("ICEWS") << msg.str();
				}
			}
		}
	}

	// Now remove all the events that we selected for deletion.
	eventMentionSet->removeEventMentions(overriddenEvents);
}

void ICEWSEventMentionFinder::removeTemporaryEvents(ICEWSEventMentionSet *eventMentionSet) {
	std::set<ICEWSEventMention_ptr> eventsToDelete; // Events we will delete.
	BOOST_FOREACH(ICEWSEventMention_ptr eventMention, (*eventMentionSet)) {
		if (eventMention->getEventType()->discardEventsWithThisType())
			eventsToDelete.insert(eventMention);
	}
	// Now remove all the events that we selected for deletion
	if (!eventsToDelete.empty())
		eventMentionSet->removeEventMentions(eventsToDelete);
}


void ICEWSEventMentionFinder::applyReplaceEventTypePatterns(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory) {	
	if (!_replaceEventTypePatternSet) return;

	// Get all matches (document level and sentence level)
	PatternMatcher_ptr patternMatcher = PatternMatcher::makePatternMatcher(docTheory, _replaceEventTypePatternSet);
	std::vector<PatternFeatureSet_ptr> matches = patternMatcher->getDocumentSnippets();
	for (int sentno=0; sentno<docTheory->getNSentences(); ++sentno) {
		SentenceTheory *sentTheory = patternMatcher->getDocTheory()->getSentenceTheory(sentno);
		std::vector<PatternFeatureSet_ptr> sentMatches = patternMatcher->getSentenceSnippets(sentTheory, 0, true);
		matches.insert(matches.end(), sentMatches.begin(), sentMatches.end());
	}
	// Process each match
	BOOST_FOREACH(const PatternFeatureSet_ptr &match, matches) {
		Symbol patternLabel = match->getTopLevelPatternLabel();
		if (patternLabel.is_null())
			patternLabel = UNLABELED_PATTERN_SYM;
		for (size_t f = 0; f < match->getNFeatures(); f++) {
			if (ICEWSEventMentionReturnPFeature_ptr empf = boost::dynamic_pointer_cast<ICEWSEventMentionReturnPFeature>(match->getFeature(f))) {
				// Look up the new (replacement) event code.
				std::wstring new_event_code = empf->getReturnValue(L"new-event-code");
				if (new_event_code.empty()) {
					std::ostringstream err;
					err << "Pattern " << patternLabel.to_debug_string() << " does not return a new-event-code value";
					throw UnexpectedInputException("ICEWSEventMentionFinder::applyReplaceEventTypePatterns", err.str().c_str());
				}
				ICEWSEventType_ptr eventType = ICEWSEventType::getEventTypeForCode(new_event_code.c_str());
				if (!eventType) {
					std::ostringstream err;
					err << "Pattern " << patternLabel.to_debug_string() << " returns unknown new-event-code value " << new_event_code;
					throw UnexpectedInputException("ICEWSEventMentionFinder::applyReplaceEventTypePatterns", err.str().c_str());
				}
				// Update the event code.
				ICEWSEventMention_ptr eventMention = empf->getEventMention();
				std::wostringstream msg;
				msg << L"Changing event type of:\n";
				empf->getEventMention()->dump(msg, "      ");
				msg << L"\n    ... to EVENT-" << new_event_code 
					<< L" (" << eventType->getName() << L")"
					<< L"\n    ... because it matches pattern \""
					<< patternLabel << "\"";
				SessionLogger::info("ICEWS") << msg.str();
				eventMention->setEventType(eventType);
			}
		}
	}
}

void ICEWSEventMentionFinder::removeBlockedEvents(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory, PatternSet_ptr patternSet) {

	std::set<ICEWSEventMention_ptr> blockedEvents; // Events we will delete.
	std::vector<PatternFeatureSet_ptr> matches = getICEWSEventMatches(eventMentionSet, docTheory, patternSet);

	// Process each match
	BOOST_FOREACH(const PatternFeatureSet_ptr &match, matches) {
		for (size_t f = 0; f < match->getNFeatures(); f++) {
			if (ICEWSEventMentionReturnPFeature_ptr empf = boost::dynamic_pointer_cast<ICEWSEventMentionReturnPFeature>(match->getFeature(f))) {
				if (empf->getReturnLabel() == BLOCK_SYM || empf->hasReturnValue(BLOCK_SYM.to_string())) {
					blockedEvents.insert(empf->getEventMention());
					std::wostringstream msg;
					msg << L"Discarding event:\n";
					empf->getEventMention()->dump(msg, "      ");
					Symbol patternLabel = match->getTopLevelPatternLabel();
					if (patternLabel.is_null())
						patternLabel = UNLABELED_PATTERN_SYM;
					msg << L"\n    ...because it matches the block_event pattern \""
						<< patternLabel << "\"";
					SessionLogger::info("ICEWS") << msg.str();
				}
			}
		}
	}

	// Now remove all the events that we selected for deletion
	if (!blockedEvents.empty())
		eventMentionSet->removeEventMentions(blockedEvents);
}


void ICEWSEventMentionFinder::tagEventTense(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory) {

	std::vector<PatternFeatureSet_ptr> matches = getICEWSEventMatches(eventMentionSet, docTheory, _eventTensePatternSet);

	// Process each match
	BOOST_FOREACH(const PatternFeatureSet_ptr &match, matches) {
		for (size_t f = 0; f < match->getNFeatures(); f++) {
			if (ICEWSEventMentionReturnPFeature_ptr empf = boost::dynamic_pointer_cast<ICEWSEventMentionReturnPFeature>(match->getFeature(f))) {
				if (empf->hasReturnValue(EVENT_TENSE_SYM.to_string())) {			
					Symbol tense = Symbol(empf->getReturnValue(EVENT_TENSE_SYM.to_string()));
					if (ICEWSEventMention::isValidTense(tense))
						empf->getEventMention()->setEventTense(tense);
					else {
						SessionLogger::err("ICEWS") << "Unexpected tense (skipping): " << tense;
						continue;
					}
					std::wostringstream msg;
					msg << L"Tagging event:\n";
					empf->getEventMention()->dump(msg, "      ");					
					Symbol patternLabel = match->getTopLevelPatternLabel();
					if (patternLabel.is_null())
						patternLabel = UNLABELED_PATTERN_SYM;					
					msg << L"\n    ...with tense " << tense << L" because it matches the tag_event_tense pattern \""
						<< patternLabel << "\"";
					SessionLogger::info("ICEWS") << msg.str();
				}
			}
		}
	}

}





std::vector<PatternFeatureSet_ptr> ICEWSEventMentionFinder::getICEWSEventMatches(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory, PatternSet_ptr patternSet) {
	
	
	if (!patternSet) return std::vector<PatternFeatureSet_ptr>();
	
	// Get all matches (document level and sentence level)
	PatternMatcher_ptr patternMatcher = PatternMatcher::makePatternMatcher(docTheory, patternSet);
	std::vector<PatternFeatureSet_ptr> matches = patternMatcher->getDocumentSnippets();
	for (int sentno=0; sentno<docTheory->getNSentences(); ++sentno) {
		SentenceTheory *sentTheory = patternMatcher->getDocTheory()->getSentenceTheory(sentno);
		std::vector<PatternFeatureSet_ptr> sentMatches = patternMatcher->getSentenceSnippets(sentTheory, 0, true);
		matches.insert(matches.end(), sentMatches.begin(), sentMatches.end());
	}

	return matches;
}

void ICEWSEventMentionFinder::removeDuplicateEvents(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory, size_t event_grouping) {
	std::set<ICEWSEventMention_ptr> duplicateEvents; // Events we will delete.
	std::map<std::wstring, EventMentionGroup> eventMentionGroups = divideEventsIntoUniquenessGroups(eventMentionSet, docTheory, event_grouping, true);
	typedef std::pair<std::wstring, EventMentionGroup> KeyAndEventMentionGroup;
	BOOST_FOREACH(const KeyAndEventMentionGroup &keyAndGroup, eventMentionGroups) {
		const EventMentionGroup& group = keyAndGroup.second;
		if (group.size() == 1) continue; // no chance for duplicates.
		markAllButOneAsDuplicate(group, duplicateEvents, keyAndGroup.first);
	}

	// Now remove all the events that we selected for deletion.
	if (!duplicateEvents.empty()) {
		eventMentionSet->removeEventMentions(duplicateEvents);
	}
}

void ICEWSEventMentionFinder::markAllButOneAsDuplicate(const std::vector<ICEWSEventMention_ptr> &eventMentionGroup, std::set<ICEWSEventMention_ptr> &duplicateEvents, const std::wstring& duplicateRemovalKey) {
	// Determine which event mention in the group is best. 
	std::pair<ICEWSEventMention_ptr, float> best = std::make_pair(ICEWSEventMention_ptr(), -1.0f);
	BOOST_FOREACH(ICEWSEventMention_ptr eventMention, eventMentionGroup) {
		if (duplicateEvents.find(eventMention) == duplicateEvents.end()) {
			float score = getScoreForDuplicateEventMention(eventMention);
			if (score > best.second)
				best = std::make_pair(eventMention, score);
		}
	}
	// Now add every event mention except the best one to the duplicates list.
	BOOST_FOREACH(ICEWSEventMention_ptr eventMention, eventMentionGroup) {
		if ((eventMention != best.first) && (duplicateEvents.find(eventMention) == duplicateEvents.end())) {
			duplicateEvents.insert(eventMention);
			std::wostringstream msg;
			msg << L"Discarding event:\n";
			eventMention->dump(msg, "      ");
			msg << L"\n    ...because it is a duplicate of...\n";
			best.first->dump(msg, "      ");
			msg << L"\n    ...with key: \"" << duplicateRemovalKey << "\"";
			SessionLogger::info("ICEWS") << msg.str();
		}
	}
}

float ICEWSEventMentionFinder::getScoreForDuplicateEventMention(ICEWSEventMention_ptr eventMention) {
	float score = 0;

	// More specific events (which have longer event codes) score higher.
	score += wcslen(eventMention->getEventType()->getEventCode().to_string());

	const ParticipantMap &participants = eventMention->getParticipantMap();
	typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
	BOOST_FOREACH(const ParticipantPair &participantPair, participants) {
		ActorMention_ptr participant = participantPair.second;
		const Mention* mention = participant->getEntityMention();
		// Known actors score higher than unknown ones.
		if (ProperNounActorMention_ptr actor = boost::dynamic_pointer_cast<ProperNounActorMention>(participant)) {
			if (!actor->getActorId().isNull())
				score += 20;
		} else if (CompositeActorMention_ptr actor = boost::dynamic_pointer_cast<CompositeActorMention>(participant)) {
			if (!actor->getPairedActorId().isNull()) score += 10;
			if (!actor->getPairedAgentId().isNull()) score += 10;
		}
		// Names score higher than descriptions score higher than other types.
		if (mention->getMentionType() == Mention::NAME) score += 5;
		else if (mention->getMentionType() == Mention::DESC) score += 3;
		// Longer strings score higher
		float length = static_cast<float>(mention->toCasedTextString().size());
		score += ActorMentionFinder::countToScore(length, 20, 5);
	}
	return score;
}

void ICEWSEventMentionFinder::propagateActorLabels(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory) {
	if (!_propagageActorLabelPatternSet) return;

	// Get all matches (document level and sentence level)
	PatternMatcher_ptr patternMatcher = PatternMatcher::makePatternMatcher(docTheory, _propagageActorLabelPatternSet);
	std::vector<PatternFeatureSet_ptr> matches = patternMatcher->getDocumentSnippets();
	for (int sentno=0; sentno<docTheory->getNSentences(); ++sentno) {
		SentenceTheory *sentTheory = patternMatcher->getDocTheory()->getSentenceTheory(sentno);
		std::vector<PatternFeatureSet_ptr> sentMatches = patternMatcher->getSentenceSnippets(sentTheory, 0, true);
		matches.insert(matches.end(), sentMatches.begin(), sentMatches.end());
	}
	// Process each match
	BOOST_FOREACH(const PatternFeatureSet_ptr &match, matches) {
		Symbol patternLabel = match->getTopLevelPatternLabel();
		if (patternLabel.is_null())
			patternLabel = UNLABELED_PATTERN_SYM;
		ActorMention_ptr src; // only one source
		std::vector<ActorMention_ptr> dsts; // it's ok to have multiple dst
		for (size_t f = 0; f < match->getNFeatures(); f++) {
			if (ActorMentionReturnPFeature_ptr amrpf = boost::dynamic_pointer_cast<ActorMentionReturnPFeature>(match->getFeature(f))) {
				if (amrpf->getReturnLabel() == COPY_ACTOR_SRC_SYM) {
					if (src)
						throw UnexpectedInputException("ICEWSEventMentionFinder::propagateActorLabels",
							"Multiple COPY_ACTOR_SRC matches in a single pattern!");
					src = amrpf->getActorMention();
				} else if (amrpf->getReturnLabel() == COPY_ACTOR_DST_SYM) {
					dsts.push_back(amrpf->getActorMention());
				} else {
					throw UnexpectedInputException("ICEWSEventMentionFinder::propagateActorLabels",
							"Unexpected actor label", amrpf->getReturnLabel().to_debug_string());
				}
			}
		}
		if (!src)
			throw UnexpectedInputException("ICEWSEventMentionFinder::propagateActorLabels",
				"Expected exactly one actor to return COPY_ACTOR_SRC");
		if (dsts.empty())
			throw UnexpectedInputException("ICEWSEventMentionFinder::propagateActorLabels",
				"Expected at least one actor to return COPY_ACTOR_DST");
		ActorMention::ActorIdentifiers src_ids;
		if (ProperNounActorMention_ptr p = boost::dynamic_pointer_cast<ProperNounActorMention>(src))
			src_ids = ActorMention::ActorIdentifiers(p->getActorId(), p->getActorCode(), p->getActorPatternId());
		else if (CompositeActorMention_ptr p = boost::dynamic_pointer_cast<CompositeActorMention>(src))
			src_ids = ActorMention::ActorIdentifiers(p->getPairedActorId(), p->getPairedActorCode(), p->getPairedActorPatternId());
		if (!src_ids.id.isNull()) {
			BOOST_FOREACH(ActorMention_ptr dst, dsts) {
				if (CompositeActorMention_ptr cdst = boost::dynamic_pointer_cast<CompositeActorMention>(dst)) {
					if (cdst->getPairedActorId().isNull()) {
						SessionLogger::info("ICEWS") << "  Setting paired actor for " << dst
							<< " because it matches a propagate_event_actor_label pattern with src=" << src
							<< " based on pattern " << patternLabel.to_string();
						cdst->setPairedActorIdentifiers(src_ids, L"PROPAGATE_EVENT_ACTOR_LABEL");
						cdst->addSourceNote(patternLabel);
					}
				}
			}
		}
	}
}




} // end of namespace ICEWS



