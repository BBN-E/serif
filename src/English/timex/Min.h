// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef EN_MIN_H
#define EN_MIN_H

#include "English/timex/TimeUnit.h"
#include <string>

class Min : public TimeUnit {
public:
	Min();
	Min(int val);
	Min(const wstring &min);
	Min* clone() const;
	void clear();
};

#endif
