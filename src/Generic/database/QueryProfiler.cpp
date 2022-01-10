// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/database/QueryProfiler.h"
#include "Generic/common/GenericTimer.h"
#include "Generic/common/SessionLogger.h"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#include <boost/regex.hpp>
#pragma warning(pop)
#include <iomanip>

struct QueryProfiler::Impl {
	std::map<std::string, GenericTimer> queryTimes;
};

namespace {
	boost::regex INTEGER_RE("[0-9]+");
	boost::regex STRING_RE("'[^']+'");
	boost::regex INTEGER_LIST_RE("NNN(, NNN)+");
	boost::regex STRING_LIST_RE("'\\.\\.\\.'(, '\\.\\.\\.')+");
	boost::regex WRAP_RE("(.{1,58})( +|$\n?)|(.{1,58})");
	boost::regex EMPTY_LINE_RE("\n *\\| *\\| *$");
}


QueryProfiler::QueryProfiler(): _impl(_new Impl()) {}
QueryProfiler::~QueryProfiler() { delete _impl; }

QueryProfiler::QueryTimer QueryProfiler::timerFor(std::string query, bool strip_ints, bool strip_strings) {
	// We need to wrap this in a try block, because for VERY long queries, the regex is going to crash here
	try {
		if (strip_ints) {
			query = boost::regex_replace(query, INTEGER_RE, "NNN");
			query = boost::regex_replace(query, INTEGER_LIST_RE, "NNN, ...");
		}
		if (strip_strings) {
			query = boost::regex_replace(query, STRING_RE, "'...'");
			boost::algorithm::replace_all(query, "''", "");
			query = boost::regex_replace(query, STRING_LIST_RE, "'...', ...");
		}
	} catch( ... ){
		size_t min_int = query.find("NNN");
		size_t min_str = query.find("...");
		size_t len = 0;
		if (min_int != std::string::npos)
			len = min_int;
		if (min_str != std::string::npos && min_str < len) { len = min_str; }
		if (500 < len) { len = 500; }
		query = query.substr(0, len - 1);
		query += " [[PROBLEMS GENERALIZING TOO-LONG QUERY IN PROFILER]]";
	}
	return QueryTimer(_impl->queryTimes[query]);
}

void QueryProfiler::QueryTimer::start() {
	_timer.startTimer();
	_timer.increaseCount();
}
void QueryProfiler::QueryTimer::stop() {
	_timer.stopTimer();
}

std::string QueryProfiler::getResults(double cutoff_percent) {
	typedef std::pair<double, std::string> TimeQueryPair;
	std::vector<TimeQueryPair> pairs;
	double total_time = 0;
	for (std::map<std::string, GenericTimer>::iterator it=_impl->queryTimes.begin(); it != _impl->queryTimes.end(); ++it) {
		pairs.push_back(TimeQueryPair((*it).second.getTime(), (*it).first));
		total_time += (*it).second.getTime();
	}
	std::sort(pairs.begin(), pairs.end());
	std::ostringstream msg;
	msg << std::setprecision(2) << std::fixed;
	msg << "SQL Query Times (total: " << (total_time/1000) << " seconds)\n";
	std::string div = ("-------+-------+----------------------"
	                   "--------------------------------------\n");
	msg << "  Time | Count | Query\n" << div;
	BOOST_FOREACH(TimeQueryPair p, pairs) {
		double pct_time = (p.first*100/total_time);
		unsigned long count = _impl->queryTimes[p.second].getCount();
		if (pct_time < cutoff_percent) continue;
		std::string wrapped_query = boost::regex_replace(p.second, WRAP_RE, "\\1\n       |       | \\3");
		wrapped_query = boost::regex_replace(wrapped_query, EMPTY_LINE_RE, "");
		boost::trim(wrapped_query);
		msg << std::setw(6) << pct_time << "%|" 
			<< std::setw(7) << count << "| "
			<< wrapped_query << "\n" << div;
	}
	return msg.str();
}
