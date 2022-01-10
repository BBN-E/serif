// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <boost/shared_ptr.hpp>

#include "Generic/common/LocatedString.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Token.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"

#include <wchar.h>
#include <map>
#include <string>

// Back-doors for unit tests:
#ifdef _DEBUG
#define _protected    public
#define _private      public
#else
#define _protected    protected
#define _private      private
#endif

typedef std::map< std::wstring, std::wstring > StringStringMap;

class RegExTokenizer;
class Tokenizer {
public:
	/** Create and return a new Tokenizer. */
	static Tokenizer *build() { return _factory()->build(); }
	/** Hook for registering new Tokenizer factories */
	struct Factory { virtual Tokenizer *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~Tokenizer() {}

	virtual void resetForNewSentence(const Document *doc, int sent_no) = 0;
	
	// This does the work. It puts an array of pointers to TokenSequences
	// where specified by results arg, and returns its size. It returns
	// 0 if something goes wrong. The client is responsible for deleting
	// the array and the TokenSequences.
	virtual int getTokenTheories(TokenSequence **results, int max_theories,
								 const LocatedString *string, 
								 bool beginOfSentence = true,
								 bool endOfSentence =true) = 0;
protected:
	Tokenizer() {}

public:
	/**********************************************************************
	 ** Helper Static Methods, which can be used by subclasses.
	 **********************************************************************/
	static int matchCharacter(const LocatedString *string, int index, wchar_t character);
	static int matchOpenQuotes(const LocatedString *string, int index);
	static int matchCloseQuotes(const LocatedString *string, int index);
	static bool matchURL(const LocatedString *string, int index);

	// Returns true iff it is legal to insert whitespace at index
	static bool isSplittable(const LocatedString *string, int index);

	// Removes control codes from the string, Returns count of changes made.
	static int removeHarmfulUnicodes(LocatedString *string);

	// maps all variant spaces to the space expected by tokenizer and 
	// replaces fancy typography with simple ascii
	static int replaceNonstandardUnicodes(LocatedString *string);

	/// Removes non-ASCII characters (or replace with ASCII equivalents)
	static void standardizeNonASCII(LocatedString *string);

	// Replaces groups of three or more consecutive hyphens with two.
	static void replaceHyphenGroups(LocatedString *input);

	//Replace groups of three or more consecutive instances of any puncutation mark with 3
	static void replaceExcessivePunctuation(LocatedString *input);

	// Reads a tab-delimited string replacement file into a std::map
	static void readReplacementsFile(const char* file_name, StringStringMap* replacements);

	// Moves an index counter beyond closing quotation marks in input text.
	static int skipCloseQuotes(const LocatedString *string, int index);

	// Moves an index counter beyond whitespace in input text.
	static int skipWhitespace(const LocatedString *string, int index);


private:
	static boost::shared_ptr<Factory> &_factory();
};


#endif
