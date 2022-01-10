// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/GenericTimer.h"
#include "Generic/common/GenericInternalTimer.h"

GenericTimer::GenericTimer(): myInternalTimer(_new GenericInternalTimer) 
{
}

void GenericTimer::resetTimer() {
	myInternalTimer->resetTimer();
}

void GenericTimer::startTimer() {
	myInternalTimer->startTimer();
}

void GenericTimer::stopTimer() {
	myInternalTimer->stopTimer();
}

void GenericTimer::increaseCount() {
	myInternalTimer->increaseCount();
}

unsigned long GenericTimer::getCount() const {
	return 	myInternalTimer->getCount();
}

__int64 GenericTimer::getClock() const {
	return 	myInternalTimer->getClock();
}

double GenericTimer::getTime() const {
	return 	myInternalTimer->getTime();
}

