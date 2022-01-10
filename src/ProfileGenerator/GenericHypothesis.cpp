// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "ProfileGenerator/GenericHypothesis.h"
#include "ProfileGenerator/PGFact.h"
#include "ProfileGenerator/PGFactDate.h"
#include "Generic/patterns/Pattern.h"

#include "boost/foreach.hpp"
#include "boost/algorithm/string.hpp"
#include <string>
#include <iostream>
#include <sstream>

const float GenericHypothesis::DATE_THRESHOLD = .75;

GenericHypothesis::GenericHypothesis() : _best_fact_score(0), _oldest_fact_ID(-2), _newest_fact_ID(-2), 
	_best_score_group(Pattern::UNSPECIFIED_SCORE_GROUP), _best_doubly_reliable_score_group(Pattern::UNSPECIFIED_SCORE_GROUP),
	_score_cache(0), _n_reliable_english_text_facts(0), _n_reliable_facts(0), _n_english_facts(0),
	_has_external_high_confidence_fact(false), 
	_n_doubly_reliable_english_text_facts(0), _n_doubly_reliable_facts(0), _oldestCaptureTime(boost::gregorian::date()), // empty date is "not_a_date_time"
	_newestCaptureTime(boost::gregorian::date()), _startDate(PGFactDate_ptr()), _endDate(PGFactDate_ptr()), _confidence(0)
{ }

void GenericHypothesis::addSupportingHypothesis(GenericHypothesis_ptr hypo) {
	BOOST_FOREACH(PGFact_ptr fact, hypo->getSupportingFacts()) {
		addFact(fact);
	}
}
	
void GenericHypothesis::addFact(PGFact_ptr fact) {
	if (_best_fact_score < fact->getScore())
		_best_fact_score = fact->getScore();

	// Only sort by fact ids for document facts, not KBFacts
	if (fact->getDocumentId() != -1) {
		if (_oldest_fact_ID == -2 || fact->getFactId() < _oldest_fact_ID)
			_oldest_fact_ID = fact->getFactId();	
		if (_newest_fact_ID == -2 || fact->getFactId() > _newest_fact_ID)
			_newest_fact_ID = fact->getFactId();	
	}

	if (_best_score_group == Pattern::UNSPECIFIED_SCORE_GROUP || (fact->getScoreGroup() != Pattern::UNSPECIFIED_SCORE_GROUP && fact->getScoreGroup() < _best_score_group))
		_best_score_group = fact->getScoreGroup();

	// Only add to reliability counts if this is from a new document, so we avoid
	//   double counting the same bad ol' coref mistakes in a single document
	// TODO: Right now external facts are not counted as reliable English text. Maybe they should be?
	if (isFactFromNewDocument(fact)) {
		if (fact->hasReliableAgentMentionConfidence())
			_n_reliable_facts++;
		if (!fact->isMT())
			_n_english_facts++;
		if (fact->hasReliableAgentMentionConfidence() && !fact->isMT() && fact->getSourceType() == PGFact::TEXT)
			_n_reliable_english_text_facts++;
		if (fact->hasReliableAgentMentionConfidence() && fact->hasReliableAnswerMentionConfidence()) {
			_n_doubly_reliable_facts++;
			if (!fact->isMT() && fact->getSourceType() == PGFact::TEXT)
				_n_doubly_reliable_english_text_facts++;

			if (_best_doubly_reliable_score_group == Pattern::UNSPECIFIED_SCORE_GROUP || 
				(fact->getScoreGroup() != Pattern::UNSPECIFIED_SCORE_GROUP && fact->getScoreGroup() < _best_doubly_reliable_score_group))
				_best_doubly_reliable_score_group = fact->getScoreGroup();

		}
	}

	if (_oldestCaptureTime.is_not_a_date() || fact->getDocumentDate() < _oldestCaptureTime)
		_oldestCaptureTime = fact->getDocumentDate();
	if (_newestCaptureTime.is_not_a_date() || fact->getDocumentDate() > _newestCaptureTime)
		_newestCaptureTime = fact->getDocumentDate();

	if (fact->getDocumentId() == -1 && fact->getScore() > 0.9) {
		_has_external_high_confidence_fact = true;
	}

	_supportingFacts.push_back(fact);	
	addHypothesisSpecificFact(fact);
}

bool GenericHypothesis::isFactFromNewDocument(PGFact_ptr newFact) {

	BOOST_FOREACH(PGFact_ptr fact, _supportingFacts) {
		if (newFact->getDocumentId() == fact->getDocumentId())
			return false;
	}
	return true;
}

void GenericHypothesis::computeScore() {

	// just some testing stuff... this function isn't live yet, nor is the score used anywhere :)
	if (_best_score_group == 1 && _n_english_facts > 0 && _n_reliable_facts > 0 && 
		_supportingFacts.size() > 1)
	{
		_score_cache = 1.0;
	}

	_score_cache = 0;
}

int GenericHypothesis::rankAgainst(GenericHypothesis_ptr hypo) {

	if (hasExternalHighConfidenceFact() && !hypo->hasExternalHighConfidenceFact())
		return BETTER;
	else if (hypo->hasExternalHighConfidenceFact() && !hasExternalHighConfidenceFact())
		return WORSE;

	if (nSupportingFacts() > hypo->nSupportingFacts())
		return BETTER;
	else if (hypo->nSupportingFacts() > nSupportingFacts())
		return WORSE;

	if (getBestFactScore() > hypo->getBestFactScore())
		return BETTER;
	else if (hypo->getBestFactScore() > getBestFactScore())
		return WORSE;

	// Break ties and show more recent epoch facts (with id as proxy) ahead of older epoch
	if (getNewestFactID() > hypo->getNewestFactID())
		return BETTER;
	else if (hypo->getNewestFactID() > getNewestFactID())
		return WORSE;

	// should never happen unless they share facts, which they shouldn't
	return SAME;

}

std::vector<GenericHypothesis::kb_arg_t> GenericHypothesis::getKBArguments(int actor_id, ProfileSlot_ptr slot) {
	std::vector<kb_arg_t> kb_args;
	
	kb_arg_t focus;
	focus.actor_id = actor_id;
	focus.value = L"";
	focus.role = slot->getFocusRole();
	kb_args.push_back(focus);

	kb_arg_t answer;
	answer.actor_id = -1;
	answer.value = getDisplayValue();
	answer.role = slot->getAnswerRole();
	kb_args.push_back(answer);

	return kb_args;
}


/*********************
   HYPOTHESIS DATES
**********************/

void GenericHypothesis::generateHypothesisDates() {
	
	clearDates();

	// get all fact dates
	std::vector<PGFactDate_ptr> startDates;
	std::vector<PGFactDate_ptr> endDates;
	std::vector<PGFactDate_ptr> holdDates;
	std::vector<PGFactDate_ptr> nonHoldDates;
	std::vector<PGFactDate_ptr> activityDates;

	std::set<std::string> nonHoldDateStrings;

	PGFactDate_ptr earliestHoldDate = PGFactDate_ptr();
	PGFactDate_ptr latestHoldDate = PGFactDate_ptr();
	PGFactDate_ptr earliestNonHoldDate = PGFactDate_ptr();
	PGFactDate_ptr latestNonHoldDate = PGFactDate_ptr();

	bool has_date = false;
	BOOST_FOREACH(PGFact_ptr fact, _supportingFacts) {

		BOOST_FOREACH(PGFactDate_ptr factDate, fact->getFactDates()) {
			has_date = true;
			if (factDate->getDateType() == PGFactDate::START)
				startDates.push_back(factDate);
			if (factDate->getDateType() == PGFactDate::END)
				endDates.push_back(factDate);
			if (factDate->getDateType() == PGFactDate::HOLD) {
				storeIfEarliestOrLatest(factDate, earliestHoldDate, latestHoldDate);
				holdDates.push_back(factDate);
			}
			if (factDate->getDateType() == PGFactDate::NON_HOLD && fact->hasReliableAgentMentionConfidence()) {
				storeIfEarliestOrLatest(factDate, earliestNonHoldDate, latestNonHoldDate);
				nonHoldDates.push_back(factDate);
				if (factDate->isFullDate())
					nonHoldDateStrings.insert(factDate->getDBString());
			}
			if (factDate->getDateType() == PGFactDate::ACTIVITY)
				activityDates.push_back(factDate);
		}
	}

	if (!has_date) return;

	int num_good_dates = 0;
	int num_bad_dates = 0;
	
	// get a list of Start date sets that are all consistent within each set
	std::vector<PGFactDateSet_ptr> startDateSets = breakIntoConsistentSets(startDates);
	_startDate = getBestDate(startDateSets, static_cast<int>(startDates.size())); // will return PGFactDate_ptr() if it can't find a good date
	if (_startDate != PGFactDate_ptr())
		num_good_dates++;

	// get a list of End date sets that are all consistent within each set
	std::vector<PGFactDateSet_ptr> endDateSets = breakIntoConsistentSets(endDates);
	_endDate = getBestDate(endDateSets, static_cast<int>(endDates.size())); // will return PGFactDate_ptr() if it can't find a good date
	if (_endDate != PGFactDate_ptr())
		num_good_dates++;

	if (_startDate != PGFactDate_ptr() && _endDate != PGFactDate_ptr() &&
		_startDate->compareToDate(_endDate) == PGFactDate::LATER) 
	{
		// bail!
		clearDates();
		return;
	}

	// Add in activity, hold, and non-hold dates. If they don't 
	// seem to match Start and End dates, or each other, 
	// bail out and clear dates

	BOOST_FOREACH(PGFactDate_ptr factDate, holdDates) {
		// Often we guessed at fact hold dates for employment. If a hold
		// date is also a non-hold date (tends to be more reliable), 
		// that's a strong indication we guessed wrong.
		if (nonHoldDateStrings.find(factDate->getDBString()) != nonHoldDateStrings.end()) {
			num_bad_dates++;
			continue;
		}

		// remove stray hold dates that fall inside of non hold date range
		if (excludeQuestionableHoldDates() && 
			(float)holdDates.size() / (holdDates.size() + nonHoldDates.size()) < 0.26 &&
			!factDate->isOutsideOf(earliestNonHoldDate, latestNonHoldDate))
		{
			//std::cerr << "Excluding bad hold date: " << factDate->getDBString() << "\n";
			num_bad_dates++;
			continue;
		}

		if (factDate->isBetween(_startDate, _endDate)) { 
			_holdDates.push_back(factDate);
			num_good_dates++;
		} else {
			num_bad_dates++;
		}
	}

	BOOST_FOREACH(PGFactDate_ptr factDate, activityDates) {
		if (factDate->isBetween(_startDate, _endDate)) {
			_activityDates.push_back(factDate);
			num_good_dates++;
		} else {
			num_bad_dates++;
		}
	}

	int suspicious_non_hold_dates = 0;
	BOOST_FOREACH(PGFactDate_ptr factDate, nonHoldDates) {

		if (excludeQuestionableHoldDates() && 
			!factDate->isOutsideOf(earliestHoldDate, latestHoldDate))
		{
			suspicious_non_hold_dates++;
		}

		// remove stray non-hold dates that fall inside of hold date range
		if (excludeQuestionableHoldDates() && 
			(float)nonHoldDates.size() / (nonHoldDates.size() + holdDates.size()) < 0.26 &&
			!factDate->isOutsideOf(earliestHoldDate, latestHoldDate))
		{
			// std::cerr << "Excluding bad non hold date: " << factDate->getDBString() << "\n";
			num_bad_dates++;
			continue;
		}

		if (factDate->isOutsideOf(_startDate, _endDate)) {
			_nonHoldDates.push_back(factDate);
			num_good_dates++;
		} else {
			num_bad_dates++;
		}
	}

	if ((num_good_dates + num_bad_dates > 0) &&
		(float)num_good_dates / (num_good_dates + num_bad_dates) < DATE_THRESHOLD) 
	{
		clearDates(); 
		return;
	}

	// we have a mix of a small number of hold and non-hold dates, so we can't be confident of what's going on here
	if (_nonHoldDates.size() + _holdDates.size() < 5 && _nonHoldDates.size() > 0 && _holdDates.size() > 0) {
		clearDates(); 
		return;
	}

	// if enough of the non_hold dates fell within the hold date range, then don't trust the non_hold dates
	if (_holdDates.size() > _nonHoldDates.size() && suspicious_non_hold_dates > _nonHoldDates.size() / 5.0) {
		_nonHoldDates.clear();
		return;
	}

	// Make sure we have enough non hold dates from english docs to be confident
	int num_reliable_non_hold_dates = 0;
	BOOST_FOREACH(PGFactDate_ptr date, _nonHoldDates) {
		PGFact_ptr fact = date->getFact();
		if (!fact->isMT())
			num_reliable_non_hold_dates++;
	}
	if (num_reliable_non_hold_dates < 3) {
		_nonHoldDates.clear();
		return;
	}
}

void GenericHypothesis::clearDates() {
	_startDate = PGFactDate_ptr();
	_endDate = PGFactDate_ptr();

	_holdDates.clear();
	_nonHoldDates.clear();
	_activityDates.clear();
}

std::vector<PGFactDateSet_ptr> GenericHypothesis::breakIntoConsistentSets(std::vector<PGFactDate_ptr> dates) {
	// More specific dates first, so less specific dates can fit into several sets.
	// If you have 2008-10, 2008-08-15, and 2008 -- 2008 fits into both sets
	sort(dates.begin(), dates.end(), PGFactDate::specificitySorter);

	std::vector<PGFactDateSet_ptr> results;
	BOOST_FOREACH(PGFactDate_ptr date, dates) {
		bool added_to_set = false;
		BOOST_FOREACH(PGFactDateSet_ptr dateSet, results) {
			if (date->matchesSet(dateSet)) {
				dateSet->insert(date);
				added_to_set = true;
			}
		}
		if (!added_to_set) {
			PGFactDateSet_ptr newSet = boost::make_shared<PGFactDateSet>();
			newSet->insert(date);
			results.push_back(newSet);
		}
	}

	return results;
}

PGFactDate_ptr GenericHypothesis::getBestDate(std::vector<PGFactDateSet_ptr> sets, int num_dates) {
	if (sets.size() == 0)
		return PGFactDate_ptr();

	// If the longest set doesn't have enough dates compared to the rest
	// of the sets, return null. If it has enough, return the most specific 
	// date.
	size_t num_dates_longest_set = 0;
	PGFactDateSet_ptr longestSet = PGFactDateSet_ptr();
	BOOST_FOREACH(PGFactDateSet_ptr set, sets) {
		if (set->size() > num_dates_longest_set) {
			longestSet = set;
			num_dates_longest_set = set->size();
		}
	}

	if ((float)num_dates_longest_set / num_dates < DATE_THRESHOLD)
		return PGFactDate_ptr();

	PGFactDate_ptr mostSpecificDate = PGFactDate_ptr();
	BOOST_FOREACH(PGFactDate_ptr date, *longestSet) {
		if (mostSpecificDate == PGFactDate_ptr() || date->compareSpecificityTo(mostSpecificDate) == PGFactDate::MORE_SPECIFIC)
			mostSpecificDate = date;
	}

	return mostSpecificDate;
}

PGFactDate_ptr GenericHypothesis::getStartDate() {
	return _startDate;
}

PGFactDate_ptr GenericHypothesis::getEndDate() {
	return _endDate;
}

std::vector<PGFactDate_ptr>& GenericHypothesis::getHoldDates() {
	return _holdDates;
}

std::vector<PGFactDate_ptr>& GenericHypothesis::getActivityDates() {
	return _activityDates;
}

std::vector<PGFactDate_ptr>& GenericHypothesis::getNonHoldDates() {
	return _nonHoldDates;
}

std::vector<PGFactDate_ptr> GenericHypothesis::getAllDates() {
	std::vector<PGFactDate_ptr> result;

	if (_startDate != PGFactDate_ptr())
		result.push_back(_startDate);
	if (_endDate != PGFactDate_ptr())
		result.push_back(_endDate);
	BOOST_FOREACH(PGFactDate_ptr date, _holdDates) {
		result.push_back(date);
	}
	BOOST_FOREACH(PGFactDate_ptr date, _activityDates) {
		result.push_back(date);
	}
	BOOST_FOREACH(PGFactDate_ptr date, _nonHoldDates) {
		result.push_back(date);
	}
	return result;
}

std::vector<PGFactDate_ptr> GenericHypothesis::getAllNonHoldDates() {
	std::vector<PGFactDate_ptr> result;

	if (_startDate != PGFactDate_ptr())
		result.push_back(_startDate);
	if (_endDate != PGFactDate_ptr())
		result.push_back(_endDate);
	BOOST_FOREACH(PGFactDate_ptr date, _holdDates) {
		result.push_back(date);
	}
	BOOST_FOREACH(PGFactDate_ptr date, _activityDates) {
		result.push_back(date);
	}
	return result;
}

void GenericHypothesis::storeIfEarliestOrLatest(PGFactDate_ptr date, PGFactDate_ptr &earliest, PGFactDate_ptr &latest) {
	if (earliest == PGFactDate_ptr() || 
		date->compareToDate(earliest) == PGFactDate::EARLIER ||
		(date->compareToDate(earliest) == PGFactDate::UNKNOWN && date->compareSpecificityTo(earliest) == PGFactDate::MORE_SPECIFIC))
	{
		earliest = date;
	}

	if (latest == PGFactDate_ptr() || 
		date->compareToDate(latest) == PGFactDate::LATER ||
		(date->compareToDate(latest) == PGFactDate::UNKNOWN && date->compareSpecificityTo(latest) == PGFactDate::MORE_SPECIFIC))
	{
		latest = date;
	}
}

void GenericHypothesis::clearHoldDates() {
	_holdDates.clear();
}

void GenericHypothesis::clearNonHoldDates() {
	_nonHoldDates.clear();
}
