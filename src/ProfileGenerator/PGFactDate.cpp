// Copyright (c) 2012 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UnicodeUtil.h"
#include "ProfileGenerator/PGFactDate.h"

#include "boost/foreach.hpp"

const boost::wregex PGFactDate::_timex_regex_ymd(L"([12][0-9][0-9][0-9])-([0123]?[0-9])-([0123]?[0-9])");
const boost::wregex PGFactDate::_timex_regex_ym(L"([12][0-9][0-9][0-9])-([0123]?[0-9])");
const boost::wregex PGFactDate::_timex_regex_y(L"([12][0-9][0-9][0-9])");

// Opposite of getDateTypeString. 
// These strings match the strings in the PGFactFinder pattern DATE return values
void PGFactDate::setDateType(std::string dateTypeString) {
	if (dateTypeString == "start")
		_dateType = START;
	else if (dateTypeString == "end") 
		_dateType = END;
	else if (dateTypeString == "hold")
		_dateType = HOLD;
	else if (dateTypeString == "non_hold")
		_dateType = NON_HOLD;
	else if (dateTypeString == "activity")
		_dateType = ACTIVITY;
	else { 
		std::stringstream errstr;
		errstr << "Unknown date type: " << dateTypeString;
		throw UnrecoverableException("PGFactDate::setDateType", errstr.str().c_str());
	}
}

// Opposite of setDateType
// These strings match the strings in the PGFactFinder pattern DATE return values
std::string PGFactDate::getDateTypeString() {
	if (_dateType == START)
		return "start";
	else if (_dateType == END) 
		return "end";
	else if (_dateType == HOLD)
		return "hold";
	else if (_dateType == NON_HOLD)
		return "non_hold";
	else if (_dateType == ACTIVITY)
		return "activity";
	else 
		throw UnrecoverableException("PGFactDate::getDateTypeString", "Unknown date type");
}

int PGFactDate::compareSpecificityTo(PGFactDate_ptr factdate) {
	if (!_year_specified && !factdate->_year_specified)
		return SAME;
	if (_year_specified && !factdate->_year_specified)
		return MORE_SPECIFIC;
	if (!_year_specified && factdate->_year_specified)
		return LESS_SPECIFIC;

	// both years are specified

	if (!_month_specified && !factdate->_month_specified)
		return SAME;
	if (_month_specified && !factdate->_month_specified)
		return MORE_SPECIFIC;
	if (!_month_specified && factdate->_month_specified)
		return LESS_SPECIFIC;

	// both months are specified

	if (!_day_specified && !factdate->_day_specified)
		return SAME;
	if (_day_specified && !factdate->_day_specified)
		return MORE_SPECIFIC;
	if (!_day_specified && factdate->_day_specified)
		return LESS_SPECIFIC;

	// both days are specified

	return SAME;
}

int PGFactDate::compareToDate(PGFactDate_ptr factdate) {
	// compare years
	if (!_year_specified || !factdate->_year_specified)
		return UNKNOWN;
	if (_year < factdate->_year)
		return EARLIER;
	if (_year > factdate->_year)
		return LATER;

	// compare months
	if (!_month_specified || !factdate->_month_specified)
		return UNKNOWN;
	if (_month < factdate->_month)
		return EARLIER;
	if (_month > factdate->_month)
		return LATER;

	// compare days
	if (!_day_specified || !factdate->_day_specified)
		return UNKNOWN;
	if (_day < factdate->_day)
		return EARLIER;
	if (_day > factdate->_day)
		return LATER;

	return EQUAL;
}

PGFactDate::DateType PGFactDate::getDateType() {
	return _dateType;
}

bool PGFactDate::isBetween(PGFactDate_ptr startDate, PGFactDate_ptr endDate) {
	if (startDate != PGFactDate_ptr() && compareToDate(startDate) == EARLIER)
		return false;

	if (endDate != PGFactDate_ptr() && compareToDate(endDate) == LATER)
		return false;

	// often can't tell if it's really between, assume it's OK
	return true;
}

bool PGFactDate::isOutsideOf(PGFactDate_ptr startDate, PGFactDate_ptr endDate) {
	if (startDate != PGFactDate_ptr() && compareToDate(startDate) == EARLIER)
		return true;

	if (endDate != PGFactDate_ptr() && compareToDate(endDate) == LATER)
		return true;

	if (startDate == PGFactDate_ptr() || endDate == PGFactDate_ptr())
		return true; 

	if (compareToDate(startDate) == LATER && compareToDate(endDate) == EARLIER)
		return false;

	// often can't tell if it's really outside, assume it's OK
	return true;
}

bool PGFactDate::matchesSet(PGFactDateSet_ptr set) {
	BOOST_FOREACH(PGFactDate_ptr date, (*set)) {
		int comparison = compareToDate(date);
		if (comparison == EARLIER || comparison == LATER)
			return false;
	}
	return true;
}

bool PGFactDate::specificitySorter(PGFactDate_ptr i, PGFactDate_ptr j) {
	return i->compareSpecificityTo(j) == MORE_SPECIFIC;
}

std::string PGFactDate::toSimpleString() {
	std::string typeString = "UNKNOWN";
	
	if (_dateType == START)
		typeString = "START";
	else if (_dateType == END)
		typeString = "END";
	else if (_dateType == HOLD)
		typeString = "HOLD";
	else if (_dateType == NON_HOLD)
		typeString = "NON_HOLD";
	else if (_dateType == ACTIVITY)
		typeString = "ACTIVITY";
	else if (_dateType == NONE)
		typeString = "NONE";

	return typeString + ": " + getDBString();
}

std::string PGFactDate::getDBString() {
	std::string result = "";
	
	if (_year_specified)
		result += pad(_year, 4);
	else 
		return result;

	if (_month_specified)
		result += "-" + pad(_month, 2);
	else
		return result;

	if (_day_specified)
		result += "-" + pad(_day, 2);

	return result;
}

std::wstring PGFactDate::getDBStringW() {
	return UnicodeUtil::toUTF16StdString(getDBString());
}

std::string PGFactDate::pad(int num, size_t places) {
	std::stringstream ss;
	ss << num;
	std::string result = ss.str();
	while (result.size() < places) {
		result = "0" + result;
	}
	return result;
}

bool PGFactDate::isFullDate() {
	return _day_specified;
}

bool PGFactDate::isDaySpecified() { 
	return _day_specified; 
}

std::wstring PGFactDate::getNaturalDate() {
	std::stringstream naturalDate;


	if (_month_specified) {
		if (_month == 1) naturalDate << "January";
		if (_month == 2) naturalDate << "February";
		if (_month == 3) naturalDate << "March";
		if (_month == 4) naturalDate << "April";
		if (_month == 5) naturalDate << "May";
		if (_month == 6) naturalDate << "June";
		if (_month == 7) naturalDate << "July";
		if (_month == 8) naturalDate << "August";
		if (_month == 9) naturalDate << "September";
		if (_month == 10) naturalDate << "October";
		if (_month == 11) naturalDate << "November";
		if (_month == 12) naturalDate << "December";

		if (_day_specified)
			naturalDate << " " << _day << ",";
	}

	if (_year_specified) {
		if (naturalDate.str().length() > 0) naturalDate << " ";
		naturalDate << _year;
	}
	
	return UnicodeUtil::toUTF16StdString(naturalDate.str());
}
	
PGFact_ptr PGFactDate::getFact() {
	return _fact;
}
