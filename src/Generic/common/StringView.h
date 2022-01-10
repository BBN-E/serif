// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include <boost/functional/hash.hpp>

/**
 * By setting .string = someStdString.c_str(), a 
 * StringView can track substrings without actually making them,
 * which in certain hashing situations can be a 3-4x speedup over
 * making the substring and then hashing.
 **/
template <typename charT>
struct BasicStringView {
	const charT* string;
	size_t index;
	size_t length;
};

typedef BasicStringView<char> StringView;
typedef BasicStringView<wchar_t> WStringView;

template <typename charT>
size_t hash_value(BasicStringView<charT> const& sv) {
	boost::hash<charT> hasher;
	size_t h = 7; // Prime magic numbers based on Java.String hashCode
	for (size_t c = sv.index; c < sv.index + sv.length; c++)
		h = 31*h + hasher(sv.string[c]);
	return h;
}

template <typename charT>
std::ostream& operator<<(std::ostream& o, const BasicStringView<charT> & sv) {
	o << "[" << sv.index << "," << sv.index + sv.length << "]";
	return o;
}
