// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ABBREVTABLE_H
#define ABBREVTABLE_H

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolArray.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/DebugStream.h"
#include "Generic/common/UTF8Token.h"

/***
 
 This is a table whose purpose is to store translation mappings of symbol arrays.
 The original target use of the table was for name linking: we want to say e.g. that "american" and "america",
 "corp." and "corporation", etc. corefer. We loosely refer to these forced coreferences as "abbreviations."
 
 * What we do specifically is:
		- denote all of these forced coreferences in a data file in Sexp format, e.g. (fullword abbr1 abbr2 abbr3).
		- when we encounter e.g. "abbr1" in the text of a mention, we will resolve it to "fullword."
		- "resolve" means "replace symbols inline," i.e., act as if we had actually seen "fullword" and not "abbr1."
		- in resolving, we do not literally reach in and change the text of the mention; we simply take as 
		  input an array of symbols and output a "resolved" array. This resolved array is then used by the namelinker
          as the "real text" of the mention, but only for name linking purposes.
 * There are two tables present in AbbrevTable, STATIC and DYNAMIC.
		- The STATIC table is loaded from the data file and is not changed during processing.
		- The DYNAMIC table is initially empty, and can be updated and added to based on what we see in the text.
		- For English, we use the dynamic table to resolve acronyms: e.g., "CIA" becomes "Central Intelligence Agency."
		- Specifically, when we see something with >1 significant word (significant meaning e.g. "of" doesn't count) 
		  which is an ORG or GPE, we find various possible ways to "acronymize" the mention, and then tell the dynamic
		  table: if you see one of these acronyms, replace it with this string of words.
		- Thus, we can only resolve "CIA" on the spot if we have already seen a mention which can be acronymized "CIA." 
			e.g. "Carlisle International Academy" or "Central Intelligence Agency" or whatever.
		- Suppose we see the acronym first and later the full resolved form. We are able to "forward-resolve" the 
		  acronym by using NameLinkFunctions::recomputeCounts(), which is called at each name linking decision and which
		  updates an entity's unigram lexical model according to whatever new information might be in the dynamic table.
 * Additional notes:
		- The resolution process allows for "one symbol -> many symbols" mention text coercions, as well as 
		  "full mention text -> many symbols" coercions. e.g.:
				"CIA -> Central Intelligence Agency" (first case)
				"United States -> America" (second case; special coercion which happens only when "United States" is the 
						full mention text. "many symbols" obviously encompasses the case of "one symbol.")
		- it would not be too hard to add general "many -> many" functionality, but it would require a few changes.
		- TODO notes about the relative ordering of dynamic/static resolution
 
// 7/21/04: Fixed a bug. Now whole-mention bindings aren't the only thing that happen


***/


class AbbrevTable {
public:
	static void initialize();
	static void destroy();
	static void cleanUpAfterDocument();

	static int resolveSymbols(const Symbol words[], int nWords, Symbol results[], int max_results);
	static void add(Symbol key[], int nKey, Symbol value[], int nValue);
	
	
private:
	enum TIndex {STATIC = 0, DYNAMIC, NUM_TABLES};

	static bool add(SymbolArray *key, SymbolArray *value, TIndex index);
	static int lookupPhrase(const Symbol words[], int nWords, Symbol results[], int max_results);
	
	class FileReader {
	public:
		FileReader(UTF8InputStream & file);
		void getLeftParen() throw(UnexpectedInputException);
		void getRightParen() throw(UnexpectedInputException);
		UTF8Token getNonEOF() throw(UnexpectedInputException);
		UTF8Token getToken(int specifier);
		bool hasMoreTokens();
		int getSymbolArray(Symbol results[], int max_results) throw(UnexpectedInputException);
		int getOptionalSymbolArray(Symbol results[], int max_results) throw(UnexpectedInputException);
		void pushBack(UTF8Token token);
	private:
		UTF8InputStream & _file;
		enum TokenType {
			LPAREN = 1 << 0,
			RPAREN = 1 << 1,
			EOFTOKEN = 1 << 2,
			WORD   = 1 << 3};
		int _nTokenCache;
		static const int MAX_CACHE = 5;
		UTF8Token _tokenCache[5];
	};


	// _table is the actual abbreviation table (which comes in two parts, a STATIC table and a DYNAMIC table)
	static SymbolArraySymbolArrayMap *_table[2];
	static DebugStream _debugOut;
	static bool is_initialized;
};

#endif
