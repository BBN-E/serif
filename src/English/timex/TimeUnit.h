// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef EN_TIMEUNIT_H
#define EN_TIMEUNIT_H

#include <string>

using namespace std;

class TimeUnit {
	
protected:
	wstring strVal;
	int intVal;
	
public:
	TimeUnit();
	virtual ~TimeUnit() { }
	virtual TimeUnit* clone() const = 0;
	virtual wstring toString() const;
	virtual int value() const;
	virtual bool isEmpty() const;
   	virtual int compareTo(const TimeUnit* t) const;
	virtual TimeUnit* sub(int val) const;	
	virtual TimeUnit* add(int val) const;
	virtual void clear();

	bool underspecified;
	
};

#endif
