// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/timex/en_TemporalNormalizer.h"

#include "English/timex/TimeObj.h"
#include "English/timex/TimePoint.h"
#include "English/timex/TimeAmount.h"
#include "English/timex/TimeUnit.h"
#include "English/timex/Year.h"
#include "English/timex/Month.h"
#include "English/timex/Day.h"
#include "English/timex/Hour.h"
#include "English/timex/Min.h"
#include "English/timex/Strings.h"

#include "Generic/common/SessionLogger.h"

#include <iostream>
#include <string>

using namespace std;

/* Fields to be added later in accordance with TIMEX2 standards */
//private boolean _nonSpecific;
TimeObj::TimeObj() {
	//std::cerr << "Creating1\n";
	_dateAndTime = _new TimePoint();
	_anchor = _new TimePoint();
	_duration = _new TimeAmount();
	_generic = L"";
	_mod = L"";
	_anchor_dir = L"";
	_set = false;
	_granularity = _new TimeAmount();
	_periodicity = _new TimeAmount();
}

/* Creates a TimeObj from a date string of the form: "YYYY-MM-DDTHH:MM" or 
"YYYY-WNN-DDTHH:MM" or an abbreviated string of either type */
TimeObj::TimeObj(const wstring &dateStr) {
	//std::cerr << "Creating2\n";
	_anchor = _new TimePoint();
	_duration = _new TimeAmount();
	_generic = L"";
	_mod = L"";
	_anchor_dir = L"";
	_set = false;
	_granularity = _new TimeAmount();
	_periodicity = _new TimeAmount();
	_dateAndTime = _new TimePoint(dateStr);
}

TimeObj::~TimeObj() {
	//std::cerr << "Destroying\n";
	delete _anchor;
	delete _duration;
	delete _granularity;
	delete _periodicity;
	delete _dateAndTime;
}


TimeObj* TimeObj::clone() {
	//std::cerr << "Cloning\n";
	TimeObj *temp = new TimeObj();

	temp->_dateAndTime = _dateAndTime->clone();
	temp->_anchor = _anchor->clone();
	temp->_duration = _duration->clone();
	temp->_generic = _generic;
	temp->_mod = _mod;
	temp->_anchor_dir = _anchor_dir;
	temp->_set = _set;
	temp->_granularity = _granularity->clone();
	temp->_periodicity = _periodicity->clone();
	return temp;
}

bool TimeObj::isWeekMode() {
	return _dateAndTime->isWeekMode();
}

bool TimeObj::isDuration() {
	return _duration->toString().length() > 0;
}

bool TimeObj::isSpecificDate() {
	return !isDuration();
}

bool TimeObj::isGeneric() {
	return _generic.length() > 0;
}

bool TimeObj::isSet() {
	return _set == true;
}

/* returns true if none of _dateAndTime's values have been set to anything other
than the default "XXXX-XX-XXTXX:XX" */
bool TimeObj::isEmpty() {
	return (isSpecificDate() && _dateAndTime->isEmpty());
}

bool TimeObj::isEmpty(const wchar_t *slot) {
	return (!isSpecificDate() || _dateAndTime->isEmpty(slot));
}

wstring TimeObj::toString() {
	wstring result = L"TYPE=\"VALUE\" ";
	result += L"VAL=\"";
	if (isDuration()) { 
		result += L"P" + _duration->toString();
	}
	else if (isGeneric()) {
		result += _generic;
	}
	else if (isSpecificDate() && !isEmpty()) {
		result += _dateAndTime->toString();
	}
	result += L"\" ";

	// NOT NECESSARY FOR ACE 
	//if (isSet()) {
	//	result += "SET=\"YES\" ";
	//	result += "GRANULARITY=\"G" + _granularity.toString() + "\" ";
	//	result += "PERIODICITY=\"F" + _periodicity.toString() + "\" ";
	//}

	result += L"MOD=\"";
	if (_mod.length() > 0) result += _mod;
	result += L"\" ";

	result += L"ANCHOR_VAL=\"";
	if (!_anchor->isEmpty()) {
		result += _anchor->toString();
	}
	result += L"\" ";

	result += L"ANCHOR_DIR=\"";
	if (_anchor_dir.length() > 0) result += _anchor_dir;
	result += L"\"";

	return result;
}

wstring TimeObj::valToString() {
	wstring result = L"";
	if (isDuration()) { 
		result += L"P" + _duration->toString();
	}
	else if (isGeneric()) {
		result += _generic;
	}
	else if (isSpecificDate() && !isEmpty()) {
		result += _dateAndTime->toString();
	}
	return result;
}

wstring TimeObj::modToString() {
	return _mod;
}


/* Sets higher order values of this TimeObj to match context until a
non-default value is found */
void TimeObj::addContext(TimePoint *context) {
	if (isSpecificDate() && !isSet()) {
		_dateAndTime->addContext(context);
	}
}

/* Sets higher order values of the anchor to match context until a
non-default value is found */
void TimeObj::addAnchorContext(TimePoint *context) {
	if (isGeneric()) {
		_anchor->addContext(context);
	}
}

/* Returns -1 if the date contained in this is prior to t, 1 if it is after t, 
and 0 if the two dates are equivalent or an ordering cannot be determined */
int TimeObj::compareTo(TimeObj *t) {
	if (isSpecificDate() && t->isSpecificDate()) {
		return _dateAndTime->compareTo(t->_dateAndTime);
	} else return 0;
}

int TimeObj::compareTo(TimePoint *t) {
	if (isSpecificDate()) {
		return _dateAndTime->compareTo(t);
	} else return 0;
}

void TimeObj::setMod(const wchar_t *mod) {
	setMod(wstring(mod));
}

void TimeObj::setMod(const wstring &m) {
	_mod = m;
}

void TimeObj::setAnchorDir(const wchar_t *anchor_dir) {
	setAnchorDir(wstring(anchor_dir));
}

void TimeObj::setAnchorDir(const wstring &a) {
	_anchor_dir = a;
}

void TimeObj::setDuration(const wchar_t *slot, int amt) {
	_duration->set(slot, amt);
	_dateAndTime->clear();
	_anchor->clear();
	_generic = L"";
	_anchor_dir = L"";
}

void TimeObj::setDuration(const wchar_t *slot, float amt) {
	_duration->set(slot, amt);
	_dateAndTime->clear();
	_anchor->clear();
	_generic = L"";
	_anchor_dir = L"";
}

int TimeObj::getDuration(const wchar_t *slot) {
	if (isDuration()) {
		_duration->get(slot);
	}
	return -1;
}

void TimeObj::setGeneric(const wstring &g) {
	_generic = g;
	_duration->clear();
	_dateAndTime->clear();
	_anchor->clear();
	_anchor_dir = L"";
}

void TimeObj::setTime(const wchar_t *slot, TimeUnit *val) {
	_dateAndTime->set(slot, val);
	_anchor->clear();
	_duration->clear();
	_generic = L"";
	_anchor_dir = L"";
}

void TimeObj::setTime(const wchar_t *slot, const wchar_t *val) {
	setTime(slot, wstring(val));
}	

void TimeObj::setTime(const wchar_t *slot, const wstring &val) {
	_dateAndTime->set(slot, val);
	_anchor->clear();
	_duration->clear();
	_generic = L"";
	_anchor_dir = L"";
}	

TimeUnit* TimeObj::getTime(const wchar_t *slot) {
	return _dateAndTime->get(slot);
}

void TimeObj::normalizeTime() {
	if (isSpecificDate()) {
		_dateAndTime->normalizeTime();
	}
}

void TimeObj::normalizeAnchor() {
	if (!_anchor->isEmpty()) {
		_anchor->normalizeTime();
	}
}

/* Sets current TimeObj to a specific date/time in relation to context by
looking at the values of mod, unit and num.  Ex: mod = "ago", unit = "year",
num = "3" --> set to 3 years before context
*/
void TimeObj::setRelativeTime(TimePoint *context, const wstring &verbTense, const wstring &mod, const wstring &unit, int num) {	
	_dateAndTime->setRelativeTime(context, verbTense, mod, unit, num);

	_set = false;
	_anchor->clear();
	_duration->clear();
	_generic = L"";	
	_anchor_dir = L"";
}

/* Can only take names of pureTimeUnits are argument for unit because other units
are not implemented with fractional amounts */
void TimeObj::setRelativeTime(TimePoint *context, const wstring &verbTense, const wstring &mod, const wstring &unit, float amt) {
	_dateAndTime->setRelativeTime(context, verbTense, mod, unit, amt);

	//_mod = "APPROX";

	_set = false;
	_anchor->clear();
	_duration->clear();
	_generic = L"";
	_anchor_dir = L"";
}

/* Sets current anchor to a specific date/time in relation to context by
looking at the values of mod, unit and num.  Ex: mod = "ago", unit = "year",
num = "3" --> set to 3 years before context
*/
void TimeObj::setRelativeAnchorTime(TimePoint *context, const wstring &verbTense, const wstring &mod, const wstring &unit, int num) {	
	_anchor->setRelativeTime(context, verbTense, mod, unit, num);			
}

void TimeObj::setGranularity(const wchar_t *slot, int amt) {
	if (isSet()) {
		_granularity->clear();
		_granularity->set(slot, amt);
	}
}

void TimeObj::setPeriodicity(const wchar_t *slot, int amt) {
	if (isSet()) {
		_periodicity->clear();
		_periodicity->set(slot, amt);
	}
}


void TimeObj::setSet() {
	_set = true;
	_granularity->clear();
	_periodicity->clear();
}

void TimeObj::setSet(const wchar_t *unit) {
	setSet(wstring(unit));
}

void TimeObj::setSet(const wstring &unit) {
	_set = true;

	_granularity->clear();
	_periodicity->clear();

	_anchor->clear();
	_duration->clear();
	_generic = L"";
	_anchor_dir = L"";

	if (EnglishTemporalNormalizer::isPureTimeUnit(unit)) {
		_granularity->set(EnglishTemporalNormalizer::timeUnit2Code(unit).c_str(), 1);
		_periodicity->set(EnglishTemporalNormalizer::timeUnit2Code(unit).c_str(), 1);
	} else if (Month::isMonth(unit)) {
		setTime(L"MM", _new Month(Month::month2Num(unit)));
		_granularity->set(L"M", 1);
		_periodicity->set(L"Y", 1);
	} else if (Day::isDayOfWeek(unit)) {
		setTime(L"MM", wstring(L"WXX"));
		setTime(L"DD", _new Day(Day::dayOfWeek2Num(unit), true));
		_granularity->set(L"D", 1);
		_periodicity->set(L"W", 1);
	} else if (Strings::startsWith(unit, wstring(L"weekend"))) {
		setTime(L"MM", wstring(L"WXX"));
		setTime(L"DD", wstring(L"WE"));
		_granularity->set(L"D", 2);
		_periodicity->set(L"W", 1);
	} else if (Strings::startsWith(unit, wstring(L"morning"))) {
		setTime(L"HH", wstring(L"MO"));
		_granularity->setOther(wstring(L"1MO"));
		_periodicity->set(L"D", 1);
	} else if (Strings::startsWith(unit, wstring(L"afternoon"))) {
		setTime(L"HH", wstring(L"AF"));
		_granularity->setOther(wstring(L"1AF"));
		_periodicity->set(L"D", 1);
	} else if (Strings::startsWith(unit, wstring(L"evening"))) {
		setTime(L"HH", wstring(L"EV"));
		_granularity->setOther(wstring(L"1EV"));
		_periodicity->set(L"D", 1);
	} else if (Strings::startsWith(unit, wstring(L"night"))) {
		setTime(L"HH", wstring(L"NI"));
		_granularity->setOther(wstring(L"1NI"));
		_periodicity->set(L"D", 1);
	} else if (Strings::startsWith(unit, wstring(L"spring"))) {
		setTime(L"MM", wstring(L"SP"));
		_granularity->setOther(wstring(L"1SP"));
		_periodicity->set(L"Y", 1);
	} else if (Strings::startsWith(unit, wstring(L"summer"))) {
		setTime(L"MM", wstring(L"SU"));
		_granularity->setOther(wstring(L"1SU"));
		_periodicity->set(L"Y", 1);
	} else if (Strings::startsWith(unit, wstring(L"fall")) || Strings::startsWith(unit, wstring(L"autumn"))) {
		setTime(L"MM", wstring(L"FA"));
		_granularity->setOther(wstring(L"1FA"));
		_periodicity->set(L"Y", 1);
	} else if (Strings::startsWith(unit, wstring(L"winter"))) {
		setTime(L"MM", wstring(L"WI"));
		_granularity->setOther(wstring(L"1WI"));
		_periodicity->set(L"Y", 1);
	} else {
		SessionLogger::warn("unknown_temporal_unit") << "Unit " << Symbol(unit.c_str()).to_debug_string() << " not implemented: ignoring\n";
	}
}


TimeAmount* TimeObj::getDuration() { return _duration; }
TimePoint* TimeObj::getDateAndTime() { return _dateAndTime; }
TimePoint* TimeObj::getAnchor() { return _anchor; }
wstring TimeObj::getGeneric() { return _generic; }
wstring TimeObj::getMod() { return _mod; }
wstring TimeObj::getAnchorDir() { return _anchor_dir; }
TimeAmount* TimeObj::getGranularity() { return _granularity; }
TimeAmount* TimeObj::getPeriodicity() { return _periodicity; }
