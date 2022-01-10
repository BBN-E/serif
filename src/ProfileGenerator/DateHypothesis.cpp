// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "ProfileGenerator/DateHypothesis.h"
#include "ProfileGenerator/PGFact.h"
#include "Generic/common/UnicodeUtil.h"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

// YYYY-MM-DD
const boost::wregex DateHypothesis::_timex_regex_ymd(L"^([12][0-9][0-9][0-9])-([0123]?[0-9])-([0123]?[0-9]).*");

// YYYY-MM-DD
const boost::wregex DateHypothesis::_timex_regex_ymd_exact(L"^([12][0-9][0-9][0-9])-([0123]?[0-9])-([0123]?[0-9])");

// YYYY-MM
const boost::wregex DateHypothesis::_timex_regex_ym(L"^([12][0-9][0-9][0-9])-([0123]?[0-9]).*");

// YYYY-MM
const boost::wregex DateHypothesis::_timex_regex_ym_exact(L"^([12][0-9][0-9][0-9])-([0123]?[0-9])");

// YYYY-W##
const boost::wregex DateHypothesis::_timex_regex_yw(L"^([12][0-9][0-9][0-9])-W([012345][0-9]).*");

// YYYY
const boost::wregex DateHypothesis::_timex_regex_y_anywhere(L".*([12][0-9][0-9][0-9]).*");

// YYYY
const boost::wregex DateHypothesis::_timex_regex_y(L"^([12][0-9][0-9][0-9]).*");

// YYYY exact
const boost::wregex DateHypothesis::_timex_regex_y_exact(L"^([12][0-9][0-9][0-9])$");

// YYYY-
const boost::wregex DateHypothesis::_timex_regex_y_hyphen(L"^([12][0-9][0-9][0-9])-.*");


bool DateHypothesis::isEquiv(GenericHypothesis_ptr hypoth) {
	DateHypothesis_ptr dateHypoth = boost::dynamic_pointer_cast<DateHypothesis>(hypoth);
	if (dateHypoth == DateHypothesis_ptr()) // cast failed
		return false;
	return isEquivValue(dateHypoth->getResolvedDate()); 
}

void DateHypothesis::addSupportingHypothesis(GenericHypothesis_ptr hypo) {
	BOOST_FOREACH(PGFact_ptr fact, hypo->getSupportingFacts()) {
		addFact(fact);
		PGFactArgument_ptr answerArg = fact->getAnswerArgument();
		if (answerArg && isMoreSpecificDate(answerArg->getResolvedStringValue())) {
			setDateString(answerArg->getResolvedStringValue());
		}
	}
}

void DateHypothesis::setDateString(std::wstring date_string) {
	_resolved_date = date_string;
	if (regex_match(_resolved_date, _timex_regex_y_exact) ||
		regex_match(_resolved_date, _timex_regex_ym_exact) ||
		regex_match(_resolved_date, _timex_regex_ymd_exact))
	{
		// all set
	} else if (regex_match(_resolved_date, _timex_regex_ymd)) {
		_resolved_date = _resolved_date.substr(0, 10);
	} else if (regex_match(_resolved_date, _timex_regex_ym)) {
		_resolved_date = _resolved_date.substr(0, 7);
	} else if (regex_match(_resolved_date, _timex_regex_y)) {
		_resolved_date = _resolved_date.substr(0, 4);
	} else {
		_resolved_date = L"";
	}
}


bool DateHypothesis::isEquivValue(std::wstring otherTimex) {
	// Only handle timex values that start with YYYY for now
	if (!regex_match(_resolved_date, _timex_regex_y) || 
		!regex_match(otherTimex, _timex_regex_y))
	{
		return false;
	}

	// Divide date up into components (YYYY)-(MM)-(DD)
	std::vector<std::wstring> compsThis, compsThat;
	boost::split(compsThis, _resolved_date, boost::is_any_of("-"));
	boost::split(compsThat, otherTimex, boost::is_any_of("-"));

	int min_components = static_cast<int>(compsThis.size() < compsThat.size() ? compsThis.size() : compsThat.size());

	bool retVal = false;
	const int year_idx = 0;
	const int month_idx = 1;
	const int day_idx = 2;

	const int year_len = 4;
	const int month_len = 2;
	const int day_len = 2;

	// Compare years (or other TIMEX val like PAST_REF)
	if (min_components >= 1) {
		retVal = boost::starts_with(compsThis.at(year_idx), compsThat.at(year_idx).substr(0, year_len));
	}

	if (min_components >= 2) {
		retVal = retVal && boost::starts_with(compsThis.at(month_idx), compsThat.at(month_idx).substr(0, month_len));
	}

	if (min_components >= 3) {
		retVal = retVal && boost::starts_with(compsThis.at(day_idx), compsThat.at(day_idx).substr(0, day_len));
	}

	return retVal;
}


// Helper function for isMoreSpecificDate below
int countHyphens(std::wstring str) {
	int n_hyph = 0;

	BOOST_FOREACH (wchar_t wch, str) {
		if (wch == L'-')
			n_hyph++;
	}

	return n_hyph;
}

bool DateHypothesis::isMoreSpecificDate(std::wstring otherTimex) {
	if (regex_match(otherTimex, _timex_regex_y) &&
		regex_match(_resolved_date, _timex_regex_y))
	{
		// Count hyphens -- more hyphens means more specific
		int n_hyph_val_that = countHyphens(otherTimex);
		int n_hyph_val_this = countHyphens(_resolved_date);

		if (n_hyph_val_that > n_hyph_val_this)
			return true;

		return false;
	}

	// If both time values aren't of the form YYYY, don't bother for now
	return false;
}

std::wstring DateHypothesis::getDisplayValue() {
	return _resolved_date;
}

bool DateHypothesis::isIllegalHypothesis(ProfileSlot_ptr slot, std::string& rationale) {	
	// We only trust reliable answers and agents
	if (nReliableEnglishTextFacts() == 0) {
		rationale = "not reliable";
		return true;
	}
	const std::vector<PGFact_ptr>& supportingFacts = getSupportingFacts();

	// This is a consistent problem, so until we fix it in SERIF, we hack it here
	for (std::vector<PGFact_ptr>::const_iterator iter = supportingFacts.begin(); iter != supportingFacts.end(); iter++) {
		std::wstring support_lower = (*iter)->getSupportChunkValue();
		std::transform(support_lower.begin(), support_lower.end(), support_lower.begin(), towlower);	
		if (support_lower.find(L" sr.") != std::wstring::npos || support_lower.find(L" senior") != std::wstring::npos || support_lower.find(L" elder") != std::wstring::npos)
		{
			rationale = "dates about a 'senior' are not reliable for now";
			return true;
		}
	}

	if (slot->getUniqueId() == ProfileSlot::DEATHDATE_UNIQUE_ID && nReliableEnglishTextFacts() < 3) {		
		bool has_external_fact = false;
		for (std::vector<PGFact_ptr>::const_iterator iter = supportingFacts.begin(); iter != supportingFacts.end(); iter++) {
			if ( (*iter)->getDocumentId() == -1 ) {
				has_external_fact = true;
				break;
			}
		}
		if (!has_external_fact) {
			rationale = "not reliable for death date";
			return true;
		}
	}
	if (slot->isDistantPastDate()) {
		// only dates with absolute years are allowed here
		bool found_year = false;
		for (std::vector<PGFact_ptr>::const_iterator iter = supportingFacts.begin(); iter != supportingFacts.end(); iter++) {
			
			boost::wsmatch literalMatches;
			boost::wsmatch resolvedMatches;
			PGFact_ptr fact = (*iter);

			PGFactArgument_ptr answerArg = fact->getAnswerArgument();
			if (!answerArg)
				continue;

			// These have to be local variables for regex to work
			std::wstring literalValue = answerArg->getLiteralStringValue();
			std::wstring resolvedValue = answerArg->getResolvedStringValue();

			bool literal_matched = boost::regex_match(literalValue, literalMatches, _timex_regex_y_anywhere);
			bool resolved_matched = boost::regex_match(resolvedValue, resolvedMatches, _timex_regex_y_anywhere);

			if (literal_matched && resolved_matched) {
				std::wstring literalYear(literalMatches[1].first, literalMatches[1].second);
				std::wstring resolvedYear(resolvedMatches[1].first, resolvedMatches[1].second);

				if (literalYear == resolvedYear) {
					found_year = true;
					break;
				}
			}
		}
		if (!found_year) {
			rationale = "no matching year found in literal text; not trustworthy for date of birth, founding, or date-to-market (for now)";
			return true;
		}
	}
	if (!regex_match(_resolved_date, _timex_regex_y_exact) &&
		!regex_match(_resolved_date, _timex_regex_ym_exact) &&
		!regex_match(_resolved_date, _timex_regex_ymd_exact))
	{
		rationale = "not simple date format";
		return true;
	}
	return false;
}	
