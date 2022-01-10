// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/Symbol.h"
#include "English/timex/TimePoint.h"
#include "English/timex/TimeUnit.h"
#include "English/timex/Year.h"
#include "English/timex/Month.h"
#include "English/timex/Day.h"
#include "English/timex/Hour.h"
#include "English/timex/Min.h"
#include "English/timex/Strings.h"

#include "Generic/common/SessionLogger.h"

#include <string>
#include <iostream>

using namespace std;

TimePoint::TimePoint() {
	vYear = _new Year();
	vMonth = _new Month();
	vDay = _new Day();
	vHour = _new Hour();
	vMin = _new Min();
	vSec = _new Min();
	is_gmt = false;
	additionalTime = L"";
}

TimePoint::~TimePoint() {
	delete vYear;
	delete vMonth;
	delete vDay;
	delete vHour;
	delete vMin;
	delete vSec;
}

TimePoint::TimePoint(const wstring &dateStr) 
{
	additionalTime = L"";
	int pos = 0;
	
	wstring yearString = readUntil(dateStr, L'-', &pos);
	if (yearString.length() == 0) 
		vYear = _new Year();
	else 
		vYear = _new Year(yearString);
	pos++;
	//cout << "Year string " << Symbol(yearString.c_str()).to_debug_string() << "\n";

	wstring monthString = readUntil(dateStr, L'-', &pos);
	if (monthString.length() == 0)
		vMonth = _new Month();
	else 
		vMonth = _new Month(monthString);
	pos++;

	wstring dayString = readUntil(dateStr, L'T', &pos);
	if (dayString.length() == 0)
		vDay = _new Day();
	else if (pos >= (int)dateStr.length()) 
		vDay = _new Day(dayString, isWeekMode());
	else
		vDay = _new Day(dayString);
	pos++;

	wstring hourString = readUntil(dateStr, L':', &pos);
	if (hourString.length() == 0)
		vHour = _new Hour();
	else
		vHour = _new Hour(hourString);
	pos++;

	wstring minString = readUntil(dateStr, L':', &pos);
	if (minString.length() == 0)
		vMin = _new Min();
	else
		vMin = _new Min(minString);
	pos++;

	wstring secString = readUntil(dateStr, L' ', &pos);
	if (secString.length() == 0)
		vSec = _new Min();
	else 
		vSec = _new Min(secString);

	is_gmt = false;
}
	

void TimePoint::clear() {
	vYear->clear();
	vMonth->clear();
	vDay->clear();
	vHour->clear();
	vMin->clear();
	vSec->clear();
	is_gmt = false;
	additionalTime = L"";
}


wstring TimePoint::readUntil(const wstring &str, wchar_t end, int *pos) 
{
	wstring result = L"";
	while ((*pos) < (int)str.length()) {
		if (str.at(*pos) == end) break;

		result = result.append(str.substr(*pos, 1));
		(*pos)++;
	}
	return result;
}

TimePoint* TimePoint::clone() {
	TimePoint *temp = _new TimePoint();
	temp->set(L"YYYY", vYear->clone());
	temp->set(L"MM", vMonth->clone());
	temp->set(L"DD", vDay->clone());
	temp->set(L"HH", vHour->clone());
	temp->set(L"MN", vMin->clone());
	temp->set(L"SS", vSec->clone());
	temp->is_gmt = is_gmt;
	temp->additionalTime = additionalTime;
	return temp;
}

bool TimePoint::isWeekMode() {
	return static_cast<Month *>(vMonth)->isWeekMode;
}

bool TimePoint::isEmpty() {
	return (vYear->isEmpty() && static_cast<Month *>(vMonth)->isEmpty() && vDay->isEmpty() && 
		    vHour->isEmpty() && vMin->isEmpty() && vSec->isEmpty());
}

bool TimePoint::isEmpty(const wchar_t *slot) {
	if (!wcscmp(slot, L"YYYY")) return vYear->isEmpty();
	if (!wcscmp(slot, L"MM")) return static_cast<Month *>(vMonth)->isEmpty();
	if (!wcscmp(slot, L"DD")) return vDay->isEmpty();
	if (!wcscmp(slot, L"HH")) return vHour->isEmpty();
	if (!wcscmp(slot, L"MN")) return vMin->isEmpty();
	if (!wcscmp(slot, L"SS")) return vSec->isEmpty();
	return false;
}

wstring TimePoint::toString() {
	wstring result = L"";
	if (!vYear->isEmpty() || !static_cast<Month *>(vMonth)->isEmpty() || !vDay->isEmpty()) {
		wstring temp = vYear->toString();
		if (!vYear->isEmpty()) 
			while (temp.at(temp.length() -1) == L'X') 
				temp = temp.substr(0, temp.length() - 1);

		result += temp;

		if (!static_cast<Month *>(vMonth)->isEmpty() || !vDay->isEmpty()) 
			result += wstring(L"-") + vMonth->toString();

		if (!vDay->isEmpty()) result += L"-" + vDay->toString();
	} 
	if (!vHour->isEmpty() || !vMin->isEmpty() || !vSec->isEmpty()) {
		result += L"T";
		result += vHour->toString();
		if (!vMin->isEmpty() || !vSec->isEmpty()) result += L":" + vMin->toString();
		if (!vSec->isEmpty()) result += L":" + vSec->toString();
	}
	if (additionalTime.length() > 0) 
		result += additionalTime;
	if (is_gmt) 
		result += L"Z";

	return result;
}

/* Sets higher order values of this TimeObj to match context until a
non-default value is found */
void TimePoint::addContext(TimePoint *context) {
	//cout << "adding context to " << Symbol(toString().c_str()).to_debug_string() << "\n";
	
	if (vYear->isEmpty()) {
		delete vYear;
		vYear = context->vYear->clone();
	
		// JSG - if just the hour/min/sec is filled in, don't apply the context 
		//if( !vHour->isEmpty() && vMonth->isEmpty() && vDay->isEmpty() )
		//	return;
		
		if (vMonth->isEmpty()) {
			delete vMonth;
			vMonth = context->vMonth->clone();

			if (vDay->isEmpty()) {
				delete vDay;
				vDay = context->vDay->clone();

				if (vHour->isEmpty()) {
					delete vHour;
					vHour = context->vHour->clone();

					if (vMin->isEmpty()) {
						delete vMin;
						vMin = context->vMin->clone();

						if (vSec->isEmpty()) {
							delete vSec;
							vSec = context->vSec->clone();
						}
					}

					is_gmt = context->getIsGmt();
					additionalTime = context->getAdditionalTime();
				}
			}
		}
	}
}

/* Returns -1 if the date contained in this is prior to t, 1 if it is after t, 
and 0 if the two dates are equivalent or an ordering cannot be determined */
int TimePoint::compareTo(TimePoint *t) {
	int temp = 0;

	// compare years
	if (!vYear->isEmpty() && !t->vYear->isEmpty()) {
		if (!vYear->underspecified && !t->vYear->underspecified) {
			temp = vYear->compareTo(t->vYear);
		} 
		else if (!vYear->toString().compare(t->vYear->toString())) 
			temp = 0;
		else return 0; // underspecified, but not equal: cannot determine
	} else return 0; // years not set, cannot determine

	// if years equal, compare months
	if (temp == 0) {
		if (!vMonth->isEmpty() && !t->vMonth->isEmpty()) {
			// Both months specified and in same mode
			if (!vMonth->underspecified && !t->vMonth->underspecified &&
				(static_cast<Month *>(vMonth)->isWeekMode == static_cast<Month *>(t->vMonth)->isWeekMode))
				{ 
					temp = vMonth->compareTo(t->vMonth);	
					// months underspecified, but equal
				} 
				else if (!vMonth->toString().compare(t->vMonth->toString())) 
				{
					temp = 0;
					// months underspecified, not equal and/or in different modes
				} 
				else 
				{
					if (Month::isBefore(static_cast<Year *>(vYear), 
									    static_cast<Month *>(vMonth), 
										static_cast<Year *>(t->vYear), 
										static_cast<Month *>(t->vMonth)))
					{
					    temp = -1;
					}
				
					else if (Month::isAfter(static_cast<Year *>(vYear), 
						                    static_cast<Month *>(vMonth), 
											static_cast<Year *>(t->vYear), 
											static_cast<Month *>(t->vMonth)))
					{
						temp = 1;
					}
					
					else return 0; // cannot determine truth
				}

		} else return 0; // months not set, cannot determine truth
	}


	// years and months equal, compare days 
	if (temp == 0) {
		if (!vDay->isEmpty() && !t->vDay->isEmpty()) {
			// Both specified and in the same mode
			if (!vDay->underspecified && !t->vDay->underspecified && 
				static_cast<Day *>(vDay)->isWeekMode == static_cast<Day *>(t->vDay)->isWeekMode) 
			{
				temp = vDay->compareTo(t->vDay);
			} 
			else if (!vDay->toString().compare(t->vDay->toString())) 
			{
				temp = 0;
			}
			else return 0; // cannot determine truth
		} else return 0; // days not set, cannot determine
	}

	// years, months, days equal, now compare hours
	if (temp == 0) {
		if (!vHour->isEmpty() && !t->vHour->isEmpty()) {
			if (!vHour->underspecified && !t->vHour->underspecified) {
				temp = vHour->compareTo(t->vHour);
			} else if (!vHour->toString().compare(t->vHour->toString())) temp = 0;
			else {
				if (static_cast<Hour *>(vHour)->isBefore(static_cast<Hour *>(t->vHour))) temp = -1;
				else if (static_cast<Hour *>(vHour)->isAfter(static_cast<Hour *>(t->vHour))) temp = 1;
				else return 0; // cannot determine
			}
		} else return 0; // hours not set, cannot determine
	}

	// years, months, days, hours equal, now compare mins
	if (temp == 0) {
		if (!vMin->isEmpty() && !t->vMin->isEmpty()) {
			temp = vMin->compareTo(t->vMin);
		}
		else return 0; // min not set, cannot determine
	}

	// years, months, days, hours, minutes equals, compare secs
	if (temp == 0) {
		if (!vSec->isEmpty() && !t->vSec->isEmpty()) {
			temp = vSec->compareTo(t->vSec);
		}
		else return 0; // sec not set, cannot determin
	}

	return temp;
}


void TimePoint::set(const wchar_t *slot, TimeUnit *val) {
	if (!wcscmp(slot, L"YYYY")) {
		delete vYear;
		vYear = val;
	} else if (!wcscmp(slot, L"MM")) {
		delete vMonth;
		vMonth = val;
	} else if (!wcscmp(slot, L"DD")) {
		delete vDay;
		vDay = val;
	} else if (!wcscmp(slot, L"HH")) {
		delete vHour;
		vHour = val;
	} else if (!wcscmp(slot, L"MN")) {
		delete vMin;
		vMin = val;
	} else if (!wcscmp(slot, L"SS")) {
		delete vSec;
		vSec = val;
	}
}

void TimePoint::set(const wchar_t *slot, const wstring &val) {
	if (!wcscmp(slot, L"YYYY")) {
		delete vYear;
		vYear = new Year(val);
	} else if (!wcscmp(slot, L"MM")) {
		delete vMonth;
		vMonth = new Month(val);
	} else if (!wcscmp(slot, L"DD")) {
		delete vDay;
		vDay = new Day(val, isWeekMode());
	} else if (!wcscmp(slot, L"HH")) {
		delete vHour;
		vHour = new Hour(val);
	} else if (!wcscmp(slot, L"MN")) {
		delete vMin;
		vMin = new Min(val);
	} else if (!wcscmp(slot, L"SS")) {
		delete vSec;
		vSec = new Min(val);
	}
}

TimeUnit* TimePoint::get(const wchar_t *slot) {
	if (!wcscmp(slot, L"YYYY")) return vYear;
	if (!wcscmp(slot, L"MM")) return vMonth;
	if (!wcscmp(slot, L"DD")) return vDay;
	if (!wcscmp(slot, L"HH")) return vHour;
	if (!wcscmp(slot, L"MN")) return vMin;
	if (!wcscmp(slot, L"SS")) return vSec;
	return NULL;
}

void TimePoint::normalizeTime() {
	normalizeSlot(L"MM");
	normalizeSlot(L"DD");
	normalizeSlot(L"HH");
	normalizeSlot(L"MN");
	normalizeSlot(L"SS");
}

void TimePoint::normalizeSlot(const wchar_t *slot) {

	if (!wcscmp(slot, L"SS") && !vSec->underspecified) {
		while (vSec->value() < 0) {
			TimeUnit *temp = vSec;
			vSec = vSec->add(60);
			delete temp;

			if (!vMin->underspecified) {
				TimeUnit *temp = vMin;
				vMin = vMin->sub(1);
				delete temp;
				normalizeSlot(L"MN");
			}
		}
		while (vSec->value() > 59) {
			TimeUnit *temp = vSec;
			vSec = vSec->sub(60);
			delete temp;
			if (vMin->underspecified) {
				TimeUnit *temp = vMin;
				vMin = vMin->add(1);
				delete temp;
				normalizeSlot(L"MN");
			}
		}
	}
	else if (!wcscmp(slot, L"MN") && !vMin->underspecified) {
		while (vMin->value() < 0) {
			TimeUnit *temp = vMin;
			vMin = vMin->add(60);
			delete temp;
			if (!vHour->underspecified) {
				TimeUnit *temp = vHour;
				vHour = vHour->sub(1);
				delete temp;
				normalizeSlot(L"HH");
			}
		}
		while (vMin->value() > 59) {
			TimeUnit *temp = vMin;
			vMin = vMin->sub(60);
			delete temp;
			if (vHour->underspecified) {
				TimeUnit *temp = vHour;
				vHour = vHour->add(1);
				delete temp;
				normalizeSlot(L"HH");
			}
		}
	}
	else if (!wcscmp(slot, L"HH") && !vHour->underspecified) {
		while (vHour->value() < 0) {
			TimeUnit *temp = vHour;
			vHour = vHour->add(24);
			delete temp;
			if (!vDay->underspecified) { 
				TimeUnit *temp = vDay;
				vDay = vDay->sub(1);
				delete temp;
				normalizeSlot(L"DD");
			}
		}
		while ((vHour->value() > 24) || (vHour->value() == 24 && vMin->value() > 0)) {
			TimeUnit *temp = vHour;
			vHour = vHour->sub(24);
			delete temp;
			if (!vDay->underspecified) {
				TimeUnit *temp = vDay;
				vDay = vDay->add(1);
				delete temp;
				normalizeSlot(L"DD");
			}
		}
	}
	else if (!wcscmp(slot, L"DD") && !vDay->underspecified) {
		if (static_cast<Day *>(vDay)->isWeekMode) {
			while (vDay->value() <= 0) {
				TimeUnit *temp = vDay;
				vDay = vDay->add(7);
				delete temp;
				if (!vMonth->underspecified) {
					TimeUnit *temp = vMonth;
					vMonth = vMonth->sub(1);
					delete temp;
					normalizeSlot(L"MM");
				}
			}
			while (vDay->value() > 7) {
				TimeUnit *temp = vDay;
				vDay = vDay->sub(7);
				delete temp;
				if (!vMonth->underspecified) {
					TimeUnit *temp = vMonth;
					vMonth = vMonth->add(1);
					delete temp;
					normalizeSlot(L"MM");
				}
			}
		}
		else {	
			while (vDay->value() <= 0) {
				if (!vMonth->underspecified) {
					TimeUnit *temp = vMonth;
					vMonth = vMonth->sub(1);
					delete temp;
					normalizeSlot(L"MM");
					if (!vYear->underspecified) {
						TimeUnit *temp = vDay;
						vDay = vDay->add(Month::monthLength(vMonth->value(), vYear->value())); 
						delete temp;
					}
					else {
						TimeUnit *temp = vDay;
						vDay = vDay->add(Month::monthDays[vMonth->value()]);
						delete temp;
					}
				}
				else {
					TimeUnit *temp = vDay;
					vDay = vDay->add(31);
					delete temp;
				}
			}
			if (!vMonth->underspecified && !vYear->underspecified)  {				
				while (vDay->value() > Month::monthLength(vMonth->value(), vYear->value())) {
					TimeUnit *temp = vDay;
					vDay = vDay->sub(Month::monthLength(vMonth->value(), vYear->value()));
					delete temp;
					temp = vMonth;
					vMonth = vMonth->add(1);
					delete temp;
					normalizeSlot(L"MM");
				}
			} else if (!vMonth->underspecified) {
				while (vDay->value() > Month::monthDays[vMonth->value()]) {
					TimeUnit *temp = vDay;
					vDay = vDay->sub(Month::monthDays[vMonth->value()]);
					delete temp;
					temp = vMonth;
					vMonth = vMonth->add(1);
					delete temp;
					normalizeSlot(L"MM");
				}
			} else {
				while (vDay->value() > 31) {
					TimeUnit *temp = vDay;
					vDay = vDay->sub(31);
					delete temp;
				}
			}
		}
	}
	else if (!wcscmp(slot, L"MM") && !vMonth->underspecified) {
		if (isWeekMode()) {
			while (vMonth->value() <= 0) {
				TimeUnit *temp = vMonth;
				vMonth = vMonth->add(53);
				delete temp;
				if (!vYear->underspecified) {
					TimeUnit *temp = vYear;
					vYear = vYear->sub(1);
					delete temp;
				}
			}
			while (vMonth->value() > 53) {
				TimeUnit *temp = vMonth;
				vMonth = vMonth->sub(53);
				delete temp;
				if (!vYear->underspecified) {
					TimeUnit *temp = vYear;
					vYear = vYear->add(1);
					delete temp;
				}
			}
		}
		else {
			while (vMonth->value() <= 0) {
				TimeUnit *temp = vMonth;
				vMonth = vMonth->add(12);
				delete temp;
				if (!vYear->underspecified) {
					TimeUnit *temp = vYear;
					vYear =  vYear->sub(1);
					delete temp;
				}
			}
			while (vMonth->value() > 12) {
				TimeUnit *temp = vMonth;
				vMonth = vMonth->sub(12);
				delete temp;
				if (!vYear->underspecified) {
					TimeUnit *temp = vYear;
					vYear = vYear->add(1);
					delete temp;
				}
			}
		}
	}
}


void TimePoint::setRelativeTime(TimePoint *context, const wstring &verbTense, const wstring &mod, const wstring &unit, int num) {
	// whenever anything is taken from context, a clone is made, so no need to clone context
	//context = context->clone();
	int mult = 1;
	wstring slot = L"";

	//wcout << L"Call to setRelativeTime with unit: " << unit << L" mod: " << mod << L" num: " << num << L"\n";

	/* Deal with modifiers */
	if (!mod.compare(L"last") || !mod.compare(L"past") || !mod.compare(L"previous") || !mod.compare(L"ago")) 
		num *= -1;
	else if (!mod.compare(L"this"))
		num = 0;
	else if (!mod.compare(L"next") || !mod.compare(L"away"))
		num = num; // no change

	/* Deal with verb tense */
	bool isPastTense = false;
	bool isFutureTense = false;
	if (!verbTense.compare(L"past"))
		isPastTense = true;
	if (!verbTense.compare(L"future"))
		isFutureTense = true;

	/* Deal with units - set slot to update and multiplier */
	if (!unit.compare(L"century") || !unit.compare(L"centuries")) {
		const TimeUnit *year = context->get(L"YYYY");
		if (isEmpty() && !year->underspecified) {
			int upper = (int)(year->value() / 100) + num;
			wstring u = Strings::valueOf(upper);

			if (u.length() == 1) u = L"0" + u;
			set(L"YYYY", new Year(u + L"XX"));
		} else { 	
			mult = 100;
			slot = L"YYYY";
		}
	} else if (!unit.substr(0, 5).compare(L"month")) {
		slot = L"MM";
	} else if (!unit.substr(0, 7).compare(L"weekend")) {
		set(L"DD", new Day(wstring(L"WE"), true));
		if (context->isWeekMode()) { 
			slot = L"MM"; 
		}
		else {
			if (!context->vYear->underspecified && !context->vMonth->underspecified && !context->vDay->underspecified) {
				slot = L"MM";
				std::pair<int, int> year_week = Month::weekNum(context->vYear->value(), 
					context->vMonth->value(), 
					context->vDay->value());
				if (year_week.first >= 0) {
					context->set(L"YYYY", Strings::valueOf(year_week.first));
					std::wstring strVal = Strings::valueOf(year_week.second);
					if (strVal.length() < 2) strVal = L"0" + strVal;
					context->set(L"MM", L"W" + strVal);
				} else slot = L""; // shouldn't ever happen; I'm not sure this is what we're supposed to do if it does, but it'll do for now
			}
			else {  slot = L""; } // cannot handle key word 'weekend'														  
		}

	} else if (!unit.substr(0, 4).compare(L"week")) {
		if (context->isWeekMode()) { 
			slot = L"MM"; 
		}
		else {
			if (!context->vYear->underspecified && !context->vMonth->underspecified && !context->vDay->underspecified) {
				slot = L"MM";
				std::pair<int, int> year_week = Month::weekNum(context->vYear->value(), 
					context->vMonth->value(), 
					context->vDay->value());
				if (year_week.first >= 0) {
					context->set(L"YYYY", Strings::valueOf(year_week.first));
					std::wstring strVal = Strings::valueOf(year_week.second);
					if (strVal.length() < 2) strVal = L"0" + strVal;
					context->set(L"MM", L"W" + strVal);
				} else slot = L""; // shouldn't ever happen; I'm not sure this is what we're supposed to do if it does, but it'll do for now
			}
			else { 
				slot = L"DD"; 
				mult = 7;
			}														  
		}
	} else if (!unit.substr(0, 4).compare(L"year")) {
		slot = L"YYYY";
	} else if (!unit.substr(0, 3).compare(L"day")) {
		slot = L"DD";
	} else if (!unit.substr(0, 6).compare(L"decade")) {
		const TimeUnit *year = context->get(L"YYYY");
		if (isEmpty() && !year->underspecified) {
			int upper = (int)(year->value()/10) + num;
			wstring u = Strings::valueOf(upper); 
			if (u.length() == 1) u = L"0" + u;
			set(L"YYYY", new Year(u + L"X"));
		} else  {
			slot = L"YYYY";
			mult = 10;
		}
	} else if (!unit.compare(L"millennium") || !unit.compare(L"millennia")) {
		const TimeUnit *year = context->get(L"YYYY");
		if (isEmpty() && !year->underspecified) {
			int upper = (int)(year->value() / 1000) + num;
			wstring u = Strings::valueOf(upper); 
			if (u.length() == 0) u = L"0" + u;
			set(L"YYYY", new Year(u + L"XXX"));
		} else {
			slot = L"YYYY";
			mult = 1000;
		}
	} else if (!unit.substr(0, 4).compare(L"hour")) {
		slot = L"HH";
	} else if (!unit.substr(0, 6).compare(L"minute")) { 
		slot = L"MN";
	} else if (!unit.substr(0, 6).compare(L"second")) {
		slot = L"SS";
	} else if (!unit.substr(0, 7).compare(L"morning")) {
		slot = L"DD";
		delete vHour;
		vHour = new Hour(wstring(L"MO"));
	} else if (!unit.substr(0, 9).compare(L"afternoon")) {
		slot = L"DD";
		delete vHour;
		vHour = new Hour(wstring(L"AF"));
	} else if (!unit.substr(0, 7).compare(L"evening")) {
		slot = L"DD";
		delete vHour;
		vHour = new Hour(wstring(L"EV"));
	} else if (!unit.substr(0, 5).compare(L"night")) {
		slot = L"DD";
		delete vHour;
		vHour = new Hour(wstring(L"NI"));
	} else if (!unit.substr(0, 6).compare(L"spring")) {
		slot = L"YYYY";
		delete vMonth;
		vMonth = new Month(wstring(L"SP"));
		// still needs to deal with week mode
		if (!context->vMonth->underspecified && !context->isWeekMode()) {
			if ((num == 0) && (Month::month2Num(wstring(L"may")) < context->vMonth->value()) && isFutureTense) num++;
			else if ((num == 0) && (Month::month2Num(wstring(L"mar")) > context->vMonth->value()) && isPastTense) num--;
			else if ((num < 0) && (Month::month2Num(wstring(L"may")) < context->vMonth->value())) num++;
			else if ((num > 0) && (Month::month2Num(wstring(L"mar")) > context->vMonth->value())) num--;
		} else if (!context->vMonth->toString().compare(L"SU") || !context->vMonth->toString().compare(L"FA")) {
			if (num < 0) num++;
		}
	} else if (!unit.substr(0, 6).compare(L"summer")) {
		slot = L"YYYY";
		delete vMonth;
		vMonth = new Month(wstring(L"SU"));
		if (!context->vMonth->underspecified && !context->isWeekMode()) {
			if ((num == 0) && (Month::month2Num(wstring(L"aug")) < context->vMonth->value()) && isFutureTense) num++;
			else if ((num == 0) && (Month::month2Num(wstring(L"jun")) > context->vMonth->value()) && isPastTense) num--;
			else if ((num < 0) && (Month::month2Num(wstring(L"aug")) < context->vMonth->value())) num++;
			else if ((num > 0) && (Month::month2Num(wstring(L"jun")) > context->vMonth->value())) num--;
		} else if (!context->vMonth->toString().compare(L"FA")) {
			if (num < 0) num++;
		} else if (!context->vMonth->toString().compare(L"SP") || !context->vMonth->toString().compare(L"WI")) {
			if (num > 0) num--;
		}
	} else if (!unit.substr(0, 4).compare(L"fall") || !unit.substr(0, 6).compare(L"autumn")) {
		slot = L"YYYY";
		delete vMonth;
		vMonth = new Month(wstring(L"FA"));
		if (!context->vMonth->underspecified && !context->isWeekMode()) {
			if ((num == 0) && (Month::month2Num(wstring(L"nov")) < context->vMonth->value()) && isFutureTense) num++;
			else if ((num == 0) && (Month::month2Num(wstring(L"sep")) > context->vMonth->value()) && isPastTense) num--;
			else if ((num < 0) && (Month::month2Num(wstring(L"nov")) < context->vMonth->value())) num++;
			else if ((num > 0) && (Month::month2Num(wstring(L"sep")) > context->vMonth->value())) num--;
		} else if (!context->vMonth->toString().compare(L"WI") || !context->vMonth->toString().compare(L"SP") ||
			!context->vMonth->toString().compare(L"SU")) {
				if (num < 0) num++;
		} 
	} else if (!unit.substr(0, 6).compare(L"winter")) {
		slot = L"YYYY";
		delete vMonth;
		vMonth = new Month(wstring(L"WI"));
		if (!context->vMonth->underspecified && !context->isWeekMode()) {
			if ((num == 0) && (Month::month2Num(wstring(L"feb")) < context->vMonth->value()) && isFutureTense) num++;
			else if ((num < 0) && (Month::month2Num(wstring(L"feb")) < context->vMonth->value())) num++;
		} else if (!context->vMonth->toString().compare(L"SP") || !context->vMonth->toString().compare(L"SU") ||
			!context->vMonth->toString().compare(L"FA")) {
				if (num > 0) num--;
			}
	} else if (Month::isMonth(unit)) {
		slot = L"YYYY";
		int m = Month::month2Num(unit);
		delete vMonth;
		vMonth = new Month(m);
		// still needs to deal with cases like where context is fall and string is 'last may'
		if (!context->vMonth->underspecified && !context->isWeekMode()) {
			// adjust num to account for current context month
			int cmonth = context->vMonth->value();  // get the context month
			if ((num < 0) && (m < cmonth)) num++;
			else if ((num > 0) && (m > cmonth)) num--;
			else if (num == 0) {
				if ((m > cmonth) && (((Month::numMonths - m + cmonth) < (m - cmonth)) || isPastTense) && !isFutureTense)
					num--;
				if ((m < cmonth) && (((Month::numMonths - cmonth + m) < (cmonth - m)) || isFutureTense) && !isPastTense)
					num++;
			}
		}
	} else if (Day::isDayOfWeek(unit)) {
		slot = L"MM";
		int d = Day::dayOfWeek2Num(unit);
		if (context->isWeekMode()) {
			delete vDay;
			vDay = new Day(d, true);
			if (!context->vDay->underspecified) {
				// adjust num to find closest instance of d to context day
				if ((num < 0) && (d < context->vDay->value())) num++;
				else if ((num > 0) && (d > context->vDay->value())) num--;
				else if (num == 0) {
					if ((d > context->vDay->value()) && ((7-d + context->vDay->value()) < (d - context->vDay->value()))) 
						num--;
					if ((d < context->vDay->value()) && ((7-context->vDay->value() + d) < (context->vDay->value() - d))) 
						num++;
				}
			}
		}
		else {
			mult = 7;
			int cday = d; // set default context day to same as unit
			// if context day of week can be determined, get it
			if (!context->vYear->underspecified && !context->vMonth->underspecified && !context->vDay->underspecified) {
				cday = Day::dayOfWeek(context->vYear->value(), context->vMonth->value(), context->vDay->value());
			}
			// adjust num to find closest instance of d to context day
			if ((num < 0) && (d < cday)) num++;
			else if ((num > 0) && (d > cday)) num--;
			else if (num == 0) {
				if ((d > cday) && (((7-d + cday) < (d - cday)) || isPastTense) && !isFutureTense) 
					num--;
				if ((d < cday) && (((7-cday + d) < (cday - d)) || isFutureTense) && !isPastTense) {
					num++;
				}
			}
			// adjust context date to same day of week as unit
			if (vDay->isEmpty()) {
				if (cday < d) {
					TimeUnit *temp = vDay;
					vDay = context->vDay->add(d - cday);
					delete temp;
				} else if (cday > d) {
					TimeUnit *temp = vDay;
					vDay = context->vDay->sub(cday - d);
					delete temp;
				} else {
					TimeUnit *temp = vDay;
					vDay = context->vDay->clone();
					delete temp;
				}
				// update day for modifiers 
				TimeUnit *temp = vDay;
				vDay = vDay->add(num * mult);
				delete temp;
			}
			num = 0;	// set num to 0 so that month won't be changed
		}			
	} else {
		SessionLogger::warn("unknown_temporal_unit") << "Unit " << Symbol(unit.c_str()).to_debug_string() << " not implemented: ignoring";
	}
	if (slot.compare(L"")) {
		const TimeUnit *tu = context->get(slot.c_str());
		//wcout << L"Adjusting " << slot << L" by " << num << L" * " << mult << L" = " << num*mult;
		//wcout << L" from " << tu->toString() << L"\n";
		set(slot.c_str(), tu->add(num * mult));
		//wcout << L"Result: " << this->toString() << L"\n";
	}
	addContext(context);
	normalizeTime();		
}

void TimePoint::setRelativeTime(TimePoint *context, const wstring &verbTense, const wstring &mod, const wstring &unit, float amt) {
	// whenever anything is taken from context, a clone is made, so no need to clone context
	//context = context->clone();
	int n = 0, mult = 1;
	wstring slot = L"";
	wstring slot2 = L"";

	/* Deal with fractional portion of num */
	int num = (int)amt;
	float fraction = amt - num;		

	/* Deal with modifiers */
	if (!mod.compare(L"last") || !mod.compare(L"past") || !mod.compare(L"previous") || !mod.compare(L"ago")) {
		num *= -1; amt *= -1; fraction *= -1;
	}
	else if (!mod.compare(L"this")) {
		num = 0; amt = 0; fraction = 0;
	}
	else if (!mod.compare(L"next") || !mod.compare(L"away")) {
		num = num; amt = amt; fraction = fraction; // no change
	}

	/* Deal with units - set slot to update and multiplier */
	if (!unit.substr(0, 4).compare(L"year")) {
		slot = L"YYYY";
	} else if (!unit.substr(0, 5).compare(L"month")) {
		slot = L"MM";
	} else if (!unit.substr(0, 3).compare(L"day")) {
		slot = L"DD";
	} else if (!unit.substr(0, 4).compare(L"hour")) {
		slot = L"HH";
	} else if (!unit.substr(0, 6).compare(L"minute")) { 
		slot = L"MN";
	} else if (!unit.substr(0, 6).compare(L"second")) {
		slot = L"SS";
	}

	/* Finish any straightforward units and return */
	if (slot.length() != 0) {
		const TimeUnit *tu = context->get(slot.c_str());
		set(slot.c_str(), tu->add(num));
		if (fraction != 0) {
			if (!wcscmp(slot.c_str(), L"YYYY")) { slot2 = L"MM"; n = 12; }
			if (!wcscmp(slot.c_str(), L"MM"))   { slot2 = L"DD"; n = 30; }
			if (!wcscmp(slot.c_str(), L"DD"))   { slot2 = L"HH"; n = 24; }
			if (!wcscmp(slot.c_str(), L"HH"))   { slot2 = L"MN"; n = 60; }
			if (!wcscmp(slot.c_str(), L"MN"))   { slot2 = L"SS"; n = 60; }
			if (!wcscmp(slot.c_str(), L"SS"))   { slot2 = L"";   n = 0;  }
			const TimeUnit *tu2 = context->get(slot2.c_str());
			set(slot2.c_str(), tu2->add((int)(fraction*n)));
		}
		addContext(context);
		normalizeTime();
		return;
	}

	/* Deal with more difficult units */

	if (!unit.substr(0, 4).compare(L"week")) {
		if (context->isWeekMode()) { 
			const TimeUnit *month = context->get(L"MM");
			const TimeUnit *day = context->get(L"DD");
			set(L"MM", month->add(num));
			set(L"DD", day->add((int)(fraction*7)));
		}
		else {
			if (!context->vYear->underspecified && !context->vMonth->underspecified && !context->vDay->underspecified) {
				std::pair<int, int> year_week = Month::weekNum(context->vYear->value(), 
					context->vMonth->value(), 
					context->vDay->value());
				if (year_week.first >= 0) {
					context->set(L"YYYY", Strings::valueOf(year_week.first));
					std::wstring strVal = Strings::valueOf(year_week.second);
					if (strVal.length() < 2) strVal = L"0" + strVal;
					context->set(L"MM", L"W" + strVal);
					int d = Day::dayOfWeek(context->vYear->value(), context->vMonth->value(), context->vDay->value());
					context->set(L"DD", _new Day(d, true));
					const TimeUnit *month = context->get(L"MM");
					const TimeUnit *day = context->get(L"DD");
					set(L"MM", month->add(num));
					set(L"DD", day->add((int)(fraction*7)));
				} else slot = L""; // shouldn't ever happen; I'm not sure this is what we're supposed to do if it does, but it'll do for now
			}
			else {  
				const TimeUnit *contextDay = context->get(L"DD");
				const TimeUnit *day = get(L"DD");

				set(L"DD", contextDay->add(num*7));
				set(L"DD", day->add((int)(fraction*7)));
			}														  
		}
		return;
	}


	if (!unit.compare(L"millennium") || !unit.compare(L"millennia")) {
		slot = L"YYYY";
		mult = 1000; 
		const TimeUnit *year = context->get(L"YYYY");
		// let millennium be underspecified if no other fields set already
		if (isEmpty() && !year->underspecified) {
			int y = (int)(year->value() + (amt * mult));
			wstring u = Strings::valueOf(y);
			if (u.length() == 3) 
				u = L"0" + u;
			if (fraction == 0) {
				set(L"YYYY", _new Year(u.substr(0,1) + L"XXX"));
			} else {
				set(L"YYYY", _new Year(u.substr(0,2) + L"XX"));
			}
		}
	} else if (!unit.compare(L"century") || !unit.compare(L"centuries")) {
		mult = 100;
		slot = L"YYYY";
	} else if (!unit.substr(0, 6).compare(L"decade")) {
		slot = L"YYYY";
		mult = 10;
	} else {
		SessionLogger::warn("unknown_temporal_unit") << "Unit " << Symbol(unit.c_str()).to_debug_string() 
			<< " not implemented: ignoring";
	}

	if (slot.length() != 0)  {
		const TimeUnit *tu = context->get(slot.c_str());

		set(slot.c_str(), tu->add((int)(amt * mult)));

		addContext(context);
		normalizeTime();
	}
}
