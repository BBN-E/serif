#ifndef _TIMEX_UTILS_H_
#define _TIMEX_UTILS_H_

#include <string>
#include <vector>
#include "common/Symbol.h"
#include <boost/optional.hpp>
#pragma warning(push, 0)
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/regex.hpp>
#pragma warning(pop)
#include <boost/date_time/gregorian/gregorian.hpp>

class DocTheory;

class TimexUtils {
public:
	static bool hasYear(const std::wstring& s);

	static std::wstring toYearOnly(const std::wstring& s);

	static bool hasYearMonthDay(const std::wstring& s);

	static std::wstring toYearMonthDayOnly(const std::wstring& s);

	static std::vector<std::wstring> parseYYYYMMDD(const std::wstring& s, 
			bool strip_suffixes = true);
	static void parseYYYYMMDD(const std::wstring& s, 
			std::vector<std::wstring>& output, bool strip_suffixes = true);
	static bool parseYYYYWW(const std::wstring& s, int& year, int& week);

	static bool isParticularDay(const std::wstring& s);
	static bool isParticularMonth(const std::wstring& s);
	static bool isParticularYear(const std::wstring& s);

	static boost::gregorian::date_period ISOWeek(int year, int week);
	// ISO weeks run Monday->Sunday
	static boost::gregorian::date_period ISOContainingWeekFromDate(
			boost::gregorian::date d);

	static boost::gregorian::date_period datePeriodFromTimex(const std::wstring& timex);
	static boost::gregorian::date dateFromTimex(const std::wstring& timex, 
			bool strip_suffix = true);

	static std::wstring dateToTimex(const boost::gregorian::date& d);
	static std::vector<std::wstring> parseNonWeekDatesToYYYYMMDD(const std::wstring& s);
	
	static std::wstring extractDateFromDocId(Symbol docid);

	static bool startsWith8DigitDate(const std::wstring& s, std::wstring& output);

	static boost::optional<boost::posix_time::ptime> ptimeFromString(
		std::string &timeString, std::string format);

	static void parseDateTime(std::wstring timeString,
					          boost::optional<boost::gregorian::date> &date, 
			                  boost::optional<boost::posix_time::time_duration> &timeOfDay, 
                              boost::optional<boost::local_time::posix_time_zone> &timeZone);

private:
	static const boost::wregex _timex_regex_y;
	static const boost::wregex _timex_regex_ymd;

	static const boost::wregex _timex_regex_year_particular;
	static const boost::wregex _timex_regex_month_particular;
	static const boost::wregex _timex_regex_day_particular;

	static const boost::wregex _timex_regex_time_zone;

	static const boost::wregex docid_date_re;

	
};

#endif
