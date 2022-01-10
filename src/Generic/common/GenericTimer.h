// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef GENERIC_TIMER_H
#define GENERIC_TIMER_H

#if defined(_WIN32)
// __int64 is a Windows-specific type
#else
typedef int __int64;
#endif

#include <boost/shared_ptr.hpp>

class GenericInternalTimer;

class GenericTimer {
public:
	GenericTimer();

	void resetTimer();
	void startTimer();
	void stopTimer();
	void increaseCount();
	unsigned long getCount() const;
	__int64 getClock() const;
	double getTime() const;

protected:
	boost::shared_ptr<GenericInternalTimer> myInternalTimer;
};

#endif
