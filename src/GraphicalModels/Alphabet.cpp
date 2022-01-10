#include <boost/foreach.hpp>

#include "Alphabet.h"
#include <iostream>
#include "Generic/common/foreach_pair.hpp"

using std::wstring;
using std::map;
using std::wcout;
using std::endl;
using namespace GraphicalModel;

wstring UNKNOWN_FEATURE = L"unknown_feature";

Alphabet::Alphabet() {}

unsigned int Alphabet::feature(const wstring& featureString) {
	StringToInt::const_iterator probe = stringToInt.find(featureString);
	if (probe == stringToInt.end()) {
		unsigned int next_idx = stringToInt.size();
		stringToInt.insert(make_pair(featureString, next_idx));
		intToString.insert(make_pair(next_idx, featureString));
		return next_idx;
	} else {
		return probe->second;
	}
}

unsigned int Alphabet::feature(const wstring& featureString) const {
	return featureAlreadyPresent(featureString);
}

unsigned int Alphabet::featureAlreadyPresent(const wstring& featureString) const {
	StringToInt::const_iterator probe = stringToInt.find(featureString);
	if (probe == stringToInt.end()) {
		std::wcout << featureString << endl;
		throw BadLookupException();
	} else {
		return probe->second;
	}
}

const wstring& Alphabet::name(unsigned int featureIdx) const {
	IntToString::const_iterator probe = intToString.find(featureIdx);
	if (probe == intToString.end()) {
		return UNKNOWN_FEATURE;
	} else {
		return probe->second;
	}
}

void Alphabet::dump() const {
	BOOST_FOREACH_PAIR(unsigned int idx, const wstring& name, intToString) {
		std::wcout << idx << L": " << name << endl;
	}
}

size_t Alphabet::size() const {
	return stringToInt.size();
}

