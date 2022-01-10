// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include "Generic/linuxPort/serif_port.h"
#include <boost/lexical_cast.hpp>

#ifndef __TP_MAIN
#  include "English/timex/en_TemporalParser.h"
#else
#  include "en_TemporalParser.h"
#endif


using namespace std;

#ifndef BOOST

// provide dummy functions
bool normalize_temporal (temporal_timex2& timex2, 
						 const wchar_t *expr)
{
	timex2.clear();
	wcerr << L"error: ** normalize_temporal() is called without a valid implementation!"
		<< endl;
	return false;
}

bool normalize_temporal (temporal_timex2& timex2, 
						 const std::wstring& expr)
{
	return normalize_temporal (timex2, expr.c_str());
}

#else

//#define BOOST_ALL_DYN_LINK
//#define BOOST_LIB_DIAGNOSTIC

#include <boost/regex.hpp>

using namespace boost;

#ifdef _WIN32
# define strncasecmp   _strnicmp
# define strcasecmp    _stricmp
# define wcsncasecmp   _wcsnicmp
# define wcscasecmp    _wcsicmp
#endif

#define TP_BUFSIZE   64

#define TP_SEP L"[\\s,-]+"

#define TP_MON L"(Jan(\\.|uary)?|" \
	L"Feb(\\.|ruary)?|" \
	L"Mar(\\.|ch)?|" \
	L"Apr(\\.|il)?|" \
	L"May|" \
	L"Jun(\\.|e)?|" \
	L"Jul(\\.|y)?|" \
	L"Aug(\\.|ust)?|" \
	L"Sep(\\.|tember)?|" \
	L"Oct(\\.|ober)?|" \
	L"Nov(\\.|ember)?|" \
	L"Dec(\\.|ember)?)"

#define TP_WNUM L"(eleven(th)?|" \
	L"twelve(th)?|" \
	L"thirteen(th)?|" \
	L"fourteen(th)?|" \
	L"fifteen(th)?|" \
	L"sixteen(th)?|" \
	L"seventeen(th)?|" \
	L"eighteen(th)?|" \
	L"nineteen(th)?|" \
	L"twenty|twentieth|" \
	L"one|first|" \
	L"two|second|" \
	L"three|thir(d|ty)|" \
	L"four(th|ty)?|" \
	L"five|fift(h|y)|" \
	L"six(th|ty)?|" \
	L"seven(th|ty)?|" \
	L"eight(h|y)?|" \
	L"nine(ty)?|ninth|" \
	L"ten(th)?|" \
	L"half|" \
	L"twice|couple|" \
	L"fews?|" \
	L"millions?|" \
	L"thousands?|" \
	L"hundreds?)"

#define TP_FNUM L"(((\\d+)" TP_SEP L"(and" TP_SEP L")" TP_WNUM L"(" TP_SEP TP_WNUM L")?)|" \
	L"(" TP_WNUM TP_SEP L"(and" TP_SEP L")" TP_WNUM L"(" TP_SEP TP_WNUM L")?))"

#define TP_DAY_OF_WEEK L"(Mon(\\.|days|day)|" \
	L"Tue(\\.|sdays|sday)|" \
	L"Wed(\\.|nesdays|nesday)|" \
	L"Thu(\\.|rsdays|rsday)|" \
	L"Fri(\\.|days|day)|" \
	L"Sat(\\.|urdays|urday)|" \
	L"Sun(\\.|days|day))"

#define TP_TIME_PERIOD L"(centur(y|ies)|millenni(um|a)|decades?|" \
	L"month(s|ly)?|weekends?|week(s|ly)?|days?|daily|year(s|ly)?|" \
	L"hour(s|ly)?|minutes?|seconds?|annual|semiannual|mnth|min|sec|yr|hr|wk)"

#define TP_SEASON L"(winters?|falls?|springs?|autumns?|summers?)"

#define TP_NON_SPECIFIC_TIME L"(tomorrow|yesterday|today|" \
	L"morning|afternoon|evening|midnight|midday|tonight|night|noon|now)"

#define TP_DAY L"(\\d{1,2})"
#define TP_NDAY L"(\\d{1,2}(th|st|rd))"
#define TP_YEAR L"(\\d{4}'?s?|\'\\d{2}s?|\\d{2}'?s|\\d{2})"
#define TP_ZONE L"([+-]\\d{4}|[+-]\\d{2}:\\d{2}|EDT|EST|DST|GMT|PST|" \
	L"Eastern Standard Time|Eastern Daylight Time|Pacific Standard Time)"

#define TP_TIME L"((\\d{4})\\s+" TP_ZONE L"|" \
	TP_WNUM L"\\s+(am|pm|a\\.m\\.|p\\.m\\.|oclock|o'clock)|" \
	L"(\\d{1,2})\\s*(am|pm|a\\.m\\.|p\\.m\\.|oclock|o'clock)|" \
	L"(\\d{1,2}):(\\d{2})(:(\\d{2})(\\.\\d+)?)?(\\s*(am|pm|a\\.m\\.?|p\\.m\\.?))?)"

#define TP_FY L"(FY|fiscal year|fiscal)"

#define TP_MOD L"(early|late[^r]|about|around|nearly|almost|" \
	L"(no\\s+)?more(\\s+than)?|(no\\s+)?less(\\s+than)?|dawn|just(\\s+over)?|" \
	L"mid-|some|approximately|start|end|middle|beginning|at\\s+least|or\\s+so)"


struct temporal_parser
{
	const wchar_t *pattern;
	void (*normalizer)(temporal_timex2& timex2, boost::wcmatch& matches);
	wregex *re;
	wregex re_mod; // this should be static

	temporal_parser (const wchar_t *p, 
		void (*parser)(temporal_timex2&, boost::wcmatch& ) = 0)
		:pattern(p), normalizer(parser), 
		
		re_mod(wregex (TP_MOD L"+", boost::regex::perl|boost::regex::icase))
	{
		try {
			re = _new wregex (p, boost::regex::perl|boost::regex::icase);
		}
		catch (std::exception& ex) {
			wcerr << L"exception encountered for pattern: " << p << endl;
			wcerr << ex.what() << endl;
		}
	}

	temporal_parser (const temporal_parser& parser)
	{
		this->operator=(parser);
	}

	virtual ~temporal_parser ()
	{
		delete re;
	}

	temporal_parser& operator=(const temporal_parser& copy)
	{
		if (this != &copy) {
			pattern = copy.pattern;
			normalizer = copy.normalizer;
			re = _new wregex (pattern, boost::regex::perl|boost::regex::icase);
			re_mod = copy.re_mod;
		}
		return *this;
	}

	bool parse (const wchar_t *str, temporal_timex2& out)
	{
		out.clear();
		bool matched (false);
		if (normalizer != 0) {
			boost::wcmatch matches; // or boost::wcmatch boost::wsmatch
			//boost::smatch matches;
			boost::match_flag_type flags = boost::match_default;
			if (boost::regex_search(str, matches, *re, flags)) {
#if TP_DEBUG > 1
				wcout << L"[" << str << L"]" << endl;
				for (unsigned i (0); i < matches.size(); ++i) {
					wcout << L" " << i << L" " << matches.str(i) << endl;
				}
#endif	    
				normalizer (out, matches);

				// post processing to set the mod if it hasn't been set
				boost::wcmatch hits;
				if (out.MOD.length() == 0
					&& boost::regex_search(str, hits, re_mod/*, boost::match_continuous*/)) {
					wstring mod = hits.str(1);
					bool is_dur (out.VAL.substr(0,1) == L"P");
					if (wcsncasecmp (mod.c_str(), L"around", 6) == 0
						|| wcsncasecmp (mod.c_str(), L"about", 5) == 0
						|| wcsncasecmp (mod.c_str(), L"approx", 6) == 0
						|| wcsncasecmp (mod.c_str(), L"or", 2) == 0) {
						out.MOD = L"APPROX";
					}
					else if (wcsncasecmp (mod.c_str(), L"early", 5) == 0
							|| wcsncasecmp (mod.c_str(), L"begin", 5) == 0
							|| wcsncasecmp (mod.c_str(), L"start", 5) == 0
							|| wcsncasecmp (mod.c_str(), L"beginning", 9) == 0) {
						out.MOD = L"START";
					}
					else if (wcsncasecmp (mod.c_str(), L"almost", 6) == 0
							|| wcsncasecmp (mod.c_str(), L"nearly", 6) == 0
							|| wcsncasecmp (mod.c_str(), L"less", 4) == 0) {
						out.MOD = is_dur ? L"LESS_THAN" : L"AFTER";
					}
					else if (wcsncasecmp (mod.c_str(), L"more", 4) == 0) {
						out.MOD = is_dur ? L"MORE_THAN" : L"BEFORE";
					}
					else if (wcsncasecmp (mod.c_str(), L"no less", 7) == 0
							|| wcsncasecmp (mod.c_str(), L"at least", 8) == 0) {
						out.MOD = is_dur ? L"EQUAL_OR_MORE" : L"ON_OR_AFTER";
					}
					else if (wcsncasecmp (mod.c_str(), L"no more", 7) == 0) {
						out.MOD = is_dur ? L"EQUAL_OF_LESS" : L"ON_OR_BEFORE";
					}
					else if (wcsncasecmp (mod.c_str(), L"late", 4) == 0
							|| wcsncasecmp (mod.c_str(), L"end", 3) == 0) {
						out.MOD = L"END";
					}
#if TP_DEBUG > 1
					wcout << "post processing: " << str << endl;
					for (unsigned i (0); i < hits.size(); ++i) {
						wcout << L" " << i << L" " << hits.str(i) << endl;
					}
					wcout << "MOD=" << out.MOD << endl;
#endif	    		
				}
				matched = true;
			}
		}
		return matched;
	}
};

static int
month_value (const char *month)
{
	if (strncasecmp ("jan", month, 3) == 0)
		return 1;
	if (strncasecmp ("feb", month, 3) == 0)
		return 2;
	if (strncasecmp ("mar", month, 3) == 0)
		return 3;
	if (strncasecmp ("apr", month, 3) == 0)
		return 4;
	if (strncasecmp ("may", month, 3) == 0)
		return 5;
	if (strncasecmp ("jun", month, 3) == 0)
		return 6;
	if (strncasecmp ("jul", month, 3) == 0)
		return 7;
	if (strncasecmp ("aug", month, 3) == 0)
		return 8;
	if (strncasecmp ("sep", month, 3) == 0)
		return 9;
	if (strncasecmp ("oct", month, 3) == 0)
		return 10;
	if (strncasecmp ("nov", month, 3) == 0)
		return 11;
	if (strncasecmp ("dec", month, 3) == 0)
		return 12;

	return -1;
}

static int
month_value (const wchar_t *month)
{
	if (wcsncasecmp (L"jan", month, 3) == 0)
		return 1;
	if (wcsncasecmp (L"feb", month, 3) == 0)
		return 2;
	if (wcsncasecmp (L"mar", month, 3) == 0)
		return 3;
	if (wcsncasecmp (L"apr", month, 3) == 0)
		return 4;
	if (wcsncasecmp (L"may", month, 3) == 0)
		return 5;
	if (wcsncasecmp (L"jun", month, 3) == 0)
		return 6;
	if (wcsncasecmp (L"jul", month, 3) == 0)
		return 7;
	if (wcsncasecmp (L"aug", month, 3) == 0)
		return 8;
	if (wcsncasecmp (L"sep", month, 3) == 0)
		return 9;
	if (wcsncasecmp (L"oct", month, 3) == 0)
		return 10;
	if (wcsncasecmp (L"nov", month, 3) == 0)
		return 11;
	if (wcsncasecmp (L"dec", month, 3) == 0)
		return 12;

	return -1;
}

static bool
is_ordinal (const wchar_t *expr)
{
	return (wcsncasecmp (expr, L"nd", 2) == 0
		|| wcsncasecmp (expr, L"st", 2) == 0
		|| wcsncasecmp (expr, L"rd", 2) == 0
		|| wcsncasecmp (expr, L"th", 2) == 0);
}

static int
wnum_value (const wstring& wnum)
{
	const wchar_t *w (wnum.c_str());
	if (wcsncasecmp (L"one", w, 3) == 0)
		return 1;
	if (wcsncasecmp (L"two", w, 3) == 0
		|| wcsncasecmp (L"second", w, 5) == 0)
		return 2;
	if (wcsncasecmp (L"three", w, 5) == 0
		|| wcsncasecmp (L"third", w, 5) == 0)
		return 3;
	if (wcsncasecmp (L"twelve", w, 6) == 0)
		return 12;
	if (wcsncasecmp (L"thirteen", w, 8) == 0)
		return 13;
	if (wcsncasecmp (L"thirty", w, 6) == 0)
		return 30;
	if (wcsncasecmp (L"fourteen", w, 8) == 0)
		return 14;
	if (wcsncasecmp (L"fourty", w, 6) == 0)
		return 40;
	if (wcsncasecmp (L"fifteen", w, 7) == 0)
		return 15;
	if (wcsncasecmp (L"fifty", w, 5) == 0)
		return 50;
	if (wcsncasecmp (L"sixteen", w, 7) == 0)
		return 16;
	if (wcsncasecmp (L"sixty", w, 5) == 0)
		return 60;
	if (wcsncasecmp (L"seventeen", w, 9) == 0)
		return 17;
	if (wcsncasecmp (L"seventy", w, 7) == 0)
		return 70;
	if (wcsncasecmp (L"eighteen", w, 8) == 0)
		return 18;
	if (wcsncasecmp (L"eighty", w, 6) == 0)
		return 80;
	if (wcsncasecmp (L"nineteen", w, 8) == 0)
		return 19;
	if (wcsncasecmp (L"ninety", w, 6) == 0)
		return 90;
	if (wcsncasecmp (L"twenty", w, 6) == 0)
		return 20;
	if (wcsncasecmp (L"four", w, 4) == 0)
		return 4;
	if (wcsncasecmp (L"five", w, 4) == 0
		|| wcsncasecmp (L"fifth", w, 5) == 0)
		return 5;
	if (wcsncasecmp (L"six", w, 3) == 0)
		return 6;
	if (wcsncasecmp (L"seven", w, 5) == 0)
		return 7;
	if (wcsncasecmp (L"eight", w, 5) == 0)
		return 8;
	if (wcsncasecmp (L"nine", w, 4) == 0
		|| wcsncasecmp (L"nin", w, 3) == 0)
		return 9;
	if (wcsncasecmp (L"ten", w, 3) == 0)
		return 10;
	if (wcsncasecmp (L"eleven", w, 6) == 0)
		return 11;

	return -1;
}

static short
day_of_week_value (const wstring& day_of_week)
{
	const wchar_t *dow (day_of_week.c_str());
	if (wcsncasecmp (dow, L"mon", 3) == 0)
		return 1;
	if (wcsncasecmp (dow, L"tue", 3) == 0)
		return 2;
	if (wcsncasecmp (dow, L"wed", 3) == 0)
		return 3;
	if (wcsncasecmp (dow, L"thu", 3) == 0)
		return 4;
	if (wcsncasecmp (dow, L"fri", 3) == 0)
		return 5;
	if (wcsncasecmp (dow, L"sat", 3) == 0)
		return 6;
	if (wcsncasecmp (dow, L"sun", 3) == 0)
		return 7;
	return -1;
}

static wstring
format_month (const wstring& month)
{
	wchar_t monbuf[6];
	(void) swprintf (monbuf, 6, L"%02d", month_value (month.c_str()));
	return wstring (monbuf);
}

static wstring
format_timezone (const wstring& zone)
{
	wstring tzone;
	if (wcsncasecmp (zone.c_str(), L"east", 4) == 0
		|| wcsncasecmp (zone.c_str(), L"est", 3) == 0)
		tzone = L"-05";
	else if (wcsncasecmp (zone.c_str(), L"edt", 3) == 0)
		tzone = L"-04";
	else if (wcsncasecmp (zone.c_str(), L"pst", 3) == 0
		|| wcsncasecmp (zone.c_str(), L"pacific", 7) == 0)
		tzone = L"-08";
	else if (wcsncasecmp (zone.c_str(), L"gmt", 3) == 0
		|| wcsncasecmp (zone.c_str(), L"greenwich", 9) == 0)
		tzone = L"Z";
	else if (wcsncasecmp (zone.c_str(), L"-", 1) == 0) {
		// zone is already in the -XXXX format; e.g., -0400
		tzone = zone.substr(0, 3);
	}
	else {
		// ??
	}

	return tzone;
}

static wstring
format_time (const wstring& time)
{
	wstring formatted (time);

	boost::wregex re_time (TP_TIME, boost::wregex::perl|boost::wregex::icase);
	boost::wcmatch what;
	wchar_t timebuf[32];

	/*
	* The TP_TIME pattern has 31 possible submatches.
	* Notice that these positions are VERY sensitive to the regular
	* expresion pattern TP_TIME! 
	*/
	if (boost::regex_search (time.c_str(), what, re_time)) {
#ifdef TP_DEBUG
		cout << "** parsing TP_TIME submatch" << endl;
		for (unsigned i (0); i < what.size(); ++i) {
			wcout << " " << i << " " << what.str(i) << endl;
		}
#endif
		if (what.length(2) > 0) {
			// 1627 GMT
			wstring expr (what.str(2));
			formatted = expr.substr(0, 2);
			formatted += L":";
			formatted += expr.substr(2, 4);
			formatted += format_timezone (what.str(3));
		}
		else if (what.length(4) > 0) {
			// six pm 
			int hour (wnum_value (what.str(4)));
			if (wcsncasecmp (L"p", what.str(22).c_str(), 1) == 0) {
				if (hour < 12)
					hour += 12;
			}
			else if (wcsncasecmp (L"a", what.str(22).c_str(), 1) == 0) {
				if (hour == 12)
					hour = 0;
			}
			(void) swprintf (timebuf, 32, L"%02d:00", hour);
			formatted = timebuf;
		}
		else if (what.length(23) > 0) {
			// 4pm or 4 pm
			int hour (_wtoi (what.str(23).c_str()));
			if (wcsncasecmp (L"p", what.str(24).c_str(), 1) == 0) { // pm
				if (hour < 12)
					hour += 12;
			}
			else if (wcsncasecmp (L"a", what.str(24).c_str(), 1) == 0) { // am
				if (hour == 12)
					hour = 0;
			}
			(void) swprintf (timebuf, 32, L"%02d:00", hour);
			formatted = timebuf;
		}
		else if (what.length(25) > 0) {
			int hour (_wtoi (what.str(25).c_str()));
			int minute (_wtoi (what.str(26).c_str()));

			// 11:13:05.014 [am/pm]
			if (what.length(31) > 0) {
				if (wcsncasecmp (L"p", what.str(31).c_str(), 1) == 0) {
					// pm
					if (hour < 12)
						hour += 12;
				}
				else {
					// am
					if (hour == 12)
						hour = 0;
				}
			}

			(void) swprintf (timebuf, 32, L"%02d:%02d", hour, minute);
			formatted = timebuf;
			formatted += what.str(27); // :05.xxx if any
		}	
		else {
			// ??
		}
	}

	return formatted;
}

static void
format_time (wchar_t *timebuf, const boost::posix_time::ptime& pt, bool show_milliseconds = false)
{
	if (show_milliseconds) {
		(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dT%02d:%02d:%02d.%03d",
			static_cast<int>(pt.date().year()),
			static_cast<int>(pt.date().month()),
			static_cast<int>(pt.date().day()),
			static_cast<int>(pt.time_of_day().hours()),
			static_cast<int>(pt.time_of_day().minutes()),
			static_cast<int>(pt.time_of_day().seconds()),
			static_cast<int>(pt.time_of_day().fractional_seconds()));
	}
	else {
		(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dT%02d:%02d:%02d",
			static_cast<int>(pt.date().year()),
			static_cast<int>(pt.date().month()),
			static_cast<int>(pt.date().day()),
			static_cast<int>(pt.time_of_day().hours()),
			static_cast<int>(pt.time_of_day().minutes()),
			static_cast<int>(pt.time_of_day().seconds()));
	}
}

static wstring
format_season (const wstring& str)
{
	const wchar_t *s (str.c_str());
	wstring season;

	if (wcsncasecmp (s, L"winter", 6) == 0)
		season = L"WI";
	else if (wcsncasecmp (s, L"spring", 6) == 0)
		season = L"SP";
	else if (wcsncasecmp (s, L"fall", 4) == 0)
		season = L"FA";
	else if (wcsncasecmp (s, L"summer", 6) == 0)
		season = L"SU";

	return season;
}

static wstring
non_time_duration_unit (const wstring& unit)
{
	wstring u;
	if (wcsncasecmp (unit.c_str(), L"day", 3) == 0)
		u = L"D";
	else if (wcsncasecmp (unit.c_str(), L"weekend", 7) == 0)
		u = L"WE";
	else if (wcsncasecmp (unit.c_str(), L"week", 4) == 0)
		u = L"W";
	else if (wcsncasecmp (unit.c_str(), L"month", 5) == 0)
		u = L"M";
	else if (wcsncasecmp (unit.c_str(), L"year", 4) == 0)
		u = L"Y";
	else if (wcsncasecmp (unit.c_str(), L"decade", 6) == 0)
		u = L"DE";
	else if (wcsncasecmp (unit.c_str(), L"centur", 6) == 0)
		u = L"CE";
	else if (wcsncasecmp (unit.c_str(), L"millen", 6) == 0)
		u = L"ML";
	return u;
}

static wstring
format_non_specific_time (const wstring& str)
{
	const wchar_t *s (str.c_str());
	wstring nst;

	if (wcscasecmp (s, L"morning") == 0)
		nst = L"MO";
	else if (wcscasecmp (s, L"afternoon") == 0)
		nst = L"AF";
	else if (wcsncasecmp (s, L"night", 5) == 0)
		nst = L"NI";

	return nst;
}

boost::posix_time::ptime
non_specific_time_normalizer (const boost::posix_time::ptime& anchor, 
							  const wchar_t *nst)
{
	boost::posix_time::ptime pt (anchor);

	if (wcscasecmp (nst, L"today") == 0) {
		// do nothing
	}
	else if (wcscasecmp (nst, L"tomorrow") == 0) {
		pt += boost::gregorian::days(1);
	}
	else if (wcscasecmp (nst, L"yesterday") == 0) {
		pt -= boost::gregorian::days(1);
	}
	return pt;
}

boost::posix_time::ptime
day_of_week_normalizer (const boost::posix_time::ptime& ref,
						const wchar_t *day_of_week, int dir = 1) 
{
	// dir > 0: forward, dir < 0: backward
	short dow = day_of_week_value (day_of_week);
	if (wcsncasecmp (day_of_week, L"sunday", 6) == 0)
		dow = 0;
	boost::gregorian::greg_weekday gw(dow);
	boost::posix_time::ptime anchor (ref);

	boost::gregorian::date_duration days = boost::gregorian::days_until_weekday (anchor.date(), gw);
	if (dir > 0) {
		anchor += days;
	}
	else if (dir < 0) {
		anchor -= days;
	}
	else {
		// find out which one is smaller than go with that
		boost::gregorian::date_duration back = 
			boost::gregorian::days_before_weekday (anchor.date(), gw);
		if (back < days) {
			anchor -= back;
		}
		else {
			anchor += days;
		}
	}
	return anchor;
}

static void
empty_normalizer (temporal_timex2& timex2, boost::wcmatch& matches)
{
	timex2.clear();
}

static void
normalizer1 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer1: " << matches.str() << endl;
#endif

	/*
	* This normalizer matches pattern of the form:
	*  January 21, 1994 08:29 Eastern Standard Time
	* There are 46 submatches in this pattern.  Here
	* are the relavent submatches:
	*  pos 1    - the month (January)
	*  pos 13   - date (21)
	*  pos 14   - year (1994)
	*  pos 15   - time (08:29)
	*  pos 46   - time zone (Eastern Standard Time)
	*/
	wchar_t timebuf[TP_BUFSIZE];

	(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%ls-%02dT%ls%ls", 
		_wtoi (matches.str(14).c_str()),
		format_month (matches.str(1)).c_str(),
		_wtoi (matches.str(13).c_str()),
		format_time (matches.str(15)).c_str(),
		format_timezone (matches.str(46)).c_str()
		);

	timex2.VAL = timebuf;
}

static void
normalizer2 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer2: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE];

	(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%ls-%02dT%ls", 
		_wtoi (matches.str(45).c_str()),
		format_month (matches.str(32)).c_str(),
		_wtoi (matches.str(44).c_str()),
		format_time (matches.str(1)).c_str());

	timex2.VAL = timebuf;
}

static void
normalizer3 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer3: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE];

	(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%ls-%02d",
		_wtoi (matches.str(14).c_str()),
		format_month (matches.str(1)).c_str(),
		_wtoi (matches.str(13).c_str()));

	timex2.VAL = timebuf;
}

static void
normalizer4 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer4: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};

	if (matches.length(1) > 0) {
		(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%ls-%02d",
						_wtoi (matches.str(15).c_str()),
						format_month (matches.str(2)).c_str(),
						_wtoi (matches.str(1).c_str()));
	}
	else if (matches.length(15) == 4) {
		(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%ls",
						_wtoi (matches.str(15).c_str()),
						format_month (matches.str(2)).c_str());
	}
	else { // April 15
		(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%ls-%02d",
						static_cast<int>(timex2.anchor.date().year()),
						format_month (matches.str(2)).c_str(),
						_wtoi (matches.str(15).c_str()));
	}

	timex2.VAL = timebuf;
}

static void
normalizer5 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer5: " << matches.str() << endl;
#endif
	timex2.clear();
	wchar_t timebuf[TP_BUFSIZE] = {0};

#if 1
	int year = timex2.anchor.date().year();
	(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%ls-%02d",
		year, format_month (matches.str(1)).c_str(),
		_wtoi (matches.str(13).c_str()));
#else
	(void) swprintf (timebuf, TP_BUFSIZE, L"XXXX-%ls-%02d",
		format_month (matches.str(1)).c_str(),
		_wtoi (matches.str(13).c_str()));
#endif

	timex2.VAL = timebuf;
}

static void
normalizer6 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer6: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};

	int year = _wtoi (matches.str(3).c_str());
	int mon = _wtoi (matches.str(2).c_str());
	int day = _wtoi (matches.str(1).c_str());
	if (mon > 12) { // hmmm... swap...
		int tmp = mon;
		mon = day;
		day = tmp;
	}
	if (matches.length(3) == 2) {
		// two-digit year
		if (year < 10) year += 2000; // sigh...
		else year += 1900;
	}

	(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dT%ls", year, mon, day,
					format_time (matches.str(4)).c_str());

	timex2.VAL = timebuf;
}

static void
normalizer6b (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer6b: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};
	int year = _wtoi (matches.str(36).c_str());
	int mon = _wtoi (matches.str(34).c_str());
	int day = _wtoi (matches.str(35).c_str());
	if (mon > 12) {
		int tmp = mon;
		mon = day;
		day = tmp;
	}
	if (matches.length(36) == 2) {
		if (year < 10) year += 2000;
		else year += 1900;
	}
	(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dT%ls", year, mon, day,
					format_time (matches.str(1)).c_str());

	timex2.VAL = timebuf;
}

static void
normalizer7 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer7: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};

	int year = _wtoi (matches.str(3).c_str());
	int mon = _wtoi (matches.str(1).c_str());
	int day = _wtoi (matches.str(2).c_str());
	if (mon > 12) {
		int tmp = mon;
		mon = day;
		day = mon;
	}
	if (matches.length(3) == 2) {
		// two-digit year
		if (year < 10) year += 2000; // sigh...
		else year += 1900;
	}
	(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", year, mon, day);

	timex2.VAL = timebuf;
}

static void
normalizer8 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer8: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};

	(void) swprintf (timebuf, TP_BUFSIZE, L"XXXX-%02d-%02d",
		_wtoi (matches.str(1).c_str()), // month
		_wtoi (matches.str(2).c_str())); // day

	timex2.VAL = timebuf;
}

static void
normalizer9 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer9: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};

	(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%ls", 
		_wtoi (matches.str(3).c_str()),
		format_season (matches.str(1)).c_str());

	timex2.VAL = timebuf;
}

static void
normalizer9b (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer9b: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};
	int day = 0;
	if (matches.length(3) > 0) {
		day = _wtoi (matches.str(3).c_str());
	}
	else if (matches.length(5) > 0) {
		day = wnum_value (matches.str(5));
	}
	else {
	}

	if (day > 0) {
		(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d",
						static_cast<int>(timex2.anchor.date().year()),
						month_value (matches.str(24).c_str()), day);
	}
	timex2.VAL = timebuf;
}

static void
normalizer10 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer10: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};

	int year = _wtoi (matches.str(2).c_str());
	if (year < 100)
		year += 1900;

	(void) swprintf (timebuf, TP_BUFSIZE, L"FY%04d", year);

	timex2.VAL = timebuf;
}


static void
normalizer11 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer11: " << matches.str() << endl;
#endif

	wstring nst (matches.str(32));
	boost::gregorian::date anchor (timex2.anchor.date());
	
	if (wcsncasecmp (nst.c_str(), L"yesterday", 9) == 0)
		anchor -= boost::gregorian::days(1);
	else if (wcsncasecmp (nst.c_str(), L"tomorrow", 8) == 0)
		anchor += boost::gregorian::days(1);

	wchar_t timebuf[TP_BUFSIZE];

	(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dT%ls", 
					static_cast<int>(anchor.year()), 
					static_cast<int>(anchor.month()), 
					static_cast<int>(anchor.day()),
					format_time (matches.str(1)).c_str());

	timex2.VAL = timebuf;
}

static void
normalizer12 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer12: " << matches.str() << endl;
#endif

	// without any other info, we assume reference to day_of_week is forward
	//  reference
	boost::posix_time::ptime anchor 
		(day_of_week_normalizer (timex2.anchor, matches.str(1).c_str()));

	boost::gregorian::date::ymd_type ymd = anchor.date().year_month_day();
	int yr = ymd.year, mon = ymd.month, day = ymd.day;

	wchar_t timebuf[TP_BUFSIZE] = {0};
	(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dT%ls",
		yr, mon, day, 
		format_non_specific_time (matches.str(9)).c_str());

	timex2.VAL = timebuf;
}

static void
normalizer13 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer13: " << matches.str() << endl;
#endif

	wchar_t timebuf[TP_BUFSIZE] = {0};
	int year = _wtoi (matches.str(1).c_str());
	if (matches.length(1) == 2) {
		if (year < 10) { // hmm....
			year += 2000;
		}
		else {
			year += 1900;
		}
	}
	int mon = _wtoi (matches.str(2).c_str());
	int day = _wtoi (matches.str(3).c_str());

	if (matches.length(4) > 0) {
		if (matches.length(38) > 0) { // timezone
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dT%ls%ls", year, mon, day,			
							format_time (matches.str(6)).c_str(),
							format_timezone (matches.str(38)).c_str());
		}
		else {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dT%ls", year, mon, day,			
					format_time (matches.str(6)).c_str());
		}
	}
	else {
		(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", year, mon, day);
	}

	timex2.VAL = timebuf;
}

static void
normalizer14 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer14: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};
	(void) swprintf (timebuf, TP_BUFSIZE, L"%ls-%ls-%ls",
		matches.str(1).c_str(),
		matches.str(2).c_str(),
		matches.str(3).c_str());

	timex2.VAL = timebuf;
	if (matches.length(5) > 0) {
		timex2.VAL += L"T";
		timex2.VAL += format_time (matches.str(5));
	}
}

static void
normalizer15 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer15: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};

	(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%ls-%02dT%ls%ls",
		_wtoi (matches.str(14).c_str()),
		format_month (matches.str(2)).c_str(),
		_wtoi (matches.str(1).c_str()),
		format_time (matches.str(15)).c_str(),
		format_timezone (matches.str(46)).c_str());

	timex2.VAL = timebuf;
}

static void
normalizer16 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer16: " << matches.str() << endl;
#endif
	int head (0);
	double fraction (-1.f);

	bool isOrdinal (false);
	size_t len3 (matches.length(3));
	if (len3 > 2) {
		wstring number = matches.str(3).substr(0, len3-2);
		wstring order = matches.str(3).substr(len3-2);
		isOrdinal = is_ordinal (order.c_str())  
			&& (wnum_value (number) > 0 || _wtoi (number.c_str()) > 0);
	}

	wchar_t timebuf[TP_BUFSIZE] = {0};

	// something like 300(2) thousand(6) years(32) ago
	if (matches.length(3) > 0 && matches.length(4) > 0) {
		// assume backward reference
		if (matches.length(5) > 0 && matches.length(7) > 0) { // 2,300
			if (matches.str(6).substr(0,1) == L".") {
				fraction = boost::lexical_cast<double>(matches.str(6).c_str());
				head = _wtoi (matches.str(5).c_str());
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%ls%ls", matches.str(5).c_str(),
								matches.str(7).c_str());
				head = _wtoi (timebuf);
			}
		}
		else {
			head = _wtoi (matches.str(4).c_str());
		}

		wstring extent (matches.str(8));
		if (wcsncasecmp (extent.c_str(), L"million", 8) == 0) {
			if (fraction > 0.f) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"MA%d%.1f", head, fraction);
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, L"MA%d", head);
			}
		}
		else if (wcsncasecmp (extent.c_str(), L"billion", 7) == 0) {
			if (fraction > 0.f) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"BA%d%.1f", head, fraction);
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, L"BA%d", head);
			}
		}
		else if (wcsncasecmp (extent.c_str(), L"thousand", 8) == 0) {
			if (fraction > 0.f) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"KA%d%.1f", head, fraction);
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, L"KA%d", head);
			}
		}
	}
	else if (matches.length(3) > 0) { // 300 years ago
		wstring extent (matches.str(32));
		wstring prefix (matches.str(1)); // e.g., past, next
		wstring postfix (matches.str(40));

		int year = timex2.anchor.date().year();

		if (matches.length(8) > 0) {
			if (wcsncasecmp (matches.str(8).c_str(), L"half", 4) == 0) {
				fraction = .5f;
			}
			head = wnum_value (matches.str(8));
		}
		else if (matches.length(27) > 0) {
			if (matches.length(28) > 0 && matches.length(30) > 0) { // 2,300
				if (matches.str(29).substr(0,1) == L".") {
					fraction = boost::lexical_cast<double>(matches.str(29).c_str());
					head = _wtoi (matches.str(28).c_str());
				}
				else {
					(void) swprintf (timebuf, TP_BUFSIZE, L"%ls%ls", matches.str(28).c_str(),
									matches.str(30).c_str());
					head = _wtoi (timebuf);
				}
			}
			else {
				head = _wtoi (matches.str(27).c_str());
			}
		}

		if (wcsncasecmp (extent.c_str(), L"centur", 6) == 0) {
			if (isOrdinal) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%02d", head-1);
			}
			else if (head > 0) {
				if (fraction > 0.f) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"P%d%.1fCE", head, fraction);
				}
				else {
					(void) swprintf (timebuf, TP_BUFSIZE, L"P%dCE", head);
				}
			}
			else if (fraction > 0.f) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"P%.1fCE", fraction);
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, L"PXCE");
			}
		}
		else if (wcsncasecmp (extent.c_str(), L"millenni", 8) == 0) {
			if (isOrdinal) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%d", head-1); 
			}
			else if (head > 0) {
				if (fraction > 0.f) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"P%d%.1fML", head, fraction);
				}
				else {
					(void) swprintf (timebuf, TP_BUFSIZE, L"P%dML", head);
				}
			}
			else if (fraction > 0.f) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"P%.1fML", fraction);
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, L"PXML");
			}
		}
		else if (!isOrdinal) {
			int dir (0); // direction: dir > 0, forward; dir < 0 backward
			int duration (0);

			if (wcsncasecmp (prefix.c_str(), L"past", 4) == 0
				|| wcsncasecmp (prefix.c_str(), L"since", 5) == 0
				|| wcsncasecmp (postfix.c_str(), L"ended", 5) == 0
				|| wcsncasecmp (postfix.c_str(), L"old", 3) == 0) {
				duration = -1;
				if (wcsncasecmp (postfix.c_str(), L"ended", 5) == 0) {
					dir = -1;
				}
			}
			else if (wcsncasecmp (prefix.c_str(), L"next", 4) == 0
				|| wcsncasecmp (prefix.c_str(), L"in", 2) == 0
				|| wcsncasecmp (postfix.c_str(), L"since", 5) == 0
				|| wcsncasecmp (postfix.c_str(), L"long", 4) == 0) {
				duration = 1;
				if (wcsncasecmp (postfix.c_str(), L"since", 5) == 0) {
					dir = -1;
				}
			}
			else if (wcsncasecmp (postfix.c_str(), L"ago", 3) == 0) {
				dir = -1;
			}
			else if (postfix.length() > 0) {
				dir = 1;
			}

			if (wcsncasecmp (extent.c_str(), L"decade", 6) == 0) {
				if (head > 0) {
					if (fraction > 0.f) {
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%d%.1fDE", head, fraction);
					}
					else {
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%dDE", head);
					}
				}
				else if (fraction > 0.f) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"P%.1fDE", fraction);
				}
				else {
					(void) swprintf (timebuf, TP_BUFSIZE, L"PXDE");
				}
			}
			else if (wcsncasecmp (extent.c_str(), L"year", 4) == 0
					|| wcsncasecmp (extent.c_str(), L"yr", 2) == 0) {
				if (duration < 0) {
					timex2.ANCHOR_DIR = L"ENDING";
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", 
									static_cast<int>(timex2.anchor.date().year()));
					timex2.ANCHOR_VAL = timebuf;
					if (head > 0) {
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%dY", head);
					}
					else {
						(void) swprintf (timebuf, TP_BUFSIZE, L"PXY");
					}
				}
				else if (duration > 0) {
					timex2.ANCHOR_DIR = L"STARTING";
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d",
									static_cast<int>(timex2.anchor.date().year()));
					timex2.ANCHOR_VAL = timebuf;
					if (head > 0) {
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%dY", head);
					}
					else {
						(void) swprintf (timebuf, TP_BUFSIZE, L"PXY");
					}
				}
				else if (dir < 0) {
					if (head > 0) {
						if (head > year) { // BC
							(void) swprintf (timebuf, TP_BUFSIZE, L"BC%04d", head-year);
						}
						else {
							(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", year-head);
						}
					}
					else {
						// few years ago
						timex2.ANCHOR_DIR = L"BEFORE";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", 
									static_cast<int>(timex2.anchor.date().year()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"PAST_REF");
					}
				}
				else if (dir > 0) {
					if (head > 0) {
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", year+head);
					}
					else {
						timex2.ANCHOR_DIR = L"AFTER";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", 
									static_cast<int>(timex2.anchor.date().year()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"FUTURE_REF");
					}
				}
				else if (head > 0) {
					if (fraction > 0.f) {
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%d%.1fY", head, fraction);
					}
					else {
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%dY", head);
					}
				}
				else if (fraction > 0.f) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"P%.1fY", fraction);
				}
				else {
					(void) swprintf (timebuf, TP_BUFSIZE, L"PXY");
				}
			}
			else if (wcsncasecmp (extent.c_str(), L"month", 5) == 0
					|| wcsncasecmp (extent.c_str(), L"mnth", 4) == 0) {
				boost::gregorian::date d = timex2.anchor.date();
				if (duration < 0) {
					if (head > 0) {
						if (dir < 0) {
							d -= boost::gregorian::months(head);
						}
						timex2.ANCHOR_DIR = L"ENDING";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d", 
										static_cast<int>(d.year()),	static_cast<int>(d.month()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%dM", head);
					}
					else {
						timex2.ANCHOR_DIR = L"BEFORE";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d", 
										static_cast<int>(d.year()),	static_cast<int>(d.month()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"PAST_REF");
					}
				}
				else if (duration > 0) {
					if (head > 0) {
						if (dir < 0) {
							d -= boost::gregorian::months(head);
						}
						timex2.ANCHOR_DIR = L"STARTING";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d",
										static_cast<int>(d.year()), static_cast<int>(d.month()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%dM", head);
					}
					else {
						timex2.ANCHOR_DIR = L"AFTER";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d",
										static_cast<int>(d.year()), static_cast<int>(d.month()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"FUTURE_REF");
					}
				}
				else if (dir < 0) {
					if (head > 0) {
						d -= boost::gregorian::months(head);
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d", static_cast<int>(d.year()),
										static_cast<int>(d.month()));
					}
					else {
						timex2.ANCHOR_DIR = L"BEFORE";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d", static_cast<int>(d.year()),
										static_cast<int>(d.month()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"PAST_REF");
					}
				}
				else if (dir > 0) {
					if (head > 0) {
						d += boost::gregorian::months(head);
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d", static_cast<int>(d.year()),
										static_cast<int>(d.month()));
					}
					else {
						timex2.ANCHOR_DIR = L"AFTER";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d", static_cast<int>(d.year()),
										static_cast<int>(d.month()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"FUTURE_REF");
					}
				}
				else if (head > 0) {
					if (fraction > 0.f) {
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%d%.1fM", head, fraction);
					}
					else {
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%dM", head);
					}
				}
				else if (fraction > 0.f) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"P%.1fM", fraction);
				}
				else {
					(void) swprintf (timebuf, TP_BUFSIZE, L"PXM");
				}
			}
			else if (wcsncasecmp (extent.c_str(), L"week", 4) == 0) {
				const wchar_t *end = 0;
				if (wcsncasecmp (extent.c_str(), L"weekend", 7) == 0) {
					end = L"WE";
				}

				boost::gregorian::date d = timex2.anchor.date();
				if (duration < 0) {
					if (head > 0) {
						timex2.ANCHOR_DIR = L"ENDING";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%02d", 
										static_cast<int>(d.year()), d.week_number()-1);
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%dW", head);
					}
					else {
						timex2.ANCHOR_DIR = L"BEFORE";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%02d", 
										static_cast<int>(d.year()), d.week_number()-1);
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"PAST_REF");
					}
				}
				else if (duration > 0) {
					if (head > 0) {
						timex2.ANCHOR_DIR = L"STARTING";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%02d",
										static_cast<int>(d.year()), d.week_number()-1);
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%dW", head);
					}
					else {
						timex2.ANCHOR_DIR = L"AFTER";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%02d", 
										static_cast<int>(d.year()), d.week_number()-1);
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"FUTURE_REF");
					}
				}
				else if (dir < 0) {
					if (head > 0) {
						d -= boost::gregorian::weeks(head-1);
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%02d", static_cast<int>(d.year()), 
										d.week_number()-1);
					}
					else {
						timex2.ANCHOR_DIR = L"BEFORE";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%02d", static_cast<int>(d.year()),
										d.week_number()-1);
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"PAST_REF");
					}
				}
				else if (dir > 0) {
					if (head > 0) {
						d += boost::gregorian::weeks(head);
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%02d", static_cast<int>(d.year()), 
										d.week_number()-1);
					}
					else {
						timex2.ANCHOR_DIR = L"AFTER";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%d", static_cast<int>(d.year()),
										d.week_number()-1);
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"FUTURE_REF");
					}
				}
				else if (head > 0) {
					if (fraction > 0.f) {
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%d%.1fW", head, fraction);
					}
					else {
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%d%ls", head, end ? end : L"W");
					}
				}
				else if (fraction > 0.f) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"P%.1fW", fraction);
				}
				else {
					(void) swprintf (timebuf, TP_BUFSIZE, L"PXW");
				}
			}
			else if (wcsncasecmp (extent.c_str(), L"day", 3) == 0) {
				boost::gregorian::date d = timex2.anchor.date();
				if (duration < 0) {
					if (head > 0) {
						timex2.ANCHOR_DIR = L"ENDING";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", 
										static_cast<int>(d.year()),
										static_cast<int>(d.month()),
										static_cast<int>(d.day()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%dD", head);
					}
					else {
						timex2.ANCHOR_DIR = L"BEFORE";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", 
										static_cast<int>(d.year()),
										static_cast<int>(d.month()),
										static_cast<int>(d.day()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"PAST_REF");
					}
				}
				else if (duration > 0) {
					if (head > 0) {
						timex2.ANCHOR_DIR = L"STARTING";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", 
										static_cast<int>(d.year()),
										static_cast<int>(d.month()),
										static_cast<int>(d.day()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%dD", head);
					}
					else {
						timex2.ANCHOR_DIR = L"AFTER";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", 
										static_cast<int>(d.year()),
										static_cast<int>(d.month()),
										static_cast<int>(d.day()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"FUTURE_REF");
					}
				}
				else if (dir < 0) {
					if (head > 0) {
						d -= boost::gregorian::days(head);
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", 
										static_cast<int>(d.year()),
										static_cast<int>(d.month()),
										static_cast<int>(d.day()));
					}
					else {
						timex2.ANCHOR_DIR = L"BEFORE";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d",
										static_cast<int>(d.year()),
										static_cast<int>(d.month()),
										static_cast<int>(d.day()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"PAST_REF");
					}
				}
				else if (dir > 0) {
					if (head > 0) {
						d += boost::gregorian::days(head);
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", 
										static_cast<int>(d.year()),
										static_cast<int>(d.month()),
										static_cast<int>(d.day()));
					}
					else {
						timex2.ANCHOR_DIR = L"AFTER";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", 
										static_cast<int>(d.year()),
										static_cast<int>(d.month()),
										static_cast<int>(d.day()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"FUTURE_REF");
					}
				}
				else if (head > 0) {
					if (fraction > 0.f) {
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%d%.1fD", head, fraction);
					}
					else {
						(void) swprintf (timebuf, TP_BUFSIZE, L"P%dD", head);
					}
				}
				else if (fraction > 0.f) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"P%.1fD", fraction);
				}
				else {
					(void) swprintf (timebuf, TP_BUFSIZE, L"PXD");
				}
			}
			else if (wcsncasecmp (extent.c_str(), L"hour", 4) == 0
					|| wcsncasecmp (extent.c_str(), L"hr", 2) == 0) {
				boost::posix_time::ptime pt = timex2.anchor;
				wchar_t *ptr;
				if (duration < 0) {
					timex2.ANCHOR_DIR = L"ENDING";
					format_time (timebuf, pt);
#if 0
					ptr = wcsstr (timebuf, L":");
					if (ptr != 0) {
						*ptr = 0;
					}
#endif
					timex2.ANCHOR_VAL = timebuf;
					if (head > 0)
						(void) swprintf (timebuf, TP_BUFSIZE, L"PT%dH", head);
					else if (fraction > 0.f) {
						//	(void) swprintf (timebuf, TP_BUFSIZE, L"PT%0fM", 60*fraction);
						(void) swprintf (timebuf, TP_BUFSIZE, L"PT30M");
					}
					else
						(void) swprintf (timebuf, TP_BUFSIZE, L"PTXH");
				}
				else if (duration > 0) {
					if (wcsncasecmp (prefix.c_str(), L"next", 4) == 0) {
						timex2.ANCHOR_DIR = L"AFTER";
					}
					else {
						timex2.ANCHOR_DIR = L"STARTING";
					}
					format_time (timebuf, pt);
#if 0
					ptr = wcsstr (timebuf, L":");
					if (ptr != 0) {
						*ptr = 0;
					}
#endif
					timex2.ANCHOR_VAL = timebuf;
					if (head > 0)
						(void) swprintf (timebuf, TP_BUFSIZE, L"PT%dH", head);
					else if (fraction > 0.f) {
						//(void) swprintf (timebuf, TP_BUFSIZE, L"PT%0fM", 60.0*fraction);
						(void) swprintf (timebuf, TP_BUFSIZE, L"PT30M");
					}
					else
						(void) swprintf (timebuf, TP_BUFSIZE, L"PTXH");
				}
				else if (dir < 0) {
					if (head > 0) {
						pt -= boost::posix_time::hours(head);
						format_time (timebuf, pt);
					}
					else {
						timex2.ANCHOR_DIR = L"BEFORE";
						format_time (timebuf, pt);
						ptr = wcsstr (timebuf, L":");
						if (ptr != 0) {
							*ptr = 0;
						}
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"PAST_REF");
					}
				}
				else if (dir > 0) {
					if (head > 0) {
						pt += boost::posix_time::hours(head);
						format_time (timebuf, pt);
					}
					else {
						timex2.ANCHOR_DIR = L"AFTER";
						format_time (timebuf, pt);
						ptr = wcsstr (timebuf, L":");
						if (ptr != 0) {
							*ptr = 0;
						}
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"FUTURE_REF");
					}
				}
				else if (head > 0) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"PT%dH", head);
				}
				else {
					(void) swprintf (timebuf, TP_BUFSIZE, L"PTXH");
				}
			}
			else if (wcsncasecmp (extent.c_str(), L"minute", 6) == 0
					|| wcsncasecmp (extent.c_str(), L"min", 3) == 0) {
				boost::posix_time::ptime pt (timex2.anchor);
				if ((wcsncasecmp (postfix.c_str(), L"to", 2) == 0
					 || wcsncasecmp (postfix.c_str(), L"til", 3) == 0
					 || wcsncasecmp (postfix.c_str(), L"past", 4) == 0
					 || wcsncasecmp (postfix.c_str(), L"later", 5) == 0)
					&& matches.length(42) > 0) {
					// ten minutes to 3 or ten minutes past 3
					int hour = 0;
					if (matches.length(43) > 0) {
						hour = wnum_value (matches.str(43));
					}
					else if (matches.length(61) > 0) {
						hour = _wtoi (matches.str(61).c_str());
					}

					if (hour < 12) { // implicitly assume pm?
						hour += 12;
					}

					if (wcsncasecmp (postfix.c_str(), L"past", 4) == 0
						|| wcsncasecmp (postfix.c_str(), L"later", 5) == 0) {
						boost::posix_time::time_duration td (boost::posix_time::hours(hour)
															+ boost::posix_time::minutes(head));
						boost::posix_time::ptime tmp (timex2.anchor.date(), td);
						pt = tmp;
					}
					else {
						boost::posix_time::time_duration td (boost::posix_time::hours(hour)
															- boost::posix_time::minutes(head));
						boost::posix_time::ptime tmp (timex2.anchor.date(), td);
						pt = tmp;
					}
					format_time (timebuf, pt);
				}
				else if (duration < 0 || duration > 0) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dTXX:XX",
									static_cast<int>(pt.date().year()),
									static_cast<int>(pt.date().month()),
									static_cast<int>(pt.date().day()));
					timex2.ANCHOR_VAL = timebuf;
					timex2.ANCHOR_DIR = duration < 0 ? L"ENDING" : L"STARTING";
					(void) swprintf (timebuf, TP_BUFSIZE, L"PT%dM", head);
				}
				else if (dir < 0) {
					if (head > 0) {
						pt -= boost::posix_time::minutes(head);
						format_time (timebuf, pt);
					}
					else {
						timex2.ANCHOR_DIR = L"BEFORE";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dTXX:XX",
										static_cast<int>(pt.date().year()),
										static_cast<int>(pt.date().month()),
										static_cast<int>(pt.date().day()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"PAST_REF");
					}
				}
				else if (dir > 0) {
					if (head > 0) {
						pt += boost::posix_time::minutes(head);
						format_time (timebuf, pt);
					}
					else {
						timex2.ANCHOR_DIR = L"AFTER";
						(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dTXX:XX",
										static_cast<int>(pt.date().year()),
										static_cast<int>(pt.date().month()),
										static_cast<int>(pt.date().day()));
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"FUTURE_REF");
					}
				}
				else if (head > 0) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"PT%dM", head);
				}
				else {
					(void) swprintf (timebuf, TP_BUFSIZE, L"PTXM");
				}
			}
			else if (wcsncasecmp (extent.c_str(), L"second", 6) == 0
					|| wcsncasecmp (extent.c_str(), L"sec", 3) == 0) {
				boost::posix_time::ptime pt (timex2.anchor);
				if (duration < 0) {
					timex2.ANCHOR_DIR = L"ENDING";
					format_time (timebuf, pt);
					timex2.ANCHOR_VAL = timebuf;
					(void) swprintf (timebuf, TP_BUFSIZE, L"PT%dS", head);
				}
				else if (duration > 0) {
					timex2.ANCHOR_DIR = L"STARTING";
					format_time (timebuf, pt);
					timex2.ANCHOR_VAL = timebuf;
					(void) swprintf (timebuf, TP_BUFSIZE, L"PT%dS", head);
				}
				else if (dir < 0) {
					if (head > 0) {
						pt -= boost::posix_time::seconds(head);
						format_time (timebuf, pt);
					}
					else {
						timex2.ANCHOR_DIR = L"BEFORE";
						format_time (timebuf, pt);
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"PAST_REF");
					}
				}
				else if (dir > 0) {
					if (head > 0) {
						pt += boost::posix_time::seconds(head);
						format_time (timebuf, pt);
					}
					else {
						timex2.ANCHOR_DIR = L"AFTER";
						format_time (timebuf, pt);
						timex2.ANCHOR_VAL = timebuf;
						(void) swprintf (timebuf, TP_BUFSIZE, L"FUTURE_REF");
					}
				}
				else if (head > 0) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"PT%dS", head);
				}
				else {
					(void) swprintf (timebuf, TP_BUFSIZE, L"PTXS");
				}
			}
		}
	}
	timex2.VAL = timebuf;
}

static void
normalizer17 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer17: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};

	wstring tp (matches.str(6));
	wstring unit = non_time_duration_unit(tp);

	int whole = 0;
	if (matches.length(1) > 0) 
		whole = _wtoi (matches.str(1).c_str());

	int num = _wtoi (matches.str(4).c_str());
	int den = _wtoi (matches.str(5).c_str());
	if (num == 1 && den == 2) { // half
		if (whole > 0) 
			(void) swprintf (timebuf, TP_BUFSIZE, L"P%d.5%ls", whole, unit.c_str());
		else
			(void) swprintf (timebuf, TP_BUFSIZE, L"P.5%ls", unit.c_str());
	}
	else if (num == 3 && den == 4) { // 3/4
		if (whole > 0)
			(void) swprintf (timebuf, TP_BUFSIZE, L"P%d.75%ls", whole, unit.c_str());
		else
			(void) swprintf (timebuf, TP_BUFSIZE, L"P.75%ls", unit.c_str());
	}
	else if (num == 1 && den == 3) {
		if (whole > 0)
			(void) swprintf (timebuf, TP_BUFSIZE, L"P%d.3%ls", whole, unit.c_str());
		else
			(void) swprintf (timebuf, TP_BUFSIZE, L"P.3%ls", unit.c_str());
	}
	else if (num == 1 && den == 4) {
		if (whole > 0)
			(void) swprintf (timebuf, TP_BUFSIZE, L"P%d.25%ls", whole, unit.c_str());
		else
			(void) swprintf (timebuf, TP_BUFSIZE, L"P.25%ls", unit.c_str());
	}
	else if (den != 0) {
		if (whole > 0)
			(void) swprintf (timebuf, TP_BUFSIZE, L"P%d%.1f%ls", whole, 
							static_cast<float>(num)/den, unit.c_str());
		else
			(void) swprintf (timebuf, TP_BUFSIZE, L"P%.1f%ls", static_cast<float>(num)/den, 
							unit.c_str());
	}

	timex2.VAL = timebuf;
}

static void
normalizer18 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer18: " << matches.str() << endl;
#endif
	int day (_wtoi (matches.str(1).c_str()));
	int year (_wtoi (matches.str(15).c_str()));
	wchar_t timebuf[TP_BUFSIZE] = {0};
	(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", year, 
					month_value (matches.str(2).c_str()), day);
	timex2.VAL = timebuf;
}

static void
normalizer19 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer19: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};

	int start = _wtoi (matches.str(1).c_str());
	int end = _wtoi (matches.str(3).c_str());
	if (matches.length(3) < matches.length(1)) { // e.g., 1992-5
		wstring str (matches.str(1).substr(0, 
							matches.length(1) - matches.length(3)));
		str += matches.str(3);
		end = _wtoi (str.c_str());
	}
	(void) swprintf (timebuf, TP_BUFSIZE, L"P%dY", end - start);
	timex2.VAL = timebuf;
	(void) swprintf (timebuf, TP_BUFSIZE, L"%4d", start);
	timex2.ANCHOR_VAL = timebuf;
	timex2.ANCHOR_DIR = L"STARTING";
}

static void
normalizer20 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer20: " << matches.str() << endl;
#endif
	wstring tp (matches.str(99));
	wstring unit (non_time_duration_unit (tp));
	wchar_t timebuf[TP_BUFSIZE] = {0};

	const wchar_t *prefix = L"P";
	if (unit == L"") {
		if (wcsncasecmp (tp.c_str(), L"hour", 4) == 0)
			unit = L"H";
		else if (wcsncasecmp (tp.c_str(), L"minute", 6) == 0)
			unit = L"M";
		else if (wcsncasecmp (tp.c_str(), L"second", 6) == 0)
			unit = L"S";
		prefix = L"PT";
	}

	int whole (0), num (0), den (0);
	if (matches.length(2) > 0) {
		whole = _wtoi (matches.str(3).c_str());
		num = wnum_value (matches.str(5));
		if (wcsncasecmp (matches.str(24).c_str(), L"half", 4) == 0) {
			den = 2;
		}
		else {
			den = wnum_value (matches.str(24));
		}
	}
	else if (matches.length(42) > 0) {
		whole = wnum_value (matches.str(43));
		num = wnum_value (matches.str(62));
		if (wcsncasecmp (matches.str(81).c_str(), L"half", 4) == 0) {
			den = 2;
		}
		else {
			den = wnum_value (matches.str(81));
		}
	}

	if (whole > 0) {
		if (num == 1 && den == 2) {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%ls%d.5%ls", prefix, whole, unit.c_str());
		}
		else if (num == 3 && den == 4) {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%ls%d.75%ls", prefix, whole, unit.c_str());
		}
		else if (den != 0) {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%ls%.2f%ls", prefix, static_cast<double>(whole) 
						+ static_cast<double>(num)/den, unit.c_str());
		}
	}
	else { // huh? who would type 0 and two third??
		if (num == 1 && den == 2) {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%ls.5%ls", prefix, unit.c_str());
		}
		else if (num == 3 && den == 4) {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%ls%.75%ls", prefix, unit.c_str());
		}
		else if (den != 0) {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%ls%.2f%ls", prefix,
							static_cast<double>(num)/den, unit.c_str());
		}
	}
	timex2.VAL = timebuf;
}

static void
normalizer21 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer21: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};
	wstring nst (matches.str(1));
	wstring unit (non_time_duration_unit (nst));
	if (unit.length() == 0) { // must be time
		if (wcsncasecmp (nst.c_str(), L"hour", 4) == 0)
			unit = L"H";
		else if (wcsncasecmp (nst.c_str(), L"minute", 6) == 0)
			unit = L"M";
		else if (wcsncasecmp (nst.c_str(), L"second", 6) == 0)
			unit = L"S";
		(void) swprintf (timebuf, TP_BUFSIZE, L"PT1.5%ls", unit.c_str());
	}
	else {
		(void) swprintf (timebuf, TP_BUFSIZE, L"P1.5%ls", unit.c_str());
	}
	timex2.VAL = timebuf;
}

static void
normalizer22 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer22: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};

	int len = 0;
	if (matches.length(3) > 0) {
		len = _wtoi (matches.str(3).c_str());
	}
	else if (matches.length(5) > 0) {
		len = wnum_value (matches.str(5));
	}
	else {
		// huh?
	}

	if (len > 0) {
		boost::gregorian::date date = timex2.anchor.date();
		(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d",
						static_cast<int>(date.year()),
						static_cast<int>(date.month()),
						static_cast<int>(date.day()));
		timex2.ANCHOR_VAL = timebuf;
		timex2.ANCHOR_DIR = L"ENDING";
		(void) swprintf (timebuf, TP_BUFSIZE, L"P%dY", len);
	}
	else {
		(void) swprintf (timebuf, TP_BUFSIZE, L"PXY");
	}
	timex2.VAL = timebuf;
}


static void
normalizer23 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer23: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};

	wstring nst (matches.str(1));
	boost::posix_time::ptime ref 
		(non_specific_time_normalizer (timex2.anchor, nst.c_str()));
	(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dT%ls", 
					static_cast<int>(ref.date().year()),
					static_cast<int>(ref.date().month()), 
					static_cast<int>(ref.date().day()),
					format_non_specific_time (matches.str(2)).c_str());
	timex2.VAL = timebuf;
}


static void
normalizer24 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer24: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};

	int len (0);
	if (matches.length(3) > 0) { // 3rd
		len = _wtoi (matches.str(3).c_str());
	}
	else if (matches.length(5) > 0) { // third
		len = wnum_value (matches.str(5));
	}
	else {
	}

	if (len > 0) {
		timex2.ANCHOR_DIR = L"ENDING";

		wstring tp (matches.str(24));
		const wchar_t *unit (L"");
		if (wcsncasecmp (tp.c_str(), L"year", 4) == 0) {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", 
						static_cast<int>(timex2.anchor.date().year()));
			timex2.ANCHOR_VAL = timebuf;
			unit = L"Y";
		}
		else if (wcsncasecmp (tp.c_str(), L"month", 5) == 0) {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d", 
						static_cast<int>(timex2.anchor.date().year()),
						static_cast<int>(timex2.anchor.date().month()));
			timex2.ANCHOR_VAL = timebuf;
			unit = L"M";
		}
		else if (wcsncasecmp (tp.c_str(), L"day", 3) == 0) {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", 
						static_cast<int>(timex2.anchor.date().year()),
						static_cast<int>(timex2.anchor.date().month()),
						static_cast<int>(timex2.anchor.date().day()));
			timex2.ANCHOR_VAL = timebuf;
			unit = L"D";
		}
		else if (wcsncasecmp (tp.c_str(), L"week", 4) == 0) {
			if (wcsncasecmp (tp.c_str(), L"weekend", 7) == 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%02d-WE", 
							static_cast<int>(timex2.anchor.date().year()),
							timex2.anchor.date().week_number()-1);
				unit = L"WE";
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%02d", 
							static_cast<int>(timex2.anchor.date().year()),
							timex2.anchor.date().week_number()-1);
				unit = L"W";
			}
			timex2.ANCHOR_VAL = timebuf;
		}

		(void) swprintf (timebuf, TP_BUFSIZE, L"P%d%ls", len, unit);
	}
	timex2.VAL = timebuf;
}

static void
normalizer25 (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"normalizer25: " << matches.str() << endl;
#endif
	wchar_t timebuf[TP_BUFSIZE] = {0};

	boost::gregorian::date date (timex2.anchor.date());

	// end of XXXX [XXXX]
	timex2.MOD = L"END";
	if (matches.length(19) > 0) {
		wstring mod (matches.str(4));
		wstring period (matches.str(19));

		int dir = 0;
		if (wcsncasecmp (mod.c_str(), L"this", 4) == 0
			|| wcsncasecmp (mod.c_str(), L"the", 3) == 0) {
		}
		else if (wcsncasecmp (mod.c_str(), L"last", 4) == 0
				|| wcsncasecmp (mod.c_str(), L"previous", 8) == 0) {
			dir = -1;
		}
		else if (wcsncasecmp (mod.c_str(), L"next", 4) == 0
			|| wcsncasecmp (mod.c_str(), L"coming", 6) == 0) {
			dir = 1;
		}

		if (wcsncasecmp (period.c_str(), L"centur", 6) == 0) {
			if (dir > 0)
				date += boost::gregorian::years(100);
			else if (dir < 0)
				date -= boost::gregorian::years(100);
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", static_cast<int>(date.year()));
			timebuf[2] = 0; // chop of the last two numbers
		}
		else if (wcsncasecmp (period.c_str(), L"decade", 6) == 0) {
			if (dir > 0)
				date += boost::gregorian::years(10);
			else if (dir < 0)
				date -= boost::gregorian::years(10);
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", static_cast<int>(date.year()));
			timebuf[3] = 0; // chop of the last number
		}
		else if (wcsncasecmp (period.c_str(), L"year", 4) == 0) {
			if (dir > 0)
				date += boost::gregorian::years(1);
			else if (dir < 0)
				date -= boost::gregorian::years(1);
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", static_cast<int>(date.year()));
		}
		else if (wcsncasecmp (period.c_str(), L"month", 5) == 0) {
			if (dir > 0)
				date += boost::gregorian::months(1);
			else if (dir < 0)
				date -= boost::gregorian::months(1);
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d",
							static_cast<int>(date.year()),
							static_cast<int>(date.month()));
		}
		else if (wcsncasecmp (period.c_str(), L"week", 4) == 0) {
			if (dir > 0)
				date += boost::gregorian::weeks(1);
			else if (dir < 0)
				date -= boost::gregorian::weeks(1);
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%02d", 
							static_cast<int>(date.year()),
							date.week_number());
		}
		/*
		else if (wcsncasecmp (period.c_str(), L"day", 3) == 0) {
			if (dir > 0)
				date += boost::gregorian::days(1);
			else if (dir < 0)
				date -= boost::gregorian::days(1);
		}
		else if (wcsncasecmp (period.c_str(), L"hour", 4) == 0) {
		}
		*/
	}
	else if (matches.length(7) > 0) {
		(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d", static_cast<int>(date.year()),
						month_value (matches.str(7).c_str()));
	}
	else if (matches.length(6) > 0) {
		// specific year
		int year = _wtoi (matches.str(5).c_str());

		if (matches.length(5) == 2) {
			if (year < 10) year += 2000; 
			else year += 1999;
		}

		(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", year);
	}
	timex2.VAL = timebuf;
}

static void
generic_normalizer (temporal_timex2& timex2, boost::wcmatch& matches)
{
#ifdef TP_DEBUG
	wcout << L"generic_normalizer: " << matches.str() << endl;
#endif

	wchar_t timebuf[TP_BUFSIZE] = {0};
	boost::posix_time::ptime date (timex2.anchor);
	boost::gregorian::date::ymd_type ymd = date.date().year_month_day();
	wstring postfix (matches.str(84));

	if (matches.length(17) > 0) { // TP_TIME
		wstring s = format_time (matches.str(17));
		const wchar_t *tz (L"");
		if (wcsncasecmp (postfix.c_str(), L"est", 3) == 0
			|| wcsncasecmp (postfix.c_str(), L"-05", 3) == 0)
			tz = L"-05";
		else if (wcsncasecmp (postfix.c_str(), L"gmt", 3) == 0
				|| wcsncasecmp (postfix.c_str(), L"green", 5) == 0)
			tz = L"Z";
		else if (wcsncasecmp (postfix.c_str(), L"pst", 3) == 0)
			tz = L"-08";
#if 1
		(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dT%ls%ls", static_cast<int>(ymd.year),
						static_cast<int>(ymd.month), static_cast<int>(ymd.day),
						s.c_str(), tz);
#else
		(void) swprintf (timebuf, TP_BUFSIZE, L"T%ls", s.c_str());
#endif
	}
	else if (matches.length(51) > 0) { // TP_DAY_OF_WEEK
		wstring day (matches.str(51));

		int dir = 0; // directionality
	    bool is_plural = wcsncmp (day.substr(day.length()-1).c_str(), L"s", 1) == 0;
		if (matches.length(50) > 0) {
			wstring mod (matches.str(50));
			if (wcsncasecmp (mod.c_str(), L"each", 4) == 0
				|| wcsncasecmp (mod.c_str(), L"every", 5) == 0
				|| wcsncasecmp (mod.c_str(), L"per", 3) == 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"XXXX-WXX-%d", day_of_week_value (day));
				timex2.SET = L"YES";
			}
			else if (wcsncasecmp (mod.c_str(), L"next", 4) == 0
					|| wcsncasecmp (mod.c_str(), L"following", 9) == 0
					|| wcsncasecmp (mod.c_str(), L"coming", 6) == 0) {
				dir = 1; // forward
			}
			else {
				dir = -1; // back reference
			}
		}

		if (is_plural) {
			if (dir < 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%02d", static_cast<int>(ymd.year),
								date.date().week_number()-1);
				timex2.ANCHOR_VAL = timebuf;
				timex2.ANCHOR_DIR = L"BEFORE";
			}
			else if (dir > 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%02d", static_cast<int>(ymd.year),
								date.date().week_number()-1);
				timex2.ANCHOR_VAL = timebuf;
				timex2.ANCHOR_DIR = L"AFTER";
			}
			else {
			}
			(void) swprintf (timebuf, TP_BUFSIZE, L"PXXXX-WXX-%d", day_of_week_value (day));
		}
		else /*if (dir != 0)*/ {
			if (dir > 0) {
				date += boost::gregorian::weeks(1);
			}
			else if (dir < 0) {
				date -= boost::gregorian::weeks(1);
			}
		
			// let the normalizer determine the directionality if 
			//   we don't have one
			date = day_of_week_normalizer (date, day.c_str(), dir);			
			ymd = date.date().year_month_day();
#if 1
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", static_cast<int>(ymd.year), 
							static_cast<int>(ymd.month), static_cast<int>(ymd.day));
#else
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%d-%d", static_cast<int>(ymd.year),
							date.date().week_number(), day_of_week_value (day));
#endif
		}
		/*
		else {
			(void) swprintf (timebuf, TP_BUFSIZE, L"PXXXX-WXX-%d", day_of_week_value (day));
		}
		*/
	}
	else if (matches.length(78) > 0) { // TP_NON_SPECIFIC_TIME
		wstring nst (matches.str(78));
		const wchar_t *part_of_day = 0;
		if (wcsncasecmp (nst.c_str(), L"night", 5) == 0
			|| wcsncasecmp (nst.c_str(), L"tonight", 7) == 0)
			part_of_day = L"NI";
		else if	(wcsncasecmp (nst.c_str(), L"morning", 7) == 0)
			part_of_day = L"MO";
		else if (wcsncasecmp (nst.c_str(), L"afternoon",9) == 0)
			part_of_day = L"AF";
		else if (wcsncasecmp (nst.c_str(), L"evening", 7) == 0)
			part_of_day = L"EV";
		else if (wcsncasecmp (nst.c_str(), L"midday", 6) == 0)
			part_of_day = L"MI";
		else if (wcsncasecmp (nst.c_str(), L"midnight", 8) == 0)
			part_of_day = L"24:00";

		date = non_specific_time_normalizer (date, nst.c_str());
		wstring mod (matches.str(77));
		if (wcsncasecmp (mod.c_str(), L"last", 4) == 0
			|| wcsncasecmp (mod.c_str(), L"previous", 8) == 0)
			date -= boost::gregorian::days(1);
		else if (matches.length(77) > 0)
			date += boost::gregorian::days(1);

		ymd = date.date().year_month_day();
		if (wcsncasecmp (nst.c_str(), L"now", 3) == 0) {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", static_cast<int>(ymd.year), 
							static_cast<int>(ymd.month), static_cast<int>(ymd.day)); 
			timex2.ANCHOR_VAL = timebuf;
			timex2.ANCHOR_DIR = L"AS_OF";
			(void) swprintf (timebuf, TP_BUFSIZE, L"PRESENT_REF");
		}
		else if (part_of_day) {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dT%ls", static_cast<int>(ymd.year), 
							static_cast<int>(ymd.month), static_cast<int>(ymd.day), 
							part_of_day);
		}
		else {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", static_cast<int>(ymd.year), 
							static_cast<int>(ymd.month), static_cast<int>(ymd.day));
		}
	}
	else if (matches.length(59) > 0) { //TP_YEAR
		wstring yr (matches.str(59));
		if (wcsncmp (yr.substr(yr.length()-1).c_str(), L"s", 1) == 0) {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%ls", yr.substr(0,3).c_str()); //1960s => 196
		}
		else if (matches.length(59) == 4) {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", _wtoi (yr.c_str()));
		}
		else if (matches.length(59) == 2) {
			int year = _wtoi (yr.c_str());
			if (year < 10) {
				year += 2000;
			}
			else {
				year += 1900;
			}
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", year);
		}
	}
	else if (matches.length(5) > 0) { //TP_MON
		wstring mod (matches.str(4));
		if (wcsncasecmp (mod.c_str(), L"last", 4) == 0
			|| wcsncasecmp (mod.c_str(), L"previous", 8) == 0)
			date -= boost::gregorian::years(1);
		else if (mod.length() > 0) {
			date += boost::gregorian::years(1);
		}
		(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d", static_cast<int>(date.date().year()),
						month_value (matches.str(5).c_str()));
	}
	else if (matches.length(63) > 0) { //TP_TIME_PERIOD
		wstring nst (matches.str(63));
		wstring mod (matches.str(62));

		int dir = 0;
		int dur = 0; // duration
		bool plural (wcsncasecmp (nst.substr(nst.length()-1).c_str(), L"s", 1) == 0);

		if (wcsncasecmp (mod.c_str(), L"last", 4) == 0
			|| wcsncasecmp (mod.c_str(), L"previous", 8) == 0
			|| wcsncasecmp (postfix.c_str(), L"ago", 3) == 0) {
			dir = -1;
		}
		else if (wcsncasecmp (mod.c_str(), L"every", 5) == 0
				|| wcsncasecmp (mod.c_str(), L"each", 4) == 0
				|| wcsncasecmp (mod.c_str(), L"per", 3) == 0
				|| (nst.length() > 2 && nst.substr(nst.length()-2) == L"ly")) {
			timex2.SET = L"YES";
			plural = true;
		}
		else if (wcsncasecmp (mod.c_str(), L"another", 7) == 0
				|| wcsncasecmp (mod.c_str(), L"within", 7) == 0
				|| wcsncasecmp (mod.c_str(), L"in", 2) == 0) {
			dur = 1;
		}
		else if (wcscasecmp (mod.c_str(), L"a") == 0
				|| wcsncasecmp (mod.c_str(), L"past", 4) == 0) {
			dur = -1;
		}
		else if (mod.length() > 0 
				/*&& !(wcsncasecmp (mod.c_str(), L"the", 3) == 0)*/) {
			dir = 1;
		}
		else {
		}

		if (wcsncasecmp (nst.c_str(), L"centur", 6) == 0) {
			int cen = static_cast<int>(date.date().year())/100 + 1;
			if (wcsncasecmp (mod.c_str(), L"this", 4) == 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%02d", cen);
			}
			else if (dur) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%02d", cen);
				timex2.ANCHOR_VAL = timebuf;
				timex2.ANCHOR_DIR = L"STARTING";
				(void) swprintf (timebuf, TP_BUFSIZE, L"P1CE");
			}
			else if (dir) {
				if (dir < 0) --cen;
				else if (dir > 0) ++cen;
				(void) swprintf (timebuf, TP_BUFSIZE, L"%02d", cen);
			}
			else { 
				(void) swprintf (timebuf, TP_BUFSIZE, L"PXCE");
			}
		}
		else if (wcsncasecmp (nst.c_str(), L"millen", 6) == 0) {
			int mil = static_cast<int>(date.date().year())/1000;
			if (wcsncasecmp (mod.c_str(), L"this", 4) == 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%1d", mil);
			}
			else if (dur) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%d", mil);
				timex2.ANCHOR_VAL = timebuf;
				timex2.ANCHOR_DIR = dur > 0 ? L"STARTING" : L"ENDING";
				(void) swprintf (timebuf, TP_BUFSIZE, L"P1ML");
			}
			else if (dir) {
				if (dir < 0) --mil;
				else if (dir > 0) ++mil;
				(void) swprintf (timebuf, TP_BUFSIZE, L"%1d", mil);
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, L"PXML");
			}
		}
		else if (wcsncasecmp (nst.c_str(), L"decade", 6) == 0) {
			if (wcsncasecmp (mod.c_str(), L"this", 4) == 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%03d", static_cast<int>(date.date().year())/10);
			}
			else if (dir || dur) {
				if (dir < 0)
					date -= boost::gregorian::years(10);
				else if (dir > 0)
					date += boost::gregorian::years(10);
				(void) swprintf (timebuf, TP_BUFSIZE, L"%03d", static_cast<int>(date.date().year())/10);
				if (dur) {
					timex2.ANCHOR_VAL = timebuf;
					timex2.ANCHOR_DIR = dur > 0 ? L"STARTING" : L"ENDING";
					(void) swprintf (timebuf, TP_BUFSIZE, L"P1DE");
				}
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, L"PXDE");
			}
		}
		else if (wcsncasecmp (nst.c_str(), L"month", 5) == 0
				|| wcsncasecmp (nst.c_str(), L"mnth", 4) == 0) {
			if (wcsncasecmp (mod.c_str(), L"this", 4) == 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d", 
								static_cast<int>(date.date().year()),
								static_cast<int>(date.date().month()));
			}
			else if (wcsncasecmp (mod.c_str(), L"recent", 6) == 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d", 
								static_cast<int>(date.date().year()),
								static_cast<int>(date.date().month()));
				timex2.ANCHOR_VAL = timebuf;
				timex2.ANCHOR_DIR = L"AS_OF";
				(void) swprintf (timebuf, TP_BUFSIZE, L"PRESENT_REF");
			}
			else if (wcsncasecmp (mod.c_str(), L"several", 7) == 0) {
				if (wcsncasecmp (postfix.c_str(), L"later", 5) == 0) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-XX", 
									static_cast<int>(date.date().year()));
				}
				else {
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d", 
									static_cast<int>(date.date().year()),
									static_cast<int>(date.date().month()));
					timex2.ANCHOR_VAL = timebuf;
					timex2.ANCHOR_DIR = L"BEFORE";
					(void) swprintf (timebuf, TP_BUFSIZE, L"PAST_REF");
				}
			}
			else if (dir || dur) {
				if (plural) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d", 
									static_cast<int>(date.date().year()),
									static_cast<int>(date.date().month()));
					timex2.ANCHOR_VAL = timebuf;
					timex2.ANCHOR_DIR = dur > 0 ? L"STARTING" : L"ENDING";
					(void) swprintf (timebuf, TP_BUFSIZE, L"PXM");
				}
				else {
					if (dir < 0)
						date -= boost::gregorian::months(1);
					else if (dir > 0)
						date += boost::gregorian::months(1);
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d", static_cast<int>(date.date().year()),
									static_cast<int>(date.date().month()));
					if (dur) {
						timex2.ANCHOR_VAL = timebuf;
						timex2.ANCHOR_DIR = dur > 0 ? L"STARTING" : L"ENDING";
						(void) swprintf (timebuf, TP_BUFSIZE, L"P1M");
					}
				}
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, timex2.SET.length() > 0 || !plural ? L"XXXX-XX" : L"PXM");
			}
		}
		else if (wcsncasecmp (nst.c_str(), L"week", 4) == 0
				|| wcsncasecmp (nst.c_str(), L"wk", 2) == 0) {
			bool weekend (wcsncasecmp (nst.c_str(), L"weekend", 7) == 0);
			if (wcsncasecmp (mod.c_str(), L"this", 4) == 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%02d", 
								static_cast<int>(date.date().year()),
								date.date().week_number()-1);
			}
			else if (wcsncasecmp (mod.c_str(), L"recent", 6) == 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%02d", 
								static_cast<int>(date.date().year()),
								date.date().week_number()-1);
				timex2.ANCHOR_VAL = timebuf;
				timex2.ANCHOR_DIR = L"AS_OF";
				(void) swprintf (timebuf, TP_BUFSIZE, L"PRESENT_REF");
			}
			else if (wcsncasecmp (mod.c_str(), L"several", 7) == 0) {
				if (wcsncasecmp (postfix.c_str(), L"later", 5) == 0) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-WXX", 
									static_cast<int>(date.date().year()));
				}
				else {
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-W%02d", 
									static_cast<int>(date.date().year()),
									date.date().week_number()-1);
					timex2.ANCHOR_VAL = timebuf;
					timex2.ANCHOR_DIR = L"BEFORE";
					(void) swprintf (timebuf, TP_BUFSIZE, L"PAST_REF");
				}
			}
			else if (dir || dur) {
				if (plural) {
					(void) swprintf (timebuf, TP_BUFSIZE, weekend ? L"%04d-W%02d-WE" : L"%04d-W%02d", 
									static_cast<int>(date.date().year()),
									date.date().week_number()-1);
					timex2.ANCHOR_VAL = timebuf;
					timex2.ANCHOR_DIR = dur > 0 ? L"STARTING" : L"ENDING";
					(void) swprintf (timebuf, TP_BUFSIZE, weekend ? L"PXWE" : L"PXW");
				}
				else {
					if (dir < 0)
						date -= boost::gregorian::weeks(1);
					else if (dir > 0)
						date += boost::gregorian::weeks(1);
					(void) swprintf (timebuf, TP_BUFSIZE, weekend ? L"%04d-W%02d-WE" : L"%04d-W%02d", 
									static_cast<int>(date.date().year()),
									date.date().week_number()-1);
					if (dur) {
						timex2.ANCHOR_VAL = timebuf;
						timex2.ANCHOR_DIR = dur > 0 ? L"STARTING" : L"ENDING";
						(void) swprintf (timebuf, TP_BUFSIZE, weekend ? L"P1WE" : L"P1W");
					}
				}
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, timex2.SET.length() > 0 || !plural ?  
									(weekend ? L"XXXX-WXX-WE" : L"XXXX-WXX") : L"PXW");
			}
		}
		else if (wcsncasecmp (nst.c_str(), L"day", 3) == 0) {
			if (wcsncasecmp (mod.c_str(), L"final", 5) == 0
				|| wcsncasecmp (mod.c_str(), L"that", 4) == 0
				|| wcsncasecmp (mod.c_str(), L"some", 4) == 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", 
								static_cast<int>(date.date().year()),
								static_cast<int>(date.date().month()), 
								static_cast<int>(date.date().day()));
				timex2.ANCHOR_VAL = timebuf;
				timex2.ANCHOR_DIR = L"AFTER";
				if (plural) {
					timex2.SET = L"YES";
				}
				(void) swprintf (timebuf, TP_BUFSIZE, L"FUTURE_REF");
			}
			else if (wcsncasecmp (mod.c_str(), L"recent", 6) == 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", 
								static_cast<int>(date.date().year()),
								static_cast<int>(date.date().month()), 
								static_cast<int>(date.date().day()));
				timex2.ANCHOR_VAL = timebuf;
				timex2.ANCHOR_DIR = L"AS_OF";
				(void) swprintf (timebuf, TP_BUFSIZE, L"PRESENT_REF");
			}
			else if (wcsncasecmp (mod.c_str(), L"several", 7) == 0) {
				if (wcsncasecmp (postfix.c_str(), L"later", 5) == 0) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-XX", 
									static_cast<int>(date.date().year()),
									static_cast<int>(date.date().month()));
				}
				else {
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", 
									static_cast<int>(date.date().year()),
									static_cast<int>(date.date().month()),
									static_cast<int>(date.date().day()));
					timex2.ANCHOR_VAL = timebuf;
					timex2.ANCHOR_DIR = L"BEFORE";
					(void) swprintf (timebuf, TP_BUFSIZE, L"PAST_REF");
				}
			}
			else if (dir || dur) {
				if (plural) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", 
									static_cast<int>(date.date().year()),
									static_cast<int>(date.date().month()),
									static_cast<int>(date.date().day()));
					timex2.ANCHOR_VAL = timebuf;
					timex2.ANCHOR_DIR = dur > 0 ? L"STARTING" : L"ENDING";
					(void) swprintf (timebuf, TP_BUFSIZE, L"PXD");
				}
				else {
					if (dir < 0)
						date -= boost::gregorian::days(1);
					else if (dir > 0)
						date += boost::gregorian::days(1);
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02d", 
									static_cast<int>(date.date().year()),
									static_cast<int>(date.date().month()), 
									static_cast<int>(date.date().day()));
					if (dur) {
						timex2.ANCHOR_VAL = timebuf;
						timex2.ANCHOR_DIR = dur > 0 ? L"STARTING" : L"ENDING";
						(void) swprintf (timebuf, TP_BUFSIZE, L"P1D");
					}
				}
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, timex2.SET.length() > 0 || !plural 
									? L"XXXX-XX-XX" : L"PXD");
			}
		}
		else if (wcsncasecmp (nst.c_str(), L"year", 4) == 0
				|| wcsncasecmp (nst.c_str(), L"yr", 2) == 0) {
			if (wcsncasecmp (mod.c_str(), L"this", 4) == 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", 
								static_cast<int>(date.date().year()));
			}
			else if (wcsncasecmp (mod.c_str(), L"recent", 6) == 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", static_cast<int>(date.date().year()));
				timex2.ANCHOR_VAL = timebuf;
				timex2.ANCHOR_DIR = L"AS_OF";
				(void) swprintf (timebuf, TP_BUFSIZE, L"PRESENT_REF");
			}
			else if (wcsncasecmp (mod.c_str(), L"several", 7) == 0) {
				if (wcsncasecmp (postfix.c_str(), L"later", 5) == 0) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"XXXX");
				}
				else {
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", 
									static_cast<int>(date.date().year()));
					timex2.ANCHOR_VAL = timebuf;
					timex2.ANCHOR_DIR = L"BEFORE";
					(void) swprintf (timebuf, TP_BUFSIZE, L"PAST_REF");
				}
			}
			else if (dir || dur) {
				if (plural) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", 
									static_cast<int>(date.date().year()));
					timex2.ANCHOR_VAL = timebuf;
					timex2.ANCHOR_DIR = dur > 0 ? L"STARTING" : L"ENDING";
					(void) swprintf (timebuf, TP_BUFSIZE, L"PXY");
				}
				else {
					if (dir < 0)
						date -= boost::gregorian::years(1);
					else if (dir > 0)
						date += boost::gregorian::years(1);
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d", 
									static_cast<int>(date.date().year()));
					if (dur) {
						timex2.ANCHOR_VAL = timebuf;
						timex2.ANCHOR_DIR = dur > 0 ? L"STARTING" : L"ENDING";
						(void) swprintf (timebuf, TP_BUFSIZE, L"P1Y");
					}
				}
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, timex2.SET.length() > 0 || !plural ? L"XXXX" : L"PXY");
			}
		}
		else if (wcsncasecmp (nst.c_str(), L"hour", 4) == 0
				|| wcsncasecmp (nst.c_str(), L"hr", 2) == 0) {
			if (wcsncasecmp (mod.c_str(), L"last", 4) == 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dTXX", 
								static_cast<int>(date.date().year()),
								static_cast<int>(date.date().month()), 
								static_cast<int>(date.date().day()));
			}			
			else if (wcsncasecmp (mod.c_str(), L"several", 7) == 0) {
				if (wcsncasecmp (postfix.c_str(), L"later", 5) == 0) {
					(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dTXX", 
									static_cast<int>(date.date().year()),
									static_cast<int>(date.date().month()),
									static_cast<int>(date.date().day()));
				}
				else {
					format_time (timebuf, date);
					timex2.ANCHOR_VAL = timebuf;
					timex2.ANCHOR_DIR = L"BEFORE";
					(void) swprintf (timebuf, TP_BUFSIZE, L"PAST_REF");
				}
			}
			else if (dir || dur) {
				if (plural) {
					format_time (timebuf, timex2.anchor);
					timex2.ANCHOR_VAL = timebuf;
					timex2.ANCHOR_DIR = dur > 0 ? L"STARTING" : L"ENDING";
					(void) swprintf (timebuf, TP_BUFSIZE, L"PTXH");
				}
				else {
					if (dir < 0)
						date -= boost::posix_time::hours(1);
					else if (dir > 0)
						date += boost::posix_time::hours(1);
					format_time (timebuf, date);
					if (dur) {
						timex2.ANCHOR_VAL = timebuf;
						timex2.ANCHOR_DIR = dur > 0 ? L"STARTING" : L"ENDING";
						(void) swprintf (timebuf, TP_BUFSIZE, L"PT1H");
					}
				}
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, timex2.SET.length() > 0 || !plural ? L"TXX" : L"PTXH");
			}
		}
		else if (wcsncasecmp (nst.c_str(), L"minute", 6) == 0
				|| wcsncasecmp (nst.c_str(), L"min", 3) == 0) {
			if (wcsncasecmp (mod.c_str(), L"last", 4) == 0) {
				(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%02d-%02dTXX:XX", 
								static_cast<int>(date.date().year()),
								static_cast<int>(date.date().month()), 
								static_cast<int>(date.date().day()));
			}
			else if (dir || dur) {
				if (dir < 0)
					date -= boost::posix_time::minutes(1);
				else if (dir > 0)
					date += boost::posix_time::minutes(1);
				format_time (timebuf, date);
				if (dur) {
					timex2.ANCHOR_VAL = timebuf;
					timex2.ANCHOR_DIR = dur > 0 ? L"STARTING" : L"ENDING";
					(void) swprintf (timebuf, TP_BUFSIZE, L"PT1M");
				}
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, L"PTXM");
			}
		}
		else if (wcsncasecmp (nst.c_str(), L"second", 6) == 0
				|| wcsncasecmp (nst.c_str(), L"sec", 3) == 0) {
			if (dir || dur) {
				if (dir < 0)
					date -= boost::posix_time::seconds(1);
				else if (dir > 0)
					date += boost::posix_time::seconds(1);
				format_time (timebuf, date);
				if (dur) {
					timex2.ANCHOR_VAL = timebuf;
					timex2.ANCHOR_DIR = dur > 0 ? L"STARTING" : L"ENDING";
					(void) swprintf (timebuf, TP_BUFSIZE, L"PT1S");
				}
			}
			else {
				(void) swprintf (timebuf, TP_BUFSIZE, L"PTXS");
			}
		}
		else if (wcsncasecmp (nst.c_str(), L"daily", 5) == 0) {
			(void) swprintf (timebuf, TP_BUFSIZE, L"XXXX-XX-XX");
		}
		else if (wcsncasecmp (nst.c_str(), L"annual", 6) == 0) {
			timex2.SET = L"YES";
			(void) swprintf (timebuf, TP_BUFSIZE, L"XXXX");
		}
		else if (wcsncasecmp (nst.c_str(), L"semiannual", 10) == 0) {
			timex2.SET = L"YES";
			(void) swprintf (timebuf, TP_BUFSIZE, L"XXXX-H2");
		}
	}
	else if (matches.length(82) > 0) {
		wstring sea (matches.str(82));
		wstring mod (matches.str(81));

		int dir = 0;
		if (wcsncasecmp (mod.c_str(), L"last", 4) == 0
			|| wcsncasecmp (mod.c_str(), L"previous", 8) == 0) {
			date -= boost::gregorian::years(1);
			dir = -1;
		}
		else if (mod.length() > 0) {
			date += boost::gregorian::years(1);
			dir = 1;
		}
			
		if (dir) {
			(void) swprintf (timebuf, TP_BUFSIZE, L"%04d-%ls", static_cast<int>(date.date().year()),
							format_season (sea).c_str());
		}
		else {
			// check for plural
			if (sea.substr(sea.length()-1) == L"s") {
				timex2.SET = L"YES";
			}
			(void) swprintf (timebuf, TP_BUFSIZE, L"XXXX-%ls", format_season (sea).c_str());
		}
	}
	timex2.VAL = timebuf;
}

boost::posix_time::ptime 
parse_anchor (const char *iso_expr)
{
	char semi_iso[TP_BUFSIZE*2] = {0};

	// reformat expr into something that's parsable by ptime
	size_t len (strlen (iso_expr));
	bool hasT (false);
	for (size_t i (0); i < len && i < sizeof (semi_iso); ++i) {
		if (iso_expr[i] != 'T') {
			semi_iso[i] = iso_expr[i];
		}
		else {
			semi_iso[i] = ' '; // replace T with a space so that it can be parsed
			hasT = true;
		}
	}

	string proper_expr (semi_iso);
	if (!hasT) {
		proper_expr += " 00:00:00";
	}

	return boost::posix_time::time_from_string(proper_expr);
#if 0
	wcout << "set_anchor in: " << iso_expr << " out: " << anchor << endl;
	cout << "proper: " << proper_expr << endl;
#endif
}

void temporal_timex2::set_anchor (const wstring& expr)
{
	if (normalize_temporal (*this, expr)) {
		wstring year = VAL.substr(0, 4);
		wstring mon = VAL.substr(5, 2);
		wstring day = VAL.substr(8, 2);
		boost::gregorian::date date (_wtoi(year.c_str()), 
									_wtoi(mon.c_str()), _wtoi(day.c_str()));
		if (VAL.length() > 10) {
			wstring hour = VAL.substr(11, 2);
			wstring min = VAL.substr(14, 2);
			if (VAL.length() > 14) {
				wstring sec = VAL.substr(17,2);
				this->anchor = boost::posix_time::ptime
					(date, boost::posix_time::hours(_wtoi(hour.c_str()))
					+ boost::posix_time::minutes(_wtoi (min.c_str()))
					+ boost::posix_time::seconds(_wtoi (sec.c_str())));
			}
			else {
				this->anchor = boost::posix_time::ptime(date,
					boost::posix_time::hours(_wtoi(hour.c_str()))
					+ boost::posix_time::minutes(_wtoi(min.c_str())));
			}
		}
		else {
			this->anchor = boost::posix_time::ptime(date);
		}
	}
}

/*
* array of regular expressions for matching temporal expressions
* the patterns in this array must be specified such that the most
* specific matches are on top
*/
static temporal_parser re_temporal[] = {
	temporal_parser (TP_MON 
	TP_SEP 
	TP_DAY 
	TP_SEP 
	TP_YEAR
	TP_SEP
	TP_TIME
	TP_SEP
	TP_ZONE, 
	normalizer1),

	temporal_parser (TP_TIME
	TP_SEP
	TP_MON
	TP_SEP
	TP_DAY
	TP_SEP
	TP_YEAR,
	normalizer2),

	temporal_parser (TP_DAY
	TP_SEP
	TP_MON
	TP_SEP
	TP_YEAR
	TP_SEP
	TP_TIME
	TP_SEP
	TP_ZONE,
	normalizer15),

	temporal_parser (TP_MON
	TP_SEP
	TP_DAY
	TP_SEP
	TP_YEAR,
	normalizer3),

	temporal_parser (L"(\\d{1,2}" TP_SEP L")?" TP_MON
	TP_SEP L"(of" TP_SEP L")?"
	TP_YEAR,
	normalizer4),

	temporal_parser (TP_MON
	TP_SEP
	TP_DAY, 
	normalizer5),

	temporal_parser (TP_DAY L"/" TP_DAY L"/" TP_YEAR 
	TP_SEP 
	TP_TIME,
	normalizer6),

	temporal_parser (TP_TIME L"(" TP_SEP L"(on|in|during))?" 
					TP_SEP TP_DAY L"/" TP_DAY L"/" TP_YEAR,
	normalizer6b),

	temporal_parser (TP_DAY L"/" TP_DAY L"/" TP_YEAR, 
	normalizer7),

	temporal_parser (L"(\\d{1,2})" 
					TP_SEP TP_MON 
					TP_SEP L"(\\d{4})",
					normalizer18),

	// fraction 
	temporal_parser (L"((\\d+)" TP_SEP L"(and" TP_SEP L")?)?(\\d{1})/(\\d{1})" 
					TP_SEP TP_TIME_PERIOD, 
					normalizer17),

	// year range
	temporal_parser (L"(\\d{4})[[:space:]]*(-|to|thru|through)[[:space:]]*(\\d{4})",
					normalizer19),

	// 04/03
	temporal_parser (L"(\\d{2})/(\\d{2})", normalizer8),

	temporal_parser (TP_SEASON TP_SEP L"(of" TP_SEP L")?" TP_YEAR,
	normalizer9),

	temporal_parser (L"(((\\d+)(st|nd|rd|th))|" TP_WNUM L")" TP_SEP L"(of" TP_SEP L")?" TP_MON,
	normalizer9b),

	temporal_parser (TP_FY L"(\\s*" TP_YEAR L"|\\d{2})?", 
	normalizer10),

	temporal_parser (TP_TIME L"\\s+" TP_NON_SPECIFIC_TIME,
	normalizer11),

	temporal_parser (TP_DAY_OF_WEEK L"[[:space:]]+" TP_NON_SPECIFIC_TIME,
	normalizer12),

	temporal_parser (TP_YEAR L"-" TP_DAY L"-" TP_DAY
	L"(([[:space:]]|T)" TP_TIME L"([[:space:]]?" TP_ZONE L")?)?", 
	normalizer13),

	temporal_parser (L"(\\d{4})([01]\\d)([0-3]\\d)(-" TP_TIME L")?", 
	normalizer14),

	temporal_parser (TP_FNUM TP_SEP TP_TIME_PERIOD, normalizer20),
			
	/*
	* eleventh century 
	* 2nd millennium
	* 210 million years ago
	* ten minutes to 3
	*/
	temporal_parser (L"((next|past|within|in)" TP_SEP L")?"
	L"(((\\d+)([,.](\\d+))?" TP_SEP L")?" TP_WNUM L"|" 
	L"(((\\d+)([,.](\\d+))?)(nd|st|th|rd)?))" 
	TP_SEP TP_TIME_PERIOD L"(" TP_SEP 
	L"(ago|ended|since|before|after|later|old|to|u?till?|earlier|long|from)(" 
	TP_SEP L"(" TP_WNUM L"|(\\d+)))?)?",
	normalizer16),

	temporal_parser (L"an?" TP_SEP TP_TIME_PERIOD 
					L"(" TP_SEP L"(and)" TP_SEP L"a" TP_SEP L"half)",
					normalizer21),

	temporal_parser (L"(his|her|theirs?|its|ours?|mine|yours?)" 
					TP_SEP L"((\\d+)(st|rd|th|nd)|" TP_WNUM L")(" TP_SEP L"(\\w+))?",
					normalizer22),

	temporal_parser (TP_NON_SPECIFIC_TIME TP_SEP TP_NON_SPECIFIC_TIME,
					normalizer23),

	temporal_parser (L"(((\\d+)(rd|st|th|nd))|" TP_WNUM L")" 
					TP_SEP L"(straight)" TP_SEP TP_TIME_PERIOD,
					normalizer24),

	temporal_parser (L"(end)" TP_SEP L"(of)" TP_SEP 
					L"((the|this|last|next|previous|coming)" TP_SEP L")?" 
					L"(" TP_YEAR L"|" TP_MON L"|" TP_TIME_PERIOD L")",
					normalizer25),

	temporal_parser (L"((((last|previous|next|following|coming)" TP_SEP L")?" TP_MON L")|" 
	TP_TIME L"|" 
	L"(((last|previous|past|next|following|coming|per|every|each|a)" 
	TP_SEP L")?" TP_DAY_OF_WEEK L")|" TP_YEAR L"|"
	L"(((last|previous|past|next|following|coming|per|"
	L"the|this|another|every|a|each|within|in|final|that|some|recent|several)" 
	TP_SEP L")?" TP_TIME_PERIOD L")|"
	L"(((the)" TP_SEP L")?" TP_NDAY L")|"
	L"(((last|previous|next|following|the|this|a|each|right)" TP_SEP L")?" TP_NON_SPECIFIC_TIME L")|"
	L"(((last|previous|next|following|the|this|another|every|a|each)" TP_SEP L")?" TP_SEASON 
	L"))(" TP_SEP L"(\\w+))?"
	,generic_normalizer)
};

bool
normalize_temporal (temporal_timex2& timex2, const wstring& expr)
{
	return normalize_temporal (timex2, expr.c_str());
}

bool
normalize_temporal (temporal_timex2& timex2, const wchar_t *expr)
{
	size_t size (sizeof (re_temporal)/sizeof (re_temporal[0]));
	for (unsigned i (0); i < size; ++i) {
		if (re_temporal[i].parse(expr, timex2))
			return true;
	}

	return false;
}


#ifdef __TP_MAIN
#include <fstream>

int
main (int argc, char *argv[])
{
	temporal_timex2 timex2;
	timex2.set_anchor(L"1999-07-15T12:39:02");

	// these are taken from the timex standard
	const wchar_t *test_cases[] = {
		L"several weeks ago",
		L"end of September",
		L"end of the century",
		L"the end of 2003",
		L"for at least a year",
		L"the next few weeks",
		L"a couple of days",
		L"just days after",
		L"late Wednesday",
		L"the last weeks",
		L"a year ago",
		L"last month",
		L"the third straight year",
		L"the 3rd straight year",
		L"the beginning of 2005",
		L"the end of this year",
		L"this year",
		L"2003-03-03T19:00:00-05:00",
		L"a long time",
		L"tomorrow night",
		L"next half hour",
		L"his 70th birthday",
		L"his ninth birthday",
		L"a few hours from now",
		L"the day",
		L"recent days",
		L"every day",
		L"the past few days",
		L"daily",
		L"late Wednesday",
		L"March 27",
		L"right now",
		L"the last minute", //VAL=2003-03-04TXX:XX
		L"a day of reckoning for the Iraqi regime",
		L"1:28pm on 11/12/04", //VAL=2004-11-12T13:28
		L"2 and one half minutes",
		L"one and two third years",
		L"3.1415926353 months",
		L"1770 to 1831",
		L"12 April 1966",
		L"11:00 a.m. on the east coast",
		L"10 AM on a Wednesday",
		L"2 1/2 day",
		L"04-03-03 02:16:00EST",
		L"04 Nov 2004",
		L"June of 1998",
		L"1999-07-15",
		L"1999-07-15T15:32:23",
		L"0627 GMT",
		L"12:27:05.032 aM",
		L"six pm",
		L"4pm",
		L"twelve o'clock",
		L"Thursday",
		L"yesterday",
		L"August 6",
		L"today",
		L"eleventh century", // VAL=10
		L"11th century", // VAL=10
		L"Thursday",//	 VAL=1999-07-22
		L"ahead",//	 VAL=FUTURE_REF ANCHOR_DIR=AFTER ANCHOR_VAL=1999-0722
		L"early last night",//	 VAL=2000-10-31TNI MOD=START
		L"1994",//	 VAL=1994
		L"November",//	 VAL=1998-11
		L"yesterday",//	 VAL=1999-07-14
		L"the second of December",//	 VAL=1998-12-02
		L"August 6",//	 VAL=1999-08-06
		L"8",//	 VAL=1999-08-08
		L"1992",//	 VAL=1992
		L"1995",//	 VAL=1995
		L"today",//	 VAL=1999-07-15
		L"today",//	 VAL=1999-0715
		L"next Tuesday",//	 VAL=1999-07-20
		L"the 1960s",//	 VAL=196
		L"the next century",//	 VAL=20
		L"11th century",//	 VAL=10
		L"this millennium",//	 VAL=1
		L"500 AD",//	 VAL=0500
		L"2,300 years ago",//	 VAL=BC0301
		L"4,000 years ago",//	 VAL=BC2001
		L"210 million years ago",//	 VAL=MA210
		L"2,100 million years ago",//	 VAL=
		L"1.5 million years ago",//	 VAL=
		L"ten minutes to three",//	 VAL=1999-07-15T14:50
		L"10/08/1998 21:36:42.85",//	 VAL=1998-10-08T21:36:42.85
		L"twelve o'clock January 3, 1984",//	 VAL=1984-01-03T12:00
		L"one",//	 VAL=T01:00
		L"the nineteenth",//	 VAL=1999-07-19
		L"eleven in the morning",//	 VAL=1999-0719T11:00
		L"11:59 p.m.",//	 VAL=1998-12-31T23:59
		L"Sixty seconds later",//	 VAL=1998-12-31T24:00:00
		L"midnight",//	 VAL=1999-07-14T24:00
		L"April 11, 1996 11:13:05 a.m. GMT",//	 VAL=1996-04-11T11:13Z
		L"January 21, 1994 08:29 Eastern Standard Time",//	 VAL=1994-01-21T08:29-05
		L"September 12, 2001",//	 VAL=2001-09-12
		L"12:27 PM EDT",//	 VAL=2001-09-12T12:27-04
		L"1627 GMT",//	 VAL=2001-09-12T16:27Z
		L"8:45 a.m.",//	 VAL=2001-09-11T08:45-04
		L"9:03 a.m.",//	 VAL=2001-09-11T09:03-04
		L"at 5:00 p.m.",//	 VAL=1999-07-15T17:00
		L"1300 GMT",//	 VAL=1999-07-15T13:00Z
		L"next week",//	 VAL=1999-W29
		L"next Thursday",//	 VAL=1999-07-22
		L"March 31",//	 VAL=1999-03-31
		L"three-hour",//	 VAL=PT3H ANCHOR_DIR=WITHIN ANCHOR_VAL=1999-0715
		L"today",//	 VAL=1999-0715
		L"three-day",//	 VAL=P3D ANCHOR_DIR=WITHIN ANCHOR_VAL=1999-W29
		L"next week",//	 VAL=1999-W29
		L"52-year",//	 VAL=P52Y ANCHOR_DIR=ENDING ANCHOR_VAL=1999
		L"31-year-old",//	 VAL=P31Y ANCHOR_DIR=ENDING ANCHOR_VAL=1990 
		L"Thursday",//	 VAL=1999-07-15
		L"the past four years",//	 VAL=P4Y ANCHOR_DIR=ENDING ANCHOR_VAL=1999
		L"the fifth straight day in a row",//	 VAL=P5D ANCHOR_DIR=ENDING ANCHOR_VAL=1999-07-15
		L"the next three days",//	 VAL=P3D ANCHOR_DIR=STARTING ANCHOR_VAL=1999-07-15
		L"another year",//	 VAL=P1Y ANCHOR_DIR=STARTING ANCHOR_VAL=1999-0715
		L"the past three weeks",//	 VAL=P3W ANCHOR_DIR=ENDING ANCHOR_VAL=1999W28
		L"the sixth-straight year",//	 VAL=P6Y ANCHOR_DIR=ENDING ANCHOR_VAL=1999 
		L"1997",//	 VAL=1997
		L"September 1, 1985",//	 VAL=1985-09-01
		L"1912",//	 VAL=1912
		L"73 years",//	 VAL=P73Y ANCHOR_DIR=STARTING ANCHOR_VAL=1912
		L"six months",//	 VAL=P6M ANCHOR_DIR=STARTING ANCHOR_VAL=1999-01
		L"the two months since the crisis hit",//	 VAL=P2M ANCHOR_DIR=STARTING ANCHOR_VAL=1999-05
		L"three weeks",//	 VAL=P3W ANCHOR_DIR=AFTER ANCHOR_VAL=1999-07-15
		L"three weeks",//	 VAL=P3W ANCHOR_DIR=BEFORE ANCHOR_VAL=1999-07-15
		L"10 years and 1 month",//	 VAL=P10Y1M ANCHOR_DIR=AFTER ANCHOR_VAL=1999
		L"six and a half years",//	 VAL= P6.5Y ANCHOR_DIR=BEFORE ANCHOR_VAL=1999-07-15 
		L"the next two and a half weeks",//	 VAL=P2.5W ANCHOR_DIR=STARTING ANCHOR_VAL=1999-07-15 
		L"a decade",//	 VAL=P1DE ANCHOR_DIR=ENDING ANCHOR_VAL=199 
		L"two millennia",//	 VAL=P2ML ANCHOR_DIR=ENDING ANCHOR_VAL=1 
		L"half-century",//	 VAL=P.5CE ANCHOR_DIR=ENDING ANCHOR_VAL=1999-0717
		L"Saturday",//	 VAL=1999-07-17
		L"five days ago",//	 VAL=1999-07-10
		L"five days",//	 VAL=P5D
		L"a year ago",//	 VAL=1998
		L"six weeks ago",//	 VAL=1999-W23
		L"a year",//	 VAL=2000
		L"Now",//	 VAL=PRESENT_REF ANCHOR_DIR=AS_OF ANCHOR_VAL=1999-0715
		L"today",//	 VAL=PRESENT_REF ANCHOR_DIR=AS_OF ANCHOR_VAL=1999-0715
		L"current",//	 VAL=PRESENT_REF ANCHOR_DIR=AS_OF ANCHOR_VAL=199907-15
		L"now",//	 VAL=PRESENT_REF ANCHOR_DIR=AS_OF ANCHOR_VAL=1999-07-15
		L"these days",//	 VAL=PRESENT_REF ANCHOR_DIR=AS_OF ANCHOR_VAL=1999-07-15
		L"the time being",//	 VAL=PRESENT_REF ANCHOR_DIR=AS_OF ANCHOR_VAL=1999-07-15
		L"the past",//	 VAL=PAST_REF ANCHOR_DIR=BEFORE ANCHOR_VAL=1999-07-15
		L"lately",//	 VAL=PAST_REF ANCHOR_DIR=BEFORE ANCHOR_VAL=1999-07-15
		L"recent",//	 VAL=PAST_REF ANCHOR_DIR=BEFORE ANCHOR_VAL=1999-0715
		L"former",//	 VAL=PAST_REF ANCHOR_DIR=BEFORE ANCHOR_VAL=1999-0715
		L"several weeks ago",//	 VAL=PAST_REF ANCHOR_DIR=BEFORE ANCHOR_VAL=1999-W28
		L"22-year-old",//	 VAL=P22Y ANCHOR_DIR=ENDING ANCHOR_VAL=1999
		L"future",//	 VAL=FUTURE_REF ANCHOR_DIR=AFTER ANCHOR_VAL=1999-0715
		L"the future of our peoples",//	 VAL=FUTURE_REF ANCHOR_DIR=AFTER ANCHOR_VAL=199907-15
		L"a matter of days",//	 VAL=FUTURE_REF ANCHOR_DIR=AFTER ANCHOR_VAL=1999-07-15
		L"a few months",//	 VAL=FUTURE_REF ANCHOR_DIR=AFTER ANCHOR_VAL=1999-07
		L"Thursday",//	 VAL=1999
		L"a few days later",//	 VAL=FUTURE_REF ANCHOR_DIR=AFTER ANCHOR_VAL=1999-07-15
		L"1848",//	 VAL=1848
		L"the future",//	 VAL=FUTURE_REF ANCHOR_DIR=AFTER ANCHOR_VAL=1848
		L"August 1, 1947",//	 VAL=1947-08-01
		L"yesterday",//	 VAL=1947-07-31
		L"long ago",//	 VAL=PAST_REF ANCHOR_DIR=BEFORE ANCHOR_VAL=1947-04-28 
		L"April 28",//	 VAL=1947-0428
		L"the past",//	 VAL=PAST_REF ANCHOR_DIR=BEFORE ANCHOR_VAL=1947-04-28 
		L"1966",//	 VAL=1966
		L"five years",//	 VAL=P5Y ANCHOR_DIR=ENDING ANCHOR_VAL=1966
		L"one time",//	 VAL=PAST_REF ANCHOR_DIR=BEFORE ANCHOR_VAL=1961
		L"A couple of minutes ago",//	 VAL=PAST_REF ANCHOR_DIR=BEFORE ANCHOR_VAL=1999-07-15TXX:XX
		L"today",//	 VAL=1999-07-15
		L"Fall 1998",//	 VAL=1998-FA
		L"summer of 1964",//	 VAL=1964-SU
		L"1964",//	 VAL=1964
		L"the summer",//	 VAL=1964-SU
		L"last summer",//	 VAL=1998-SU
		L"the fall semester",//	 VAL=1998-FA
		L"an unusually mild winter",//	 VAL=1999-WI
		L"all spring",//	 VAL=P1SP ANCHOR_DIR=WITHIN ANCHOR_VAL=1999
		L"all winter",//	 VAL=P1WI ANCHOR_DIR=STARTING ANCHOR_VAL=1999
		L"the past three summers",//	 VAL=P3SU ANCHOR_DIR=BEFORE ANCHOR_VAL=1998-03-22
		L"fiscal 1998",//	 VAL=FY1998
		L"FY98",//	 VAL=P1FY ANCHOR_DIR=WITHIN ANCHOR_VAL=FY1998
		L"the 4th quarter",//	 VAL=1998-Q4
		L"its second-best quarter ever",//	 VAL=1998-Q4
		L"this year",//	 VAL=1999
		L"the first three quarters",//	 VAL=P3QX ANCHOR_DIR=STARTING ANCHOR_VAL=1999
		L"the first quarter of 1998",//	 VAL=P1Q1 ANCHOR_DIR=STARTING ANCHOR_VAL=1998
		L"FY1998",//	 VAL=FY1998
		L"this weekend",//	 VAL=1999-W28-WE
		L"the weekend",//	 VAL=P1WE ANCHOR_DIR=WITHIN ANCHOR_VAL=1999-W28
		L"two weekends",//	 VAL=P2WE ANCHOR_DIR=WITHIN ANCHOR_VAL=1999
		L"that long holiday weekend",//	 VAL=1999-W21-WE
		L"8:00",//	 VAL=1999-07-15T08:00
		L"this morning",//	 VAL=1999-07-15TMO
		L"the morning",//	 VAL=1999-01-01TMO
		L"now",//	 VAL=PRESENT_REF ANCHOR_VAL=1999-07-15 ANCHOR_DIR=AS_OF
		L"Monday morning",//	 VAL=1999-07-19TMO
		L" last night",//	 VAL=1999-07-14TNI
		L"last night",//	 VAL=1999-07-15-TNI
		L"1 a.m.",//	 VAL=1999-0715T01:00
		L"the day",//	 VAL=P1DT ANCHOR_DIR=WITHIN ANCHOR_VAL=1999-07-15
		L"the night",//	 VAL=P1NI ANCHOR_DIR=WITHIN ANCHOR_VAL=1999-07-14
		L"day",//	 VAL=P1DT ANCHOR_DIR=WITHIN ANCHOR_VAL=1999-07-15
		L"a Saturday afternoon",//	 VAL=1995-WXX-6TAF
		L"September",//	 VAL=1995-09
		L"October ",//	 VAL=1995-10
		L"November 1995",//	 VAL=1995-11
		L"Feb. 14",//	 VAL=1998-02-14
		L"months of renewed hostility",//	 VAL=PXM ANCHOR_DIR=ENDING ANCHOR_VAL=1999-07
		L"recent decades",//	 VAL=PXDE ANCHOR_DIR=BEFORE ANCHOR_VAL=199
		L"the coming months",//	 VAL=PXM ANCHOR_DIR=AFTER ANCHOR_VAL=1999-07 
		L"the coming weeks",//	 VAL=PXW ANCHOR_DIR=AFTER ANCHOR_VAL=1999-W28
		L"months",//	 VAL=PXM ANCHOR_DIR=AFTER ANCHOR_VAL=1999-07
		L"the years ahead",//	 VAL=PXY ANCHOR_DIR=AFTER ANCHOR_VAL=1999
		L"millennia",//	 VAL=PXML ANCHOR_DIR=ENDING ANCHOR_VAL=1
		L"the past few years",//	 VAL=PXY ANCHOR_DIR=ENDING ANCHOR_VAL=1999
		L"a couple of years",//	 VAL=PXY ANCHOR_DIR=BEFORE ANCHOR_VAL=1999
		L"millions of years ago",//	 VAL=MAX
		L"fall 1998",//	 VAL=1998-FA
		L"Tuesday",//	 VAL=1999-07-15 ID=1
		L"more than a decade ago",//	 VAL=1989 MOD=BEFORE
		L"now",//	 VAL=PRESENT_REF ANCHOR_DIR=AS_OF ANCHOR_VAL=1999-07-15 
		L"more than 4,000 years ago",//	 VAL=BC2001 MOD=BEFORE
		L"nearly four decades of experience",//	 VAL=P4DE MOD=LESS_THAN ANCHOR_DIR=BEFORE ANCHOR_VAL=1999-07-15
		L"more than a month",//	 VAL=P1M MOD=MORE_THAN ANCHOR_DIR=ENDING ANCHOR_VAL=1999-07-15
		L"Friday",//	 VAL=1999-07-16
		L"no more than two days",//	 VAL=P2D MOD=EQUAL_OR_LESS ANCHOR_DIR=STARTING ANCHOR_VAL=1999-07-16
		L"at least the next year",//	 VAL=P1Y MOD=EQUAL_OR_MORE ANCHOR_DIR=STARTING ANCHOR_VAL=1999 
		L"two",//	 VAL=P2Y MOD=EQUAL_OR_MORE ANCHOR_DIR=STARTING ANCHOR_VAL=1999 
		L"the dawn of 2000",//	 VAL=2000 MOD=START
		L"the early 1960s",//	 VAL=196 MOD=START
		L"the mid nineteenth century",//	 VAL=18 MOD=MID
		L"late last night",//	 VAL=1999-07-14TNI MOD=END
		L"about three years ago",//	 VAL=1996 MOD=APPROX
		L"some 2,300 years ago",//	 VAL=BC0301 MOD=APPROX
		L"Nearly five years ago",//	 VAL=1994 MOD=AFTER ANCHOR_DIR=BEFORE ANCHOR_VAL=1995
		L"just over two years ago",//	 VAL=1997 MOD=BEFORE ANCHOR_DIR=AFTER ANCHOR_VAL=1996
		L"almost a year and a half ago",//	 VAL=1998-01 MOD=AFTER ANCHOR_DIR=BEFORE ANCHOR_VAL=1999-07-15
		L"1999",//	 VAL=1999
		L"decades",//	 VAL=PXDE ANCHOR_DIR=ENDING ANCHOR_VAL=199
		L"every month",//	 VAL=XXXX-XX SET=YES
		L"Friday nights",//	 SET=YES VAL=1998-WXX-5TNI
		L"Two years ago",//	 VAL=1997
		L"each week",//	 SET=YES VAL=1997-WXX
		L"1999",//	 VAL=1999
		L"monthly",//	 SET=YES VAL=XXXX-XX
		L"annual",//	 SET=YES VAL=XXXX
		L"semiannual",//	 SET=YES VAL=XXXX-HX
		L"per hour",//	 SET=YES VAL=PT1H
		L"a year",//	 SET=YES VAL=P1Y
		L"twice a day",//	 SET=YES VAL=XXXX-XX-XX
		L"one hour",//	 VAL=PT1H
		L"each day",//	 SET=YES VAL=XXXX-XX-XX
		L"Last summer",//	 VAL=1998-SU
		L"numerous Saturdays",//	 SET=YES VAL=1998-WXX-6
		L"1998",//	 VAL=1998
		L"Some winters",//	 VAL=XXXX-WI SET=YES
		L"TUESDAY",//	 VAL=1999-07-20
		L"the hours that you are not",//	 VAL=1999-0720TXX
		L"almost weekly",//	 SET=YES VAL=XXXX-WXX
		L"eight hours",//	 VAL=PT8H
		L"15 minutes after the hour",//	 VAL=TXX:15
		L"January",//	 VAL=XXXX-01
		L"March ",//	 VAL=XXXX03
		L"a decent year",//	 VAL=XXXX
		L"Each year",//	 VAL=XXXX SET=YES
		L"Winters",//	 VAL=XXXX-WI
		L"the morning",//	 VAL=TMO
		L"night",//	 VAL=TNI
		L"the winter",//	 VAL=XXXX-WI
		L"February 2",//	 VAL=XXXX-02-02
		L"spring",//	 VAL=XXXX-SP
		L"December",//	 VAL=XXXX-12
		L"a Tuesday",//	 VAL=1998-WXX-2
		L"June",//	 VAL=199906
		L"June",//	 VAL=199906
		L"the 20th",//	 VAL=1999-06-20
		L"Today",//	 VAL=1999-07-15
		L"a sunny day",//	 VAL=1999-07-15
		L"all day",//	 VAL=P1DT
		L"The gestation period in humans",//	
		L"nine months",//	 VAL=P9M
		L"12 weeks",//	 VAL=P12W
		L"hours ",//	 VAL=PXH
		L"days",//	 VAL=PXD
		L"a few days",//	 VAL=PXD
		L"weeks",//	 VAL=PXW
		L"the day",//	 VAL=PXDT
		L"noon",//	 VAL=T12:00
		L"midnight",//	 VAL=T24:00
		L"7 p.m.",//	 VAL=T19:00
		L"7 a.m.",//	 VAL=T07:00
		L"Election Day",//	 VAL=XXXX-XX-XXSET=YES
		L"Super Bowl Sunday",//	 VAL=XXXX-WXX-7 SET=YES
		L"April",//	 VAL=XXXX-04 SET=YES
		L"some nights",//	 VAL=TNI SET=YES
		L"each year",//	 VAL=XXXX SET=YES
		L"monthly",//	 VAL=XXXX-XX SET=YES
		L"tomorrow",//	 VAL=1999-07-16
		L"the last minute",//	 VAL=TXX:XX
		L"Good morning",//	 VAL=TMO
		L"Tuesday",//	 VAL=1999-07-13
		L"that",//	 VAL=1999-07-13
		L"That night",//	 VAL=1999-07-14TNI
		L"last May",//	 VAL=1999-05
		L"then",//	 VAL=1999-05
		L"this year",//	 VAL=1999
		L"his last with the symphony",//	 VAL=1999
		L"a strange couple of days",//	 VAL=P2D ANCHOR_VAL=1999-07-15 ANCHOR_DIR=ENDING
		L"time for the news",//	 VAL=1999-07-15T13:00
		L"the day that Roosevelt died",//	 VAL=XXXX-XX-XX
		L"three days after the fire",//	 VAL=XXXX-XX-XX
		L"a few weeks earlier",//	 VAL=XXXX-WXX
		L"Tuesday night",//	 VAL=1999-0713TNI
		L"40 minutes after the killing",//	 VAL=1999-07-13TXX:XX
		L"the 30 years since Neil Armstrong walked on the Moon",//	 VAL=P30Y
		L"the first nine minutes of the game",//	 VAL=PT9M
		L"the months before the Oklahoma City bombing",//	 VAL=PXM
		L"the last 10 minutes of the Roman Empire",//	 VAL=PT10M
		L"the five days following his graduation",//	 VAL=P5D
		L"the night before",//	 VAL=TNI
		L"the next day",//	 VAL=XXXX-XX-XX
		L"the first few weeks of their babies' lives",//	 VAL=PXW
		L"Tuesday",//	 VAL=1999-07-20
		L"two days after Netanyahu",//	 VAL=1999-07-22
		L"06/05/1998 20:15:00",//	 VAL=1998-06-05T20:15:00
		L"last Aug. 31",//	 VAL=1997-08-31
		L"the time of the accident",//	 VAL=1997-0831
		L"1954",//	 VAL=1954
		L"12 years later",//	 VAL=1966
		L"4 p.m.",//	 VAL=1998-07-14T16:00
		L"Now",//	 VAL=PRESENT_REF ANCHOR_VAL=1936-11 ANCHOR_DIR=AS_OF
		L"the time to recognize the possibilities which lie before us in the taking up and developing of this part of our forefathers' vision",//	 VAL=PRESENT_REF ANCHOR_VAL=193611 ANCHOR_DIR=AS_OF
		L"Friday ",//	 VAL=1999-07-09
		L"December twenty-ninth",//	 VAL=199812-29
		L"02/19/1998",//	 VAL=1998-02-19
		L"Thursday",//	 VAL=1998-0219
		L"the 12th anniversary of the space station's launch",//	 VAL=1998-02-19
		L"the regular season",//	 NONSPECIFIC=YES
		L"1994",//	 VAL=1994
		L"New Year's Day",//	 VAL=XXXX-XX-XX
		L"Super Bowl Sunday",//	 VAL=XXXX-WXX-7 SET=YES
		L"11",//	 VAL=P11M
		L"12 months",//	 VAL=P12M
		L"nine hours",//	 VAL=PT9H
		L"six",//	 VAL=PT6H
		L"September 11",//	 VAL=XXXX-09-11
		L"9/11",//	 VAL=XXXX-09-11
		L"June 4",//	 VAL=XXXX-0604
		L"4,000 years ago",//	 VAL=BC2001 COMMENT=I doubt the writer really meant 4000 exactly
		L"recent",//	 VAL=PAST_REF ANCHOR_DIR=BEFORE ANCHOR_VAL=1999-0715
		L"8:00",//	 VAL=1999-07-15T08:00
		L"this morning",//	 VAL=1999-07-15TMO
		L"Saturday",//	 VAL=1999-07-17
		L"the days of free love",//	 VAL=196
		L"1992",//	 VAL=1992
		L"1995",//	 VAL=1995
		L"August 6",//	 VAL=1999-08-06
		L"8",//	 VAL=1999-08-08
		L"3",//	 VAL=1999-07-15T15
		L"6 pm today",//	 VAL=1999-07-15T18
		L"five",//	 VAL=1999-07-16T17
		L"six pm tomorrow",//	 VAL=1999-07-16T18
		L"five",//	 VAL=P5Y
		L"six years",//	 VAL=P6Y
		L"now",//	 VAL=PRESENT_REF ANCHOR_VAL=1999-07-15 ANCHOR_DIR=AS_OF
		L"Monday morning",//	 VAL=1999-07-19TMO
		L"six months",//	 VAL=2000-01
		L"now",//	 VAL=PRESENT_REF ANCHOR_VAL=1999-07-15 ANCHOR_DIR=AS_OF
		L"at least the next year",//	 VAL=P1Y MOD=EQUAL_OR_MORE ANCHOR_VAL=1999 ANCHOR_DIR=AFTER
		L"two",//	 VAL=P2Y MOD=EQUAL_OR_MORE ANCHOR_VAL=1999 ANCHOR_DIR=AFTER
		L"the short",//	 VAL=FUTURE_REF ANCHOR_VAL=2000-09-15 ANCHOR_DIR=AFTER
		L"mid-term",//	 VAL=FUTURE_REF ANCHOR_VAL=2000-09-15 ANCHOR_DIR=AFTER
		L"six months or more",//	 VAL=P6M MOD=EQUAL_OR_MORE
		L"next Tuesday",//	 VAL=1999-0720
		L"today",//	 VAL=1996-07-15
		L"This year",//	 VAL=1999
		L"September 2",//	 VAL=1999-09-02
		L"holiday",//	
		L"June 2004",//	 VAL=2004-06
		L"December 12",//	 VAL=1999-12-12
		L"April",//	 VAL=XXXX-04
		L"June",//	 VAL=XXXX06
		L"14 hour",//	 VAL=PT14H
		L"8:00 p.m.",//	 VAL=1999-07-16T20:00
		L"Friday",//	 VAL=1999-0716
		L"Friday",//	 VAL=1999-07-16
		L"8:00 p.m.",//	 VAL=1999-0716T20:00
		L"Tuesday",//	 VAL=1999-07-20
		L"12 PM",//	 VAL=1999-0720T12:00
		L"some Thursdays",//	 VAL=1998-WXX-4 SET=YES
		L"'68",//	 VAL=1968
		L"Day of Thanks",//	 VAL=1623-07-XX
		L"July, 1623",//	 VAL=1623-07
		L"This month",//	 VAL=1623-07
		L"a day of thanks",//	 VAL=1623-07-XX
		L"that day",//	 VAL=1623-07-XX
		L"The first day of thanks",//	 VAL=1621-FA-XX
		L"1621",//	 VAL=1621
		L"1620",//	 VAL=1620
		L"The first winter",//	 VAL=1621-WI
		L"a long, hard one",//	 VAL=1621-WI
		L"spring",//	 VAL=1621-SP
		L"fall",//	 VAL=1621-FA
		L"the winter",//	 VAL=1622-WI
		L"three days",//	 VAL=P3D ANCHOR_DIR=WITHIN ANCHOR_VAL=1621FA
		L"a day of thanks",//	 VAL=XXXX-XX-XX
		L"each year",//	 SET=YES VAL=XXXX
		L"the first day of thanks",//	 VAL=1621-FA-XX
		0
	};

	if (argc > 1) {
		boost::scoped_ptr<UTF8InputStream> ifs_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream &ifs(*ifs_scoped_ptr);

		ifs.open(argv[1]);
		if (ifs.is_open()) {
			wchar_t line[BUFSIZ];
			while (!ifs.eof()) {
				ifs.getline(line, BUFSIZ);

				wchar_t *p0 = wcsstr (line, L"\t");
				if (p0 == 0)
					continue;

				*p0 = 0;
				wstring expr0 = line; // timex2 expr

				wchar_t *p1 = wcsstr (++p0, L"\t");
				*p1 = 0;
				wstring expr1 = p0; // truth
				wstring expr2 = p1+1; // anchor date

				timex2.set_anchor(expr2);

				//wcout << expr << L" //" << wline.substr(pos) << endl;
				wcout << expr0 << L"\t";

				if (normalize_temporal (timex2, expr0)) {
					wcout << L"VAL=" << timex2.VAL ;
					if (timex2.ANCHOR_VAL.length() > 0) {
						wcout << L" ANCHOR_VAL=" << timex2.ANCHOR_VAL
							<< L" ANCHOR_DIR=" << timex2.ANCHOR_DIR;
					}
					if (timex2.MOD.length() > 0) {
						wcout << L" MOD=" << timex2.MOD;
					}
					if (timex2.SET.length() > 0) {
						wcout << L" SET=" << timex2.SET;
					}
				}
				wcout << L" //" << expr1 << L"\t" << expr2 <<  endl;
			}
			ifs.close();
		}
		else {
			wcerr << "can't open file '" << argv[1] << "' for reading!" << endl;
		}
	}
	else {
		for (unsigned i (0); test_cases[i] != 0; ++i) {
			wcout << test_cases[i] << ": ";
			if (normalize_temporal (timex2, test_cases[i])) {
				wcout << L"VAL=" << timex2.VAL 
					<< L" ANCHOR_VAL=" << timex2.ANCHOR_VAL
					<< L" ANCHOR_DIR=" << timex2.ANCHOR_DIR
					<< L" MOD=" << timex2.MOD
					<< L" SET=" << timex2.SET;
			}
			wcout << endl;
		}
	}

	return 0;
}
#endif //__TP_MAIN
#endif // BOOST
