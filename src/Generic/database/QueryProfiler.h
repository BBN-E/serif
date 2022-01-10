// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef QUERY_PROFILER_H
#define QUERY_PROFILER_H

#include <boost/noncopyable.hpp>
#include <string>

// Forward declarations.
class GenericTimer;

class QueryProfiler: boost::noncopyable {
public:
	QueryProfiler();
	~QueryProfiler();
	std::string getResults(double cutoff_percent=1.0);

	/** QueryTimer objects are intended to be passed by value. */
	class QueryTimer {
	public:
		void start();
		void stop();
	private:
		GenericTimer &_timer;
		QueryTimer(GenericTimer& timer): _timer(timer) {};
		friend class QueryProfiler;
	};

	/** Return the timer that is used to keep track of how long is spent
	  * in the given query.  The caller is responsible for calling the
	  * start() and end() methods on the returned timer object. */
	QueryTimer timerFor(std::string query, bool strip_ints=true, bool strip_strings=true);

private:
	struct Impl;
	Impl* _impl;
};

#endif
