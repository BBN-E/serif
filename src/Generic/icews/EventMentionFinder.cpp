// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/icews/EventMentionFinder.h"
#include "Generic/icews/EventType.h"
#include "Generic/icews/ICEWSEventMention.h"
#include "Generic/icews/ICEWSEventMentionSet.h"
#include "Generic/icews/ActorMentionPattern.h"
#include "Generic/icews/EventMentionPattern.h"
#include "Generic/icews/SentenceSpan.h"
#include "Generic/icews/TenseDetection.h"
#include "Generic/icews/ICEWSActorInfo.h"
#include "Generic/icews/EventMentionPattern.h"
#include "Generic/actors/ActorMentionFinder.h"
#include "Generic/theories/ActorMentionSet.h"
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
	Symbol BLOCK_LOCATION_SYM(L"BLOCK_LOCATION");
	Symbol TARGET_LOCATION_SYM(L"TARGET_LOCATION");
	Symbol SOURCE_LOCATION_SYM(L"SOURCE_LOCATION");
	Symbol DEFAULT_LOCATION_SYM(L"DEFAULT_LOCATION");
	Symbol LOCATION_SYM(L"LOCATION");
	Symbol SOURCE_SYM(L"SOURCE");
	Symbol TARGET_SYM(L"TARGET");
	Symbol EVENT_TENSE_SYM(L"event-tense");

}

ICEWSEventMentionFinder::ICEWSEventMentionFinder()
{
	_print_events_to_stdout = ParamReader::getOptionalTrueFalseParamWithDefaultVal("print_icews_events_to_stdout", false);
	_actorMentionFinder = _new ActorMentionFinder();
	_actorInfo = ActorInfo::getAppropriateActorInfoForICEWS();
	
	// Determine what domain we should be unique over.
	_event_uniqueness_groups = parseEventUniquenessGroupSpec("icews_event_uniqueness", 
		"ONE_GROUP_PER_MENTION_PAIR_PER_EVENT_GROUP_PER_ICEWS_SENTENCE"); // <-- default value
	_event_override_groups = parseEventUniquenessGroupSpec("icews_event_overrides",
		"ONE_GROUP_PER_MENTION_PAIR"); // <-- default value
	BOOST_FOREACH(size_t event_overrid_group, _event_override_groups) {
		if (event_overrid_group & UNIQUE_PER_EVENT_TYPE)
			throw UnexpectedInputException("ICEWSEventMentionFinder::ICEWSEventMentionFinder", 
				"icews_event_overrides should not include UNIQUE_PER_EVENT_TYPE");
	}

	_actor_event_sentence_cutoff = ParamReader::getOptionalIntParamWithDefaultValue("actor_event_sentence_cutoff", INT_MAX);
	// _icews_add_locations_to_events determines if event mention finder should attempt to add LOCATION role to an icews event mention
	_icews_add_locations_to_events = ParamReader::getOptionalTrueFalseParamWithDefaultVal("icews_add_locations_to_events", false);
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
			std::stringstream errMsg;
			errMsg << "Bad value for icews_event_codes parameter: " << ParamReader::getParam("icews_event_codes") << "\n";
			errMsg << "Should be comma separated list of event codes with optional wild card markers after base (two-digit) event codes";
			throw UnexpectedInputException("ICEWSEventMentionFinder::ICEWSEventMentionFinder", errMsg.str().c_str());
		}
	}
	if (_enabledEventCodes.empty()) 
		_enabledEventCodes.push_back(boost::wregex(L".*"));

	// Load our pattern sets.
	bool encrypted_patterns = ParamReader::isParamTrue("icews_encrypt_patterns");
	loadPatternSets(ParamReader::getParam("icews_event_models"), _patternSets);
	SessionLogger::info("ICEWS") << "Loaded " << _patternSets.size() << " pattern sets" << std::endl;

	std::string blockedEventFilename = ParamReader::getParam("icews_block_event_models");
	if (!blockedEventFilename.empty())
		_blockEventPatternSet = boost::make_shared<PatternSet>(blockedEventFilename.c_str(), encrypted_patterns);

	// blocked event location patterns & target event location patterns
	std::string eventLocationFilename = ""; 
	std::string blockedEventLocationFilename = "";	
	if (_icews_add_locations_to_events) {
		 eventLocationFilename = ParamReader::getRequiredParam("icews_event_location_patterns");
		 blockedEventLocationFilename = ParamReader::getRequiredParam("icews_block_event_location_patterns");
	}
	if (!eventLocationFilename.empty())
		_eventLocationPatternSet = boost::make_shared<PatternSet>(eventLocationFilename.c_str(), encrypted_patterns);
	if (!blockedEventLocationFilename.empty())
		_blockEventLocationPatternSet = boost::make_shared<PatternSet>(blockedEventLocationFilename.c_str(), encrypted_patterns);

	std::string blockedContingentEventFilename = ParamReader::getParam("icews_block_contingent_event_models");
	if (!blockedContingentEventFilename.empty())
		_blockContingentEventPatternSet = boost::make_shared<PatternSet>(blockedContingentEventFilename.c_str(), encrypted_patterns);
	
	std::string eventTenseFilename = ParamReader::getParam("icews_event_tense_models");
	if (!eventTenseFilename.empty())
		_eventTensePatternSet = boost::make_shared<PatternSet>(eventTenseFilename.c_str(), encrypted_patterns);

	std::string blockEventLocPairedActorPatternsFilename = ParamReader::getParam("icews_block_event_loc_paired_actor_patterns");
	if (!blockEventLocPairedActorPatternsFilename.empty())
		_blockEventLocPairedActorPatternSet = boost::make_shared<PatternSet>(blockEventLocPairedActorPatternsFilename.c_str(), encrypted_patterns);

	std::string propagateActorLabelPatternsFilename = ParamReader::getParam("icews_propagate_event_actor_label_models");
	if (!propagateActorLabelPatternsFilename.empty())
		_propagageActorLabelPatternSet = boost::make_shared<PatternSet>(propagateActorLabelPatternsFilename.c_str(), encrypted_patterns);

	std::string replaceEventTypePatternsFilename = ParamReader::getParam("icews_replace_event_type_models");
	if (!replaceEventTypePatternsFilename.empty())
		_replaceEventTypePatternSet = boost::make_shared<PatternSet>(replaceEventTypePatternsFilename.c_str(), encrypted_patterns);

	_do_not_binarize_icews_events = ParamReader::getOptionalTrueFalseParamWithDefaultVal("do_not_binarize_icews_events", false);	

	_verbosity = ParamReader::getOptionalIntParamWithDefaultValue("actor_event_verbosity", 1);
	#ifdef BLOCK_FULL_SERIF_OUTPUT
	_verbosity = 0;
	#endif
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
		typedef boost::split_iterator<std::string::iterator> SplitIter;
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
		errmsg << "Failed to open icews_event_models file \"" << patternSetFileList << "\"" << " specified by parameter icews_encrypt_patterns";
		throw UnexpectedInputException("ICEWSEventMentionFinder::loadPatternSets()", errmsg.str().c_str() );
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

	// Find actor mentions.
	ActorMentionSet *actorMentions = docTheory->getActorMentionSet();
	if (!actorMentions) {
		SessionLogger::warn("ICEWS") << "No ICEWS ActorMentionSet found; creating an empty one!";
		actorMentions = new ActorMentionSet();
		docTheory->takeActorMentionSet(actorMentions);
	}

	// Construct an event mention set to store our results.
	ICEWSEventMentionSet* eventMentionSet = _new ICEWSEventMentionSet();
	docTheory->takeICEWSEventMentionSet(eventMentionSet);

	int n_sentences = std::min(IcewsSentenceSpan::icewsSentenceNoToSerifSentenceNo(_actor_event_sentence_cutoff, docTheory), 
		docTheory->getNSentences());		

	// Check for mentions that should not be assigned paired actors.
	PatternMatcher_ptr blockPairedActorsPatternMatcher;
	if (_blockEventLocPairedActorPatternSet)
		blockPairedActorsPatternMatcher = PatternMatcher::makePatternMatcher(docTheory, _blockEventLocPairedActorPatternSet);
	std::map<MentionUID, Symbol> blockedPairedActors = ActorMentionFinder::findMentionsThatBlockDefaultPairedActor(docTheory, blockPairedActorsPatternMatcher);

	// Run each pattern set on the document.
	int global_event_id_counter = 0;
	BOOST_FOREACH(PatternSetAndCode pair, _patternSets) {
		if (pair.first->getNTopLevelPatterns() == 0) 
			continue; // this is just a stub file; it contains no real patterns yet.
		PatternMatcher_ptr patternMatcher = PatternMatcher::makePatternMatcher(docTheory, pair.first);
		actorMentions->addEntityLabels(patternMatcher, _actorInfo);
		applyPatternSet(docTheory, eventMentionSet, patternMatcher, pair.second, actorMentions, n_sentences, blockedPairedActors, global_event_id_counter);
	}
	// Propagate actor labels between event participants
	propagateActorLabels(eventMentionSet, docTheory);

	// Run the block_event patterns, and discard any events that they match.
	removeBlockedEvents(eventMentionSet, docTheory, _blockEventPatternSet);

	// Run the replace_event_type patterns, and replace any event types that
	// are indicated by those patterns.
	applyReplaceEventTypePatterns(eventMentionSet, docTheory);

	// Eliminate monadic events when there is a dyadic event in the same sentence of the same type, etc.
	removeParticipantOverriddenEvents(eventMentionSet, docTheory);

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

	// Discard evenets where the same entity plays multiple roles
	removeSameEntityEvents(eventMentionSet, docTheory);

	boost::optional<boost::gregorian::date> documentDate = docTheory->getDocument()->getDocumentDate();
	if (documentDate) {
		// Tag events with tense
		tagEventTense(eventMentionSet, docTheory);
	} else {
		// If there's no document date, tense is not meaningful, so set it to null
		BOOST_FOREACH(ICEWSEventMention_ptr eventMention, *eventMentionSet) {
			eventMention->setEventTense(ICEWSEventMention::NULL_TENSE);
		}
	}	

	// Add location to events
	if (_icews_add_locations_to_events) { 
		addLocationsToEvents(eventMentionSet, actorMentions, docTheory);
	}

	if (_do_not_binarize_icews_events) {
		// Just create a whole new event mention set; it's much easier
		ICEWSEventMentionSet* nonBinaryEventMentionSet = unbinarizeEvents(eventMentionSet);
		docTheory->takeICEWSEventMentionSet(nonBinaryEventMentionSet); // this will delete the old one
		eventMentionSet = nonBinaryEventMentionSet; // just for bookkeeping below
	}

	std::wostringstream msg;
	msg << L"Found " << eventMentionSet->size() << L" events in " 
		<< docTheory->getDocument()->getName() << ":" << std::endl;
	BOOST_FOREACH(ICEWSEventMention_ptr eventMention, *eventMentionSet) {
		eventMention->dump(msg, "  ");
		msg << L"\n\n";
	}
	SessionLogger::info("ICEWS") << msg.str();
	if (_print_events_to_stdout) std::wcout << msg.str();

}

void ICEWSEventMentionFinder::applyPatternSet(DocTheory *docTheory, ICEWSEventMentionSet* eventMentionSet, PatternMatcher_ptr patternMatcher, Symbol defaultEventCode, 
											  ActorMentionSet *actorMentions, int n_sentences, const std::map<MentionUID, Symbol> &blockedPairedActors, int& global_event_id_counter) {
	for (int sentno=0; sentno<n_sentences; ++sentno) {
		SentenceTheory *sentTheory = patternMatcher->getDocTheory()->getSentenceTheory(sentno);
		std::vector<PatternFeatureSet_ptr> matches = patternMatcher->getSentenceSnippets(sentTheory, 0, true);
		BOOST_FOREACH(PatternFeatureSet_ptr match, matches) {
			MatchData matchData = extractMatchData(match, defaultEventCode);
			matchData.originalSentNo = sentno;
			if (!checkMatchData(matchData))
				continue;
			ProperNounActorMention_ptr pairedActorLocation = getPairedActorLocation(match, sentTheory, actorMentions);
			if (pairedActorLocation) {
				setDefaultLocations(docTheory, matchData, pairedActorLocation, actorMentions, sentTheory, blockedPairedActors);
			}
			TenseDetection::setTense(match, matchData, patternMatcher->getDocTheory(), sentTheory);
			ICEWSEventType_ptr eventType = ICEWSEventType::getEventTypeForCode(matchData.eventCode);
			std::wstringstream idStr;
			idStr << global_event_id_counter;
			addEventMentions(eventMentionSet, patternMatcher->getDocTheory(), matchData, actorMentions, Symbol(idStr.str()));
			global_event_id_counter++;
		}
	}
}

void ICEWSEventMentionFinder::addLocationsToEvents(ICEWSEventMentionSet* eventMentionSet, const ActorMentionSet *actorMentions, const DocTheory* docTheory) {
	ProperNounActorMention_ptr defaultEventLoc = _actorMentionFinder->getDefaultCountryActorMention(actorMentions);
	std::set<ICEWSEventMention_ptr> blockedEvents = blockEventLocationResolutions(eventMentionSet, docTheory, _blockEventLocationPatternSet);

	for (size_t p = 0; p < _eventLocationPatternSet->getNTopLevelPatterns(); p++) {
		Pattern_ptr pattern = _eventLocationPatternSet->getNthTopLevelPattern(p);
		PatternMatcher_ptr patternMatcher = PatternMatcher::makePatternMatcher(docTheory, pattern);
		ICEWSEventMentionMatchingPattern_ptr iemp = boost::dynamic_pointer_cast<ICEWSEventMentionMatchingPattern>(pattern);
		if (!iemp)
			continue;
		
		// loop over each event and attempt to resolve event location
		for (ICEWSEventMentionSet::const_iterator iter1 = eventMentionSet->begin(); iter1 != eventMentionSet->end(); ++iter1) {
			ICEWSEventMention_ptr eventMention = (*iter1);
	
			// if this is an event for which location resolution is supposed to be blocked, move on
			if (!blockedEvents.empty() && blockedEvents.find(eventMention) != blockedEvents.end())
				continue;

			PatternFeatureSet_ptr match = iemp->matchesICEWSEventMention(patternMatcher, eventMention);
			if (!match)
				continue;
			bool event_blocked = false;
			ProperNounActorMention_ptr eventLoc;

			for (size_t f = 0; f < match->getNFeatures(); f++) {
				if (ICEWSEventMentionReturnPFeature_ptr empf = boost::dynamic_pointer_cast<ICEWSEventMentionReturnPFeature>(match->getFeature(f))) {
					if (empf->getEventMention() != eventMention) // if this is a document level feature for the wrong event move on
						continue;
					Symbol resolution_case = empf->getReturnLabel();
					if (resolution_case == TARGET_LOCATION_SYM) { // resolve to target actor country / geo
						ActorMention_ptr target = eventMention->getParticipant(TARGET_SYM);
						if (ProperNounActorMention_ptr pnTarget = boost::dynamic_pointer_cast<ProperNounActorMention>(target)) {
							if (pnTarget->isResolvedGeo()) {
								eventLoc = pnTarget;
							} else {
								Symbol isoCode = _actorInfo->getIsoCodeForActor(pnTarget->getActorId());
								if (!isoCode.is_null()) {
									eventLoc = findCountryActor(actorMentions, isoCode);
								}
							}
						} else if (CompositeActorMention_ptr cpTarget = boost::dynamic_pointer_cast<CompositeActorMention>(target)) {
							Symbol isoCode = _actorInfo->getIsoCodeForActor(cpTarget->getPairedActorId());
							if (!isoCode.is_null()) {
								eventLoc = findCountryActor(actorMentions, isoCode);
							}		
						}
					} else if (resolution_case == SOURCE_LOCATION_SYM) { // resolve to source actor country / geo
						ActorMention_ptr target = eventMention->getParticipant(SOURCE_SYM);
						if (ProperNounActorMention_ptr pnTarget = boost::dynamic_pointer_cast<ProperNounActorMention>(target)) {
							if (pnTarget->isResolvedGeo()) {
								eventLoc = pnTarget;
							} else {
								Symbol isoCode = _actorInfo->getIsoCodeForActor(pnTarget->getActorId());
								if (!isoCode.is_null()) {
									eventLoc = findCountryActor(actorMentions, isoCode);
								}
							}
						} else if (CompositeActorMention_ptr cpTarget = boost::dynamic_pointer_cast<CompositeActorMention>(target)) {
							Symbol isoCode = _actorInfo->getIsoCodeForActor(cpTarget->getPairedActorId());
							if (!isoCode.is_null()) {
								eventLoc = findCountryActor(actorMentions, isoCode);
							}		
						}
					} else if (resolution_case == DEFAULT_LOCATION_SYM) { // resolve to default country / geo for document
						eventLoc = defaultEventLoc;
					} else if (resolution_case == LOCATION_SYM) { // resolve to location indicated by pattern
						if (MentionReturnPFeature_ptr mrpf = boost::dynamic_pointer_cast<MentionReturnPFeature>(empf)) {
							std::vector<std::wstring> roles;
							std::wstring returnLabel = mrpf->getReturnLabel().to_string();
							const Mention *returnMention = mrpf->getMention();
							ActorMention_ptr actor = actorMentions->find(returnMention->getUID());
							if (ProperNounActorMention_ptr pnActor = boost::dynamic_pointer_cast<ProperNounActorMention>(actor)) 
								if (pnActor && pnActor->isResolvedGeo()) eventLoc = pnActor;	
						}
					} else if (resolution_case == BLOCK_LOCATION_SYM) {
						event_blocked = true;
					} // end switch
					break;
				} else if (MentionReturnPFeature_ptr mrpf = boost::dynamic_pointer_cast<MentionReturnPFeature>(match->getFeature(f))) { // end return p feature if
					std::vector<std::wstring> roles;
					std::wstring returnLabel = mrpf->getReturnLabel().to_string();
					const Mention *returnMention = mrpf->getMention();
					ActorMention_ptr actor = actorMentions->find(returnMention->getUID());
					if (ProperNounActorMention_ptr pnActor = boost::dynamic_pointer_cast<ProperNounActorMention>(actor)) { 
						if (pnActor && pnActor->isResolvedGeo()) { 
							eventLoc = pnActor;
							break;
						}
					}
				}
			}

			if (event_blocked) break;
			// if we have found a resolved location for the event, don't try any other patterns
			if (eventLoc && eventLoc->isResolvedGeo()) {	
				// add location role 
				eventMention->addLocationRole(eventLoc);
				std::wostringstream msg;
				msg << L"Found location:\n   ";
				eventLoc->dump(msg);
				msg << L"\nFor event:\n";
				eventMention->dump(msg, "   " );
				msg << std::endl;
				SessionLogger::info("ICEWS") << msg.str();
				break; // move on to next event
			} 
		} 
	} 
}


ProperNounActorMention_ptr ICEWSEventMentionFinder::findCountryActor(const ActorMentionSet *actorMentions, Symbol isoCode) {
	BOOST_FOREACH(ActorMention_ptr actorMention, actorMentions->getAll()) {
		if (ProperNounActorMention_ptr actor = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention)) {
			if (actor->isResolvedGeo()) {
				if (actor->getGeoResolution()->countrycode == isoCode) {
					return actor;
				}
			}
		}
	}
	return ProperNounActorMention_ptr();
}

std::set<ICEWSEventMention_ptr> ICEWSEventMentionFinder::blockEventLocationResolutions(ICEWSEventMentionSet* eventMentionSet, const DocTheory* docTheory, PatternSet_ptr patternSet) {
	std::set<ICEWSEventMention_ptr> blockedEvents; 
	std::vector<PatternFeatureSet_ptr> matches = getICEWSEventMatches(eventMentionSet, docTheory, patternSet);

	// Process each match
	BOOST_FOREACH(const PatternFeatureSet_ptr &match, matches) {
		for (size_t f = 0; f < match->getNFeatures(); f++) {
			if (ICEWSEventMentionReturnPFeature_ptr empf = boost::dynamic_pointer_cast<ICEWSEventMentionReturnPFeature>(match->getFeature(f))) {
				if (empf->getReturnLabel() == BLOCK_LOCATION_SYM || empf->hasReturnValue(BLOCK_LOCATION_SYM.to_string())) {
					blockedEvents.insert(empf->getEventMention());
					std::wostringstream msg;
					msg << L"Blocking event location resolution:\n";
					empf->getEventMention()->dump(msg, "      ");
					Symbol patternLabel = match->getTopLevelPatternLabel();
					if (patternLabel.is_null())
						patternLabel = UNLABELED_PATTERN_SYM;
					msg << L"\n    ...because it matches the block_event_location pattern \""
						<< patternLabel << "\"";
					SessionLogger::info("ICEWS") << msg.str();
				}
			}
		}
	}

	return blockedEvents;
}

void ICEWSEventMentionFinder::setDefaultLocations(DocTheory *docTheory, const MatchData& matchData, ProperNounActorMention_ptr eventLoc, ActorMentionSet *actorMentions, const SentenceTheory *sentTheory, const std::map<MentionUID, Symbol> &blockedPairedActors) {
	typedef std::pair<Symbol, std::vector<const Mention*> > RoleAndMentions;
	BOOST_FOREACH(const RoleAndMentions &roleAndMentions, matchData.roleMentions) {
		BOOST_FOREACH(const Mention* mention, roleAndMentions.second) {

			ActorMention_ptr actorMention = actorMentions->find(mention->getUID());
			if (actorMention) {
				if (CompositeActorMention_ptr ca = boost::dynamic_pointer_cast<CompositeActorMention>(actorMention)) {
					if (ca->getPairedActorId().isNull()) {
						if (ActorMentionFinder::sisterMentionHasClashingActor(ca->getEntityMention(), eventLoc, docTheory, actorMentions, _verbosity)) {
							// Message printed in sub-function
							continue;
						}
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

ProperNounActorMention_ptr ICEWSEventMentionFinder::getPairedActorLocation(PatternFeatureSet_ptr match, const SentenceTheory *sentTheory, const ActorMentionSet *actorMentions) {
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
		Symbol argRole = arg->getRoleSym();
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
	// save pattern feature set if indicated in param file
	matchData.matchedPatternFeatureSet = match;
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
			if (rsf->getReturnLabel() == BLOCK_SYM || rsf->hasReturnValue(BLOCK_SYM.to_string())) {
				matchData.eventCode = BLOCK_SYM;
				// just get the heck out of dodge
				return matchData;
			}
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
		} else if (PropPFeature_ptr ppf = boost::dynamic_pointer_cast<PropPFeature>(sf)) {
			matchData.propositions.push_back(ppf->getProp());
		}
	}
	return matchData;
}

// Todo: should this generate exceptions rather than session log messages?
bool ICEWSEventMentionFinder::checkMatchData(MatchData& matchData) {
	// Check the pattern label.
	if (matchData.patternLabel.is_null()) {
		// SessionLogger TODO
		SessionLogger::warn("ICEWS") << "Pattern without top-level ID; setting to null\n";
		matchData.patternLabel = Symbol(L"NO_PATTERN_ID");
	}
	if (matchData.eventCode == BLOCK_SYM) {
		return false;
	}

	ICEWSEventType_ptr eventType = ICEWSEventType::getEventTypeForCode(matchData.eventCode);

	if (!eventType) {
		SessionLogger::warn("ICEWS") << "Unexpected event type: " << matchData.eventCode;
		return false;
	}

	if (!ICEWSEventMention::isValidTense(matchData.tense))
	{
		SessionLogger::warn("ICEWS") << "Unexpected tense (resetting to NEUTRAL): " << matchData.tense;
		matchData.tense = ICEWSEventMention::NEUTRAL_TENSE;
	}

	// Check the event participants.
	typedef std::pair<Symbol, std::vector<const Mention*> > RoleMentionsPair;
	BOOST_FOREACH(RoleMentionsPair roleMentionsPair, matchData.roleMentions) {
		const Symbol &role = roleMentionsPair.first;
		if (!eventType->hasRole(role)) {
			SessionLogger::warn("ICEWS") << "Unexpected role \"" << role
				<< "\" for event type \"" << matchData.eventCode << "\"";
			return false;
		}
	}
	BOOST_FOREACH(Symbol role, eventType->getRequiredRoles()) {
		if (matchData.roleMentions.find(role) == matchData.roleMentions.end()) {
			SessionLogger::warn("ICEWS") << "Required role \"" << role
				<< "\" for event type \"" << matchData.eventCode << "\" not found; pattern = " << matchData.patternLabel;
			return false;
		}
	}

	if (matchData.roleMentions.empty()) {
		SessionLogger::warn("ICEWS") << "No participants for event type \"" << matchData.eventCode << "\"; pattern = " << matchData.patternLabel;
		return false;
	}
	return true;
}

void ICEWSEventMentionFinder::addEventMentions(ICEWSEventMentionSet* eventMentionSet, const DocTheory *docTheory, 
											   MatchData& matchData, ActorMentionSet *actorMentions, Symbol original_event_id) 
{
	// need to take all combinations of participants, and add an event for each one.  E.g., 
	// if we have two values for TARGET and two for SOURCE, then add 4 events.  What do
	// I want to do here w/ reflexive events (which really only have one participant type,
	// even though it's labeled as two)?  Or events that imply other events?

	ICEWSEventType_ptr eventType = ICEWSEventType::getEventTypeForCode(matchData.eventCode);
	std::vector<Symbol> roles;
	for (RoleMentions::iterator it=matchData.roleMentions.begin(); it!=matchData.roleMentions.end(); ++it)
		roles.push_back((*it).first);

	ParticipantList participantList;
	addEventMentionsHelper(eventMentionSet, docTheory, eventType, matchData, participantList, roles, actorMentions, original_event_id);
}

void ICEWSEventMentionFinder::addEventMentionsHelper(ICEWSEventMentionSet* eventMentionSet, // <- result goes here
													 const DocTheory *docTheory,
													 ICEWSEventType_ptr eventType,
													 MatchData& matchData,
													 ParticipantList &participantList, 
													 std::vector<Symbol> &roles, 
													 ActorMentionSet *actorMentions, 
													 Symbol original_event_id)
{
	if (roles.empty()) {
		ICEWSEventMention_ptr eventMention = boost::make_shared<ICEWSEventMention>(eventType, participantList, matchData.patternLabel, matchData.tense, matchData.timeValueMention, matchData.propositions, original_event_id, false);
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
			const SentenceTheory* sentTheory = docTheory->getSentenceTheory(mention->getSentenceNumber());
			participantList.push_back(std::make_pair(role, getActorForMention(sentTheory, mention, actorMentions)));
			addEventMentionsHelper(eventMentionSet, docTheory, eventType, matchData,
				participantList, roles, actorMentions, original_event_id);
			participantList.pop_back();
		}
		// Restore "roles" to original values
		roles.push_back(role);
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
	// to any specific actor/agent)-- otherwise we'll crash; this is a short-term fix
	if (!actorMention) {
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
	std::wostringstream complete_key;
	const ParticipantList &participants = eventMention->getParticipantList();
	typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
	int max_sentnum = 0;
	int max_icews_sentnum = 0;
	// We need these sorted in some constant order, so we store them all and then sort them	
	std::list<std::wstring> participantStrings;
	BOOST_FOREACH(const ParticipantPair &participantPair, participants) {

		std::wostringstream individual_key;
		// Locations don't count for duplication
		if (participantPair.first == LOCATION_SYM)
			continue;		
		individual_key << participantPair.first << L"=";
		ActorMention_ptr actor = participantPair.second;
		MentionUID actorMentionId = actor->getEntityMention()->getUID();
		const SynNode *actorNode = actor->getEntityMention()->getNode();
		EDTOffset actorPos = actor->getSentence()->getTokenSequence()->getToken(actorNode->getStartToken())->getStartEDTOffset();
		max_sentnum = std::max(max_sentnum, actorMentionId.sentno());
		max_icews_sentnum = std::max(max_icews_sentnum,
			IcewsSentenceSpan::edtOffsetToIcewsSentenceNo(actorPos, docTheory));
		if (event_grouping & UNIQUE_PER_MENTION_PAIR) {
			individual_key << L"M(" << actorMentionId.toInt() << L")";
		}
		if (event_grouping & UNIQUE_PER_ENTITY_PAIR) {
			const Entity *ent = docTheory->getEntitySet()->getEntityByMention(actorMentionId);
			individual_key << L"E(" << ent->getID() << L")";
		}
		if (event_grouping & UNIQUE_PER_ACTOR_PAIR) {
			if (ProperNounActorMention_ptr actor = boost::dynamic_pointer_cast<ProperNounActorMention>(participantPair.second)) {
				individual_key << L"A(" << actor->getActorId().getId() << L")";
			} else if (CompositeActorMention_ptr actor = boost::dynamic_pointer_cast<CompositeActorMention>(participantPair.second)) {
				individual_key << L"A(" << actor->getPairedActorId().getId()
					<< L"." << actor->getPairedAgentId().getId() << L")";
			} else {
				individual_key << L"A(0)"; // What do we want to do in this case?
			}
		} 
		if (event_grouping & UNIQUE_PER_PROPER_NOUN_ACTOR_PAIR) {
			if (ProperNounActorMention_ptr actor = boost::dynamic_pointer_cast<ProperNounActorMention>(participantPair.second)) {
				individual_key << L"A(" << actor->getActorId().getId() << L")";
			} else if (CompositeActorMention_ptr actor = boost::dynamic_pointer_cast<CompositeActorMention>(participantPair.second)) {
				individual_key << L"A(" << actor->getPairedActorId().getId() << L")";
			} else {
				individual_key << L"A(0)"; // What do we want to do in this case?
			}
		}
		// Only add if our constraints deal with participants in some way;
		//  we don't skip this whole inner loop because we care about sentence numbers
		if (event_grouping & UNIQUE_PER_MENTION_PAIR || event_grouping & UNIQUE_PER_ENTITY_PAIR || 
			event_grouping & UNIQUE_PER_ACTOR_PAIR || event_grouping & UNIQUE_PER_PROPER_NOUN_ACTOR_PAIR) 
		{		
			participantStrings.push_back(individual_key.str());
		}
	}

	// Sort the participant strings
	participantStrings.sort();
	BOOST_FOREACH(std::wstring participantString, participantStrings) {		
		complete_key << participantString << L" ";
	}

	if (event_grouping & UNIQUE_PER_SENTENCE) {
		complete_key << L" SENTNO=" << max_sentnum;
	}
	if (event_grouping & UNIQUE_PER_ICEWS_SENTENCE) {
		complete_key << L" SENTNO=" << max_icews_sentnum;
	}
	if (per_event_type && (event_grouping & UNIQUE_PER_EVENT_TYPE)) {
		complete_key << L" EVENT_TYPE=" << eventMention->getEventType()->getEventCode().to_string();
	}
	if (per_event_type && (event_grouping & UNIQUE_PER_EVENT_GROUP)) {
		complete_key << L" EVENT_GROUP=" << eventMention->getEventType()->getEventGroup().to_string();
	}
	return complete_key.str();
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

void ICEWSEventMentionFinder::removeParticipantOverriddenEvents(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory) {
	std::set<ICEWSEventMention_ptr> overriddenEvents; // Events we will delete.

	size_t uniqueness = 0;
	uniqueness |= UNIQUE_PER_ICEWS_SENTENCE;
	std::map<std::wstring, EventMentionGroup> eventMentionGroups = divideEventsIntoUniquenessGroups(eventMentionSet, docTheory, uniqueness, true);

	typedef std::pair<std::wstring, EventMentionGroup> KeyAndEventMentionGroup;
	BOOST_FOREACH(const KeyAndEventMentionGroup &keyAndGroup, eventMentionGroups) {
		const EventMentionGroup& group = keyAndGroup.second;
		if (group.size() == 1) continue; // no chance for duplicates.
		for (size_t i=0; i<group.size(); ++i) {
			for (size_t j=i+1; j<group.size(); ++j) {
				ICEWSEventMention_ptr eventToDiscard;
				ICEWSEventMention_ptr eventToKeep;
				if (group[i]->hasBetterParticipantsThan(group[j])) {
					eventToDiscard = group[j];
					eventToKeep = group[i];
				} else if (group[j]->hasBetterParticipantsThan(group[i])) {
					eventToDiscard = group[i];
					eventToKeep = group[j];
				} 
				if (!eventToDiscard)
					continue;

				if (eventToKeep->getEventType()->getEventGroup() == eventToDiscard->getEventType()->getEventGroup() ||
					eventToKeep->getEventType()->overridesEventType(eventToDiscard->getEventType()))
				{
					overriddenEvents.insert(eventToDiscard);
					std::wostringstream msg;
					msg << L"Discarding event:\n";
					eventToDiscard->dump(msg, "      ");
					msg << L"\n    ...because it is overridden by the participants in...\n";
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

void ICEWSEventMentionFinder::removeSameEntityEvents(ICEWSEventMentionSet *eventMentionSet, const DocTheory *docTheory) {
	std::set<ICEWSEventMention_ptr> eventsToDelete; // Events we will delete.
	BOOST_FOREACH(ICEWSEventMention_ptr eventMention, (*eventMentionSet)) {
		if (eventMention->hasSameEntityPlayingMultipleRoles(docTheory)) {
			eventsToDelete.insert(eventMention);
			std::wostringstream msg;
			msg << L"Discarding event:\n";
			eventMention->dump(msg, "      ");
			msg << L"\n    ...because it has the same entity playing multiple roles\n";
			SessionLogger::info("ICEWS") << msg.str();
		}
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

				if (empf->hasReturnValue(L"new-event-code")) {
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

				} else if (empf->hasReturnValue(L"reciprocal-event-code")) {
					ICEWSEventMention_ptr eventMention = empf->getEventMention();

					std::wstring reciprocal_event_code = empf->getReturnValue(L"reciprocal-event-code");
					if (reciprocal_event_code.empty()) {
						std::ostringstream err;
						err << "Pattern " << patternLabel.to_debug_string() << " does not return a reciprocal-event-code value";
						throw UnexpectedInputException("ICEWSEventMentionFinder::applyReplaceEventTypePatterns", err.str().c_str());
					}
					ICEWSEventType_ptr eventType = eventMention->getEventType();
					if (!boost::iequals(reciprocal_event_code, L"SAME")) {
						eventType = ICEWSEventType::getEventTypeForCode(reciprocal_event_code.c_str());
						if (!eventType) {
							std::ostringstream err;
							err << "Pattern " << patternLabel.to_debug_string() << " returns unknown reciprocal-event-code value " << reciprocal_event_code;
							throw UnexpectedInputException("ICEWSEventMentionFinder::applyReplaceEventTypePatterns", err.str().c_str());
						}
					}
					if (!empf->hasReturnValue(L"reciprocal-roles")) {
						std::ostringstream err;
						err << "Pattern " << patternLabel.to_debug_string() << " specifies reciprocal-event-code value but not reciprocal-roles";
						throw UnexpectedInputException("ICEWSEventMentionFinder::applyReplaceEventTypePatterns", err.str().c_str());
					}
					std::wstring reciprocal_roles = empf->getReturnValue(L"reciprocal-roles");
					std::vector<std::wstring> roles;
					boost::split(roles, reciprocal_roles, boost::is_any_of("+"));
					if (roles.size() != 2) {
						std::ostringstream err;
						err << "Pattern " << patternLabel.to_debug_string() << " specifies reciprocal-roles with not exactly two roles: " << reciprocal_roles;
						throw UnexpectedInputException("ICEWSEventMentionFinder::applyReplaceEventTypePatterns", err.str().c_str());
					}

					Symbol removeRole = Symbol();
					if (empf->hasReturnValue(L"reciprocal-remove-role"))
						removeRole = Symbol(empf->getReturnValue(L"reciprocal-remove-role"));

					// Create reciprocal event
					std::wostringstream msg;
					msg << L"Creating reciprocal event from:\n";
					empf->getEventMention()->dump(msg, "      ");
					msg << L"\n    ... with new event type " << reciprocal_event_code 
						<< L" (" << eventType->getName() << L")"
						<< L"\n    ... because it matches pattern \""
						<< patternLabel << "\"";
					SessionLogger::info("ICEWS") << msg.str();

					const ParticipantList &participants = eventMention->getParticipantList();
					ParticipantList newParticipants;
					typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
					BOOST_FOREACH(const ParticipantPair &participantPair, participants) {
						ActorMention_ptr participant = participantPair.second;
						const Mention* mention = participant->getEntityMention();
						Symbol role = participantPair.first;
						Symbol newRole = role;
						if (role == roles.at(0))
							newRole = roles.at(1);
						else if (role == roles.at(1))
							newRole = roles.at(0);
						if (!removeRole.is_null() && newRole == removeRole)
							continue;
						newParticipants.push_back(std::make_pair(newRole, participant));
					}

					if (newParticipants.size() > 0) {
						// Note that we do not construct this as reciprocal, since it is using a real source/target binarization. The reciprocal flag is for events where both actors are represented with the same role.
						ICEWSEventMention_ptr recipMention = boost::make_shared<ICEWSEventMention>(eventType, newParticipants, patternLabel, eventMention->getEventTense(), eventMention->getTimeValueMention(), eventMention->getPropositions(), eventMention->getOriginalEventId(), false);
						eventMentionSet->addEventMention(recipMention);
					}
				} 
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
						SessionLogger::warn("ICEWS") << "Unexpected tense (skipping): " << tense;
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

	const ParticipantList &participants = eventMention->getParticipantList();
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
		score += countToScore(length, 20, 5);
	}

	return score;
}

		
/** Return a number between zero and max_output_val, where the result stops growing after we reach max_input_val. */
float ICEWSEventMentionFinder::countToScore(float count, float max_input_val, float max_output_val) {
	if (count > max_input_val) 
		return max_output_val;
	else 
		return (max_output_val - 
		((max_output_val/max_input_val/max_input_val) * 
		((count-max_input_val) * (count-max_input_val))));
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
			src_ids = ActorMention::ActorIdentifiers(p->getActorIdentifiers());
		else if (CompositeActorMention_ptr p = boost::dynamic_pointer_cast<CompositeActorMention>(src))
			src_ids = ActorMention::ActorIdentifiers(p->getPairedActorIdentifiers());
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

ICEWSEventMentionSet *ICEWSEventMentionFinder::unbinarizeEvents(ICEWSEventMentionSet *eventMentionSet) {
	ICEWSEventMentionSet* nonBinaryEventMentionSet = _new ICEWSEventMentionSet();

	typedef std::pair<Symbol, Symbol> sym_sym_t;
	std::map<sym_sym_t, std::vector<ICEWSEventMention_ptr> > originalEventClusters;

	BOOST_FOREACH(ICEWSEventMention_ptr eventMention, *eventMentionSet) {
		// Only create original event clusters if the original event ID AND the event type are the same
		// Otherwise this is something that comes from, e.g. a replace-event-pattern which we don't want 
		//   to treat this way (the roles would get screwed up, e.g. for 042/043)
		originalEventClusters[std::make_pair(eventMention->getOriginalEventId(), eventMention->getEventType()->getEventCode())].push_back(eventMention);
	}
	for(std::map<sym_sym_t, std::vector<ICEWSEventMention_ptr> >::iterator it = originalEventClusters.begin(); it != originalEventClusters.end(); ++it) {
		std::vector<ICEWSEventMention_ptr>& vec = it->second;
		ICEWSEventMention_ptr representative = vec.at(0);
		bool is_reciprocal = false;

		std::map<Symbol, std::set<ActorMention_ptr> > rolesAndActors;
		for (std::vector<ICEWSEventMention_ptr>::iterator em_it = vec.begin(); em_it != vec.end(); em_it++) {
			typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
			BOOST_FOREACH(const ParticipantPair &participantPair, (*em_it)->getParticipantList()) {
				rolesAndActors[participantPair.first].insert(participantPair.second);
			}
		}

		// Check for classic ICEWS reciprocal-ness
		if (rolesAndActors.find(ICEWSEventMention::SOURCE_SYM) != rolesAndActors.end() && rolesAndActors.find(ICEWSEventMention::TARGET_SYM) != rolesAndActors.end()) {
			std::set<ActorMention_ptr> sourceSet = rolesAndActors[ICEWSEventMention::SOURCE_SYM];
			std::set<ActorMention_ptr> targetSet = rolesAndActors[ICEWSEventMention::TARGET_SYM];
			std::set<ActorMention_ptr> intersect;
			std::set_intersection(sourceSet.begin(),sourceSet.end(),targetSet.begin(),targetSet.end(),
                  std::inserter(intersect,intersect.begin()));
			if (intersect.size() != 0) {
				is_reciprocal = true;
			}
		}

		ParticipantList participantList;
		for (std::map<Symbol, std::set<ActorMention_ptr> >::iterator role_actor_it = rolesAndActors.begin(); role_actor_it != rolesAndActors.end(); role_actor_it++) {
			Symbol role = role_actor_it->first;
			if (role == ICEWSEventMention::TARGET_SYM && is_reciprocal)
				continue;
			BOOST_FOREACH(ActorMention_ptr am, role_actor_it->second) {
				participantList.push_back(std::make_pair(role, am));
			}
		}

		ICEWSEventMention_ptr eventMention = boost::make_shared<ICEWSEventMention>(representative->getEventType(), participantList, 
																				   representative->getPatternId(), representative->getEventTense(), representative->getTimeValueMention(), 
																				   representative->getPropositions(),
																				   representative->getOriginalEventId(), is_reciprocal);
		nonBinaryEventMentionSet->addEventMention(eventMention);
		

	}

	return nonBinaryEventMentionSet;

}
