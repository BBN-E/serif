// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef TEXT_PATTERN_H
#define TEXT_PATTERN_H

#include "common/Symbol.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/common/BoostUtil.h"

/** A container for text subexpressions that are used in RegexpPatterns.  
  * TextPattern itself is never matched against anything; instead, each
  * TextPattern is owned by a RegexpPattern, and it is the RegexpPattern
  * that performs the match.  
  * 
  * This is implemented as a Pattern subclass to make it possible to 
  * store a return value and a shortcut for this individual piece of text. */
class TextPattern: public Pattern {
private:
	TextPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(TextPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);
public:
	/** Return a reference to the text string owned by this TextPattern.  
	  * Note that this reference will only be valid as long as the 
	  * TextPattern itself continues to exist.  If you need a copy of the
	  * text string that out-lives the pattern, then you must make a copy
	  * yourself. */
	const std::wstring &getText() const { return _text; }

	// Overridden virtual methods:
	virtual std::string typeName() const { return "TextPattern"; }
	virtual void dump(std::ostream &out, int indent = 0) const;

	static int countSubstrings(const std::wstring &str, const wchar_t *sub);

private:
	// The text string:
	std::wstring _text;

	// Parameters that control how the string is assembled from the sexpression.
	// These are only used during initialization.
	bool _raw_text;
	bool _add_spaces;

	// Helpers for constructor
	bool initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	bool initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
};

typedef boost::shared_ptr<TextPattern> TextPattern_ptr;

#endif
