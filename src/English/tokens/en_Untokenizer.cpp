// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/LocatedString.h"
#include "Generic/common/Symbol.h"
#include "English/tokens/en_Untokenizer.h"


/// Tokens that should be rejoined with the token to the left.
Symbol rejoinLeft[] =
{
	Symbol(L"]"),
	Symbol(L")"),
	Symbol(L"}"),
	Symbol(L","),
	Symbol(L"\""),
	Symbol(L"''"),
	Symbol(L"'"),
	Symbol(L"'\""),
	Symbol(L"?"),
	Symbol(L"!"),
	Symbol(L":"),
	Symbol(L";")
};

/// Tokens that should be rejoined with the token to the right.
Symbol rejoinRight[] =
{
	Symbol(L"$"),
	Symbol(L"``"),
	Symbol(L"`"),
	Symbol(L"\"'"),
	Symbol(L"["),
	Symbol(L"("),
	Symbol(L"{")
};

class EnglishUntokenizer;

void EnglishUntokenizer::untokenize(LocatedString& source) const
{
	// TODO: these should really be case-insensitive
	source.replace(L"&LR;", L"");
	source.replace(L"&lr;", L"");
	source.replace(L"&UR;", L"");
	source.replace(L"&ur;", L"");
	source.replace(L"&MD;", L"--");
	source.replace(L"&md;", L"--");
	source.replace(L"&AMP;", L"&");
	source.replace(L"&amp;", L"&");
	source.replace(L"-LRB-", L"(");
	source.replace(L"-RRB-", L")");
	source.replace(L"-LCB-", L"{");
	source.replace(L"-RCB-", L"}");
	source.replace(L"-LSB-", L"[");
	source.replace(L"-RSB-", L"]");
	source.replace(L"-\r\n", L"-");
	source.replace(L"-\n", L"-");

	Token previous, current;
	int offset = 0;
	while ((offset < source.length()) && getNextToken(source, offset, current))
	{
		if (previous.exists()) {
			// The one thing we don't untokenize is periods/ellipses,
			// because that won't confuse the tokenizer or sentence
			// breaker, but untokenizing them would require many special
			// cases (e.g., decimal numbers, abbreviations, etc.).
			if (matchRightRejoin(previous.source) ||
				matchLeftRejoin(current.source) ||
				matchContraction(current.source) ||
				matchHyphenation(current.source))
			{
				int num_removed = current.start - previous.end;
				source.remove(previous.end, current.start);

				current.start = previous.start;
				current.end -= num_removed;
				current.end -= replaceQuotes(source, current.start, current.end);

				LocatedString *substring = source.substring(current.start, current.end);
				current.source = substring->toSymbol();
				delete substring;
			}
		}

		previous = current;
		offset = current.end;
	}
}

int EnglishUntokenizer::replaceQuotes(LocatedString& source, int start, int end) const
{
	int num_removed = 0;

	for (int i = start; i < end; i++) {
		if (source.charAt(i) == L'`') {
			if (((i + 1) < end) && (source.charAt(i + 1) == L'`')) {
				source.replace(i, 2, L"\"");
				num_removed++;
				end--;
			}
			else {
				source.replace(i, 1, L"'");
			}
		}
		else if (source.charAt(i) == L'\'') {
			if (((i + 1) < end) && (source.charAt(i + 1) == L'\'')) {
				source.replace(i, 2, L"\"");
				num_removed++;
				end--;
			}
		}
	}

	return num_removed;
}

bool EnglishUntokenizer::getNextToken(const LocatedString& source, int offset, Token& token) const
{
	int start, end;

	if (offset >= source.length()) {
		return false;
	}

	while ((offset < source.length()) && iswspace(source.charAt(offset))) {
		offset++;
	}
	start = offset;

	while ((offset < source.length()) && !iswspace(source.charAt(offset))) {
		offset++;
	}
	end = offset;

	if (end == start) {
		return false;
	}

	// Populate the token structure.
	token.start = start;
	token.end = end;
	LocatedString *substring = source.substring(token.start, token.end);
	token.source = substring->toSymbol();
	delete substring;
	return true;
}

bool EnglishUntokenizer::matchContraction(Symbol sym) const
{
	return sym == Symbol(L"n't") ||
		   sym == Symbol(L"'m") ||
		   sym == Symbol(L"'re") ||
		   sym == Symbol(L"'s") ||
		   sym == Symbol(L"'ve") ||
		   sym == Symbol(L"'d") ||
		   sym == Symbol(L"'ll");
}

bool EnglishUntokenizer::matchHyphenation(Symbol sym) const
{
	const wchar_t *string = sym.to_string();
	const int len = (int)wcslen(string);
	return ((len > 1) && (string[0] == L'-') && (string[len - 1] != L'-'));
}

bool EnglishUntokenizer::matchLeftRejoin(Symbol sym) const
{
	return matchSymbol(sym, rejoinLeft, sizeof(rejoinLeft) / sizeof(Symbol));
}

bool EnglishUntokenizer::matchRightRejoin(Symbol sym) const
{
	return matchSymbol(sym, rejoinRight, sizeof(rejoinRight) / sizeof(Symbol));
}

bool EnglishUntokenizer::matchSymbol(Symbol sym, Symbol *array, int length) const
{
	for (int i = 0; i < length; i++) {
		if (sym == array[i]) {
			return true;
		}
	}
	return false;
}


