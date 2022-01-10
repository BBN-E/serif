// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KR_UNTOKENIZER_H
#define KR_UNTOKENIZER_H

#include "common/LocatedString.h"

class SymbolSubstitutionMap;

class KoreanUntokenizer : public Untokenizer {
private:
	friend class KoreanUntokenizerFactory;

public:
	~KoreanUntokenizer() {}

	void untokenize(LocatedString& source) const;
	static void untokenize(const wchar_t *source, wchar_t *result, int max_length);

private:
	KoreanUntokenizer();
	void replaceSubstitutions(LocatedString& source) const;
	static void replaceSubstitutions(std::wstring& str);
	
	static SymbolSubstitutionMap *_substitutionMap;
};

class KoreanUntokenizerFactory: public Untokenizer::Factory {
	virtual Untokenizer *build() { return _new KoreanUntokenizer(); }
};




#endif
