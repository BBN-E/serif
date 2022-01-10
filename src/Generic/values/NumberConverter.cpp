// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/SessionLogger.h"

#include "Generic/values/NumberConverter.h"
#include "Generic/values/xx_NumberConverter.h"
#include "Generic/linuxPort/serif_port.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#include <boost/regex.hpp>
#pragma warning(pop)

#include <string>

boost::shared_ptr<NumberConverter::Factory> &NumberConverter::_factory() {
	static boost::shared_ptr<NumberConverter::Factory> factory(new GenericNumberConverterFactory());
	return factory;
}

/**
 * Entry point for number conversion. This is not language-specific,
 * and is not overridden in language implementations.
 **/
int NumberConverter::convertNumberWord(const std::wstring& input_str) {
	// Collapse whitespace 
	static boost::wregex spaces_re(L"\\s+");
	std::wstring str = boost::regex_replace(input_str, spaces_re, L" ");

	if (str.find(L" ") != std::wstring::npos || str.find(L"-") != std::wstring::npos)
		return convertComplexNumberWord(str);

	size_t length = str.length();
	std::wstring new_str = L"";
	bool all_digits = true;
	for (size_t i = 0; i < length; i++) {
		if (iswdigit(str.at(i)))
			new_str += str.at(i);
		else if (str.at(i) == L',')
			continue;
		else {
			all_digits = false;
			break;
		}
	}
	// A string consisting solely of a comma
	// will let us get here, even though
	// "all_digits" should arguably not be true.
	// However, we will return 0, as we do for
	// all nonnumeric words, so that's probably
	// all right.
	if (all_digits)
		return _wtoi(new_str.c_str());

	int ret_val_from_word = convertSpelledOutNumberWord(str);
	if (ret_val_from_word != INT_MIN) { // INT_MIN signals invalid value
		return ret_val_from_word;
	}

	return 0;
}

/**
 * Split multi-token number strings and convert them recursively,
 * tracking places.
 **/
int NumberConverter::convertComplexNumberWord(const std::wstring& str) {
	int ret = convertComplexTwoPartNumberWord(str);
	if (ret != INT_MIN) { // INT_MIN signals invalid value
		return ret;
	}

	std::vector<std::wstring> list_of_numbers;
	std::vector<int> list_of_ints;

	// Split on hyphens or whitespace
	boost::split(list_of_numbers, str, boost::is_any_of(L"-"));
	BOOST_FOREACH(std::wstring valI, list_of_numbers) {
		std::vector<std::wstring> tmp_num_list;
		boost::split(tmp_num_list, valI, boost::is_any_of(L" "));
		BOOST_FOREACH(std::wstring valJ, tmp_num_list) {
			std::vector<std::wstring> oth_tmp_list;
			boost::split(oth_tmp_list, valJ, boost::is_any_of(L"\n"));
			BOOST_FOREACH(std::wstring valK, oth_tmp_list) {
				list_of_ints.push_back(convertNumberWord(valK));
			}
		}
	}

	// Now list_of_ints is a flat list of values
	return calculateValue(list_of_ints);
}

/**
 * Once you've converted verbal parts to int you can calculate how much it is.
 * This only works with numbers from 0 to 999999999 (less than 1 billion).  It will
 * also fail with numbers too big for int. Not language specific.
 **/
int NumberConverter::calculateValue(const std::vector<int>& values) {
	bool bad_num = true;
	for (size_t i = 0; i < values.size(); ++i) {
		if (values[i] != 0) {
			bad_num = false;
			break;
		}
	}
	if (bad_num)
		return 0;

	std::vector<int> digits;
	for (size_t i = 0; i < 12; i++)
		digits.push_back(0);

	// Fix fully fleshed out values: "270 million" should be [2, 100, 70, 1000000]
	// for the algorithm to work.
	std::list<int> fixed;
	for (size_t i = 0; i < values.size(); ++i) {
		int value = values[i];
		// Handle complex values (i.e.: 237, this splits up the 237 to [2, 100, 30, 7])
		// Because of where this is placed it will never have to deal with
		// Values greater than 999 or less than 11:
		if (value < 10 ||
			(value >= 10 && value <= 100 && value %10 == 0) ||
			value == 100 ||
			value == 1000 ||
			value == 1000000 ||
			value == 1000000000) {
			fixed.push_back(value);
		} else if ((value > 10 && value % 10 != 0 ) || // Any value greater than 10, not divisible by 10
				   (value > 90 && value != 100)) {     // Or any value greater than 90 not equal to 100
		    std::list<int> toAdd;
			if (value % 10 != 0) {
				toAdd.push_front(value % 10); // 1s place
				value -= value % 10;
			}
			if (value % 100 != 0) {
				toAdd.push_front(value % 100); // 10s place
				value -= value % 100;
			}
			if (value) {
				toAdd.push_front(100); // 100s place
				if (value != 100)
					toAdd.push_front(value/100); // how many 100s
			}
			while (toAdd.size() != 0) {
				fixed.push_back(toAdd.front());
				toAdd.pop_front();
			}
		} else {
			SessionLogger::info("NumberConverter") << "WARNING: Number conversion failed. " << value << " found mixed with textual numbers." << std::endl;
		}
	}
	
	fixed = normalizeHundreds(fixed);

	// Store the last state of the list so that we make sure it changes between iterations.
	// The check is inefficient, but at least it prevents the code from getting stuck in an
	// infinite loop. A more thorough fix should be checked in soon.
	std::list< int > last_list;
	while (fixed.size() != 0 && 
		   (last_list.empty() || last_list.size() != fixed.size() ||
		   !std::equal(fixed.begin(), fixed.end(), last_list.begin()))) {
		last_list.clear();
		last_list.resize(fixed.size());
		std::copy(fixed.begin(), fixed.end(), last_list.begin());
		int ones = 0;
		int tens = 0;
		int hundreds = 0;
		int power_of_thousand = 1;

		for (int i = 0; i < 4; ++i) {
			if (i == 0) { //Expecting hundreds
				if (fixed.front() >= 100 && fixed.front() <= 900) {
					hundreds = fixed.front()/100;
					fixed.pop_front();
					if (fixed.size() == 0)
						break;
				}
			} else if (i == 1) { // Expecting tens
				if (fixed.front() >= 10 && fixed.front() <= 90) {
					tens = fixed.front()/10;
					fixed.pop_front();
					if (fixed.size() == 0)
						break;
				}
			} else if (i == 2) { // Expecting ones
				if (fixed.front() >= 1 && fixed.front() <= 9) {
					ones = fixed.front();
					fixed.pop_front();
					if (fixed.size() == 0)
						break;
				}
			} else { // Expecting power-of-1000
				if (fixed.front() >= 100) {
					power_of_thousand = fixed.front();
					fixed.pop_front();
				}
			}
		}

		switch (power_of_thousand) {
			case 1000000000: // Billion
				digits[0] = hundreds;
				digits[1] = tens;
				digits[2] = ones;
				break;
			case 1000000: // Million
				digits[3] = hundreds;
				digits[4] = tens;
				digits[5] = ones;
				break;
			case 1000: // Thousand
				digits[6] = hundreds;
				digits[7] = tens;
				digits[8] = ones;
				break;
			case 1: // One
				digits[9] = hundreds;
				digits[10] = tens;
				digits[11] = ones;
				break;
		}
	}

	int returnVal = 0;
	int power = 1;

	// This is dangerous because "power" overflows before the end of the sequence of digits
	// is reached. However, the bad effects are seen only if there is a nonzero
	// digit in the first three or so positions, which will be rare.
	// Note that in a 32-bit implementation, INT_MAX == 2,147,483,647.
	for (std::vector<int>::reverse_iterator it = digits.rbegin(); it != digits.rend(); it++) {
		returnVal += (power*(*it));
		power *= 10;
	}

	return returnVal;
}

/**
 * Merges, e.g., [2, 100] to [200]. Helps to collapse converted integer prefixes
 * in complex number words. Called by calculateValue; not language-specific.
 **/
std::list<int> NumberConverter::normalizeHundreds(const std::list<int>& input) {
	std::list<int> returnVal;
	bool skip_next = false;
	for (std::list<int>::const_reverse_iterator it = input.rbegin(); it != input.rend(); it++) {
		if (!skip_next) {
			if (*it == 100) {
				++it;
				if (it == input.rend()) {
					break;
				}
				if (*it < 10) {
					returnVal.push_front(100 * (*it));
					--it;
					skip_next = true;
				}
			} else {
				skip_next = false;
				if (it == input.rend()) {
					break;
				}
				returnVal.push_front(*it);
			}
		}
	}
	return returnVal;
}
