// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UNICODE_UTIL_H
#define UNICODE_UTIL_H

#include <string>

/** An all-static helper class used to convert between UTF-8 (stored as 
  * char* or std::string) and UTF-16 (stored as wchar_t* or std::wstring). */
class UnicodeUtil {
public:
	/*********************************************************************
	 * UTF8 <-> UTF16 Conversions
	 *********************************************************************
	 * These methods originated from DistillUtilities.h */

	typedef enum {DIE_ON_ERROR, REPLACE_ON_ERROR}  ErrorResponse;
	static const wchar_t REPLACE_CHAR; // == L'?'

	/* Convert a UTF-16 encoded std::wstring to a UTF-8 encoded std::string */
	static std::string toUTF8StdString(const std::wstring &wstr);
	/* Convert a UTF-8 encoded std::string to a UTF-16 encoded std::wstring */
	static std::wstring toUTF16StdString(const std::string &str, ErrorResponse error_response=DIE_ON_ERROR);
	/* No-ops that might come up in certain templated contexts */
	static std::string toUTF8StdString(const std::string &str) { return str; }
	static std::wstring toUTF16StdString(const std::wstring &wstr) { return wstr; }
	
	/* Copies a char* into a wchar_t* -- this is safe, but don't do it the other way around! */
	static void copyCharToWChar(const char* cstr, wchar_t* wcstr, size_t max_len);
	/* ...although you can try: */
	static void copyWCharToChar(const wchar_t* wstr, char* cstr, size_t max_len);

	/** Write the utf-8 equivalent of the given character to dst.
	  * dst must have room to add at least 3 characters.  Return
	  * the number of characters that were added.  Raise an
	  * exception if the conversion is illegal.  This does *not*
	  * add any null-padding at the end of the newly-written string! 
	  * CharType should be either 'char' or 'unsigned char'.*/
	template<typename CharType>
	static size_t convertUTF8Char(const wchar_t src, CharType *dst);
	static int countUTF8Bytes(const wchar_t src);

	static char* toUTF8Char(const wchar_t wchar);    // It is the responsibility of the caller to delete the returned pointer
	static char* toUTF8String(const wchar_t* wstr);  // It is the responsibility of the caller to delete the returned pointer
	static char* toUTF8String(const std::wstring& wstr) { return toUTF8String(wstr.c_str()); }  // It is the responsibility of the caller to delete the returned pointer
	static char* toUTF8StringWithSpaces(const wchar_t* wstr);  // It is the responsibility of the caller to delete the returned pointer
	static char* toUTF8StringWithSpaces(const std::wstring& wstr) { return toUTF8StringWithSpaces(wstr.c_str()); }  // It is the responsibility of the caller to delete the returned pointer
	static wchar_t* toUTF16String(const char* str, ErrorResponse error_response=DIE_ON_ERROR);  // It is the responsibility of the caller to delete the returned pointer
	static wchar_t* toUTF16String(const std::string& str, ErrorResponse error_response=DIE_ON_ERROR) { return toUTF16String(str.c_str(), error_response); }  // It is the responsibility of the caller to delete the returned pointer

	/*********************************************************************
	 * String Normalization
	 *********************************************************************
	 * These methods originated from DistillUtilities.h */

	/** normalize string: 
	  *   1) replace all non alpha-numerics by spaces
	  *   2) replace multiple spaces/tabs by one space
	  *   3) remove leading and trailing spaces
	  *   4) remove capitalization */
	static std::wstring normalizeTextString(std::wstring s);

	/** normalize name:
	  *   1) if the initial word contains no letters (and is not the only
	  *      word), then remove it.
	  *
	  * Why is this done to normalize names?  If you know, please update
	  * these docs. */
	static std::wstring normalizeNameString(std::wstring str);

	/* doubles apsotrophes to satisfy SQL syntax */
	static std::wstring sqlEscapeApos(const std::wstring & ws);

	/* combine text, name, and sql apos operations */
	static std::wstring normalizeNameForSql(const std::wstring & wstr);

	/* return true if name2 is the same as name1 or a non-trivial sub name of name1 */
	static bool nonTrivialSubName(const std::wstring & name1, const std::wstring & name2);

	/* remove dashes by shortening string instead of replacing with blanks */
	static std::wstring squeezeOutDashes(const std::wstring & ws);
	
	/* replace dashes with blanks */
	static std::wstring whiteOutDashes(const std::wstring & ws);

	/* squeeze out dashes and then do the two normalizations */
	static std::wstring dashSqueezedNormalizedName(const std::wstring & name);



};
#endif
