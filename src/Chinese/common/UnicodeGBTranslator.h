// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef UNICODE_GB_TRANSLATOR
#define UNICODE_GB_TRANSLATOR

#include "Generic/common/Symbol.h"
#include "Generic/common/hash_map.h"

/**
 * Singleton utility class for translation of Chinese text from 
 * Unicode to GB encoding and vice versa. 
 */
class UnicodeGBTranslator {

public:
	/** 
	  * Returns a pointer to an instance of the singleton
	  * class UnicodeGBTranslator. Initializes the Unicode-GB 
	  * table from file on first call.
	  *
	  * @return a pointer to an instance of UnicodeGBTranslator
	  */
	static UnicodeGBTranslator* getInstance();
protected:
	UnicodeGBTranslator();
private:
	static UnicodeGBTranslator* _instance;
	
	static const float targetLoadingFactor;
	
	struct HashKey {
        size_t operator()(const Symbol& s) const {
            return s.hash_code();
        }
    };
    struct EqualKey {
        bool operator()(const Symbol& s1, const Symbol& s2) const {
            return s1 == s2;
        }
    };

    typedef serif::hash_map<Symbol, Symbol, HashKey, EqualKey> SymbolHash;
	SymbolHash *_gb2unicode;
	SymbolHash *_unicode2gb;

	static const Symbol NIL;


public:
	/** 
	  * Returns the GB equivalent of the given Unicode
	  * char.  If the given char is not found in the 
	  * Chinese Unicode table, returns black square
	  * (0x25A0).
	  *
	  * Exception: If the given char is in the ASCII 
	  * character range just returns itself.
	  *
	  * @param u_ch the Unicode character to be converted
	  * @return the GB equivalent to <code>u_ch</code>
	  */
	wchar_t unicode2GB(const wchar_t u_ch) const;

	/** 
	  * Returns the Unicode equivalent of the given GB
	  * char.  If the given char is not found in the 
	  * Chinese GB table, returns black square
	  * (0x25A0).
	  *
	  * Exception: If the given char is in the ASCII 
	  * character range just returns itself.
	  *
	  * @param gb_ch the GB character to be converted
	  * @return the Unicode equivalent to <code>gb_ch</code>
	  */
	wchar_t gb2Unicode(const wchar_t gb_ch) const;

	/**
	  * Puts the GB equivalent of the given Unicode
	  * string in the given empty GB string. 
	  *
	  * Note: <code>gb_str</code> should be equal or greater in
	  * length than <code>u_str</code>.
	  *
	  * @param u_str the Unicode string to be converted.
	  * @param gb_str the resulting GB string
	  */
	void unicode2GB(const wchar_t *u_str, wchar_t *gb_str) const;

	/**
	  * Puts the Unicode equivalent of the given GB
	  * string in the given empty Unicode string. 
	  *
	  * Note: <code>u_str</code> should be equal or greater in
	  * length than <code>gb_str</code>.
	  *
	  * @param gb_str the GB string to be converted.
	  * @param u_str the resulting Unicode string
	  */
	void gb2Unicode(const wchar_t *gb_str, wchar_t *u_str) const;

private:
	/** 
	  * Reads mapping of Unicode to GB character
	  * encodings from given file.  The first line
	  * of the file should contain the number of
	  * table entries. Each remaining line of the 
	  * file should have the format:
	  *
	  * <Unicode Hex code> <GB hex code - 0x8080>
	  *
	  * @param tableFile the file to read
	  */
	void readUnicodeGBTable(const char* tableFile); 

	/** 
	  * Reads mapping of GB to Unicode character
	  * encodings from given file.  The first line
	  * of the file should contain the number of
	  * table entries. Each remaining line of the 
	  * file should have the format:
	  *
	  * <Unicode Hex code> <GB hex code - 0x8080>
	  *
	  * @param tableFile the file to read
	  */
	void readGBUnicodeTable(const char* tableFile); 

	/** 
	  * Returns the GB Symbol equivalent of the given 
	  * Unicode Symbol. If the given Symbol is not found 
	  * in the Chinese Unicode table, returns NIL.
	  *
	  * @param ch the Unicode Symbol to be converted
	  * @return the GB Symbol equivalent to <code>ch</code>
	  */
	const Symbol unicode2GBLookup(Symbol ch) const;
	
	/** 
	  * Returns the Unicode Symbol equivalent of the given 
	  * GB Symbol. If the given Symbol is not found 
	  * in the Chinese GB table, returns NIL.
	  *
	  * @param ch the GB Symbol to be converted
	  * @return the Unicode Symbol equivalent to <code>ch</code>
	  */
	const Symbol gb2UnicodeLookup(Symbol ch) const;


};

#endif
