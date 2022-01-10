// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TOKEN_H
#define TOKEN_H

#include "Generic/common/Symbol.h"
#include "Generic/common/LocatedString.h"
#include "Generic/theories/Theory.h"
#include "Generic/common/limits.h"

#include <wchar.h>
#include <iostream>


#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;


class SERIF_EXPORTED Token : public Theory {
public:
	/** Create a new token from a LocatedString, starting at begin_pos
	  * (inclusive) and extending through end_pos (inclusive).  The new
	  * Token's symbol value is copied from the LocatedString.  All offset
	  * information is automatically set based on the LocatedString. */
	Token(const LocatedString *locString, int begin_pos, int end_pos)
		: _start(locString->startOffsetGroup(begin_pos)), _end(locString->endOffsetGroup(end_pos)),
		  _symbol(std::wstring(locString->toString()+begin_pos, locString->toString()+end_pos)) {}

	/** Create a new token from a substring of a located string, but set
	  * the symbol for the new token manually (rather than copying it from
	  * the located string.  All offset information is automatically set
	  * based on the LocatedString. */
	Token(const LocatedString *locString, int begin_pos, int end_pos, const Symbol &symbol) 
		: _start(locString->startOffsetGroup(begin_pos)), _end(locString->endOffsetGroup(end_pos)),
		  _symbol(symbol) {}

	/** Create a token with a given symbol value, whose offsets are undefined.
	  * This constructor should only be used if the resulting token's offsets
	  * will never be used. */
	explicit Token(const Symbol &symbol)
		: _start(), _end(), _symbol(symbol) {}

	/** Create a new token by changing the symbol for an existing token.  All
	  * offset information remains the same. */
	Token(const Token &source, const Symbol &newSymbol)
		: _start(source._start), _end(source._end), _symbol(newSymbol) {}

	/** Create a new token with explicitly specified offsets. */
	Token(const OffsetGroup &start, const OffsetGroup &end, Symbol symbol)
		: _start(start), _end(end), _symbol(symbol) {}

	// Copy constructor
	Token(const Token &source)
		: _start(source._start), _end(source._end), _symbol(source._symbol) {}

	// Destructor
	virtual ~Token() {}

	/** Return this token's symbol.  A token's symbol may not exactly match the 
	  * source text that it came from.  In particular, it may be normalized in
	  * various ways to facilitate processing. */
	virtual Symbol getSymbol() const { return _symbol; }

	/** Return an OffsetGroup indicating where this token begins in the source text. */
	OffsetGroup getStartOffsetGroup() const { return _start; }

	/** Return an OffsetGroup indicating where this token ends in the source text. */
	OffsetGroup getEndOffsetGroup() const { return _end; }

	/** Return an EDTOffset indicating where this token begins in the source text. */
	EDTOffset getStartEDTOffset() const { return _start.edtOffset; }

	/** Return an EDTOffset indicating where this token ends in the source text. */
	EDTOffset getEndEDTOffset() const { return _end.edtOffset; }

	/** Return a character offset indicating where this token begins in the source text. */
	CharOffset getStartCharOffset() const { return _start.charOffset; }

	/** Return a character offset indicating where this token ends in the source text. */
	CharOffset getEndCharOffset() const { return _end.charOffset; }
	
	/** Return a speech signal time indicating where this token begins in the source audio. */
	ASRTime getStartASRTime() const { return _start.asrTime; }

	/** Return a speech signal time indicating where this token ends in the source audio. */
	ASRTime getEndASRTime() const { return _end.asrTime; }
	
	/** Return an offset of the specified type, indicating where this token begins 
	  * in the source text. The OffsetType template parameter must be explicitly 
	  * specified.  E.g.: token.getStart<CharOffset>(). */
	template<typename OffsetType> OffsetType getStart() const { return _start.value<OffsetType>(); }

	/** Return an offset of the specified type, indicating where this token ends
	  * in the source text. The OffsetType template parameter must be explicitly 
	  * specified.  E.g.: token.getEnd<CharOffset>(). */
	template<typename OffsetType> OffsetType getEnd() const { return _end.value<OffsetType>(); }

	/** Send a debug string to the given stream. */
	virtual void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out, Token &it)
		{ it.dump(out, 0); return out; }

	// For saving state:
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	// For loading state:
	Token(StateLoader *stateLoader);
	virtual void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit Token(SerifXML::XMLTheoryElement elem, int sent_num, int token_num);
	virtual const wchar_t* XMLIdentifierPrefix() const;

public:
	//this is a hack to allow arabic tokens to be saved and loaded in the generic token sequence
	//use with caution (added for distillation)
	static bool _saveLexicalTokensAsDefaultTokens;

protected:
	Token(): _start(), _end(), _symbol() {}
	OffsetGroup _start;
	OffsetGroup _end;
	Symbol _symbol;
};

#endif
