// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/values/NumberConverter.h"
#include "English/values/en_NumberConverter.h"

#include <boost/algorithm/string.hpp>

#include <string>

int EnglishNumberConverter::convertComplexTwoPartNumberWord(const std::wstring& str) {
	// Simple handling of two-part numbers
	std::vector<std::wstring> two_part_vector;
	boost::split(two_part_vector, str, boost::is_any_of(L" "));
	if (two_part_vector.size() == 2) {
		int part1 = convertNumberWord(two_part_vector.at(0));
		int part2 = convertNumberWord(two_part_vector.at(1));
		if ((two_part_vector.at(1) == L"hundred" || two_part_vector.at(1) == L"dozen") &&
			part1 != 0 && part2 != 0 && part1 < 10) {
			return part1*part2;
		} else if (two_part_vector.at(1) == L"thousand" && part1 != 0 && part2 != 0 && part1 < 100) {
			return part1*part2;
		} else if (two_part_vector.at(1) == L"million" && part1 != 0 && part2 != 0 && part1 < 1000) {
			return part1*part2;
		}
	}
	return INT_MIN; // signals invalid value
}

/**
 * Hard-coded table of mappings from English number words to integers.
 * Probably want to implement a table plus language-generic version of this method.
 **/
int EnglishNumberConverter::convertSpelledOutNumberWord(const std::wstring& str) {
	if (!str.compare(L"zero")) return 0;
	if (!str.compare(L"one")) return 1;
	if (!str.compare(L"two")) return 2;
	if (!str.compare(L"three")) return 3;
	if (!str.compare(L"four")) return 4;
	if (!str.compare(L"five")) return 5;
	if (!str.compare(L"six")) return 6;
	if (!str.compare(L"seven")) return 7;
	if (!str.compare(L"eight")) return 8;
	if (!str.compare(L"nine")) return 9;
	if (!str.compare(L"ten")) return 10;
	if (!str.compare(L"eleven")) return 11;
	if (!str.compare(L"twelve")) return 12;
	if (!str.compare(L"thirteen")) return 13;
	if (!str.compare(L"fourteen")) return 14;
	if (!str.compare(L"fifteen")) return 15;
	if (!str.compare(L"sixteen")) return 16;
	if (!str.compare(L"seventeen")) return 17;
	if (!str.compare(L"eighteen")) return 18;
	if (!str.compare(L"nineteen")) return 19;
	if (!str.compare(L"twenty")) return 20;
	if (!str.compare(L"thirty")) return 30;
	if (!str.compare(L"forty")) return 40;
	if (!str.compare(L"fifty")) return 50;
	if (!str.compare(L"sixty")) return 60;
	if (!str.compare(L"seventy")) return 70;
	if (!str.compare(L"eighty")) return 80;
	if (!str.compare(L"ninety")) return 90;
	if (!str.compare(L"hundred")) return 100;
	if (!str.compare(L"thousand")) return 1000;
	if (!str.compare(L"million")) return 1000000;
	if (!str.compare(L"billion")) return 1000000000;
	if (!str.compare(L"dozen")) return 12;
	return INT_MIN; // signals invalid value
}

bool EnglishNumberConverter::isSpelledOutNumberWord(const std::wstring& str) {
	return (str.find(L"two") == 0 ||
			str.find(L"three") == 0 ||
			str.find(L"four") == 0 ||
			str.find(L"five") == 0 ||
			str.find(L"six") == 0 ||
			str.find(L"seven") == 0 ||
			str.find(L"eight") == 0 ||
			str.find(L"nine") == 0 ||
			str.find(L"ten") == 0 ||
			str.find(L"eleven") == 0 ||
			str.find(L"twelve") == 0 ||
			str.find(L"thirteen") == 0 ||
			str.find(L"fourteen") == 0 ||
			str.find(L"fifteen") == 0 ||
			str.find(L"sixteen") == 0 ||
			str.find(L"seventeen") == 0 ||
			str.find(L"eighteen") == 0 ||
			str.find(L"nineteen") == 0 ||
			str.find(L"twenty") == 0 ||
			str.find(L"thirty") == 0 ||
			str.find(L"forty") == 0 ||
			str.find(L"fifty") == 0 ||
			str.find(L"sixty") == 0 ||
			str.find(L"seventy") == 0 ||
			str.find(L"eighty") == 0 ||
			str.find(L"ninety") == 0 ||
			str.find(L"hundred") == 0 ||
			str.find(L"thousand") == 0 ||
			str.find(L"million") == 0 ||
			str.find(L"billion") == 0);
}
