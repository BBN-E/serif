// Copyright (c) 2012 by BBNT Solutions LLC
// All Rights Reserved.

// PGFactDate.h
// This file describes the PGFactDate class, which is used to store data about PGFact entries' dates

#ifndef PG_FACT_DATE_H
#define PG_FACT_DATE_H

#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include <string>
#include <set>
#include <boost/regex.hpp> 
#include "boost/date_time/gregorian/gregorian.hpp"

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(PGFact);
BSP_DECLARE(PGFactDate);

typedef std::set<PGFactDate_ptr> PGFactDateSet;
typedef boost::shared_ptr<PGFactDateSet> PGFactDateSet_ptr;

class PGFactDate
{
public:
	enum DateType { NONE, START, END, HOLD, NON_HOLD, ACTIVITY };
	enum { MORE_SPECIFIC, LESS_SPECIFIC, SAME };
	enum { EARLIER, LATER, EQUAL, UNKNOWN };

	friend PGFactDate_ptr boost::make_shared<PGFactDate>(void);
	
	void setDateType(std::string dateTypeString);
	std::string getDateTypeString();

	int compareSpecificityTo(PGFactDate_ptr PGFact);
	int compareToDate(PGFactDate_ptr PGFact);

	DateType getDateType();

	// these two functions will return true if there's not enough info to determine
	bool isBetween(PGFactDate_ptr startDate, PGFactDate_ptr endDate);
	bool isOutsideOf(PGFactDate_ptr startDate, PGFactDate_ptr endDate);

	bool matchesSet(PGFactDateSet_ptr set);

	static bool specificitySorter(PGFactDate_ptr i, PGFactDate_ptr j);

	std::string toSimpleString();
	std::string getDBString();
	std::wstring getDBStringW();
	std::wstring getNaturalDate();

	bool isFullDate();

	virtual ~PGFactDate() { }

	bool isDaySpecified();

	void setPGFact(PGFact_ptr fact);
	PGFact_ptr getFact();

protected:
	// Default constructor does nothing (provided for boost::make_shared<PGFactDate>)
	PGFactDate(void) : _dateType(NONE), _year_specified(false), _month_specified(false),
		_day_specified(false), _year(-1), _month(-1), _day(-1), _fact(PGFact_ptr())
	{}

	DateType _dateType;

	static const boost::wregex _timex_regex_ymd;
	static const boost::wregex _timex_regex_ym;
	static const boost::wregex _timex_regex_y;

	bool _year_specified;
	bool _month_specified;
	bool _day_specified;

	int _year;
	int _month; 
	int _day;

	PGFact_ptr _fact;

	// helper functions
	std::string pad(int num, size_t places);
};

#endif
