// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_UNTOKENIZER_H
#define CH_UNTOKENIZER_H

#include "Generic/common/LocatedString.h"
#include "Generic/tokens/Untokenizer.h"

class SymbolSubstitutionMap;

class ChineseUntokenizer : public Untokenizer {
private:
	friend class ChineseUntokenizerFactory;

public:
	~ChineseUntokenizer() {}

	void untokenize(LocatedString& source) const;
	static void untokenize(const wchar_t *source, wchar_t *result, int max_length);

private:
	ChineseUntokenizer();
	void removeWhitespace(LocatedString& source) const;
	static void removeWhitespace(std::wstring& str);
	void replaceSubstitutions(LocatedString& source) const;
	static void replaceSubstitutions(std::wstring& str);
	
	static SymbolSubstitutionMap *_substitutionMap;
};

class ChineseUntokenizerFactory: public Untokenizer::Factory {
	virtual Untokenizer *build() { return _new ChineseUntokenizer(); }
};




#endif
