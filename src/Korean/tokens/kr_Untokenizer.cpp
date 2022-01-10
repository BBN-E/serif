// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/LocatedString.h"
#include "common/ParamReader.h"
#include "tokens/KoreanUntokenizer.h"
#include "tokens/SymbolSubstitutionMap.h"


SymbolSubstitutionMap* KoreanUntokenizer::_substitutionMap = 0;

KoreanUntokenizer::KoreanUntokenizer()  {
	// Read and load symbol substitution parameters
	if (_substitutionMap == 0) {
		char token_subst_file[501];
		ParamReader::getRequiredParam("tokenizer_subst", token_subst_file, 500);

		_substitutionMap = _new SymbolSubstitutionMap(token_subst_file);
	}
}

void KoreanUntokenizer::untokenize(LocatedString& source) const {
	replaceSubstitutions(source);
}

void KoreanUntokenizer::untokenize(const wchar_t *source, wchar_t *result, int max_length) {
	std::wstring str(source);
	replaceSubstitutions(str);
	wcsncpy(result, str.c_str(), max_length);
	result[max_length-1] = L'\0'; // make sure it's null terminated
}


void KoreanUntokenizer::replaceSubstitutions(LocatedString& source) const {
	// Read and load symbol substitution parameters, if needed
	if (_substitutionMap == 0) {
		char token_subst_file[501];
		ParamReader::getRequiredParam("tokenizer_subst", token_subst_file, 500);

		_substitutionMap = _new SymbolSubstitutionMap(token_subst_file);
	}

	for (int i = 0; i < _substitutionMap->getSize(); i++) {
		source.replace(_substitutionMap->getValue(i).to_string(),
					   _substitutionMap->getKey(i).to_string());
	}
}

void KoreanUntokenizer::replaceSubstitutions(std::wstring& str) {
	// Read and load symbol substitution parameters, if needed
	if (_substitutionMap == 0) {
		char token_subst_file[501];
		ParamReader::getRequiredParam("tokenizer_subst", token_subst_file, 500);

		_substitutionMap = _new SymbolSubstitutionMap(token_subst_file);
	}

	for (int i = 0; i < _substitutionMap->getSize(); i++) {
		std::wstring key = _substitutionMap->getKey(i).to_string();
		std::wstring val = _substitutionMap->getValue(i).to_string();
		size_t idx = str.find(val);
		while (idx != basic_string<wchar_t>::npos) {
			str.replace(idx, val.length(), key);
			idx = str.find(val);
		}
	}
}
