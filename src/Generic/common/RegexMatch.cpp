// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/RegexMatch.h"

std::wstring RegexMatch::getString()const{
	return _value;
}
int RegexMatch::getPosition()const{
	return _position;
}
int RegexMatch::getLength()const{
	return _length;
}
