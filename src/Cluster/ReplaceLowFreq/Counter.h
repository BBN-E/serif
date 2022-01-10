#ifndef COUNTER_H
#define COUNTER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#pragma once

class Counter
{
public:
	Counter();
	Counter(int initialValue);
	~Counter();

	int increment();
	int getCount();

private:
	int count;

	
};


#endif
