// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/QuotationPattern.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/QuotePFeature.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/ShortcutPattern.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SpeakerQuotationSet.h"
#include "Generic/theories/SpeakerQuotation.h"
#include "Generic/common/Sexp.h"

// Private symbols
namespace {
	Symbol speakerSym(L"speaker");
	Symbol quoteSym(L"quote");
}

QuotationPattern::QuotationPattern(Sexp *sexp, const Symbol::HashSet &entityLabels,
							   const PatternWordSetMap& wordSets)
{
	int nkids = sexp->getNumChildren();
	if (nkids < 2)
		throwError(sexp, "too few children in QuotationPattern");
	initializeFromSexp(sexp, entityLabels, wordSets);
}

bool QuotationPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (Pattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (constraintType == speakerSym) {
		_speakerPattern = parseSexp(childSexp->getSecondChild(), entityLabels, wordSets);
		return true;
	} else if (constraintType == quoteSym) {
		_quotePattern = parseSexp(childSexp->getSecondChild(), entityLabels, wordSets);
		return true;
	} else {
		logFailureToInitializeFromChildSexp(childSexp);		
		return false;
	}
}

Pattern_ptr QuotationPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {
	replaceShortcut<MentionMatchingPattern>(_speakerPattern, refPatterns);
	replaceShortcut<SentenceMatchingPattern>(_quotePattern, refPatterns);
	return shared_from_this();
}

std::vector<PatternFeatureSet_ptr> QuotationPattern::multiMatchesDocument(PatternMatcher_ptr patternMatcher, UTF8OutputStream *debug) {
	std::vector<PatternFeatureSet_ptr> featureSets;

	if (_force_single_match_sentence) {
		PatternFeatureSet_ptr result = matchesDocument(patternMatcher, debug);
		if (result.get() != 0) {
			featureSets.push_back(result);
		}
		return featureSets;
	} 

	const SpeakerQuotationSet* quotations = patternMatcher->getDocTheory()->getSpeakerQuotationSet();
	if (!quotations) return featureSets; // no quotations!

	for (int q = 0; q < quotations->getNSpeakerQuotations(); q++) {
		const SpeakerQuotation *quote = quotations->getSpeakerQuotation(q);
		// skip single sentence quotations -- these are already handled by other patterns already.
		if (quote->getQuoteStartSentence() == quote->getQuoteEndSentence())
			continue;

		PatternFeatureSet_ptr sfs = matchesSpeakerQuotation(patternMatcher, quote);
		if (sfs) 
			featureSets.push_back(sfs);
	}
	return featureSets;	
}

PatternFeatureSet_ptr QuotationPattern::matchesDocument(PatternMatcher_ptr patternMatcher, UTF8OutputStream *debug) {
	const SpeakerQuotationSet* quotations = patternMatcher->getDocTheory()->getSpeakerQuotationSet();
	if (!quotations) return PatternFeatureSet_ptr(); // no quotations!

	PatternFeatureSet_ptr allMatches = boost::make_shared<PatternFeatureSet>();
	std::vector<float> scores;

	bool matched = false;		
	for (int q = 0; q < quotations->getNSpeakerQuotations(); q++) {
		const SpeakerQuotation *quote = quotations->getSpeakerQuotation(q);
		// skip single sentence quotations -- these are already handled by other patterns already.
		if (quote->getQuoteStartSentence() == quote->getQuoteEndSentence())
			continue;

		PatternFeatureSet_ptr sfs;
		sfs = matchesSpeakerQuotation(patternMatcher, quote);
		if (sfs) {
			allMatches->addFeatures(sfs);
			scores.push_back(sfs->getScore());
			matched = true;
		}
	}
	if (matched) {
		allMatches->setScore(ScoringFactory::scoreMax(scores, Pattern::UNSPECIFIED_SCORE));
		return allMatches;
	} else {
		return PatternFeatureSet_ptr();
	}
}

PatternFeatureSet_ptr QuotationPattern::matchesSpeakerQuotation(PatternMatcher_ptr patternMatcher, const SpeakerQuotation *quotation) {
	std::vector<float> scores;
	MentionMatchingPattern_ptr speakerPattern = _speakerPattern->castTo<MentionMatchingPattern>();
	PatternFeatureSet_ptr sfs = speakerPattern->matchesMention(patternMatcher, quotation->getSpeakerMention(), true); // allow fall-through
	if (!sfs)
		return PatternFeatureSet_ptr();
	scores.push_back(sfs->getScore());
	sfs->addFeature(boost::make_shared<QuotePFeature>(_speakerPattern, quotation->getSpeakerMention(), patternMatcher->getActiveLanguageVariant()));

	bool found_matching_sentence = false;
    for (int s = quotation->getQuoteStartSentence(); s <= quotation->getQuoteEndSentence(); s++) {
		PatternFeatureSet_ptr quoteSet;
		if (_quotePattern) {
			SentenceMatchingPattern_ptr quotePattern = _quotePattern->castTo<SentenceMatchingPattern>();
			quoteSet = quotePattern->matchesSentence(patternMatcher, patternMatcher->getDocTheory()->getSentenceTheory(s));
		}
		int stok = 0;
		int etok = patternMatcher->getDocTheory()->getSentenceTheory(s)->getTokenSequence()->getNTokens() - 1;
		if (s == quotation->getQuoteStartSentence())
			stok = quotation->getQuoteStartToken();
		if (s == quotation->getQuoteEndSentence())
			etok =  quotation->getQuoteEndToken();
		if (quoteSet) {
			found_matching_sentence = true;
			sfs->addFeatures(quoteSet);
			scores.push_back(quoteSet->getScore());
			sfs->addFeature( boost::make_shared<QuotePFeature>(_quotePattern, s, stok, etok, true, patternMatcher->getActiveLanguageVariant()) );
		} else if (_quotePattern) {
			sfs->addFeature( boost::make_shared<QuotePFeature>(Pattern_ptr(), s, stok, etok, false, patternMatcher->getActiveLanguageVariant()) );
		} else {
			sfs->addFeature( boost::make_shared<QuotePFeature>(Pattern_ptr(), s, stok, etok, true, patternMatcher->getActiveLanguageVariant()) );
		}
	}

	if (!found_matching_sentence && _quotePattern) {
		return PatternFeatureSet_ptr();
	}
	// Add a feature for the ID (if we have one)
	addID(sfs);
	// Initialize our score.
	sfs->setScore(_scoringFunction(scores, _score));
	return sfs;
}

void QuotationPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "QuotationPattern:\n";
	for (int i = 0; i < indent; i++) out << " ";
	out << "Speaker pattern:\n";
    _speakerPattern->dump(out, indent+4);
	for (int i = 0; i < indent; i++) out << " ";
	if (_quotePattern != 0) {
		out << "Quote pattern:\n";
		_quotePattern->dump(out, indent+4);
	}
}

