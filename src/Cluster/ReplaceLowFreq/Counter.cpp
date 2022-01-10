// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Counter.h"

Counter::Counter()
{
	count = 0;
}

Counter::Counter(int initialValue)
{
	count = initialValue;
}

Counter::~Counter()
{
}

int Counter::increment() {
	return count++;
}

int Counter::getCount() {
    return count;
}
