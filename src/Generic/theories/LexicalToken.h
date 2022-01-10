// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LEXICAL_TOKEN_H
#define LEXICAL_TOKEN_H
#include "Generic/common/Symbol.h"
#include "Generic/common/LocatedString.h"
#include "Generic/theories/Token.h"

#include "Generic/common/limits.h"
#include "Generic/theories/LexicalEntry.h"
#include "Generic/theories/Lexicon.h"

#include <wchar.h>
#include <iostream>

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

// XX WARNING -- STATE SAVER DOES NOT CURRENTLY SAVE CHARACTER OFFSETS!
class LexicalToken :public Token {
public:
	// [xx] should this require an original_token_index arg?
	/** Create a new token from a LocatedString, starting at begin_pos
	  * (inclusive) and extending through end_pos (inclusive).  The new
	  * Token's symbol value is copied from the LocatedString.  All offset
	  * information is automatically set based on the LocatedString. */
	LexicalToken(const LocatedString *locString, int begin_pos, int end_pos) 
		: Token(locString, begin_pos, end_pos), _original_token_index(0) {} 

	// [xx] should this require an original_token_index arg?
	/** Create a new token from a substring of a located string, but set
	  * the symbol for the new token manually (rather than copying it from
	  * the located string.  All offset information is automatically set
	  * based on the LocatedString. */
	LexicalToken(const LocatedString *locString, int begin_pos, int end_pos, const Symbol &symbol) 
		: Token(locString, begin_pos, end_pos, symbol), _original_token_index(0) {} 

	/** Create a token with a given symbol value, whose offsets are undefined.
	  * This constructor should only be used if the resulting token's offsets
	  * will never be used. */
	explicit LexicalToken(const Symbol &symbol)
		: Token(symbol), _original_token_index(0) {}

	/** Create a new token by copying an existing token and adding information 
	  * about its lexical decomposition.  The symbol and offset information
	  * are copied from the source token. */
	LexicalToken(const Token &source, int original_token_index, int n_lex_entries, LexicalEntry **lex_entries)
		: Token(source), 
		  _original_token_index(original_token_index),
		  _lex_entries(lex_entries, lex_entries+n_lex_entries) {}

	LexicalToken(const LexicalToken &source, const Symbol &newSymbol, int original_token_index)
		: Token(source, newSymbol), 
		  _original_token_index(original_token_index), 
		  _lex_entries(source._lex_entries) {}

	LexicalToken(const LexicalToken &source, const Symbol &newSymbol)
		: Token(source, newSymbol), 
		  _original_token_index(source._original_token_index), 
		  _lex_entries(source._lex_entries) {}

	LexicalToken(const OffsetGroup &start, const OffsetGroup &end, Symbol symbol, int original_token_index)
		: Token(start, end, symbol), _original_token_index(original_token_index) {}

	LexicalToken(const OffsetGroup &start, const OffsetGroup &end, Symbol symbol, int original_token_index, int n_lex_entries, LexicalEntry **lex_entries)
		: Token(start, end, symbol), _original_token_index(original_token_index),
		  _lex_entries(lex_entries, lex_entries+n_lex_entries) {}

	LexicalToken(const OffsetGroup &start, const OffsetGroup &end, Symbol symbol)
		: Token(start, end, symbol), _original_token_index(0) {}

	/** Copy constructor. */
	LexicalToken(const LexicalToken &source)
		: Token(source), 
		  _original_token_index(source._original_token_index),  
		  _lex_entries(source._lex_entries) {}

	~LexicalToken() {}

	int getOriginalTokenIndex() const { return _original_token_index; }

	// For saving state:
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	LexicalToken(StateLoader *stateLoader);
	// For XML serialization:
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit LexicalToken(SerifXML::XMLTheoryElement elem, int sent_num, int token_num);

	// Lexical state info
	int getNLexicalEntries()const {return static_cast<int>(_lex_entries.size());};
	LexicalEntry* getLexicalEntry(int i)const {return _lex_entries[i];} ;

private:
	//This integer stores the index in the original token sequence of 
	//the token from which the current token was derived.
	int _original_token_index;
	std::vector<LexicalEntry*> _lex_entries;
};

#endif

