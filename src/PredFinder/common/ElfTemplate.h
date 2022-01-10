/**
 * Defines templates (such as the function object ptr_less_than).
 *
 * @file ElfTemplate.h
 * @author afrankel@bbn.com
 * @date 2011.04.21
 **/

#pragma once

#if defined(_MSC_VER) && _MSC_VER >= 1400 
#pragma warning(push) 
#pragma warning(disable:4996) // disable bogus warnings about boost::is_any_of
#endif 

//#include "ElfString.h"
#include <boost/algorithm/string.hpp>
#include <set>
#include <vector>

// Requires that T::less_than(const T &) be implemented.
template <typename T>
struct ptr_less_than {
	bool operator()(const boost::shared_ptr<T> s1, const boost::shared_ptr<T> s2) const
	{
		if (s1 == NULL) {
			if (s2 != NULL) {
				return true;
			} else {
				return false;
			}
		} else if (s2 == NULL) {
			return false;
		} else {
			return (s1->less_than(*s2));
		}
	}
};

// Declaring this inline prevents "multiple definition" warnings.
// Example 1: 
// 	static std::set< std::wstring > UNCAPITALIZED_ROYALTY = makeStrSet(L"queen king lord duke "
//	   L"prince princess");
// fills the set UNCAPITALIZED_ROYALTY with the set of wstrings "queen", "king", etc. The
// preprocessor will join the lines beginning with "queen" and with "prince"; make sure that
// each line except the last ends with a space, and that each line begins with L.
// Example 2:
//static std::set< std::wstring > HUMAN_BLOOD = makeStrSet(
//L"human|blood|heart function|adult|congress|c|head|bone|heart|kidney|neck|prostate|human cloning|multiple fronts|one-a-day", L"|"); // L("|") is the token delimiter
inline std::set< std::wstring > makeStrSet(const std::wstring & joined_tokens, 
										   const std::wstring & possible_tok_delims = L" ") {
	std::vector<std::wstring> vec;
    boost::split(vec, joined_tokens, boost::is_any_of(possible_tok_delims));
    return std::set<std::wstring>(vec.begin(), vec.end());
}


#if defined(_MSC_VER) && _MSC_VER >= 1400 
#pragma warning(pop) 
#endif 
