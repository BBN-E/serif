// Copyright (c) 2006 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/patterns/IntersectionPattern.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/ShortcutPattern.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/GenericPFeature.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include <boost/lexical_cast.hpp>
#include "Generic/common/Sexp.h"


// Private symbols
namespace {
	Symbol membersSym = Symbol(L"members");
	Symbol greedySym = Symbol(L"GREEDY");
}

IntersectionPattern::IntersectionPattern(Sexp *sexp, const Symbol::HashSet &entityLabels,
									   const PatternWordSetMap& wordSets)
{
	int nkids = sexp->getNumChildren();
	if (nkids < 2)
		throwError(sexp, "too few children in IntersectionPattern");
	initializeFromSexp(sexp, entityLabels, wordSets);
	if (_patternList.size() == 0) {
		throwError(sexp, "no member patterns specified in IntersectionPattern");
	} else if (_patternList.size() >= 6) {
		throwError(sexp, "Too many member patterns specified in IntersectionPattern.  Currently 1-5 subpatterns are supported.");
	}
}

bool IntersectionPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels,
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

PatternFeatureSet_ptr IntersectionPattern::matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) { 
	if (_languageVariant && !patternMatcher->getActiveLanguageVariant()->matchesConstraint(*_languageVariant)) {
		return matchesAlignedSentence(patternMatcher, sTheory, _languageVariant); 
	}
	
	std::vector<float> scores;
	PatternFeatureSet_ptr allPatterns = boost::make_shared<PatternFeatureSet>(); // features from all patterns.
	for (size_t i = 0; i < _patternList.size(); i++) {
		SentenceMatchingPattern_ptr pattern = _patternList[i]->castTo<SentenceMatchingPattern>();
		PatternFeatureSet_ptr sfs = pattern->matchesSentence(patternMatcher, sTheory);
		if (sfs) {
			allPatterns->addFeatures(sfs);
			scores.push_back(sfs->getScore());
			SessionLogger::dbg("BRANDY") << getDebugID() << ": INTERSECTION member " << i << " of " << _patternList.size() << " has score " << sfs->getScore() << "\n";
		} else {
			SessionLogger::dbg("BRANDY") << getDebugID() << ": INTERSECTION member " << i << " of " << _patternList.size() << " had no match.\n";
			return PatternFeatureSet_ptr();
		}
	}
	addID(allPatterns);
	allPatterns->addFeature(boost::make_shared<GenericPFeature>(shared_from_this(), sTheory->getSentNumber(), patternMatcher->getActiveLanguageVariant()));
	allPatterns->setScore(_scoringFunction(scores, _score));	
	if (getReturn())
		allPatterns->addFeature(boost::make_shared<GenericReturnPFeature>(shared_from_this(), sTheory->getSentNumber(), patternMatcher->getActiveLanguageVariant()));
	return allPatterns;
}

std::vector<IntersectionPattern::feature_score_pair_t> IntersectionPattern::multiMatchesPatternGroup(PatternMatcher_ptr patternMatcher, 
																				 std::vector<Pattern_ptr>& patternGroup,
																				SentenceTheory *sTheory, UTF8OutputStream *debug) 
{

	std::vector<feature_score_pair_t> result;
	if (patternGroup.size() == 0)
		return result;

	// Get results for the first pattern
	SentenceMatchingPattern_ptr subpattern = patternGroup.at(0)->castTo<SentenceMatchingPattern>();
	std::vector<PatternFeatureSet_ptr> subpattern_results = subpattern->multiMatchesSentence(patternMatcher, sTheory);	

	// If nothing, we've failed
	if (subpattern_results.size() == 0)
		return result; // empty
	
	// If there is only one pattern, assemble the results and return
	// We have to keep a separate score vector, because these cannot be combined correctly
	//  until we are finished recursing and back at the top-level	
	if (patternGroup.size() == 1) {
		for (size_t j = 0; j < subpattern_results.size(); j++) {
			PatternFeatureSet_ptr subSFS = subpattern_results.at(j);
			std::vector<float> scores;
			scores.push_back(subSFS->getScore());
			result.push_back(std::make_pair(subSFS, scores));
		}
		return result;
	}

	// Otherwise, create a list of the remaining patterns
	std::vector<Pattern_ptr> remainingPatterns;
	for (size_t i = 1; i < patternGroup.size(); i++) {
		remainingPatterns.push_back(patternGroup.at(i));
	}

	// And collect their results
	std::vector<feature_score_pair_t> remaining_results = multiMatchesPatternGroup(patternMatcher, remainingPatterns, sTheory, debug);

	// If nothing, we've failed
	if (remaining_results.size() == 0)
		return result; // empty

	// Now take combine first-results = [a b c] with remaining-results [x y z] --> [ax ay az bx by bz cx cy cz]
	for (size_t j = 0; j < subpattern_results.size(); j++) {
		PatternFeatureSet_ptr subSFS = subpattern_results.at(j);
		for (size_t k = 0; k < remaining_results.size(); k++) {
			PatternFeatureSet_ptr sfs = boost::make_shared<PatternFeatureSet>(); 
			sfs->addFeatures(subSFS);

			PatternFeatureSet_ptr remainingSFS = remaining_results.at(k).first;
			sfs->addFeatures(remainingSFS);

			std::vector<float> scores = remaining_results.at(k).second;
			scores.push_back(subSFS->getScore());

			result.push_back(std::make_pair(sfs, scores));
		}

	}

	return result;
}


std::vector<PatternFeatureSet_ptr> IntersectionPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, 
																			 SentenceTheory *sTheory, UTF8OutputStream *debug) 
{
	if (_languageVariant && !patternMatcher->getActiveLanguageVariant()->matchesConstraint(*_languageVariant)) {
		return multiMatchesAlignedSentence(patternMatcher, sTheory, _languageVariant); 
	}
	
	if (_force_single_match_sentence) {
		std::vector<PatternFeatureSet_ptr> return_vector;
		PatternFeatureSet_ptr result = matchesSentence(patternMatcher, sTheory, debug);
		if (result.get() != 0) {
			return_vector.push_back(result);
		}
		return return_vector;
	}

	 std::vector<feature_score_pair_t> intermediate_results = multiMatchesPatternGroup(patternMatcher, _patternList, sTheory, debug);
	 std::vector<PatternFeatureSet_ptr> results;

	 // Add top-level features and combine scores appropriately
	 for (size_t i = 0; i < intermediate_results.size(); i++) {
		 PatternFeatureSet_ptr sfs = intermediate_results.at(i).first;
		 sfs->addFeature(boost::make_shared<GenericPFeature>(shared_from_this(), sTheory->getSentNumber(), patternMatcher->getActiveLanguageVariant()));
		 if (getReturn())
			 sfs->addFeature(boost::make_shared<GenericReturnPFeature>(shared_from_this(), sTheory->getSentNumber(), patternMatcher->getActiveLanguageVariant()));
		 sfs->setScore(_scoringFunction(intermediate_results.at(i).second, _score)); 
		 addID(sfs);
		 results.push_back(sfs);
	 }

	 return results;

}

Pattern_ptr IntersectionPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {
	for (size_t i = 0; i < _patternList.size(); ++i) {
		replaceShortcut<Pattern>(_patternList[i], refPatterns);
	}	
	return shared_from_this();
}

void IntersectionPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "IntersectionPattern: ";
	if (!getID().is_null()) out << getID();
	out << std::endl;
	for (size_t i = 0; i < _patternList.size(); i++) {
		_patternList[i]->dump(out, indent+2);
	}
}

/**
 * Redefinition of parent class's virtual method.
 *
 * @param PatternReturnVecSeq for a given pattern
 *
 * @author afrankel@bbn.com
 * @date 2010.10.20
 **/
void IntersectionPattern::getReturns(PatternReturnVecSeq & output) const {
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
Symbol IntersectionPattern::getFirstValidID() const {
	return getFirstValidIDForCompositePattern(_patternList);
}
