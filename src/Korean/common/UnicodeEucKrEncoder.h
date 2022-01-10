// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef UNICODE_EUC_KR_ENCODER
#define UNICODE_EUC_KR_ENCODER

#include "common/Symbol.h"
#include "common/hash_map.h"
#include <iostream>
#include <fstream>

/**
 * Singleton utility class for translation of Korean text from 
 * Unicode to EUC-KR encoding and vice versa. 
 */
class UnicodeEucKrEncoder {

public:
	/** 
	  * Returns a pointer to an instance of the singleton
	  * class UnicodeEucKrEncoder. Initializes the Unicode-EUC-KR 
	  * table from file on first call.
	  *
	  * @return a pointer to an instance of UnicodeEucKrEncoder
	  */
	static UnicodeEucKrEncoder* getInstance();
protected:
	UnicodeEucKrEncoder();
private:
	static UnicodeEucKrEncoder* _instance;
	
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

    typedef hash_map<Symbol, Symbol, HashKey, EqualKey> SymbolHash;
	SymbolHash *_euc2unicode;
	SymbolHash *_unicode2euc;

	static const Symbol NIL;

public:
	
	/** 
	  * Returns the EUC-KR equivalent of the given Unicode
	  * char.  If the given char is not found in the 
	  * EUC-KR Unicode table, returns black square
	  * (0x25A0).
	  *
	  * Exception: If the given char is in the ASCII 
	  * character range just returns itself.
	  *
	  * @param u_ch the Unicode character to be converted
	  * @return the EUC-KR equivalent to <code>u_ch</code>
	  */
	wchar_t unicode2EUC(const wchar_t u_ch) const;

	/** 
	  * Returns the Unicode equivalent of the given EUC-KR
	  * char.  If the given char is not found in the 
	  * Korean EUC-KR table, returns black square
	  * (0x25A0).
	  *
	  * Exception: If the given char is in the ASCII 
	  * character range just returns itself.
	  *
	  * @param euc_ch the EUC-KR character to be converted
	  * @return the Unicode equivalent to <code>euc_ch</code>
	  */
	wchar_t euc2Unicode(const wchar_t euc_ch) const;

	/**
	  * Puts the EUC-KR equivalent of the given Unicode
	  * string in the given empty EUC-KR string. 
	  *
	  * Note: <code>euc_str</code> should be equal or greater in
	  * length than <code>u_str</code>.
	  *
	  * @param u_str the Unicode string to be converted.
	  * @param euc_str the resulting EUC-KR string
	  */
	void unicode2EUC(const wchar_t *u_str, wchar_t *euc_str, int max_len) const;

	/**
	  * Puts the EUC-KR equivalent of the given Unicode
	  * string in the given empty EUC-KR string. 
	  *
	  * Note: <code>euc_str</code> should be equal or greater in
	  * length than <code>u_str</code>.
	  *
	  * @param u_str the Unicode string to be converted.
	  * @param euc_str the resulting EUC-KR string
	  */
	void unicode2EUC(const wchar_t *u_str, unsigned char *euc_str, int max_len) const;

	/**
	  * Returns the EUC-KR equivalent of the given Unicode
	  * string. 
	  *
	  * @param u_str the Unicode string to be converted.
	  * @return the resulting EUC-KR string
	  */
	std::wstring unicode2EUC(std::wstring u_str) const;

	/**
	  * Puts the Unicode equivalent of the given EUC-KR
	  * string in the given empty Unicode string. 
	  *
	  * Note: <code>u_str</code> should be equal or greater in
	  * length than <code>euc_str</code>.
	  *
	  * @param euc_str the EUC-KR string to be converted.
	  * @param u_str the resulting Unicode string
	  */
	void euc2Unicode(const wchar_t *euc_str, wchar_t *u_str, int max_len) const;

	/**
	  * Returns the Unicode equivalent of the given EUC-KR
	  * string. 
	  *
	  *
	  * @param euc_str the EUC-KR string to be converted.
	  * @return the resulting Unicode string
	  */
	std::wstring euc2Unicode(std::string euc_str) const;

	/**
	  * Returns the Unicode equivalent of the given EUC-KR
	  * string. 
	  *
	  *
	  * @param euc_str the EUC-KR string to be converted.
	  * @return the resulting Unicode string
	  */
	std::wstring euc2Unicode(std::wstring euc_str) const;

	/**
	  * Puts the Unicode equivalent of the given EUC-KR
	  * string in the given empty Unicode string. 
	  *
	  * Note: <code>u_str</code> should be equal or greater in
	  * length than <code>euc_str</code>.
	  *
	  * @param euc_str the EUC-KR string to be converted.
	  * @param u_str the resulting Unicode string
	  */
	void euc2Unicode(unsigned char *euc_str, wchar_t *u_str, int max_len) const;

	/**
	  * Puts the Unicode Hangul equivalent of the given 
	  * UTF8 string in the given empty string. 
	  *
	  * Note: <code>hangul_str</code> should be equal or greater in
	  * length than <code>str</code>.
	  *
	  * @param str the UTF8 string to be converted.
	  * @param hangul_str the resulting Hangul string
	  */
	void decomposeHangul(const wchar_t *str, wchar_t *hangul_str, int max_len) const;
	void decomposeHangul(const wchar_t *str, wchar_t *hangul_str, int *map, int max_len) const;
private:
	/** 
	  * Reads mapping of Unicode to EUC-KR character
	  * encodings from given file.  The first line
	  * of the file should contain the number of
	  * table entries. Each remaining line of the 
	  * file should have the format:
	  *
	  * <EUC hex code - 0x8080> <Unicode Hex code> 
	  *
	  * @param tableFile the file to read
	  */
	void readUnicodeEUCTable(const char* tableFile); 

	/** 
	  * Reads mapping of EUC-KR to Unicode character
	  * encodings from given file.  The first line
	  * of the file should contain the number of
	  * table entries. Each remaining line of the 
	  * file should have the format:
	  *
	  * <EUC hex code - 0x8080> <Unicode Hex code>
	  *
	  * @param tableFile the file to read
	  */
	void readEUCUnicodeTable(const char* tableFile); 

	/** 
	  * Returns the EUC-KR Symbol equivalent of the given 
	  * Unicode Symbol. If the given Symbol is not found 
	  * in the Korean Unicode table, returns NIL.
	  *
	  * @param ch the Unicode Symbol to be converted
	  * @return the EUC-KR Symbol equivalent to <code>ch</code>
	  */
	const Symbol unicode2EUCLookup(Symbol ch) const;
	
	/** 
	  * Returns the Unicode Symbol equivalent of the given 
	  * EUC-KR Symbol. If the given Symbol is not found 
	  * in the Korean EUC-KR table, returns NIL.
	  *
	  * @param ch the EUC-KR Symbol to be converted
	  * @return the Unicode Symbol equivalent to <code>ch</code>
	  */
	const Symbol euc2UnicodeLookup(Symbol ch) const;

	std::wstring convertToWideChars(std::string str) const;

};

#endif
