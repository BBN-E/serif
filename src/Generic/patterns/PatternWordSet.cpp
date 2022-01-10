// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/PatternWordSet.h"
#include "Generic/common/Sexp.h"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/UnexpectedInputException.h"

PatternWordSet::PatternWordSet(Sexp *sexp, PatternWordSetMap wordSets) {
	if (!sexp->getFirstChild()) {
		std::stringstream ss;
		ss << "Attempt to access first child of sexp with no children.\n" << sexp->to_debug_string(); 
		throw InternalInconsistencyException("PatternWordSet::PatternWordSet()", ss.str().c_str());
	}
	_label = sexp->getFirstChild()->getValue();
	if (!isWordSetSymbol(_label)) {
		std::ostringstream ss;
		ss << "PatternWordSet names must contain at least one capital letter: " << _label.to_debug_string();
		throw UnexpectedInputException("PatternWordSet::PatternWordSet()", ss.str().c_str());
	}

	int n_words = sexp->getNumChildren() - 1;
	if (n_words == 0) {
		std::stringstream ss;
		ss << "PatternWordSet (" << _label.to_debug_string() << ") must contain at least one word";
		throw UnexpectedInputException("PatternWordSet::PatternWordSet()", ss.str().c_str());
	}

	// Include other wordsets
	int wordOffset = 1;
	if (sexp->getSecondChild()->isList()) {
		Sexp* included = sexp->getSecondChild();
		n_words--;
		wordOffset++;
		if (included->getNumChildren() < 2 || included->getFirstChild()->getValue() != Symbol(L"include")) {
			std::ostringstream ss;
			ss << "PatternWordSet includes must start with the include atom and include at least one other wordset label";
			throw UnexpectedInputException("PatternWordSet::PatternWordSet()", ss.str().c_str());
		}
		for (int i = 0; i < included->getNumChildren() - 1; i++) {
			Symbol label = included->getNthChild(i + 1)->getValue();
			if (!isWordSetSymbol(label)) {
				std::ostringstream ss;
				ss << "PatternWordSet include names must contain at least one capital letter: " << label.to_debug_string();
				throw UnexpectedInputException("PatternWordSet::PatternWordSet()", ss.str().c_str());
			}
			PatternWordSetMap::const_iterator includedSetIterator = wordSets.find(label);
			if (includedSetIterator == wordSets.end()) {
				std::ostringstream ss;
				ss << "PatternWordSet include references an undefined wordset: " << label.to_debug_string();
				throw UnexpectedInputException("PatternWordSet::PatternWordSet()", ss.str().c_str());
			}
			PatternWordSet_ptr includedSet = (*includedSetIterator).second;
			for (int i = 0; i < includedSet->getNWords(); i++) {
				_words.push_back(includedSet->getNthWord(i));
			}
		}
	}

	// Read the words in this set
	for (int i = 0; i < n_words; i++) {
		_words.push_back(sexp->getNthChild(i + wordOffset)->getValue());
	}
}

Symbol PatternWordSet::getNthWord(size_t n) const { 
	if (n < _words.size())
		return _words[n];
	else
		throw InternalInconsistencyException::arrayIndexException(
		"PatternWordSet::getNthWord()", _words.size(), n);
}

std::wstring PatternWordSet::getRegexString() const {
	std::vector<std::wstring> clean_words;
	BOOST_FOREACH(Symbol word, _words) {
		std::wstring clean_word = word.to_string();
		boost::replace_all(clean_word, "_", " ");
		clean_words.push_back(clean_word);
	}
	std::wstring str = L"(" + boost::algorithm::join(clean_words, "|") + L")";
	return str;
}

bool PatternWordSet::isWordSetSymbol(const Symbol &sym) {
	for (const wchar_t* s = sym.to_string(); *s; ++s)
		if (iswupper(*s)) return true;
	return false;
}

