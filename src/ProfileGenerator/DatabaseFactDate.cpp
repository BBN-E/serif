// Copyright (c) 2012 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/UnicodeUtil.h"

#include "ProfileGenerator/DatabaseFactDate.h"

// Parse the resolved string value and grab year, month, day if they exist
DatabaseFactDate::DatabaseFactDate(int date_id, PGFact_ptr fact, std::string date_type_str,
		std::wstring literal_string_value, std::wstring resolved_string_value) : 
		_date_id(date_id), _literal_string_val(literal_string_value), _resolved_string_val(resolved_string_value)
{
	//std::cout << "Initializing date " << UnicodeUtil::toUTF8StdString(_resolved_string_val) << "\n";

	setDateType(date_type_str);
	_fact = fact;

	boost::wcmatch matches;

	if (boost::regex_search(_resolved_string_val.c_str(), matches, _timex_regex_ymd)) {
		_year_specified = true;
		_month_specified = true;
		_day_specified = true;

		std::wstring year_string(matches[1].first, matches[1].second);
		_year = boost::lexical_cast<int>(year_string);

		std::wstring month_string(matches[2].first, matches[2].second);
		_month = boost::lexical_cast<int>(month_string);

		std::wstring day_string(matches[3].first, matches[3].second);
		_day = boost::lexical_cast<int>(day_string);
	} else if (boost::regex_search(_resolved_string_val.c_str(), matches, _timex_regex_ym)) {
		_year_specified = true;
		_month_specified = true;

		std::wstring year_string(matches[1].first, matches[1].second);
		_year = boost::lexical_cast<int>(year_string);

		std::wstring month_string(matches[2].first, matches[2].second);
		_month = boost::lexical_cast<int>(month_string);
	} else if (boost::regex_search(_resolved_string_val.c_str(), matches, _timex_regex_y)) {
		_year_specified = true;

		std::wstring year_string(matches[1].first, matches[1].second);
		_year = boost::lexical_cast<int>(year_string);
	}

/*	if (_year_specified)
		std::cout << "Year: " << _year << "\n";
	if (_month_specified)
		std::cout << "Month: " << _month << "\n";
	if (_day_specified)
		std::cout << "Day: " << _day << "\n";

	if (!_year_specified) 
		std::cout << "No match!\n";
*/
}
