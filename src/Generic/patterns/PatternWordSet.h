// Copyright (c) 2006 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef PATTERN_WORDSET_H
#define PATTERN_WORDSET_H

#include "Generic/common/Symbol.h"
#include "Generic/common/BoostUtil.h"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <map>
#include <boost/make_shared.hpp>

class Sexp;

class PatternWordSet;
typedef boost::shared_ptr<PatternWordSet> PatternWordSet_ptr;
typedef Symbol::HashMap<PatternWordSet_ptr> PatternWordSetMap;

/** A set of words that is identified by a label.  Word sets are used to 
  * store a group of words that might be useful to multiple patterns. 
  * Some patterns will treat each word in the word set as a literal string;
  * others will treat them as regular expressions. 
  *
  * Create PatternReturn objects with:
  *    boost::make_shared<PatternWordSet>(Sexp*)
  */
class PatternWordSet {
private:
	PatternWordSet(Sexp *sexp, PatternWordSetMap wordSets);
	BOOST_MAKE_SHARED_2ARG_CONSTRUCTOR(PatternWordSet, Sexp*, PatternWordSetMap);

public:
	/** Return the lable used to identify this pattern word set. */
	Symbol getLabel() const { return _label; }

	/** Return the number of words in this pattern word set. */
	int getNWords() const { return static_cast<int>(_words.size()); }

	/** Return the n-th word in this pattern word set */
	Symbol getNthWord(size_t n) const;

	/** Return a regular expression which matches the words in this word set. 
	  * This regexp has the form "(w1|w2|w3|w4)".  Special regular-expression
	  * characters in the individual words are *not* escaped.  */
	std::wstring getRegexString() const;

	/** Return true if the given symbol is a legal name for a word set.  
	  * Currently, this is defined as symbols that contain at least one
	  * upper-case letter. */
	static bool isWordSetSymbol(const Symbol &sym);

private:
	Symbol _label;
	std::vector<Symbol> _words;
};

#endif
