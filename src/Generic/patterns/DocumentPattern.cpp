// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/DocumentPattern.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/common/Sexp.h"

DocumentPattern::DocumentPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets)
{
	int nkids = sexp->getNumChildren();
	if (nkids != 2)
		throwError(sexp, "should be exactly two kids in DocumentPattern");
	
	Sexp *second = sexp->getSecondChild();
	if (!second->isAtom())
		throwError(sexp, "second argument in DocumentPattern should be a symbol");
	_docPatternSym = second->getValue();
}

PatternFeatureSet_ptr DocumentPattern::matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, 
													   UTF8OutputStream *debug) 
{ 
	if (patternMatcher->matchesDocPattern(_docPatternSym)) {
		PatternFeatureSet_ptr sfs = boost::make_shared<PatternFeatureSet>();
		// Add a feature for the return value (if we have one)
		if (getReturn()) {
			int sent_no = sTheory->getTokenSequence()->getSentenceNumber();
			sfs->addFeature(boost::make_shared<GenericReturnPFeature>(shared_from_this(), sent_no, patternMatcher->getActiveLanguageVariant()));
		}
		// Add a feature for the ID (if we have one)
		addID(sfs);
		// Initialize our score.
		sfs->setScore(this->getScore());
		return sfs;
	} else {
		return PatternFeatureSet_ptr();
	}
}

// there is never a reason for DocumentPattern to fire more than once in a
// sentence ~ RMG
std::vector<PatternFeatureSet_ptr> DocumentPattern::multiMatchesSentence(
	PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug)
{
	std::vector<PatternFeatureSet_ptr> return_vector;
	PatternFeatureSet_ptr sfs = matchesSentence(patternMatcher, sTheory, debug);
	if (sfs)
		return_vector.push_back(sfs);
	return return_vector;
}


void DocumentPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "DocumentPattern: " << _docPatternSym << "\n";
}

