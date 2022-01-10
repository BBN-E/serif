// Copyright (c) 2013 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef DATE_DESCRIPTION_H
#define DATE_DESCRIPTION_H

#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"

#include "ProfileGenerator/ProfileSlot.h"
#include "ProfileGenerator/PGFactDate.h"
#include <string>

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(GenericHypothesis);
BSP_DECLARE(DateDescription);
BSP_DECLARE(Profile);

class DateDescription {
public:
	friend DateDescription_ptr boost::make_shared<DateDescription>(Profile_ptr const &, GenericHypothesis_ptr const&, ProfileSlot_ptr const&);
	std::wstring getDateDescription() { return _dateDescription; }
	
private:
	DateDescription(Profile_ptr profile, GenericHypothesis_ptr hypoth, ProfileSlot_ptr slot);
	
	std::wstring _dateDescription;

	PGFactDate_ptr getEarliestDate(std::vector<PGFactDate_ptr> &dates);
	PGFactDate_ptr getLatestDate(std::vector<PGFactDate_ptr> &dates);
	PGFactDate_ptr getLatestDateAfter(std::vector<PGFactDate_ptr> &dates, PGFactDate_ptr referenceDate);
	PGFactDate_ptr getEarliestDateAfter(std::vector<PGFactDate_ptr> &dates, PGFactDate_ptr referenceDate);
	PGFactDate_ptr getEarliestDateBefore(std::vector<PGFactDate_ptr> &dates, PGFactDate_ptr referenceDate);
	std::wstring getPreposition(PGFactDate_ptr date);
	std::vector<PGFactDate_ptr> getUniqueDates(std::vector<PGFactDate_ptr> &dates);
	static bool compareDates(PGFactDate_ptr date1, PGFactDate_ptr date2);

	std::wstring getEmployer(std::wstring entityName, PGFactDate_ptr start, PGFactDate_ptr end, std::vector<PGFactDate_ptr> &holdDates, std::vector<PGFactDate_ptr> &nonHoldDates);
	std::wstring getEducation(std::wstring entityName, std::wstring displayValue, PGFactDate_ptr start, PGFactDate_ptr end, std::vector<PGFactDate_ptr> &holdDates);
	std::wstring getSpouse(std::wstring entityName, std::wstring displayValue, PGFactDate_ptr start, PGFactDate_ptr end, std::vector<PGFactDate_ptr> &holdDates);
	std::wstring getHeadquarters(std::wstring displayValue, PGFactDate_ptr start, PGFactDate_ptr end, std::vector<PGFactDate_ptr> &holdDates);
	std::wstring getActivity(std::vector<PGFactDate_ptr> &dates, std::wstring slotWord);
	std::wstring getVisitOrg(std::wstring entityNameStartSentence, std::wstring entityNameMidSentence, std::wstring displayValue, std::vector<PGFactDate_ptr> &allDates, std::wstring verb);
	std::wstring getVisitPer(std::wstring entityName, std::wstring displayValue, std::vector<PGFactDate_ptr> &allDates);
	std::wstring getRelationship(std::wstring entityName, std::wstring displayValue, std::vector<PGFactDate_ptr> &allDates);
};

#endif
