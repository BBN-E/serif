// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/patterns/TextPattern.h"
#include "Generic/patterns/PatternWordSet.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/UnicodeUtil.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>
#pragma warning(pop)

namespace { // private symbols
	Symbol stringSym(L"string");
	Symbol rawTextSym(L"RAW_TEXT");
	Symbol dontAddSpacesSym(L"DONT_ADD_SPACES");
}

using namespace std;

TextPattern::TextPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) 
: _raw_text(false), _add_spaces(true)
{
	initializeFromSexp(sexp, entityLabels, wordSets);
	if (_raw_text) {
		// [XX] NOTE: The old Brandy code did *not* escape "(" and ")", but I believe
		// that was a bug, not a feature.
		static const boost::wregex esc(L"[\\(\\)\\{\\}\\^\\.\\$\\|\\[\\]\\*\\+\\?\\/\\\\]");
		_text = regex_replace(_text, esc, L"\\\\\\1&", boost::match_default | boost::format_sed);
	}
	if (_text.length() == 0) {
		throwError(sexp, "empty regex pattern found.");
	}
	int num_open = countSubstrings(_text, L"(") - countSubstrings(_text, L"\\(");
	int num_closed = countSubstrings(_text, L")") - countSubstrings(_text, L"\\)");
	if (num_open != num_closed) 
		throwError(sexp, "TextPatterns must have the same number of open and closed parentheses.");
}

bool TextPattern::initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) 
{
	if (Pattern::initializeFromAtom(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (childSexp->getValue() == rawTextSym) {
		if (!_text.empty()) 
			throwError(childSexp, "RAW_TEXT contraint must come before string constraint");
		_raw_text = true;
		return true;
	}
	else if (childSexp->getValue() == dontAddSpacesSym) {
		if (!_text.empty()) 
			throwError(childSexp, "DONT_ADD_SPACES contraint must come before string constraint");
		_add_spaces = false;
		return true;
	} else {
		logFailureToInitializeFromAtom(childSexp);
		return false;
	}
}

bool TextPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (Pattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (constraintType == stringSym) {
		if (childSexp->getNumChildren() < 2) 
			throwError(childSexp, "string constraint must be followed by at least 1 atomic Sexp");
		for (int j = 1; j < childSexp->getNumChildren(); j++) {
			Sexp *stringSexp = childSexp->getNthChild(j);
			if (!stringSexp->isAtom()) 
				throwError(childSexp, "string constraint must only be followed by atomic Sexps");

			PatternWordSetMap::const_iterator wordSetIter = wordSets.find(stringSexp->getValue());
			if (wordSetIter != wordSets.end()) {
				const PatternWordSet_ptr wordSet = (*wordSetIter).second;
				_text.append(wordSet->getRegexString());
				if (j != childSexp->getNumChildren() - 1 && _add_spaces) 
					_text.append(L" ");
			} else {
				wstring strWithQuotes(stringSexp->getValue().to_string());
				if (strWithQuotes.at(0) != L'"' || strWithQuotes.at(strWithQuotes.length() - 1) != L'"') {
					throwError(childSexp, "string constraint must be followed by word sets or quoted strings");
				}
				strWithQuotes = strWithQuotes.substr(1, strWithQuotes.length() - 2);
				_text.append(strWithQuotes);
				if (j != childSexp->getNumChildren() - 1 && _add_spaces) 
					_text.append(L" ");
			}	
		}
		return true;
	} else {
		logFailureToInitializeFromChildSexp(childSexp);		
		return false;
	}
} 

int TextPattern::countSubstrings(const std::wstring &str, const wchar_t *sub) {
	int result = 0;
	size_t pos = 0;
	while ((pos = str.find(sub, pos)) != std::wstring::npos) {
		result++;
		pos++;
	}
	return result;
}

void TextPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "TextPattern:\n" << UnicodeUtil::toUTF8StdString(_text) << "\n";
}

