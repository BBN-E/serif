// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/UnexpectedInputException.h"

#include <boost/algorithm/string.hpp>
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#include <boost/regex.hpp>
#pragma warning(pop)

const wchar_t UnicodeUtil::REPLACE_CHAR = L'?';

void UnicodeUtil::copyWCharToChar(const wchar_t* wstr, char* cstr, size_t max_len){
	size_t length = wcslen(wstr);
	//old code ignored buffer overflow, so just truncate w/o complaining
	if(length > (max_len-1)){
		length = max_len-1;
	}
	int char_count = 0;
	for (size_t i = 0; i < length; i++) {
		if (wstr[i] == 0x0000)
			break;
		if (wstr[i] <= 0x00ff)
			cstr[char_count++] = (char)wstr[i];
		else {
			cstr[char_count++] = (char)(wstr[i] >> 8);
			cstr[char_count++] = (char)wstr[i];
		}
	}
	cstr[char_count] = '\0';
}


void UnicodeUtil::copyCharToWChar(const char* cstr, wchar_t* wcstr, size_t max_len) {				
	size_t length = strlen(cstr);
	//old code ignored buffer overflow, so just truncate w/o complaining
	if(length > (max_len-1)){
		length = max_len-1;
	}
	int char_count = 0;
	for (size_t i = 0; i < length; i++) {
		if (cstr[i] == 0x0000)
			break;
		wcstr[char_count++] = (wchar_t)cstr[i];
	}
	wcstr[char_count] = L'\0';
}


template<typename CharType>
size_t UnicodeUtil::convertUTF8Char(const wchar_t src, CharType *dst) {
	//Masks to select parts of the UTF-16 wide character
	static const unsigned char baseMask = 0x80;
	static const unsigned char lowMask = 0x3F;

	//Check the Unicode codepoint range
	if (src <= 0x007f) {
		// < 7F -> 0xxxxxxx
		dst[0] = static_cast<CharType>(src);
		return 1;
	} else if (src <= 0x07ff) {
		// < 7FF -> 110xxxxx 10xxxxxx
		dst[0] = (0xC0 | static_cast<CharType>((src >> 6)));
		dst[1] = (baseMask | (lowMask & static_cast<CharType>(src)));
		return 2;
	} else if (src <= 0xffff) {
		// < FFFF -> 1110xxxx 10xxxxxx 10xxxxxx
		dst[0] = (0xE0 | static_cast<CharType>((src >> 12)));
		dst[1] = (baseMask | (lowMask & static_cast<CharType>((src >> 6))));
		dst[2] = (baseMask | (lowMask & static_cast<CharType>(src)));
		return 3;
	} else {
		throw UnexpectedInputException("UnicodeUtil::toUTF8Char", "Invalid Unicode wide character");
	}
}
// Explicit template instantiations for convertUTF8Char.
template size_t UnicodeUtil::convertUTF8Char(const wchar_t src, char *dst);
template size_t UnicodeUtil::convertUTF8Char(const wchar_t src, unsigned char *dst);

int UnicodeUtil::countUTF8Bytes(const wchar_t src) {
	//Masks to select parts of the UTF-16 wide character
	static const unsigned char baseMask = 0x80;
	static const unsigned char lowMask = 0x3F;

	//Check the Unicode codepoint range
	if (src <= 0x007f) {
		// < 7F -> 0xxxxxxx
		return 1;
	} else if (src <= 0x07ff) {
		// < 7FF -> 110xxxxx 10xxxxxx
		return 2;
	} else if (src <= 0xffff) {
		// < FFFF -> 1110xxxx 10xxxxxx 10xxxxxx
		return 3;
	} else {
		throw UnexpectedInputException("UnicodeUtil::countUTF8Bytes", "Invalid Unicode wide character");
	}
}

char* UnicodeUtil::toUTF8Char(const wchar_t wchar) {
	//Masks to select parts of the UTF-16 wide character
	static const unsigned char baseMask = 0x80;
	static const unsigned char lowMask = 0x3F;

	//The null-terminated C string containing 1-3 UTF-8 bytes
	char* utf8char = NULL;
	
	//Check the Unicode codepoint range
	if (wchar <= 0x007f) {
		// < 7F -> 0xxxxxxx
		utf8char = _new char[2];
		utf8char[0] = (char) wchar;
		utf8char[1] = '\0';
	} else if (wchar <= 0x07ff) {
		// < 7FF -> 110xxxxx 10xxxxxx
		utf8char = _new char[3];
		utf8char[0] = (0xC0 | ((char)(wchar >> 6)));
		utf8char[1] = (baseMask | (lowMask & ((char) wchar)));
		utf8char[2] = '\0';
	} else if (wchar <= 0xffff) {
		// < FFFF -> 1110xxxx 10xxxxxx 10xxxxxx
		utf8char = _new char[4];
		utf8char[0] = (0xE0 | ((char)(wchar >> 12)));
		utf8char[1] = (baseMask | (lowMask & ((char)(wchar >> 6))));
		utf8char[2] = (baseMask | (lowMask & ((char) wchar)));
		utf8char[3] = '\0';
	} else {
		throw UnexpectedInputException("UnicodeUtil::toUTF8Char", "Invalid Unicode wide character");
	}

	//Return the UTF8 bytes
	return utf8char;
}

char* UnicodeUtil::toUTF8String(const wchar_t* wstr) {
	//Check the input string
	if (wstr == NULL)
		return NULL;

	//Get the length of the input string
	size_t chars = wcslen(wstr);

	//The null-terminated C string containing the converted UTF-8 bytes
	//  Make it big enough to handle the worst case: all 3-byte UTF-8 characters
	char* utf8str = _new char[3*chars + 1];

	// Convert each character.
	char *p = utf8str;
	while (*wstr != L'\0') {
	    p += convertUTF8Char(*wstr, p);
	    ++wstr;
	}
	*p = '\0';

	//Return the constructed UTF-8 string
	return utf8str;
}

char* UnicodeUtil::toUTF8StringWithSpaces(const wchar_t* wstr) {
	//Check the input string
	if (wstr == NULL)
		return NULL;

	//Get the length of the input string
	size_t chars = wcslen(wstr);

	//The null-terminated C string containing the converted UTF-8 bytes
	//  Make it big enough to handle the worst case: all 3-byte UTF-8 characters,
	//  (3 bytes for each char, plus 1 for space) plus one extra for the null.
	char* utf8str = _new char[4*chars + 1];

	// Convert each character.
	char *p = utf8str;
	while (*wstr != L'\0') {
	    p += convertUTF8Char(*wstr, p);
	    *(p++) = ' ';
	    ++wstr;
	}
	*p = '\0';

	//Return the constructed UTF-8 string
	return utf8str;
}

wchar_t* UnicodeUtil::toUTF16String(const char* str, UnicodeUtil::ErrorResponse error_response) {
	//Masks to select parts of the UTF-8 bytes
	static const unsigned char baseMask = 0x80;
	static const unsigned char lowMask = 0x3F;

	//Check the input string
	if (str == NULL)
		return NULL;

	//Get the length of the input string in bytes
	size_t bytes = strlen(str);

	//The null-terminated wide string containing the converted UTF-16 characters
	//  Make it big enough to handle the worst case: all 1-byte UTF-8 characters
	wchar_t* utf16str = _new wchar_t[bytes + 1];
	memset(utf16str, '\0', bytes + 1);

	//Track the current position in each string
	size_t i, j;

	//Loop over the input UTF-8 bytes
	for (i = 0, j = 0; i <= bytes; j++) {
		//Check the current byte flag
		//std::wcout << str[i] << " ";
		if ((str[i] & 0xE0) == 0xE0) {
			// 1110xxxx 10xxxxxx 10xxxxxx -> 0x0800 - 0xFFFF
			if (bytes - i < 3) {
				if (error_response == DIE_ON_ERROR)
					throw UnexpectedInputException("UnicodeUtil::toUTF16String", "Truncated UTF-8 character");
				utf16str[j] = REPLACE_CHAR;
				++i;
			}
			if (((str[i + 1] & baseMask) != baseMask) || ((str[i + 2] & baseMask) != baseMask)) {
				if (error_response == DIE_ON_ERROR)
					throw UnexpectedInputException("UnicodeUtil::toUTF16String", "Invalid UTF-8 byte");
				utf16str[j] = REPLACE_CHAR;
				++i;
			} else {
				utf16str[j] = ((wchar_t)(0x0F & str[i])) << 12;
				utf16str[j] |= ((wchar_t)(lowMask & str[i + 1])) << 6;
				utf16str[j] |= ((wchar_t)(lowMask & str[i + 2]));
				i += 3;
			}
		} else if ((str[i] & 0xC0) == 0xC0) {
			// 110xxxxx 10xxxxxx -> 0x0080 - 0x07FF
			if (bytes - i < 2) {
				if (error_response == DIE_ON_ERROR)
					throw UnexpectedInputException("UnicodeUtil::toUTF16String", "Truncated UTF-8 character");
				utf16str[j] = REPLACE_CHAR;
				++i;
			}
			if ((str[i + 1] & baseMask) != baseMask) {
				if (error_response == DIE_ON_ERROR)
					throw UnexpectedInputException("UnicodeUtil::toUTF16String", "Invalid UTF-8 low byte");
				utf16str[j] = REPLACE_CHAR;
				++i;
			} else {
				utf16str[j] = ((wchar_t)(0x1F & str[i])) << 6;
				utf16str[j] |= ((wchar_t)(lowMask & str[i + 1]));
				i += 2;
			}
		} else if ((str[i] & 0x80) == 0x00) {
			// 0xxxxxxx -> 0x0000 - 0x007F
			utf16str[j] = (wchar_t) str[i];
			i++;
		} else {
			if (error_response == DIE_ON_ERROR)
				throw UnexpectedInputException("UnicodeUtil::toUTF16String", "Invalid UTF-8 high byte");
			utf16str[j] = REPLACE_CHAR;
			++i;
		}
	}

	//Return the constructed UTF-16 string
	return utf16str;
}

std::string UnicodeUtil::toUTF8StdString(const std::wstring &wstr) {
	char* buffer = toUTF8String(wstr.c_str());
	std::string result(buffer);
	delete[] buffer;
	return result;
}

std::wstring UnicodeUtil::toUTF16StdString(const std::string &str, UnicodeUtil::ErrorResponse error_response) {
	wchar_t* buffer = toUTF16String(str.c_str(), error_response);
	std::wstring result(buffer);
	delete[] buffer;
	return result;
}

//Boost supports unicode, and so chinese hieroglyphs and arabic letters will 
// be correctly recognized as [:alnum:].  Note that we pass the argument by
// value -- i.e., we start the function with a *copy* of the string that was
// passed in.
std::wstring UnicodeUtil::normalizeTextString(std::wstring str) {
	// regexps used by this method.  These are static so we don't have to
	// re-generate them each time this method is called.
	static const boost::wregex escapedPunct(L"(-[LR][RC]B-)|(&(amp;)+)");
	static const boost::wregex punct(L"[^[:alnum:]]+");
	static const boost::wregex whitespace(L"[\\t ]+");
	//replace special tokens by spaces
	str = boost::regex_replace(str, escapedPunct, L" ", boost::match_default | boost::format_sed);
	//replace non-alphanumerics by spaces
	str = boost::regex_replace(str, punct, L" ", boost::match_default | boost::format_sed);
	//multiple spaces ==> one ' '
	str = boost::regex_replace(str, whitespace, L" ", boost::match_default | boost::format_sed);
	//trim leading and trailing spaces
	boost::trim(str);
	//remove capitalization
	std::transform(str.begin(), str.end(), str.begin(), std::towlower);
	return str;
}

/* !!!!!! WARNING:
Boost supports unicode, and so chinese hieroglyphs and arabic letters will be recognized as [:alnum:].
However: the equivalent names for English are created using /sed/ which is not unicode-aware.
this means that if an arabic character occurs in english name, it WON'T be removed by this function,
and the otherwize similar name won't be found!!!
*/
std::wstring UnicodeUtil::normalizeNameString(std::wstring str){
	static const boost::wregex e1(L"^[^[:alpha:]]+\\s+");
	//remove all initial words that don't contain letters:
	//!!! NOTE: USING REGULAR NORMALIZATION FIRST, AND NAME NORMALIZATION NEXT IS _NOT_ THE SAME
	//	    	AS NORMALIZATION PERFORMED ON NAMES DURING EQUIV. NAMES GENERATION! (but it's close)
	return boost::regex_replace(str, e1, L"", boost::match_default | boost::format_sed);
}

std::wstring UnicodeUtil::normalizeNameForSql(const std::wstring & raw){
	std::wstring norm = UnicodeUtil::normalizeTextString(raw);
	norm = UnicodeUtil::normalizeNameString(norm);
	return UnicodeUtil::sqlEscapeApos(norm);
}

std::wstring UnicodeUtil::sqlEscapeApos(const std::wstring & ws){
	// substitute "'" with "''" for sql
	std::wstring ret = L"";
	for (size_t i = 0; i < ws.length(); i++) {
		wchar_t letter = ws[i];
		if (letter == L'\'')
			ret.append(L"''");
		else
			ret.append(1, letter);
	}
	return ret;
}

/* true if name2 is the same as name1 or a non-trivial sub name of name1 */
bool UnicodeUtil::nonTrivialSubName(const std::wstring & name1, const std::wstring & name2){
	if (name2.length() > name1.length()) return false;
	if (name1.find(L' ') == std::wstring::npos) return false;

	std::wstring low1 = name1;
	std::transform(low1.begin(), low1.end(), low1.begin(), towlower);
	std::wstring low2 = name2;
	std::transform(low2.begin(), low2.end(), low2.begin(), towlower);

	// check for trivial names -- make more efficient if list gets big
	if (low2 == L"al" || low2 == L"de" || low2 == L"bin" )
		return false;

	std::wstring padN2 = L' ' + low2 + L' ';
	std::wstring padN1 = L' ' + low1 + L' ';
	size_t pN2Len = padN2.length();
	size_t pN1Len = padN1.length();
	size_t n2InN1 = padN1.find(padN2);
	// covered by length test at start of method
	//if (n2InN1 == std::wstring::npos) return false;
	if (n2InN1 == 0  || n2InN1 == (pN1Len - pN2Len)){
		// name1 has at least one blank in it and its token sequence
		// includes token sequence of name2
		// with the match pinned at first or last of the bigger name.
		// So we think name1 is of type <First <Middle>* Last> and 
		// name2 is of type <First <Middle>*> or type <<Middle>* Last>
		return true;
	}
	return false;
}

std::wstring UnicodeUtil::squeezeOutDashes(const std::wstring & ws){
	// substitute "-" with nothing so "osama bin-laden" => "osama binladen"
	std::wstring ret = L"";
	wchar_t lastChar = ' ';
	for (size_t i = 0; i < ws.length(); i++) {
		wchar_t letter = ws[i];
		if ((lastChar == '-') && (letter == ' '))
			continue;  //remove all spaces after a dash
		if (letter != L'-')
			ret.append(1, letter);
		lastChar = letter;
	}
	return ret;
}
std::wstring UnicodeUtil::whiteOutDashes(const std::wstring & ws){
	// substitute "-" with " " so "osama bin-laden" => "osama bin laden"
	// and "Ahmed Al - Sultan" => "Ahmed Al Sultan"
	std::wstring ret = L"";
	wchar_t lastChar = ' ';
	for (size_t i = 0; i < ws.length(); i++) {
		wchar_t letter = ws[i];
		if (lastChar == ' ' && letter == ' ')
			continue;  //don't allow or create multiple spaces
		if (letter == L'-')
			letter = ' ';
		lastChar = letter;
		ret.append(1, letter);
	}
	return ret;
}

std::wstring UnicodeUtil::dashSqueezedNormalizedName(const std::wstring & name){
	std::wstring norm_name = squeezeOutDashes(name);
	norm_name = normalizeTextString(norm_name);
	norm_name = normalizeNameString(norm_name);
	return norm_name;
}
