// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_UNTOKENIZER_H
#define EN_UNTOKENIZER_H

#include "Generic/common/LocatedString.h"
#include "Generic/common/Symbol.h"
#include "Generic/tokens/Untokenizer.h"



class EnglishUntokenizer : public Untokenizer {
private:
	friend class EnglishUntokenizerFactory;

public:
	~EnglishUntokenizer() {}

	void untokenize(LocatedString& source) const;

private:
	struct Token {
	public:
		Token() : start(0), end(0) {}
		Token(Token& other) : start(other.start), end(other.end), source(other.source) {}

		int length() { return end - start; }
		bool exists() { return end > start; }

		int start;
		int end;
		Symbol source;
	};

	EnglishUntokenizer() {}
	bool getNextToken(const LocatedString& source, int offset, Token& token) const;
	int replaceQuotes(LocatedString& source, int start, int end) const;
	bool matchContraction(Symbol sym) const;
	bool matchHyphenation(Symbol sym) const;
	bool matchLeftRejoin(Symbol sym) const;
	bool matchRightRejoin(Symbol sym) const;
	bool matchSymbol(Symbol sym, Symbol *array, int length) const;
	bool isSplitChar(wchar_t c) const;
};

class EnglishUntokenizerFactory: public Untokenizer::Factory {
	virtual Untokenizer *build() { return _new EnglishUntokenizer(); }
};




#endif
