// Copyright (c) 2006 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/NegationPattern.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/ShortcutPattern.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/GenericPFeature.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/Sexp.h"


// Private symbols
namespace {
	Symbol membersSym = Symbol(L"members");
	Symbol greedySym = Symbol(L"GREEDY");
}

NegationPattern::NegationPattern(Sexp *sexp, const Symbol::HashSet &entityLabels,
									   const PatternWordSetMap& wordSets)
{
	int nkids = sexp->getNumChildren();
	if (nkids < 2)
		throwError(sexp, "too few children in NegationPattern");
	initializeFromSexp(sexp, entityLabels, wordSets);
}

bool NegationPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels,
													  const PatternWordSetMap& wordSets)
{
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (Pattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else {
		if (_pattern != 0)
			throwError(childSexp, "more than one pattern argument in NegationPattern");
		_pattern = parseSexp(childSexp, entityLabels, wordSets);
		return true;
	}
}

bool NegationPattern::initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	if (_pattern != 0) {
		throwError(childSexp, "more than one pattern argument in NegationPattern");
	} else {
		_pattern = parseSexp(childSexp, entityLabels, wordSets);
	}
	return true;
}

PatternFeatureSet_ptr NegationPattern::matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) { 
	SentenceMatchingPattern_ptr pattern = _pattern->castTo<SentenceMatchingPattern>();
	if (pattern->matchesSentence(patternMatcher, sTheory, debug)) {
		return PatternFeatureSet_ptr();
	} else {
		return matchForSentence(sTheory->getTokenSequence()->getSentenceNumber(), patternMatcher->getActiveLanguageVariant());
	}
}

PatternFeatureSet_ptr NegationPattern::matchForSentence(int sent_no, const LanguageVariant_ptr& languageVariant) {
	PatternFeatureSet_ptr sfs = boost::make_shared<PatternFeatureSet>();
	addID(sfs);
	sfs->addFeature(boost::make_shared<GenericPFeature>(shared_from_this(), sent_no, languageVariant));
	sfs->setScore(this->getScore());
	return sfs;
}

std::vector<PatternFeatureSet_ptr> NegationPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) {

	// pass through to matchesSentence-- we are never returning any real features here, so no point in doing a multi-match
	std::vector<PatternFeatureSet_ptr> return_vector;
	PatternFeatureSet_ptr result = matchesSentence(patternMatcher, sTheory, debug);
	if (result.get() != 0) {
		return_vector.push_back(result);
	}
	return return_vector;	
}

Pattern_ptr NegationPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {
	replaceShortcut<Pattern>(_pattern, refPatterns);
	return shared_from_this();
}

void NegationPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "PatternNegation:";
	if (!getID().is_null()) out << getID();
	out << std::endl;
	_pattern->dump(out, indent+2);
}
