#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include <sstream>
#include <string>
#include <stdexcept>
#include <cassert>
#include <boost/shared_ptr.hpp>
#include "Generic/common/Sexp.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternSet.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "LearnIt/LearnItPattern.h"
#include "LearnIt/MainUtilities.h"
#include "LearnIt/MatchInfo.h"

using boost::make_shared;
using boost::dynamic_pointer_cast;

LearnItPattern::LearnItPattern(Target_ptr target, const std::wstring& name, 
				 const std::wstring& pattern_string, const LanguageVariant_ptr& language, bool active, 
				 bool rejected, float precision, float recall, 
				 bool multi, const std::wstring& keywordString)
: _target(target),
_name(name),
_pattern_string(pattern_string),
_language(language),
_query_pattern_set(),
_active(active),
_rejected(rejected),
_precision(precision),
_recall(recall),
_multi(multi)
{
	boost::split(keywords, keywordString, boost::is_any_of(L";"));
	if (!ParamReader::isParamTrue("unswitched_pattern_and_name")) {
		std::wistringstream pat_str(pattern_string);
		Sexp sexp(pat_str, true);
		_query_pattern_set = make_shared<PatternSet>(&sexp);
    } else {
		std::wistringstream pat_str(pattern_string);
		Sexp sexp(pat_str, true);
		_query_pattern_set = make_shared<PatternSet>(&sexp);
	}
}

bool LearnItPattern::preFilterPattern(const SentenceTheory &sent_theory) const {
	if (keywords.size() == 0) {
		return true;
	} else {
		std::wstring sentence_string = L"";// sTheory->getTokenSequence()->toString();
		std::vector<SentenceTheory*> sents = _matcher->getDocumentSet()->getSentenceTheories(sent_theory.getSentNumber());
		for (std::size_t i=0;i<sents.size();i++) {
			sentence_string += sents[i]->getTokenSequence()->toString();
		}
		for (std::size_t i=0;i<keywords.size();i++) {
			if (sentence_string.find(keywords[i]) == std::wstring::npos) {
				return false;
			}
		}
		return true;
	}
}

std::vector<PatternFeatureSet_ptr> LearnItPattern::applyToSentence(const SentenceTheory &sent_theory) 
{	
	if (_matcher->getDocumentSet()->getDocTheory(_language)) {
		_matcher->setActiveLanguageVariant(_language);
	}

	std::vector<PatternFeatureSet_ptr> feature_sets;
	if (!preFilterPattern(sent_theory)) {
		//SessionLogger::info("LEARNIT") << "Skipping sentence because of pre-filtering" << std::endl;
		return feature_sets;
	}
	try {
		// final param of "true" guarantees we get multiple matches if they
		// are present
		feature_sets = _matcher->getSentenceSnippets(
			const_cast<SentenceTheory*>(&sent_theory), 0, true);
	} catch (std::runtime_error &e) {
		// This can happen if regexps contain things like ".*, .*" which cause more 
		// backtracking than Boost is willing to put up with.
		SessionLogger::info("LEARNIT") << "Error while applying pattern " << _pattern_string << ": " << e.what() << std::endl;
	}

/*	for (size_t i = 0; i < _query_pattern_set->getNTopLevelPatterns(); i++) {
		try {
			SentenceMatchingPattern_ptr smp = 
				dynamic_pointer_cast<SentenceMatchingPattern>(_query_pattern_set->getNthTopLevelPattern(i));
			if (!smp) continue;
			if (_multi) {
				// why does multiMatchesSentence take mutable sentence theories?!?
				std::vector<PatternFeatureSet*> feature_set_vector_i = 
					smp->multiMatchesSentence(&doc_info, 
						const_cast<SentenceTheory*>(&sent_theory), NULL);
				for (std::vector<PatternFeatureSet*>::iterator sfs_iter = feature_set_vector_i.begin(); sfs_iter != feature_set_vector_i.end(); sfs_iter++) {
					feature_sets.push_back(PatternFeatureSet_ptr(*sfs_iter));
				}
			} else {
				PatternFeatureSet* sfs = smp->matchesSentence(
						&doc_info, const_cast<SentenceTheory*>(&sent_theory), NULL);
				if (sfs) {
					feature_sets.push_back(PatternFeatureSet_ptr(sfs));
				}
			}
		} catch (std::runtime_error &e) {
			// This can happen if regexps contain things like ".*, .*" which cause more 
			// backtracking than Boost is willing to put up with.
			SessionLogger::info("LEARNIT") << "Error while applying pattern " << _pattern_string << ": " << e.what() << std::endl;
		}
	}*/
	return feature_sets;					
								
}

void LearnItPattern::makeMatcher(const AlignedDocSet_ptr docSet) {
	_matcher = PatternMatcher::makePatternMatcher(docSet, _query_pattern_set, NULL, PatternMatcher::DO_NOT_COMBINE_SNIPPETS, Symbol::HashMap<const AbstractSlot*>(), NULL, false);
}

void LearnItPattern::makeMatcher(const DocTheory* docTheory) {
	_matcher = PatternMatcher::makePatternMatcher(docTheory, _query_pattern_set, NULL, PatternMatcher::DO_NOT_COMBINE_SNIPPETS, Symbol::HashMap<const AbstractSlot*>(), NULL, false);
}

std::vector<PatternFeatureSet_ptr> LearnItPattern::applyToSentence(
	const DocTheory *docTheory, const SentenceTheory &sent_theory) 
{	
	return applyToSentence(sent_theory);			
}

std::vector<PatternFeatureSet_ptr> LearnItPattern::applyToSentence(
	const AlignedDocSet_ptr doc_set, const SentenceTheory &sent_theory) 
{	
	return applyToSentence(sent_theory);
}
