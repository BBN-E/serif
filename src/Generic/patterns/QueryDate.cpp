// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "QueryDate.h"
#include <string>
#include <math.h>
#include "theories/Value.h"
#include "common/ParamReader.h"

const std::wstring QueryDate::EARLIEST_LEGAL_DATE = L"19000101";
const std::wstring QueryDate::LATEST_LEGAL_DATE = L"20751231";

// these are some basic regular expressions for dates
// they can be expanded if we are given a set of allowable formats
// ASSUMPTION: they all must match the ENTIRE string

// YYYYMMDD  (this is what we normalize to)
const boost::wregex QueryDate::_date_regex_0(L"^\\s*([12][0-9][0-9][0-9])([01][0-9])([0123][0-9])\\s*$");

// Mmmmm YYYY or Mmmmm YY
const boost::wregex QueryDate::_date_regex_1(L"^\\s*(january|jan\\.?|february|feb\\.?|march|mar\\.?|april|apr\\.?|may|june|jun\\.?|july|jul\\.?|august|aug\\.?|september|sept\\.?|october|oct\\.?|november|nov\\.?|december|dec\\.?) ([12]?[0-9]?[0-9][0-9])\\s*$", boost::regex::perl|boost::regex::icase);

// DD Mmmmm YYYY or DD Mmmmm YY
const boost::wregex QueryDate::_date_regex_2(L"^\\s*([0123]?[0-9]) (january|jan\\.?|february|feb\\.?|march|mar\\.?|april|apr\\.?|may|june|jun\\.?|july|jul\\.?|august|aug\\.?|september|sept\\.?|october|oct\\.?|november|nov\\.?|december|dec\\.?) ([12]?[0-9]?[0-9][0-9])\\s*$", boost::regex::perl|boost::regex::icase);

// Mmmmm DD YYYY or Mmmmm DD YY or Mmmmm DD, YYYY or Mmmmm DD, YY
const boost::wregex QueryDate::_date_regex_3(L"^\\s*(january|jan\\.?|february|feb\\.?|march|mar\\.?|april|apr\\.?|may|june|jun\\.?|july|jul\\.?|august|aug\\.?|september|sept\\.?|october|oct\\.?|november|nov\\.?|december|dec\\.?) ([0123]?[0-9]),? ([12]?[0-9]?[0-9][0-9])\\s*$", boost::regex::perl|boost::regex::icase);

// M?M/D?D/YYYY
const boost::wregex QueryDate::_date_regex_4a(L"^\\s*([01]?[0-9])/([0123]?[0-9])/([12][0-9][0-9][0-9])\\s*$");

// M?M/D?D/YY
const boost::wregex QueryDate::_date_regex_4b(L"^\\s*([01]?[0-9])/([0123]?[0-9])/([0-9][0-9])\\s*$");

// M?M-D?D-YYYY
const boost::wregex QueryDate::_date_regex_4c(L"^\\s*([01]?[0-9])-([0123]?[0-9])-([12][0-9][0-9][0-9])\\s*$");

// M?M-D?D-YY
const boost::wregex QueryDate::_date_regex_4d(L"^\\s*([01]?[0-9])-([0123]?[0-9])-([0-9][0-9])\\s*$");

// M?M/YYYY
const boost::wregex QueryDate::_date_regex_5a(L"^\\s*([01]?[0-9])/([12][0-9][0-9][0-9])\\s*$");

// M?M/YY
const boost::wregex QueryDate::_date_regex_5b(L"^\\s*([01]?[0-9])/([0-9][0-9])\\s*$");

// M?M-YYYY
const boost::wregex QueryDate::_date_regex_5c(L"^\\s*([01]?[0-9])-([12][0-9][0-9][0-9])\\s*$");

// M?M-YY
const boost::wregex QueryDate::_date_regex_5d(L"^\\s*([01]?[0-9])-([0-9][0-9])\\s*$");

// YYYY
const boost::wregex QueryDate::_date_regex_6(L"^\\s*([12][0-9][0-9][0-9])\\s*$");

// YYYY-MM-DD
const boost::wregex QueryDate::_date_regex_7(L"^\\s*([12][0-9][0-9][0-9])-([0123]?[0-9])-([0123]?[0-9])\\S*\\s*$");


// these are expressions for specifically parsing TIMEX normalizations of dates

// YYYY-MM-DD
const boost::wregex QueryDate::_timex_regex_ymd(L"^([12][0-9][0-9][0-9])-([0123]?[0-9])-([0123]?[0-9]).*");

// YYYY-MM
const boost::wregex QueryDate::_timex_regex_ym(L"^([12][0-9][0-9][0-9])-([0123]?[0-9]).*");

// YYYY-W##
const boost::wregex QueryDate::_timex_regex_yw(L"^([12][0-9][0-9][0-9])-W([012345][0-9]).*");

// YYYY
const boost::wregex QueryDate::_timex_regex_y(L"^([12][0-9][0-9][0-9]).*");

// YYYY exact
const boost::wregex QueryDate::_timex_regex_y_exact(L"^([12][0-9][0-9][0-9])$");

// YYYY-
const boost::wregex QueryDate::_timex_regex_y_hyphen(L"^([12][0-9][0-9][0-9])-.*");

// For _snwprintf calls:
#define MAX_DATE_RESULT_LENGTH 256

wchar_t QueryDate::corpus1_start[9];
wchar_t QueryDate::corpus1_end[9];
wchar_t QueryDate::corpus2_start[9];
wchar_t QueryDate::corpus2_end[9];
wchar_t QueryDate::corpus3_start[9];
wchar_t QueryDate::corpus3_end[9];

QueryDate::QueryDate(const wchar_t * date_type, const wchar_t * date_start, const wchar_t * date_end) {
	int index = 0;
	for (; index < (int) wcslen(date_start) && index < 99; index++ )
	{
		if (iswupper(date_start[index])) {
			_date_start[index] = towlower(date_start[index]);
		} else _date_start[index] = date_start[index];
	}
	_date_start[index] = L'\0';

	for (index = 0; index < (int) wcslen(date_end) && index < 99; index++ )
	{
		if (iswupper(date_end[index])) {
			_date_end[index] = towlower(date_end[index]);
		} else _date_end[index] = date_end[index];
	}
	_date_end[index] = L'\0';

	wcsncpy(_date_type, date_type, 99);

	wcsncpy(_normalized_date_start, L"\0", 1);
	wcsncpy(_normalized_date_end, L"\0", 1);

	normalizeDate(_date_start, START, _normalized_date_start, _start_year, _start_month, _start_day);
	getWeekForDate(_start_year, _start_month, _start_day, _start_WF_year, _start_WF_week);
	normalizeDate(_date_end, END, _normalized_date_end, _end_year, _end_month, _end_day);
	getWeekForDate(_end_year, _end_month, _end_day, _end_WF_year, _end_WF_week);

}

QueryDate::~QueryDate()
{
}

bool QueryDate::normalizeDate(const wchar_t *date_string, int start_or_end, wchar_t *normalized_date_string, int& year, int& month, int& day)  {

	// run a regex over the date
	boost::wcmatch matchResult;
	std::wstring yearStr = L"";
	std::wstring monthStr = L"";
	std::wstring monthIntStr = L"";
	std::wstring dayStr = L"";   

	if (boost::regex_match(date_string, matchResult, _date_regex_0)) {
		yearStr = matchResult[1];
		monthIntStr = matchResult[2];
		dayStr = matchResult[3];		
    } else if (boost::regex_match(date_string, matchResult, _date_regex_1)) {
		monthStr = matchResult[1];
		yearStr = matchResult[2];
	} else if (boost::regex_match(date_string, matchResult, _date_regex_2)) {
		dayStr = matchResult[1];
		monthStr = matchResult[2];
		yearStr = matchResult[3];
	} else if (boost::regex_match(date_string, matchResult, _date_regex_3)) {
		monthStr = matchResult[1];
		dayStr = matchResult[2];
		yearStr = matchResult[3];
	} else if (boost::regex_match(date_string, matchResult, _date_regex_4a) ||
		boost::regex_match(date_string, matchResult, _date_regex_4b) ||
		boost::regex_match(date_string, matchResult, _date_regex_4c) ||
		boost::regex_match(date_string, matchResult, _date_regex_4d)) 
	{
		monthIntStr = matchResult[1];
		dayStr = matchResult[2];
		yearStr = matchResult[3];
	} else if (boost::regex_match(date_string, matchResult, _date_regex_5a) ||
		boost::regex_match(date_string, matchResult, _date_regex_5b) ||
		boost::regex_match(date_string, matchResult, _date_regex_5c) ||
		boost::regex_match(date_string, matchResult, _date_regex_5d)) 
	{
		monthIntStr = matchResult[1];
		yearStr = matchResult[2];
	} else if (boost::regex_match(date_string, matchResult, _date_regex_6)) {
		yearStr = matchResult[1];		
	} else if (boost::regex_match(date_string, matchResult, _date_regex_7)) {
		yearStr = matchResult[1];
		monthIntStr = matchResult[2];
		dayStr = matchResult[3];		
	} else {
		// we got nothin'
		return false;
	}

	// we have to have a year
	year = _wtoi(yearStr.c_str());

	// best guess: 2020 is the latest year we'll get if in a 2-year form
	if (year < 20) {
		year += 2000;
	} else if (year < 100) {
		year += 1900;
	}

	if (monthIntStr != L"") {
		month = _wtoi(monthIntStr.c_str());
	} else {
		std::wstring month_lc = L"";
		for (size_t i = 0; i < monthStr.length() && i < 3; i++) {
			month_lc += towlower(monthStr.at(i));
		}
		if (month_lc == L"jan") {
			month = 1;
		} else if (month_lc == L"feb") {
			month = 2;
		} else if (month_lc == L"mar"){
			month = 3;
		} else if (month_lc == L"apr"){
			month = 4;
		} else if (month_lc == L"may"){
			month = 5;	
		} else if (month_lc == L"jun"){
			month = 6;
		} else if (month_lc == L"jul"){
			month = 7;	
		} else if (month_lc == L"aug"){
			month = 8;	
		} else if (month_lc == L"sept"){
			month = 9;
		} else if (month_lc == L"oct"){
			month = 10;	
		} else if (month_lc == L"nov"){
			month = 11;
		} else if (month_lc == L"dec"){
			month = 12;		
		} else {
			if (start_or_end == START || start_or_end == START_MINUS_ONE)
				month = 1;
			else month = 12;
		}
	}

	if (dayStr.size() > 0) {
		day = _wtoi(dayStr.c_str());
		day = getClosestLegalDay(year, month, day);
	} else if (start_or_end == START || start_or_end == START_MINUS_ONE) {
		day = 1;
	} else {
		day = getLastDayOfMonth(year, month);
	}

	if (start_or_end == START_MINUS_ONE) {
		if (day > 1) {
			day--;
		} else if (month > 1) {
			month--;
			day = getLastDayOfMonth(year, month);
		} else {
			// must be January 1
			year--;
			month = 12;
			day = 31;
		}
	} else if (start_or_end == END_PLUS_ONE) {
		if (day + 1 <= getLastDayOfMonth(year, month)) {
			day++;
		} else if (month != 12) {
			month++;
			day = 1;
		} else {
			// must be December 31
			year++;
			month = 1;
			day = 1;
		}
	}

	createNormalizedDate(normalized_date_string, year, month, day);
	return true;
}

void QueryDate::getNormalizedDateEndPlusMonths(wchar_t *result, int n) const {

	int month = (_end_month + n) % 12;
	if (month == 0)
		month = 12;
	int year = _end_year + (int) floor((_end_month - 1 + n) / 12.0);
	int day = getClosestLegalDay(year, month, _end_day);
	createNormalizedDate(result, year, month, day);

}

void QueryDate::getNormalizedDateEndPlusDays(wchar_t *result, int n) const {
	// TODO-- for heaven's sake, use a library to do this
	int year = _end_year;
	int month = _end_month;
	int day = _end_day;
	for (int d = 0; d < n; d++) {
		int new_day = day + 1;
		int new_legal_day = getClosestLegalDay(year, month, new_day);
		if (new_day != new_legal_day) {
			month++;
			if (month == 13) {
				year++;
				month = 1;
			}
			day = 0;
		} else {
			day = new_day;
		}
	}
	createNormalizedDate(result, year, month, day);
}

void QueryDate::getStringDateEndPlusMonths(wchar_t *result, int n) const {

	int month = (_end_month + n) % 12;
	if (month == 0)
		month = 12;
	int year = _end_year + (int) floor((_end_month - 1 + n) / 12.0);
	int day = getClosestLegalDay(year, month, _end_day);
		
	createStringDate(result, year, month, day);
}

void QueryDate::createNormalizedDate(wchar_t *result, int year, int month, int day) {
	if (month < 10 && day < 10)
		_snwprintf(result, MAX_DATE_RESULT_LENGTH, L"%d0%d0%d", year, month, day);
	else if (month < 10) 
		_snwprintf(result, MAX_DATE_RESULT_LENGTH, L"%d0%d%d", year, month, day);
	else if (day < 10) 
		_snwprintf(result, MAX_DATE_RESULT_LENGTH, L"%d%d0%d", year, month, day);
	else _snwprintf(result, MAX_DATE_RESULT_LENGTH, L"%d%d%d", year, month, day);
}

void QueryDate::createStringDate(wchar_t *result, int year, int month, int day) {

	std::wstring monthStr;
	if (month == 1) {
		monthStr = L"january";
	} else if (month == 2) {
		monthStr = L"february";
	} else if (month == 3) {
		monthStr = L"march";
	} else if (month == 4) {
		monthStr = L"april";
	} else if (month == 5) {
		monthStr = L"may";
	} else if (month == 6) {
		monthStr = L"june";
	} else if (month == 7) {
		monthStr = L"july";
	} else if (month == 8) {
		monthStr = L"august";
	} else if (month == 9) {
		monthStr = L"september";
	} else if (month == 10) {
		monthStr = L"october";
	} else if (month == 11) {
		monthStr = L"november";
	} else if (month == 12) {
		monthStr = L"december";
	} 

	if (day < 10) {
		_snwprintf(result, MAX_DATE_RESULT_LENGTH, L"0%d %s %d", day, monthStr.c_str(), year);
	} else {
		_snwprintf(result, MAX_DATE_RESULT_LENGTH, L"%d %s %d", day, monthStr.c_str(), year);
	}
}

int QueryDate::getClosestLegalDay(int year, int month, int day) {
	if (day < 0) {
		return 0;
	} else if (month == 2 && day >= 29) {
		if (year % 4 == 0)
			return 29;
		else return 28;
	} else if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30) {
		return 30;
	} else if (day > 31) {
		return 31;
	} 	
	return day;
}
int QueryDate::getLastDayOfMonth(int year, int month) {
	return getClosestLegalDay(year, month, 40);
}

QueryDate::DateStatus QueryDate::getDateStatus(const Value *value) const {
	
	Symbol valSymbol = value->getTimexVal();
	if (valSymbol.is_null())
		return NOT_SPECIFIC;
	std::wstring valString = valSymbol.to_string();

	// run a regex over the date
	boost::wcmatch matchResult;
	int year; 
	int month;
	int day;
	int week;
	std::wstring wstr = L"";
	if (boost::regex_match(valString.c_str(), matchResult, _timex_regex_ymd)) {		
		wstr = matchResult[1];
		year = _wtoi(wstr.c_str());	
		wstr = matchResult[2];
		month = _wtoi(wstr.c_str());	
		wstr = matchResult[3];
		day = _wtoi(wstr.c_str());

		if (year < _start_year || year > _end_year)
			return OUT_OF_RANGE;
		if (year == _start_year && month < _start_month)
			return OUT_OF_RANGE;
		if (year == _start_year && month == _start_month && day < _start_day)
			return OUT_OF_RANGE;
		if (year == _end_year && month > _end_month)
			return OUT_OF_RANGE;
		if (year == _end_year && month == _end_month && day > _end_day)
			return OUT_OF_RANGE;

		return IN_RANGE;

	} else if (boost::regex_match(valString.c_str(), matchResult, _timex_regex_ym)) {
		wstr = matchResult[1];
		year = _wtoi(wstr.c_str());	
		wstr = matchResult[2];
		month = _wtoi(wstr.c_str());	

		if (year < _start_year || year > _end_year)
			return OUT_OF_RANGE;
		if (year == _start_year && month < _start_month)
			return OUT_OF_RANGE;
		if (year == _end_year && month > _end_month)
			return OUT_OF_RANGE;

		// April 2003 will match range { 4/16/03 - 4/17/03 }
		return IN_RANGE;

	}  else if (boost::regex_match(valString.c_str(), matchResult, _timex_regex_yw)) {
		wstr = matchResult[1];
		year = _wtoi(wstr.c_str());	
		wstr = matchResult[2];
		week = _wtoi(wstr.c_str());	

		if (year < _start_WF_year || year > _end_WF_year)
			return OUT_OF_RANGE;
		if (year == _start_WF_year && week < _start_WF_week)
			return OUT_OF_RANGE;
		if (year == _end_WF_year && week > _end_WF_week)
			return OUT_OF_RANGE;

		return IN_RANGE;

	} else if (boost::regex_match(valString.c_str(), matchResult, _timex_regex_y)) {
		wstr = matchResult[1];
		year = _wtoi(wstr.c_str());	
					
		if (year < _start_year || year > _end_year)
			return OUT_OF_RANGE;
		
		// this whole year is included in our range
		if ((year > _start_year || (year == _start_year && _start_day == 1 && _start_month == 1)) &&
			(year < _end_year || (year == _end_year && _end_day == 12 && _end_month == 31)))
			return IN_RANGE;

		return TOO_BROAD;

	} else {
		// we got nothin'
		return NOT_SPECIFIC;
	}
}

bool QueryDate::isSpecificDate(const Value *value) {
	Symbol valSymbol = value->getTimexVal();
	if (valSymbol.is_null())
		return false;
	std::wstring valString = valSymbol.to_string();

	boost::wcmatch matchResult;

	// fix 04/17/07: YYYYTNI is not really a specific date, it's something like "overnight" or "at night"
	return (boost::regex_match(valString.c_str(), matchResult, _timex_regex_y_exact) ||
		boost::regex_match(valString.c_str(), matchResult, _timex_regex_y_hyphen));
}

std::wstring QueryDate::getYMDDate(const Value *value) {
	Symbol valSymbol = value->getTimexVal();

	if (valSymbol.is_null())
		return L"";
	std::wstring valString = valSymbol.to_string();

	boost::wcmatch matchResult;
	if (boost::regex_match(valString.c_str(), matchResult, _timex_regex_ymd)) {
		return matchResult[1] + L"-" + matchResult[2] + L"-" + matchResult[3];
	} 

	return L"";
}

std::wstring QueryDate::getYDate(const Value *value) {
	Symbol valSymbol = value->getTimexVal();

	if (valSymbol.is_null())
		return L"";
	std::wstring valString = valSymbol.to_string();

	boost::wcmatch matchResult;
	if (boost::regex_match(valString.c_str(), matchResult, _timex_regex_y)) {
		return matchResult[1];
	} 

	return L"";
}

bool QueryDate::isSourceDateRestriction() const {
	return (wcscmp(_date_type, L"Source") == 0);
}
bool QueryDate::isIngestDateRestriction() const {
	return (wcscmp(_date_type, L"Ingest") == 0);
}
bool QueryDate::isActivityDateRestriction() const {
	return (wcscmp(_date_type, L"Activity") == 0);
}

bool QueryDate::isActivityDateRestrictionInCorpusRange() const {
	initCorpusRange();
	if (!isActivityDateRestriction())
		return false;
	if (isInCorpusRange(getNormalizedDateStart()) || isInCorpusRange(getNormalizedDateEnd()))
		return true;
	// spans corpus 1
	if (wcscmp(getNormalizedDateStart(), corpus1_start) <= 0 &&
		wcscmp(corpus1_end, getNormalizedDateEnd()) <= 0)
		return true;
	// spans corpus 2
	if (wcscmp(getNormalizedDateStart(), corpus2_start) <= 0 &&
		wcscmp(corpus2_end, getNormalizedDateEnd()) <= 0)
		return true;
	// spans corpus 3
	if (wcscmp(getNormalizedDateStart(), corpus3_start) <= 0 &&
		wcscmp(corpus3_end, getNormalizedDateEnd()) <= 0)
		return true;
	return false;
}

const wchar_t* QueryDate::getLastDateInCorpus() {
    initCorpusRange();
    if (wcscmp(corpus2_end, corpus1_end) <= 0 && wcscmp(corpus3_end, corpus1_end) <= 0) {
		std::wcout << "Returning corpus1_end " << corpus1_end << "\n";
        return corpus1_end;
    } else if (wcscmp(corpus1_end, corpus2_end) <= 0 && wcscmp(corpus3_end, corpus2_end) <= 0) {
		std::wcout << "Returning corpus2_end " << corpus2_end << "\n";
        return corpus2_end;
    } else if (wcscmp(corpus1_end, corpus3_end) <= 0 && wcscmp(corpus2_end, corpus3_end) <= 0) {
		std::wcout << "Returning corpus3_end " << corpus3_end << "\n";
        return corpus3_end;
    } else {
        throw UnrecoverableException("QueryDate::getLastDateInCorpus()", "Couldn't get last date in corpus");
    }
}

bool QueryDate::isInCorpusRange(const wchar_t *date) {
	return ((wcscmp(corpus1_start, date) <= 0 && wcscmp(date, corpus1_end) <= 0) ||
			(wcscmp(corpus2_start, date) <= 0 && wcscmp(date, corpus2_end) <= 0) ||
			(wcscmp(corpus3_start, date) <= 0 && wcscmp(date, corpus3_end) <= 0));
}

void QueryDate::initCorpusRange() {
	static bool init = false;
	if (!init) {
		init = true;
		char buffer[20];
		ParamReader::getRequiredParam("distillation_corpus_start_date_1", buffer, 20);
		if (strlen(buffer) != 8)
			throw UnexpectedInputException("QueryDate::initCorpusRange()", "Date must be 8 digits long");
		size_t i = 0;
		for (i = 0; i < 8; i++)
			corpus1_start[i] = (wchar_t) buffer[i];
		corpus1_start[i] = L'\0';
		ParamReader::getRequiredParam("distillation_corpus_end_date_1", buffer, 20);
		if (strlen(buffer) != 8)
			throw UnexpectedInputException("QueryDate::initCorpusRange()", "Date must be 8 digits long");
		for (i = 0; i < 8; i++)
			corpus1_end[i] = (wchar_t) buffer[i];
		corpus1_end[i] = L'\0';

		// If no corpus 2, use corpus 1
		if (!ParamReader::getParam("distillation_corpus_start_date_2", buffer, 20))
			ParamReader::getRequiredParam("distillation_corpus_start_date_1", buffer, 20);
		if (strlen(buffer) != 8)
			throw UnexpectedInputException("QueryDate::initCorpusRange()", "Date must be 8 digits long");
		for (i = 0; i < 8; i++)
			corpus2_start[i] = (wchar_t) buffer[i];
		corpus2_start[i] = L'\0';
		if (!ParamReader::getParam("distillation_corpus_end_date_2", buffer, 20))
			ParamReader::getRequiredParam("distillation_corpus_end_date_1", buffer, 20);
		if (strlen(buffer) != 8)
			throw UnexpectedInputException("QueryDate::initCorpusRange()", "Date must be 8 digits long");
		for (i = 0; i < 8; i++)
			corpus2_end[i] = (wchar_t) buffer[i];
		corpus2_end[i] = L'\0';
		
		// If no corpus 3, use corpus 1
		if (!ParamReader::getParam("distillation_corpus_start_date_3", buffer, 20))
			ParamReader::getRequiredParam("distillation_corpus_start_date_1", buffer, 20);
		if (strlen(buffer) != 8)
			throw UnexpectedInputException("QueryDate::initCorpusRange()", "Date must be 8 digits long");
		for (i = 0; i < 8; i++)
			corpus3_start[i] = (wchar_t) buffer[i];
		corpus3_start[i] = L'\0';
		if (!ParamReader::getParam("distillation_corpus_end_date_3", buffer, 20))
			ParamReader::getRequiredParam("distillation_corpus_end_date_1", buffer, 20);
		if (strlen(buffer) != 8)
			throw UnexpectedInputException("QueryDate::initCorpusRange()", "Date must be 8 digits long");
		for (i = 0; i < 8; i++)
			corpus3_end[i] = (wchar_t) buffer[i];
		corpus3_end[i] = L'\0';
	}
}

void QueryDate::getWeekForDate(int year, int month, int day, int &WF_year, int& WF_week) {
	bool leapyear = ((year % 4 == 0  &&  year % 100 != 0) || year % 400 == 0);
	bool leapyearminusone = (((year-1) % 4 == 0  &&  (year-1) % 100 != 0) || (year-1) % 400 == 0);

	int day_of_year = 0;
	if (month == 1) day_of_year = 0;
	else if (month == 2) day_of_year = 31;
	else if (month == 3) day_of_year = 59;
	else if (month == 4) day_of_year = 90;
	else if (month == 5) day_of_year = 120;
	else if (month == 6) day_of_year = 151;
	else if (month == 7) day_of_year = 181;
	else if (month == 8) day_of_year = 212;
	else if (month == 9) day_of_year = 243;
	else if (month == 10) day_of_year = 273;
	else if (month == 11) day_of_year = 304;
	else if (month == 12) day_of_year = 334;
	day_of_year += day;
	if (leapyear && month > 2)
		day_of_year++;

	int yy = (year-1) % 100;
	int c = (year-1) - yy;
	int g = yy + yy/4;
	int jan1weekday = 1 + (((((c / 100) % 4) * 5) + g) % 7);

	int h = day_of_year + (jan1weekday - 1);
	int weekday = 1 + ((h - 1) % 7);

	WF_year = year;
	
	if (day_of_year <= (8 - jan1weekday) && jan1weekday > 4) {
		WF_year--;
		if (jan1weekday == 5 || (jan1weekday == 6 && leapyearminusone))
			WF_week = 53;
		else WF_week = 52;
	}
	if (WF_year == year) {
		int numdays = 365;
		if (leapyear)
			numdays++;
		if ((numdays - day_of_year) < (4 - weekday)) {
			WF_year = year + 1;
			WF_week = 1;
		}
	}
	if (WF_year == year) {
		int j = day_of_year + (7 - weekday) + (jan1weekday - 1);
		WF_week = j / 7;
		if (jan1weekday > 4)
              WF_week--;
	}
}
