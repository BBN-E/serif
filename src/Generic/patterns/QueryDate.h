// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef QUERYDATE_H
#define QUERYDATE_H

#include <iostream>
#include <boost/regex.hpp>
class Value;

class QueryDate
{
private:
	wchar_t _date_type[100];
	wchar_t _date_start[100];
	wchar_t _date_end[100];
	int _start_year;
	int _start_month;
	int _start_day;
	int _start_WF_week;
	int _start_WF_year;
	int _end_year;
	int _end_month;
	int _end_day;
	int _end_WF_week;
	int _end_WF_year;
	wchar_t _normalized_date_start[9];
	wchar_t _normalized_date_end[9];
	
	static const boost::wregex _date_regex_0;
	static const boost::wregex _date_regex_1;
	static const boost::wregex _date_regex_2;
	static const boost::wregex _date_regex_3;
	static const boost::wregex _date_regex_4a;
	static const boost::wregex _date_regex_4b;
	static const boost::wregex _date_regex_4c;
	static const boost::wregex _date_regex_4d;
	static const boost::wregex _date_regex_5a;
	static const boost::wregex _date_regex_5b;
	static const boost::wregex _date_regex_5c;
	static const boost::wregex _date_regex_5d;
	static const boost::wregex _date_regex_6;
	static const boost::wregex _date_regex_7;

	static const boost::wregex _timex_regex_ymd;
	static const boost::wregex _timex_regex_ym;
	static const boost::wregex _timex_regex_yw;
	static const boost::wregex _timex_regex_y;
	static const boost::wregex _timex_regex_y_exact;
	static const boost::wregex _timex_regex_y_hyphen;

	static void getWeekForDate(int year, int month, int day, int &WF_year, int& WF_week);
	static void createNormalizedDate(wchar_t *result, int year, int month, int day);
	static void createStringDate(wchar_t *result, int year, int month, int day);
	static int getClosestLegalDay(int year, int month, int day);
	static int getLastDayOfMonth(int year, int month);

	static bool isInCorpusRange(const wchar_t *date);
	static wchar_t corpus1_start[9];
	static wchar_t corpus1_end[9];
	static wchar_t corpus2_start[9];
	static wchar_t corpus2_end[9];
	static wchar_t corpus3_start[9];
	static wchar_t corpus3_end[9];
	static void initCorpusRange();

public:
	QueryDate(const wchar_t * date_type, const wchar_t * date_start, const wchar_t * date_end);
	~QueryDate();

	const wchar_t * getDateStart() const {return _date_start;}
	const wchar_t * getDateEnd() const {return _date_end;}
	const wchar_t * getNormalizedDateStart() const {return _normalized_date_start;}
	const wchar_t * getNormalizedDateEnd() const {return _normalized_date_end;}
	void getNormalizedDateEndPlusMonths(wchar_t *result, int n) const;
	void getNormalizedDateEndPlusDays(wchar_t *result, int n) const;
	void getStringDateEndPlusMonths(wchar_t *result, int n) const;
	int getClosestLegalDate(int month, int day);

	bool isSourceDateRestriction() const;
	bool isIngestDateRestriction() const;
	bool isActivityDateRestriction() const;
	bool isActivityDateRestrictionInCorpusRange() const;

	typedef enum { IN_RANGE, OUT_OF_RANGE, NOT_SPECIFIC, TOO_BROAD } DateStatus;
	DateStatus getDateStatus(const Value *value) const;
    static const wchar_t* getLastDateInCorpus();
	static bool isSpecificDate(const Value *value);
	static std::wstring getYMDDate(const Value *value);
	static std::wstring getYDate(const Value *value);

	static bool normalizeDate(const wchar_t *date_string, int start_or_end, wchar_t *normalized_date_string, int& year, int& month, int& day);
	enum {START, END, START_MINUS_ONE, END_PLUS_ONE};
	static const std::wstring EARLIEST_LEGAL_DATE;
	static const std::wstring LATEST_LEGAL_DATE;


};
#endif
