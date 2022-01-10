// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Chinese/tokens/ch_Untokenizer.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"

const wchar_t CH_SPACE = 0x3000; // \343\200\200
const wchar_t EOL = L'\n';

SymbolSubstitutionMap* ChineseUntokenizer::_substitutionMap = 0;

ChineseUntokenizer::ChineseUntokenizer()  {
	// Read and load symbol substitution parameters
	if (_substitutionMap == 0) {
		std::string token_subst_file = ParamReader::getRequiredParam("tokenizer_subst");
		_substitutionMap = _new SymbolSubstitutionMap(token_subst_file.c_str());
	}
}

void ChineseUntokenizer::untokenize(LocatedString& source) const {
	removeWhitespace(source);
	replaceSubstitutions(source);
}

void ChineseUntokenizer::untokenize(const wchar_t *source, wchar_t *result, int max_length) {
	std::wstring str(source);
	removeWhitespace(str);
	replaceSubstitutions(str);
	wcsncpy(result, str.c_str(), max_length);
	result[max_length-1] = L'\0'; // make sure it's null terminated
}

void ChineseUntokenizer::removeWhitespace(LocatedString& source) const {
	int i = 0;
	while (i < source.length()) {
		// keep newlines and ideographic spaces, since they're significant in sentence breaking
		if (iswspace(source.charAt(i)) && source.charAt(i) != CH_SPACE && source.charAt(i) != EOL)
			source.remove(i, i + 1);
		else
			i++;
	}
}

void ChineseUntokenizer::removeWhitespace(std::wstring& str) {
	std::wstring result = L"";
	for (std::basic_string<wchar_t>::const_iterator iter = str.begin(); iter != str.end(); iter++) {
		// keep newlines and ideographic spaces, since they're significant in sentence breaking
		if (iswspace(*iter) && *iter != CH_SPACE && *iter != EOL)
			continue;
		result += *iter;
	}
	str = result;
}

void ChineseUntokenizer::replaceSubstitutions(LocatedString& source) const {
	// Read and load symbol substitution parameters, if needed
	if (_substitutionMap == 0) {
		std::string token_subst_file = ParamReader::getRequiredParam("tokenizer_subst");
		_substitutionMap = _new SymbolSubstitutionMap(token_subst_file.c_str());
	}

	_substitutionMap->reverseReplaceAll(source);
}

void ChineseUntokenizer::replaceSubstitutions(std::wstring& str) {
	// Read and load symbol substitution parameters, if needed
	if (_substitutionMap == 0) {
		std::string token_subst_file = ParamReader::getRequiredParam("tokenizer_subst");
		_substitutionMap = _new SymbolSubstitutionMap(token_subst_file.c_str());
	}
	_substitutionMap->reverseReplaceAll(str);
}
