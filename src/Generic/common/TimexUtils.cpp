#include "common/leak_detection.h"

#include "TimexUtils.h"
#include <string>
#include <vector>
#include <locale>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "boost/date_time/gregorian/gregorian.hpp"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UnicodeUtil.h"

using namespace boost;

const wregex TimexUtils::_timex_regex_y(L"^([012][0-9][0-9][0-9]).*");
const wregex TimexUtils::_timex_regex_ymd(L"^([012][0-9][0-9][0-9]-[01][0-9]-[0-3][0-9]).*");

const wregex TimexUtils::_timex_regex_year_particular(L"^[012][0-9][0-9][0-9]");
const wregex TimexUtils::_timex_regex_month_particular(L"^[012][0-9][0-9][0-9]-[01][0-9]");
const wregex TimexUtils::_timex_regex_day_particular(L"^[012][0-9][0-9][0-9]-[01][0-9]-[0-3][0-9]");

const wregex TimexUtils::_timex_regex_time_zone(L"([\\-\\+][012][0-9]:[0-5][0-9])$");

/**
 * This regular expression parses GALE-style document IDs with
 * hyphen-separated prefixes, as well as unprefixed LDC document IDs
 * used in AQUAINT, Machine Reading, etc., and the genre subdirectory
 * document IDs used in the LearnIt cache.
 *
 * The primary purpose is to find an 8-digit date of the form
 * YYYYMMDD, but other document ID metadata fields are exposed
 * as well.
 *
 * GALE examples:
 *		en-nw-en-src-NYT20090612.1534
 *		ch-nw-en-hierdec-XIN20090612.1534
 *		ar-wb-ar-src-AUT20090612-weird-file-name
 * LearnIt examples:
 *      gigaword/XIN20061231.0236
 * Other examples:
 *      AFP_ENG_19950107.0244
 *      VOA19980105.2300.2823
 *
 * @author nward@bbn.com
 * @date 2010.05.10
 **/
const boost::wregex TimexUtils::docid_date_re(
	// Optional: LearnIt cache prefixes
	L"((?<learnit_genre>[a-z]+)/)?"

	// Optional: GALE hyphenated prefixes
	L"("
	  L"(?<doc_lang>[a-z]{2})-"
	  L"(?<doc_genre>[a-z]{2})-"
	  L"(?<seg_lang>[a-z]{2})-"
	  L"(?<seg_field>[a-z0-9_]{2,})-"
	L")?"

	// Required: LDC document source code
	//   At least 3 letters, maybe more with
	//   optional separator, case-insensitive,
	//   non-numeric
	L"(?<doc_source_code>[a-zA-Z]{3,})[-_]?"

	// Optional: LDC document language code
	L"(_(?<doc_lang_code>[a-zA-Z]{3,})_)?"

	// Required: 8-digit date in YYYYMMDD
	L"(?<doc_date>\\d{8})"

	// Remainder of document ID, no restrictions
	//   Typically unique ID within day, with blocks of
	//   digits or letters separated by hyphens or periods.
	L"(?<docid_remainder>.*)"
);


bool TimexUtils::hasYear(const std::wstring& s) {
	return regex_match(s, _timex_regex_y);
}

std::wstring TimexUtils::toYearOnly(const std::wstring& s) {
	wsmatch m;
	if (regex_match(s, m, _timex_regex_y)) {
		return m[1];
	}
	return L"";
}

bool TimexUtils::hasYearMonthDay(const std::wstring& s) {
	return regex_match(s, _timex_regex_ymd);
}

std::wstring TimexUtils::toYearMonthDayOnly(const std::wstring& s) {
	wsmatch m;
	if (regex_match(s, m, _timex_regex_ymd)) {
		return m[1];
	}
	/* This could be used to resolve other types of timex strings.  
	int year= 0;
	int week = 0;
	if(parseYYYYWW(s, year, week)){
		std::wcout<<"Warning: toYearMonthDayOnly() on TimexWeek: "<<s<<std::endl;
	}
	std::vector<std::wstring> ymd =  parseYYYYMMDD(s, true);
	if(ymd.size() > 0){
		std::wstringstream str;
		if(ymd.size() == 1)
			str<<ymd[0];
		if(ymd.size() ==2)
			str<<ymd[0]<<"-"<<ymd[1];
		if(ymd.size()==3)
			str<<ymd[0]<<"-"<<ymd[1]<<"-"<<ymd[2];
		return str.str();
	}
	*/
	return L"";
}

// Takes a string and sees if it is a date string of the correct form for
// LearnIt. If not, it returns an empty vector. Otherwise, it returns 
// a three part vector with the year, month, and day, respectively.
std::vector<std::wstring> TimexUtils::parseYYYYMMDD(const std::wstring& s,
		bool strip_suffixes) {
	std::vector<std::wstring> ret;
	parseYYYYMMDD(s, ret, strip_suffixes);
	return ret;
}

void TimexUtils::parseYYYYMMDD(const std::wstring& s,
				std::vector<std::wstring>& output, bool strip_suffixes) 
{
	// This regex needs to be kept in sync with generate_search_queries.py's
	// seed searching ~ RMG
	static const boost::wregex date_re(L"\\A(xxxx|\\d\\d|\\d\\d\\d\\d)(?:-(xx|\\d\\d)(?:-(xx|\\d\\d))?)?\\z");
	static const boost::wregex timex_regex_m_d_extra(L"^([12][0-9][0-9][0-9])-([012]?[0-9])-([0123][0-9]).+$");
	static const boost::wregex year_week(L"\\A(\\d\\d\\d\\d)-W(\\d\\d)\\z");
	static const boost::wregex year_month_with_modifier(L"^([12][0-9][0-9][0-9])-([012]?[0-9])[A-Z]+$");
	static const boost::wregex year_with_modifier(L"^([12][0-9][0-9][0-9])-[A-Z]+$");
	
	output.clear();
	boost::wsmatch matchObj;
	if (boost::regex_match(s, matchObj, date_re)) {
		for (int i=1; i<=3; ++i) {
			output.push_back(matchObj[i]);
		}
	} else if (strip_suffixes) {
		//-W has to be treated differently
		if (boost::regex_match(s, matchObj, year_week)) {
			return;
		}
		if (boost::regex_match(s, matchObj, timex_regex_m_d_extra)) {
			for (int i=1; i<=3; ++i) {
				output.push_back(matchObj[i]);
			}
		} else if (boost::regex_match(s, matchObj, year_month_with_modifier)) {
			for (int i=1; i<=2; ++i) {
				output.push_back(matchObj[i]);
			}
		} else if (boost::regex_match(s, matchObj, year_with_modifier)) {
			output.push_back(matchObj[1]);
		} 
	}
}

bool TimexUtils::parseYYYYWW(const std::wstring& s, int& year, int& week)
{
	static const boost::wregex date_re(L"\\A(\\d\\d\\d\\d)-W(\\d\\d)\\z");
	boost::wsmatch matchObj;

	if (boost::regex_match(s, matchObj, date_re)) {
		try {
			std::wstring year_string = matchObj[1];
			std::wstring week_string = matchObj[2];

			year = boost::lexical_cast<int>(year_string);
			week = boost::lexical_cast<int>(week_string);
		} catch (boost::bad_lexical_cast&) { 
			return false;
		}
		return true;
	} 

	return false;
}

bool TimexUtils::isParticularDay(const std::wstring& s) {
	return boost::regex_match(s, _timex_regex_day_particular);
}

bool TimexUtils::isParticularMonth(const std::wstring& s) {
	return boost::regex_match(s, _timex_regex_month_particular);
}

bool TimexUtils::isParticularYear(const std::wstring& s) {
	return boost::regex_match(s, _timex_regex_year_particular);
}

boost::gregorian::date_period TimexUtils::ISOWeek(int year, int week) 
{
	using namespace boost::gregorian;
	// January 4th is always in ISO week 1
	date january_4th((unsigned short) year, Jan, 4);
	week_iterator it(january_4th);
	// go through the desired number of weeks
	for (int i=1; i<week; ++i, ++it) {}
	return ISOContainingWeekFromDate(*it);
}

// ISO weeks always run Monday->Sunday
boost::gregorian::date_period TimexUtils::ISOContainingWeekFromDate(
		boost::gregorian::date d) 
{
	using namespace boost::gregorian;
	date beginning, end;
	if (d.day_of_week() == Monday) {
		beginning = d;
	} else {
		first_day_of_the_week_before first_Monday_before(Monday);
		beginning = first_Monday_before.get_date(d);
	}

	if (d.day_of_week() == Sunday) {
		end = d;
	} else {
		first_day_of_the_week_after first_Sunday_after(Sunday);
		end = first_Sunday_after.get_date(d);
	}

	return date_period(beginning, end);
}

boost::gregorian::date_period TimexUtils::datePeriodFromTimex(
		const std::wstring& timex)
{
	using namespace boost::gregorian;
	static const date_period NULL_DATE(date(2000,Jan,2),date(2000,Jan,1));

	std::vector<std::wstring> parts;
	parseYYYYMMDD(timex, parts);

	if (!parts.empty()) {
		try {
			int year = boost::lexical_cast<int>(parts[0]);
			if (parts.size() > 1 && parts[1] != L"") {
				int month = boost::lexical_cast<int>(parts[1]);
				if (parts.size() > 2 && parts[2] != L"") {
					int day = boost::lexical_cast<int>(parts[2]);
					date d = date((unsigned short)year,(unsigned short)month,(unsigned short)day);
					return date_period(d,d);
				}
				int last_day = gregorian_calendar::end_of_month_day((unsigned short)year,(unsigned short)month);
				return date_period(date((unsigned short)year,(unsigned short)month,1),date((unsigned short)year,(unsigned short)month,(unsigned short)last_day));
			}
			return date_period(date((unsigned short)year,Jan,1), date((unsigned short)year,Dec,31));
		} catch (boost::bad_lexical_cast&) {
			SessionLogger::warn("date_from_timex")
				<< "TimexUtils::datePeriodFromTimex caught bad "
				<< "lexical cast";
		} catch (std::out_of_range&) {
			SessionLogger::warn("date_from_timex")
				<< "TimexUtils::datePeriodFromTimex caught std::out_of_range";
		}
	}

	int year = -1, week = -1;
	if (parseYYYYWW(timex, year, week)) {
		return ISOWeek(year, week);
	}
	return NULL_DATE;
}

boost::gregorian::date TimexUtils::dateFromTimex(
		const std::wstring& timex, bool strip_suffix)
{
	using namespace boost::gregorian;
	
	std::vector<std::wstring> parts;
	parseYYYYMMDD(timex, parts, strip_suffix);

	if (parts.size() == 3) {
		if (parts[0] != L"" && parts[1] != L"" && parts[2] != L"") {
			try {
				int year = boost::lexical_cast<int>(parts[0]);
				int month = boost::lexical_cast<int>(parts[1]);
				int day = boost::lexical_cast<int>(parts[2]);

				return date((unsigned short)year,(unsigned short)month,(unsigned short)day);
			} catch (boost::bad_lexical_cast&) {
				SessionLogger::warn("date_from_timex")
					<< "TimexUtils::dateFromTimex caught bad lexical cast";
				SessionLogger::warn("date_from_timex") 
					<< L"\t[" << parts[0] << L"]\t[" << parts[1] << L"]\t[" << parts[2] << L"]";
			} catch (std::out_of_range&) {
				SessionLogger::warn("date_from_timex")
					<< "TimexUtils::dateFromTimex caught std::out_of_range";
				SessionLogger::warn("date_from_timex") 
					<< L"\t" << parts[0] << L"\t" << parts[1] << L"\t" << parts[2];
			}
		}
	}

	return date(not_a_date_time);
}

std::wstring TimexUtils::dateToTimex(const boost::gregorian::date& d) {
	using namespace boost::gregorian;
	return UnicodeUtil::toUTF16StdString(to_iso_extended_string(d));
}


/**
 * Now implemented as a regular expression EnglishTemporalNormalizer::docid_date_re.
 * Returns the same session log error if no match (previously happened
 * for any document not matching GALE docid format).
 *
 * @author nward@bbn.com
 * @date 2010.05.10
 * @param docid Symbol containing document ID, probably obtained from Document::getName()
 * @return std::wstring containing an 8-digit date of the form YYYYMMDD
 **/
std::wstring TimexUtils::extractDateFromDocId(Symbol docid) {
	// Try to match using the regex
	boost::wcmatch match;
	if (boost::regex_match(docid.to_string(), match, docid_date_re)) {
		// If a LearnIt cache prefix was present, only use document dates from gigaword
		if (match.str(L"learnit_genre") == L"" || match.str(L"learnit_genre") == L"gigaword") {
			// Extract just the 8-digit date
			return match.str(L"doc_date");
		}
	}

	// No match
	return std::wstring(L"XXXX-XX-XX");
}

bool TimexUtils::startsWith8DigitDate(const std::wstring& s, std::wstring& output)
{
	static const boost::wregex date_regex(L"^\\s*([12][0-9][0-9][0-9])([01]?[0-9])([0123][0-9])");
	
	boost::wsmatch matchObj;
	if (boost::regex_search(s, matchObj, date_regex)) {
		output = matchObj[0];
		return true;
	}

	return false;
}

boost::optional<boost::posix_time::ptime> TimexUtils::ptimeFromString(
	std::string &timeString, std::string format) 
{
	std::stringstream ss;
	ss << timeString;
	ss.imbue(std::locale(std::locale::classic(),       
		new boost::local_time::local_time_input_facet(format)));

	try {
		boost::posix_time::ptime ptime;
		ss >> ptime;
		ss.exceptions(std::ios::failbit);
		return ptime;
	} catch (...) {
		// wrong format
	}
	return boost::none;
}

/** 
 * Parse dates from the ACE regression test data and 
 * non-delmited or extended ISO strings.
 **/
void TimexUtils::parseDateTime(std::wstring timeString,
							   boost::optional<boost::gregorian::date> &date, 
					           boost::optional<boost::posix_time::time_duration> &timeOfDay, 
                               boost::optional<boost::local_time::posix_time_zone> &timeZone) 
{
	boost::algorithm::trim(timeString);

	// Strip off time zone indicator if it exists
	boost::wsmatch matchObj;
	if (boost::regex_search(timeString, matchObj, _timex_regex_time_zone)) {
		std::wstring timeZoneString = matchObj[1];
		timeZone = boost::local_time::posix_time_zone(UnicodeUtil::toUTF8StdString(timeZoneString));
		timeString = boost::regex_replace(timeString, _timex_regex_time_zone, L"");
	}

	std::string ts = UnicodeUtil::toUTF8StdString(timeString);

	boost::optional<boost::posix_time::ptime> ptime;

	/* The simple formats go first because we want to catch the 
	   case where we don't have a time of day. If we tried a format 
	   with H, M, S in it, that could also match the 8 digit case, 
	   and we wouldn't know that we couldn't find a time. 
	*/

	// Just date 20050123
	ptime = ptimeFromString(ts, "%Y%m%d");
	if (ptime && ts.size() == 8) { 
		date = ptime->date();
		return;
	}

	// Just date 2005-01-23
	ptime = ptimeFromString(ts, "%Y-%m-%d");
	if (ptime && ts.size() == 10) { 
		date = ptime->date();
		return;
	}

	// Full date and time: 2005-01-23T16:39:00
	ptime = ptimeFromString(ts, "%Y-%m-%dT%H:%M:%S");
	if (ptime) {
		date = ptime->date();
		timeOfDay = ptime->time_of_day();
		return;
	}

	// Full date and time: 2005-01-23 16:39:00
	ptime = ptimeFromString(ts, "%Y-%m-%d %H:%M:%S");
	if (ptime) {
		date = ptime->date();
		timeOfDay = ptime->time_of_day();
		return;
	}

	// Full date and time: 20050123-16:39:00
	ptime = ptimeFromString(ts, "%Y%m%d-%H:%M:%S");
	if (ptime) {
		date = ptime->date();
		timeOfDay = ptime->time_of_day();
		return;
	}

	// Non-delimited ISO string: 20050123T163900
	ptime = ptimeFromString(ts, "%Y%m%dT%H%M%S");
	if (ptime) {
		date = ptime->date();
		timeOfDay = ptime->time_of_day();
		return;
	}

	return;
}
