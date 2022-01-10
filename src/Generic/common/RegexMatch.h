// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef REGEXMATCH_H
#define REGEXMATCH_H
#include <string>

#include "Generic/common/LocatedString.h"

class RegexMatch {
private:
	std::wstring _value;
	int _position;
	int _length;
public:
	RegexMatch(std::wstring value, int position, int length)
		:_value(value), _position(position), _length(length){};
	std::wstring getString()const;
	int getPosition()const;
	int getLength()const;

};



#endif
