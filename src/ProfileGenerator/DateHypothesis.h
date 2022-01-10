// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef DATE_HYPOTHESIS_H
#define DATE_HYPOTHESIS_H

#include "ProfileGenerator/GenericHypothesis.h"
#include "ProfileGenerator/PGFact.h"
#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include <boost/regex.hpp> 

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(DateHypothesis);

/*! \brief An implementation of GenericHypothesis for date-valued slot types
*/
class DateHypothesis : public GenericHypothesis
{
public:
	friend DateHypothesis_ptr boost::make_shared<DateHypothesis>(PGFact_ptr const&);

	// Virtual parent class functions, implemented here
	bool isEquiv(GenericHypothesis_ptr hypoth);
	void addSupportingHypothesis(GenericHypothesis_ptr hypo);
	bool isIllegalHypothesis(ProfileSlot_ptr slot, std::string& rationale);
	std::wstring getDisplayValue();

	// Class-specific member functions
	std::wstring getResolvedDate() { return _resolved_date; }
	
private:
	DateHypothesis(PGFact_ptr fact) {
		addFact(fact);
		PGFactArgument_ptr answerArg = fact->getAnswerArgument();
		if (answerArg)
			setDateString(answerArg->getResolvedStringValue());
	}

	// Class-specific member variables
	std::wstring _resolved_date;

	// Helper functions for dates
	void setDateString(std::wstring date_string);
	bool isMoreSpecificDate(std::wstring otherTimex);
	bool isEquivValue(std::wstring otherTimex);

	// Regular expressions for parsing TIMEX strings (static) 

	// YYYY-MM-DD.*
	static const boost::wregex _timex_regex_ymd;

	// YYYY-MM-DD exact
	static const boost::wregex _timex_regex_ymd_exact;

	// YYYY-MM.*
	static const boost::wregex _timex_regex_ym;

	// YYYY-MM exact
	static const boost::wregex _timex_regex_ym_exact;

	// YYYY-W##
	static const boost::wregex _timex_regex_yw;

	// YYYY
	static const boost::wregex _timex_regex_y_anywhere;

	// YYYY
	static const boost::wregex _timex_regex_y;

	// YYYY exact
	static const boost::wregex _timex_regex_y_exact;

	// YYYY-
	static const boost::wregex _timex_regex_y_hyphen;

};

#endif
