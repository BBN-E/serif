// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOCUMENT_PATTERN_H
#define DOCUMENT_PATTERN_H

#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternTypes.h"

/** A pattern that contains a symbolic reference to a document-level
  * pattern, and matches a sentence iff that document-level pattern
  * matched successfully.  These document-level patterns are defined
  * in the "doclevel" section of a query pattern set definition.
  *
  * In addition to the explicitly-defined document-level pattern, the
  * PatternMatcher also runs some document-level "fake patterns",
  * which can be checked even if not declared.  Currently, these include:
  *
  *   - IN_AD_RANGE
  *   - OUT_OF_AD_RANGE
  *   - AD_UNDEFINED
  *   - AD_NOT_IN_CORPUS
  *
  * Where "AD" stands for the "activity date".
  */
class DocumentPattern: public Pattern, public SentenceMatchingPattern
{
private:
	DocumentPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets);
	BOOST_MAKE_SHARED_3ARG_CONSTRUCTOR(DocumentPattern, Sexp*, const Symbol::HashSet&, const PatternWordSetMap&);

public:
	~DocumentPattern() {}

	// Matching methods
	virtual std::vector<PatternFeatureSet_ptr> multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);
	virtual PatternFeatureSet_ptr matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug = 0);

	// Overridden virtual methods:
	virtual std::string typeName() const { return "DocumentPattern"; }
	virtual void dump(std::ostream &out, int indent = 0) const;
private:
	/** The name of the doclevel pattern that this pattern refers to; this
	  * pattern should match on a sentence iff the referenced doclevel 
	  * pattern matched the document.  *OR* one of the special symbols
	  * IN_AD_RANGE, OUT_OF_AD_RANGE, AD_UNDEFINED, AD_NOT_IN_CORPUS to
	  * constrain the activity date status of the document. */
	Symbol _docPatternSym;
};


#endif
