// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#ifndef EN_TEMPORAL_STRING_H
#define EN_TEMPORAL_STRING_H

#include <string>

using namespace std;

class TokenSequence;

class TemporalString
{
public:

	static const int max_tokens = 50;
	TemporalString(const wstring str);
	int size();
	wstring toString();
	int containsWord(const wstring &word);
	int containsMonth();
	int containsDayOfWeek();
	int containsPureTimeUnit();
	wstring tokenAt(int index);
	void setToken(int index, const wstring &s);
	void remove(int index);
	void remove(int index1, int index2);
	int separateDigits(const wstring &s, wstring *result, int size);
	int numericValue(const wstring &str);
	int getNums(wstring *result, int size);

private:
	int _top;
	wstring _tokens[TemporalString::max_tokens];





};


#endif
