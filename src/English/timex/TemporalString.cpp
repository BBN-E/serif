// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/timex/TemporalString.h"
#include "English/timex/en_TemporalNormalizer.h"
#include "English/timex/Strings.h"
#include "English/timex/Month.h"
#include "English/timex/Day.h"

#include "Generic/common/SessionLogger.h"

#include <iostream>
#include <string>

using namespace std;

TemporalString::TemporalString(const wstring str) {

	_top = -1;
	wstring token = L"";
	size_t index = 0;
	while (index < str.length()) {
		if ((index == str.length() - 1 || str.at(index) == L' ') && token.length() > 0) {
			if (str.at(index) != L' ')
				token += str.at(index);

			wstring subTokens[10];
			int num_sub_tokens = separateDigits(token, subTokens, 10);

			for (int j = 0; j < num_sub_tokens; j++) {
				if (_top >= max_tokens - 1) {
					SessionLogger::warn("temporal_string") << "max_tokens reached, ignoring some tokens";
					break;
				}
				_top++;
				_tokens[_top] = subTokens[j];
			}
			token = L"";
			index++;
		} else {
			if (str.at(index) != L' ')
				token += str.at(index);
			index++;
		}
	}
}

int TemporalString::size() {
	return _top + 1;
}

wstring TemporalString::toString() {
	wstring s = L" ";
	for (int i = 0; i <= _top; i++) {
		s += _tokens[i] + L" ";
	}
	return s;
}

int TemporalString::containsWord(const wstring &word) {
	for (int i = 0; i <= _top; i++) {
		if (!_tokens[i].compare(word) || !_tokens[i].compare(word + L"s"))
			return i;
	}
	return -1;
}

int TemporalString::containsMonth() {
	int j;
	for (int i = 1; i < Month::numMonths + 1; i++)
		if ((j = containsWord(Month::monthNames[i])) != -1)
			return j;
	return -1;
}

int TemporalString::containsDayOfWeek() {
	int j;
	for (int i = 1; i < Day::numDays; i++)
		if ((j = containsWord(Day::daysOfWeek[i])) != -1)
			return j;
	return -1;
}

int TemporalString::containsPureTimeUnit() {
	int j;
	for (int i = 0; i < EnglishTemporalNormalizer::num_ptu; i++)
		if ((j = containsWord(EnglishTemporalNormalizer::pureTimeUnits[i])) != -1)
			return j;
	return -1;
}

wstring TemporalString::tokenAt(int index) {
	if (index >= 0 && index <= _top) return _tokens[index];
	else return L"";
}

void TemporalString::setToken(int index, const wstring &s) {
	if (index >= 0 && index <= _top) 
		_tokens[index] = s;
}

void TemporalString::remove(int index) {
	if (index >= 0) {
		for (int i = index; i <= _top - 1; i++) 
			_tokens[i] = _tokens[i+1];
		_top--;
	}
}

void TemporalString::remove(int index1, int index2) {
	if ((index1 >= 0) && (index2 <= _top) && (index2 >= index1)) {
		int diff = index2 - index1 + 1;
		for (int i = index1; i <= _top - diff; i++)
			_tokens[i] = _tokens[i+diff];
		_top -= diff;
	}
}		

int TemporalString::separateDigits(const wstring &s, wstring *result, int size) {
	int length = 0;
	size_t i = 0;
	wstring temp = L"";
	while (i < s.length()) {
		while (i < s.length() && !iswdigit(s.at(i))) {
			temp += s.at(i);
			i++;
		}
		if (temp.compare(L"") && length < size) 
			result[length++] = temp;

		temp = L"";

		while (i < s.length() && iswdigit(s.at(i))) {
			temp += s.at(i);
			i++;
		}
		if (temp.compare(L"") && length < size) 
			result[length++] = temp;
		temp = L"";
	}
	return length;
}

int TemporalString::numericValue(const wstring &str) {
	return Strings::parseInt(str);
}

int TemporalString::getNums(wstring *result, int size) {
	int length = 0;
	for (int i = 0; i <= _top; i++) {
		if (numericValue(_tokens[i]) > 0 && length < size) {
			result[length++] = _tokens[i];
		} 
	}
	return length;
}
