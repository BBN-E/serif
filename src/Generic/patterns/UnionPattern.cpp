// Copyright (c) 2006 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/patterns/UnionPattern.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/ShortcutPattern.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/GenericPFeature.h"
#include <boost/lexical_cast.hpp>
#include "Generic/common/Sexp.h"


// Private symbols
namespace {
	Symbol membersSym = Symbol(L"members");
	Symbol greedySym = Symbol(L"GREEDY");
}

UnionPattern::UnionPattern(Sexp *sexp, const Symbol::HashSet &entityLabels,
									   const PatternWordSetMap& wordSets):
_is_greedy(false)
{
	int nkids = sexp->getNumChildren();
	if (nkids < 2)
		throwError(sexp, "too few children in UnionPattern");
	initializeFromSexp(sexp, entityLabels, wordSets);
	if (_patternList.size() == 0)
		throwError(sexp, "no member patterns specified in UnionPattern");
}

bool UnionPattern::initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol atom = childSexp->getValue();
	if (LanguageVariantSwitchingPattern::initializeFromAtom(childSexp, entityLabels, wordSets)) {
		return true;

	} else if (atom == greedySym) {
		_is_greedy = true; return true;
	} else {
		logFailureToInitializeFromAtom(childSexp);
		return false;
	}
}


bool UnionPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels,
													  const PatternWordSetMap& wordSets)
{
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (LanguageVariantSwitchingPattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (constraintType == membersSym) {
		int n_patterns = childSexp->getNumChildren() - 1;
		for (int j = 0; j < n_patterns; j++)
			_patternList.push_back(parseSexp(childSexp->getNthChild(j+1), entityLabels, wordSets));
		return true;
	} else {
		logFailureToInitializeFromChildSexp(childSexp);
		return false;
	}
}

PatternFeatureSet_ptr UnionPattern::matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) { 
	if (_languageVariant && !patternMatcher->getActiveLanguageVariant()->matchesConstraint(*_languageVariant)) {
		return matchesAlignedSentence(patternMatcher, sTheory, _languageVariant); 
	}
	
	std::vector<float> scores;
	bool matchedOne = false;
	PatternFeatureSet_ptr allPatterns = boost::make_shared<PatternFeatureSet>(); // features from all patterns.
	
	for (size_t i = 0; i < _patternList.size(); i++) {
		SentenceMatchingPattern_ptr pattern = _patternList[i]->castTo<SentenceMatchingPattern>();
		PatternFeatureSet_ptr sfs = pattern->matchesSentence(patternMatcher, sTheory);
		if (sfs){
			matchedOne = true;
			allPatterns->addFeatures(sfs);
			scores.push_back(sfs->getScore());
			SessionLogger::dbg("BRANDY") << getDebugID() << ": UNION member " << i << " of " << _patternList.size() << " has score " << sfs->getScore() << "\n";
			if (_is_greedy)	break;
		}
	}
	if( !matchedOne ) {
		return PatternFeatureSet_ptr();
	}
	addID(allPatterns);
	allPatterns->addFeature(boost::make_shared<GenericPFeature>(shared_from_this(), -1, patternMatcher->getActiveLanguageVariant()));
	allPatterns->setScore(_scoringFunction(scores, _score));
	return allPatterns;
}

std::vector<PatternFeatureSet_ptr> UnionPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) {
	if (_languageVariant && !patternMatcher->getActiveLanguageVariant()->matchesConstraint(*_languageVariant)) {
		return multiMatchesAlignedSentence(patternMatcher, sTheory, _languageVariant); 
	}
	
	std::vector<PatternFeatureSet_ptr> return_vector;
	if (_is_greedy || _force_single_match_sentence) {
		PatternFeatureSet_ptr sfs = matchesSentence(patternMatcher, sTheory, debug);
		if (sfs != 0)
			return_vector.push_back(sfs);
		return return_vector;
	} else {
		for (size_t i = 0; i < _patternList.size(); i++) {
			SentenceMatchingPattern_ptr pattern = _patternList[i]->castTo<SentenceMatchingPattern>();
			std::vector<PatternFeatureSet_ptr> subpatternVector = pattern->multiMatchesSentence(patternMatcher, sTheory);
			for (size_t j = 0; j < subpatternVector.size(); j++) 
				return_vector.push_back(subpatternVector[j]);
		}
		return return_vector;
	}
}

Pattern_ptr UnionPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {
	for (size_t i = 0; i < _patternList.size(); ++i) {
		replaceShortcut<Pattern>(_patternList[i], refPatterns);
	}	
	return shared_from_this();
}

void UnionPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "UnionPattern: ";
	if (!getID().is_null()) out << getID();
	out << std::endl;
	for (size_t i = 0; i < _patternList.size(); i++) {
		_patternList[i]->dump(out, indent+2);
	}
}

/**
 * Redefinition of parent class's virtual method that returns a vector of 
 * pointers to PatternReturn objects for a given pattern.
 *
 * @param PatternReturnVecSeq for a given pattern
 *
 * @author afrankel@bbn.com
 * @date 2010.10.20
 **/
void UnionPattern::getReturns(PatternReturnVecSeq & output) const {
	Pattern::getReturns(output);
	for (size_t n = 0; n < _patternList.size(); ++n) {
		if (_patternList[n])
			_patternList[n]->getReturns(output);
	}
}

/**
 * Retrieves first valid ID found.
 *
 * @return First valid ID.
 *
 * @author afrankel@bbn.com
 * @date 2011.08.08
 **/
Symbol UnionPattern::getFirstValidID() const {
	return getFirstValidIDForCompositePattern(_patternList);
}
