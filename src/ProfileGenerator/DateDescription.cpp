// Copyright (c) 2013 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/UnicodeUtil.h"
#include "ProfileGenerator/DateDescription.h"
#include "ProfileGenerator/ProfileSlot.h"
#include "ProfileGenerator/Profile.h"
#include "ProfileGenerator/GenericHypothesis.h"

#include "boost/foreach.hpp"
#include <boost/algorithm/string.hpp> 
#include <algorithm>

DateDescription::DateDescription(Profile_ptr profile, GenericHypothesis_ptr hypoth, ProfileSlot_ptr slot) {

	std::wstring entityName = profile->getName();
	std::wstring answerValue = L""; // TODO if we use these again

	_dateDescription = L"";

	if (answerValue == L"United States")
		answerValue = L"the United States";

	if (answerValue == L"White House")
		answerValue = L"the White House";

	PGFactDate_ptr startDate = hypoth->getStartDate();
	PGFactDate_ptr endDate = hypoth->getEndDate();
	std::vector<PGFactDate_ptr> holdDates = hypoth->getHoldDates();
	std::vector<PGFactDate_ptr> nonHoldDates = hypoth->getNonHoldDates();
	std::vector<PGFactDate_ptr> activityDates = hypoth->getActivityDates();
	std::vector<PGFactDate_ptr> allDates = hypoth->getAllNonHoldDates();

	if (slot->getUniqueId() == ProfileSlot::EMPLOYER_UNIQUE_ID)   {
		_dateDescription += getEmployer(entityName, startDate, endDate, holdDates, nonHoldDates);
	} else if (slot->getUniqueId() == ProfileSlot::EMPLOYEE_UNIQUE_ID || slot->getUniqueId() == ProfileSlot::LEADER_UNIQUE_ID) {
		_dateDescription += getEmployer(answerValue, startDate, endDate, holdDates, nonHoldDates);
	} else if (slot->getUniqueId() == ProfileSlot::EDUCATION_UNIQUE_ID) {
		_dateDescription += getEducation(entityName, answerValue, startDate, endDate, holdDates);
	} else if (slot->getUniqueId() == ProfileSlot::SPOUSE_UNIQUE_ID) {
		_dateDescription += getSpouse(entityName, answerValue, startDate, endDate, holdDates);
	} else if (slot->getUniqueId() == ProfileSlot::HEADQUARTERS_UNIQUE_ID) {
		_dateDescription += getHeadquarters(answerValue, startDate, endDate, holdDates);
	} else if (slot->getUniqueId() == ProfileSlot::FOUNDER_UNIQUE_ID) {
		_dateDescription += getActivity(allDates, L"The founding of this organization occurred");
	} else if (slot->getUniqueId() == ProfileSlot::ASSOCIATE_UNIQUE_ID) {
		if (profile->getProfileType() == Profile::PER)
			_dateDescription += getRelationship(entityName, answerValue, allDates);
		else if (profile->getProfileType() == Profile::ORG)
			_dateDescription += getRelationship(L"this organization", answerValue, allDates);
	} else if (slot->getUniqueId() == ProfileSlot::VISIT_UNIQUE_ID) {
		if (profile->getProfileType() == Profile::PER)
			_dateDescription += getVisitPer(entityName, answerValue, allDates);
		else if (profile->getProfileType() == Profile::ORG)
			_dateDescription += getVisitOrg(L"This organization", L"this organization", answerValue, allDates, L"operating in");
	} else if (slot->isQuotation()) {
		_dateDescription += getActivity(allDates, L"This statement was made");
	} else if (slot->isSentiment()) {
		_dateDescription += getActivity(allDates, L"This sentiment was expressed");
	} else if (slot->getUniqueId() == ProfileSlot::ACTIONS_UNIQUE_ID) {
		_dateDescription += getActivity(allDates, L"This was reported");
	} 

/*	if (_dateDescription.length() > 0) {
		std::cout << "OUTPUT DATE DESCRIPTION:\n";
		std::cout << UnicodeUtil::toUTF8StdString(_dateDescription);
		std::cout << "\n";
	}*/
}

std::wstring DateDescription::getEmployer(std::wstring entityName, PGFactDate_ptr start, PGFactDate_ptr end, std::vector<PGFactDate_ptr> &holdDates, std::vector<PGFactDate_ptr> &nonHoldDates) {
	std::wstring result = L"";

	// start and end date
	if (start != PGFactDate_ptr() && end != PGFactDate_ptr()) {
		result += entityName + L" held this position starting " + start->getNaturalDate() + L" and ending " + end->getNaturalDate() + L". ";
		return result;
	}

	// start but no end
	if (start != PGFactDate_ptr() && end == PGFactDate_ptr()) {
		result += entityName + L" held this position starting " + start->getNaturalDate() + L". ";

		PGFactDate_ptr latest = getLatestDateAfter(holdDates, start);
		if (latest != PGFactDate_ptr())
			result += L"The latest known date for this position is " + latest->getNaturalDate() + L". ";

		PGFactDate_ptr reference = start;
		if (latest != PGFactDate_ptr())
			reference = latest;

		PGFactDate_ptr earliestNonHold = getEarliestDateAfter(nonHoldDates, reference);
		if (earliestNonHold != PGFactDate_ptr())
			result += L"However, as of " + earliestNonHold->getNaturalDate() + L", " + entityName + L" appears to no longer hold this position. ";
		return result;
	}

	// end but no start
	if (start == PGFactDate_ptr() && end != PGFactDate_ptr()) {
		result += entityName + L" stopped holding this position " + getPreposition(end) + L" " + end->getNaturalDate() + L". ";

		PGFactDate_ptr earliest = getEarliestDateBefore(holdDates, end);
		if (earliest != PGFactDate_ptr())
			result += L"The earliest known date for this position is " + earliest->getNaturalDate() + L". ";
		return result;
	}

	// neither start nor end
	if (start == PGFactDate_ptr() && end == PGFactDate_ptr()) {
		PGFactDate_ptr earliest = getEarliestDate(holdDates);
		PGFactDate_ptr latest = getLatestDate(holdDates);

		if (earliest == PGFactDate_ptr() || latest == PGFactDate_ptr())
			return result;

		if (earliest->getNaturalDate() == latest->getNaturalDate()) {
			result += entityName + L" held this position " + getPreposition(earliest) + L" " + earliest->getNaturalDate() + L". ";
		} else {
			result += L"Known dates for " + entityName + L" holding this position range from " + earliest->getNaturalDate() + L" to " + latest->getNaturalDate() + L". ";
		}

		PGFactDate_ptr earliestNonHold = getEarliestDateAfter(nonHoldDates, latest);
		if (earliestNonHold != PGFactDate_ptr())
			result += L"However, as of " + earliestNonHold->getNaturalDate() + L", " + entityName + L" appears to no longer hold this position. ";
		
		return result;
	}

	return result;
}

std::wstring DateDescription::getEducation(std::wstring entityName, std::wstring displayValue, PGFactDate_ptr start, PGFactDate_ptr end, std::vector<PGFactDate_ptr> &holdDates) {
	std::wstring result = L"";

	// start and end date
	if (start != PGFactDate_ptr() && end != PGFactDate_ptr()) {
		result += entityName + L" attended " + displayValue + L" from " + start->getNaturalDate() + L" to " + end->getNaturalDate() + L". ";
		return result;
	}

	// start but no end
	if (start != PGFactDate_ptr() && end == PGFactDate_ptr()) {
		result += entityName + L" attended " + displayValue + L" starting " + getPreposition(start) + L" " + start->getNaturalDate() + L". ";
		PGFactDate_ptr latest = getLatestDateAfter(holdDates, start);
		if (latest != PGFactDate_ptr())
			result += L"The latest known date when " + entityName + L" attended is " + latest->getNaturalDate() + L". ";
		return result;
	}

	// end but no start
	if (start == PGFactDate_ptr() && end != PGFactDate_ptr()) {
		result += entityName + L" attended " + displayValue + L" ending " + getPreposition(end) + L" " + end->getNaturalDate() + L". ";
		PGFactDate_ptr earliest = getEarliestDateBefore(holdDates, end);
		if (earliest != PGFactDate_ptr())
			result += L"The earliest known date when " + entityName + L" attended is " + earliest->getNaturalDate() + L". ";

		return result;
	}

	// neither start nor end
	if (start == PGFactDate_ptr() && end == PGFactDate_ptr()) {
		PGFactDate_ptr earliest = getEarliestDate(holdDates);
		PGFactDate_ptr latest = getLatestDate(holdDates);

		if (earliest == PGFactDate_ptr() || latest == PGFactDate_ptr())
			return result;

		if (earliest->getNaturalDate() == latest->getNaturalDate()) {
			result += entityName + L" was known to attend " + displayValue + L" " + getPreposition(earliest) + L" " + earliest->getNaturalDate() + L". ";
		} else {
			result += L"Known dates for " + entityName + L" attending " + displayValue + L" range from " + earliest->getNaturalDate() + L" to " + latest->getNaturalDate() + L". ";
		}

		return result;
	}

	return result;
}

std::wstring DateDescription::getSpouse(std::wstring entityName, std::wstring displayValue, PGFactDate_ptr start, PGFactDate_ptr end, std::vector<PGFactDate_ptr> &holdDates) {
	std::wstring result = L"";

	// start and end date
	if (start != PGFactDate_ptr() && end != PGFactDate_ptr()) {
		result += entityName + L" married " + displayValue + L" " + getPreposition(start) + L" " + start->getNaturalDate() + L". ";
		result += L"Their marriage ended " + getPreposition(end) + L" " + end->getNaturalDate() + L". ";
		return result;
	}

	// start but no end
	if (start != PGFactDate_ptr() && end == PGFactDate_ptr()) {
		result += entityName + L" married " + displayValue + L" " + getPreposition(start) + L" " + start->getNaturalDate() + L". ";

		PGFactDate_ptr latest = getLatestDateAfter(holdDates, start);
		if (latest != PGFactDate_ptr())
			result += L"They were known to be still married " + getPreposition(latest) + L" " + latest->getNaturalDate() + L". ";
		return result;
	}

	// end but no start
	if (start == PGFactDate_ptr() && end != PGFactDate_ptr()) {
		result += L"The marriage between " + entityName + L" and " + displayValue + L" ended " + getPreposition(end) + L" " + end->getNaturalDate() + L". ";

		PGFactDate_ptr earliest = getEarliestDateBefore(holdDates, end);
		if (earliest != PGFactDate_ptr())
			result += L"The earliest known date when they were still married is " + earliest->getNaturalDate() + L". ";

		return result;
	}

	// neither start nor end
	if (start == PGFactDate_ptr() && end == PGFactDate_ptr()) {
		PGFactDate_ptr earliest = getEarliestDate(holdDates);
		PGFactDate_ptr latest = getLatestDate(holdDates);

		if (earliest == PGFactDate_ptr() || latest == PGFactDate_ptr())
			return result;

		if (earliest->getNaturalDate() == latest->getNaturalDate()) {
			result += entityName + L" and " + displayValue + L" were known to be married " + getPreposition(earliest) + L" " + earliest->getNaturalDate() + L". ";
		} else {
			result += L"Known dates for the marriage between " + entityName + L" and " + displayValue + L" range from " + earliest->getNaturalDate() + L" to " + latest->getNaturalDate() + L". ";
		}

		return result;
	}

	return result;
}

std::wstring DateDescription::getHeadquarters(std::wstring displayValue, PGFactDate_ptr start, PGFactDate_ptr end, std::vector<PGFactDate_ptr> &holdDates) {
	std::wstring result = L"";

	// start and end date
	if (start != PGFactDate_ptr() && end != PGFactDate_ptr()) {
		result += L"These headquarters were established in " + displayValue + L" " + getPreposition(start) + L" " + start->getNaturalDate() + L" and ceased operation " + getPreposition(end) + L" " + end->getNaturalDate() + L". ";
		return result;
	}

	// start but no end
	if (start != PGFactDate_ptr() && end == PGFactDate_ptr()) {
		result += L"These headquarters were established in " + displayValue + L" " + getPreposition(start) + L" " + start->getNaturalDate() + L". ";

		PGFactDate_ptr latest = getLatestDateAfter(holdDates, start);
		if (latest != PGFactDate_ptr())
			result += L"They were known to be still operating " + getPreposition(latest) + L" " + latest->getNaturalDate() + L". ";
		return result;
	}

	// end but no start
	if (start == PGFactDate_ptr() && end != PGFactDate_ptr()) {
		result += L"These headquarters ceased operation in " + displayValue + L" " + getPreposition(end) + L" " + end->getNaturalDate() + L". ";
		
		PGFactDate_ptr earliest = getEarliestDateBefore(holdDates, end);
		if (earliest != PGFactDate_ptr())
			result += L"The earliest known date when they were still known to be in operation is " + earliest->getNaturalDate() + L". ";

		return result;
	}

	// neither start nor end
	if (start == PGFactDate_ptr() && end == PGFactDate_ptr()) {
		PGFactDate_ptr earliest = getEarliestDate(holdDates);
		PGFactDate_ptr latest = getLatestDate(holdDates);

		if (earliest == PGFactDate_ptr() || latest == PGFactDate_ptr())
			return result;

		if (earliest->getNaturalDate() == latest->getNaturalDate())
			result += L"These headquarters were known to be operating in " + displayValue + L" " + getPreposition(earliest) + L" " + earliest->getNaturalDate() + L". ";
		else 
			result += L"Known dates for these headquarters operating in " + displayValue + L" range from " + earliest->getNaturalDate() + L" to " + latest->getNaturalDate() + L". ";

		return result;
	}

	return result;
}

std::wstring DateDescription::getActivity(std::vector<PGFactDate_ptr> &dates, std::wstring preface) {
	std::wstring result = L"";
	
	if (dates.size() == 0)
		return result;

	PGFactDate_ptr date = getEarliestDate(dates);
	std::wstring preposition = getPreposition(date);
	
	result += preface + L" " + preposition + L" " + date->getNaturalDate() + L". ";
	return result;
}

bool DateDescription::compareDates(PGFactDate_ptr date1, PGFactDate_ptr date2) {
	return date1->getDBString() < date2->getDBString();
}

std::wstring DateDescription::getVisitPer(std::wstring entityName, std::wstring displayValue, std::vector<PGFactDate_ptr> &dates) {
	std::wstring result = L"";

	if (dates.size() == 0)
		return result;

	std::vector<PGFactDate_ptr> uniqueDates = getUniqueDates(dates);
	std::sort(uniqueDates.begin(), uniqueDates.end(), DateDescription::compareDates);

	std::wstring preposition = getPreposition(uniqueDates[0]);

	result += entityName + L" visited " + displayValue + L" " + preposition + L" ";
	int counter = 0;
	BOOST_FOREACH(PGFactDate_ptr date, uniqueDates) {
		if (counter > 0 && counter != (int)uniqueDates.size() - 1)
			result += L", ";
		if (counter > 0 && counter == (int)uniqueDates.size() - 1)
			result += L" and ";
		result += date->getNaturalDate();
		counter++;
	}
	result += L". ";

	return result;
}

std::wstring DateDescription::getVisitOrg(std::wstring entityNameStartSentence, std::wstring entityNameMidSentence, std::wstring displayValue, std::vector<PGFactDate_ptr> &dates, std::wstring verb) {
	std::wstring result = L"";
	
	if (dates.size() == 0)
		return result;

	PGFactDate_ptr earliest = getEarliestDate(dates);
	PGFactDate_ptr latest = getLatestDate(dates);

	if (earliest->getNaturalDate() == latest->getNaturalDate()) 
		result += entityNameStartSentence + L" was reported to be " + verb + L" " + displayValue + L" " + getPreposition(earliest) + L" " + earliest->getNaturalDate() + L". ";
	else
		result += L"Known dates for " + entityNameMidSentence + L" " + verb + L" " + displayValue + L" range from " + earliest->getNaturalDate() + L" to " + latest->getNaturalDate() + L". ";
	
	return result;
}

std::wstring DateDescription::getRelationship(std::wstring entityName, std::wstring displayValue, std::vector<PGFactDate_ptr> &dates) {
	std::wstring result = L"";
	
	if (dates.size() == 0)
		return result;

	PGFactDate_ptr earliest = getEarliestDate(dates);
	PGFactDate_ptr latest = getLatestDate(dates);

	if (earliest->getNaturalDate() == latest->getNaturalDate()) 
		result += L"The relationship between " + entityName + L" and " + displayValue + L" existed " + getPreposition(earliest) + L" " + earliest->getNaturalDate() + L". ";
	else
		result += L"Known dates for the relationship between " + entityName + L" and " + displayValue + L" range from " + earliest->getNaturalDate() + L" to " + latest->getNaturalDate() + L". ";

	return result;
}

std::wstring DateDescription::getPreposition(PGFactDate_ptr date) {
	if (date->isDaySpecified())
		return L"on";
	else
		return L"in";
}

PGFactDate_ptr DateDescription::getEarliestDate(std::vector<PGFactDate_ptr> &dates) {
	PGFactDate_ptr earliest = PGFactDate_ptr();

	BOOST_FOREACH(PGFactDate_ptr date, dates) {
		if (earliest == PGFactDate_ptr()) {
			earliest = date;
			continue;
		}
		
		if (date->compareToDate(earliest) == PGFactDate::EARLIER) {
			earliest = date;
			continue;
		}

		if (date->compareToDate(earliest) == PGFactDate::UNKNOWN &&
			date->compareSpecificityTo(earliest) == PGFactDate::MORE_SPECIFIC)
		{
			earliest = date;
		}
	}

	return earliest;
}

PGFactDate_ptr DateDescription::getEarliestDateAfter(std::vector<PGFactDate_ptr> &dates, PGFactDate_ptr referenceDate) {
	PGFactDate_ptr earliest = PGFactDate_ptr();

	BOOST_FOREACH(PGFactDate_ptr date, dates) {
		if (!(date->compareToDate(referenceDate) == PGFactDate::LATER))
			continue;

		if (earliest == PGFactDate_ptr()) {
			earliest = date;
			continue;
		}
		
		if (date->compareToDate(earliest) == PGFactDate::EARLIER) {
			earliest = date;
			continue;
		}

		if (date->compareToDate(earliest) == PGFactDate::UNKNOWN &&
			date->compareSpecificityTo(earliest) == PGFactDate::MORE_SPECIFIC)
		{
			earliest = date;
		}
	}

	return earliest;
}

PGFactDate_ptr DateDescription::getEarliestDateBefore(std::vector<PGFactDate_ptr> &dates, PGFactDate_ptr referenceDate) {
	PGFactDate_ptr earliest = PGFactDate_ptr();

	BOOST_FOREACH(PGFactDate_ptr date, dates) {
		if (!(date->compareToDate(referenceDate) == PGFactDate::EARLIER))
			continue;

		if (earliest == PGFactDate_ptr()) {
			earliest = date;
			continue;
		}
		
		if (date->compareToDate(earliest) == PGFactDate::EARLIER) {
			earliest = date;
			continue;
		}

		if (date->compareToDate(earliest) == PGFactDate::UNKNOWN &&
			date->compareSpecificityTo(earliest) == PGFactDate::MORE_SPECIFIC)
		{
			earliest = date;
		}
	}

	return earliest;
}


PGFactDate_ptr DateDescription::getLatestDate(std::vector<PGFactDate_ptr> &dates) {
	PGFactDate_ptr latest = PGFactDate_ptr();

	BOOST_FOREACH(PGFactDate_ptr date, dates) {
		if (latest == PGFactDate_ptr()) {
			latest = date;
			continue;
		}
		
		if (date->compareToDate(latest) == PGFactDate::LATER) {
			latest = date;
			continue;
		}

		if (date->compareToDate(latest) == PGFactDate::UNKNOWN &&
			date->compareSpecificityTo(latest) == PGFactDate::MORE_SPECIFIC)
		{
			latest = date;
		}
	}

	return latest;
}

PGFactDate_ptr DateDescription::getLatestDateAfter(std::vector<PGFactDate_ptr> &dates, PGFactDate_ptr referenceDate) {
	PGFactDate_ptr latest = PGFactDate_ptr();

	BOOST_FOREACH(PGFactDate_ptr date, dates) {
		if (!(date->compareToDate(referenceDate) == PGFactDate::LATER))
			continue;

		if (latest == PGFactDate_ptr()) {
			latest = date;
			continue;
		}
		
		if (date->compareToDate(latest) == PGFactDate::LATER) {
			latest = date;
			continue;
		}

		if (date->compareToDate(latest) == PGFactDate::UNKNOWN &&
			date->compareSpecificityTo(latest) == PGFactDate::MORE_SPECIFIC)
		{
			latest = date;
		}
	}

	return latest;
}

std::vector<PGFactDate_ptr> DateDescription::getUniqueDates(std::vector<PGFactDate_ptr> &dates) {
	std::set<std::wstring> knownDateStrings;
	std::vector<PGFactDate_ptr> uniqueDates;
	
	BOOST_FOREACH(PGFactDate_ptr date, dates) {
		std::wstring dateString = date->getDBStringW();
		if (knownDateStrings.find(dateString) == knownDateStrings.end()) {
			uniqueDates.push_back(date);
			knownDateStrings.insert(dateString);
		}
	}

	// remove any that have a more specific version
	std::vector<PGFactDate_ptr> finalUniqueDates;
	for (std::vector<PGFactDate_ptr>::iterator iter = uniqueDates.begin(); iter != uniqueDates.end(); iter++) {
		PGFactDate_ptr dateInQuestion = *iter;
		bool bad_date = false;
		for (std::vector<PGFactDate_ptr>::iterator iter2 = uniqueDates.begin(); iter2 != uniqueDates.end(); iter2++) {
			PGFactDate_ptr date = *iter2;
			if (dateInQuestion == date) continue;
			if (dateInQuestion->compareToDate(date) == PGFactDate::UNKNOWN &&
				dateInQuestion->compareSpecificityTo(date) == PGFactDate::LESS_SPECIFIC) 
			{
				bad_date = true;
				break;
			}
		}
		if (!bad_date)
			finalUniqueDates.push_back(dateInQuestion);
	}

	return finalUniqueDates;
}

