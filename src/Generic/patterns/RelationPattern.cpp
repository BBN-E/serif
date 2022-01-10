// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/RelationPattern.h"
#include "Generic/patterns/ScoringFactory.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/ArgumentPattern.h"
#include "Generic/patterns/features/RelMentionPFeature.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/RelMention.h"
#include "Generic/common/Sexp.h"

// Private symbols
namespace {
	Symbol relationSym = Symbol(L"relation");
	Symbol arg1Sym = Symbol(L"ARG1");
	Symbol arg2Sym = Symbol(L"ARG2");
}

RelationPattern::RelationPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets)
: ExtractionPattern(sexp, entityLabels, wordSets)
{
	if (sexp->getNthChild(0)->getValue() != relationSym) {
		std::ostringstream ostr;
		ostr << "Unexpected pattern type sym (" << sexp->getNthChild(0)->getValue() << ") for RelationPattern.";
		throwError(sexp, ostr.str().c_str());
	}
	initializeFromSexp(sexp, entityLabels, wordSets);
}

Symbol RelationPattern::getPatternTypeSym() {
	return relationSym;
}

PatternFeatureSet_ptr RelationPattern::matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) {
	int sent_no = sTheory->getTokenSequence()->getSentenceNumber();
	std::vector<float> scores;
	int n_items = sTheory->getRelMentionSet()->getNRelMentions();

	PatternFeatureSet_ptr allSentenceMatches = boost::make_shared<PatternFeatureSet>();
	bool matched = false;		
	for (int i = 0; i < n_items; i++) {
		PatternFeatureSet_ptr sfs;
		sfs = matchesRelMention(patternMatcher, sent_no, sTheory->getRelMentionSet()->getRelMention(i));

		if (sfs != 0) {
			allSentenceMatches->addFeatures(sfs);
			scores.push_back(sfs->getScore());
			matched = true;
		}
	}
	if (matched) {
		// just want the best one from this sentence, no fancy combination
		allSentenceMatches->setScore(ScoringFactory::scoreMax(scores, Pattern::UNSPECIFIED_SCORE));
		return allSentenceMatches;
	} else {
		return PatternFeatureSet_ptr();
	}
}

std::vector<PatternFeatureSet_ptr> RelationPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) {
	std::vector<PatternFeatureSet_ptr> return_vector;

	if (_force_single_match_sentence) {
		PatternFeatureSet_ptr result = matchesSentence(patternMatcher, sTheory, debug);
		if (result.get() != 0) {
			return_vector.push_back(result);
		}
		return return_vector;
	}

	int sent_no = sTheory->getTokenSequence()->getSentenceNumber();
	int n_items = sTheory->getRelMentionSet()->getNRelMentions();

	for (int i = 0; i < n_items; i++) {
		PatternFeatureSet_ptr sfs;
		sfs = matchesRelMention(patternMatcher, sent_no, sTheory->getRelMentionSet()->getRelMention(i));
		if (sfs) {
			return_vector.push_back(sfs);
		}
	}
	return return_vector;
}

PatternFeatureSet_ptr RelationPattern::matchesRelMention(PatternMatcher_ptr patternMatcher, int sent_no, const RelMention *rm) {
	if (!matchesType(rm->getType()))
		return PatternFeatureSet_ptr();

	// these don't count for scoring, because if they fire, we aren't matching!
	for (size_t i = 0; i < _blockedArgs.size(); i++) {
		ArgumentPattern_ptr blockedArgPattern = _blockedArgs[i]->castTo<ArgumentPattern>();
		PatternFeatureSet_ptr sfs = blockedArgPattern->matchesMentionAndRole(patternMatcher, arg1Sym, rm->getLeftMention(), true);
		if (sfs != 0) {
			return PatternFeatureSet_ptr();
		}
		sfs = blockedArgPattern->matchesMentionAndRole(patternMatcher, arg2Sym, rm->getRightMention(), true);
		if (sfs != 0) {
			return PatternFeatureSet_ptr();
		}		
		sfs = blockedArgPattern->matchesValueMentionAndRole(patternMatcher, rm->getTimeRole(), rm->getTimeArgument());
		if (sfs != 0) {
			return PatternFeatureSet_ptr();
		}
	}

	// Try every matching from pattern argument (i) to this argument or value argument (j).
	// Take the one with the highest score if it's greater than 0.
	std::vector<std::vector<PatternFeatureSet_ptr> > req_i_j_to_sfs;
	int max_j = 3; // hard-coded for relations
	for (size_t i = 0; i < _args.size(); i++) {
		std::vector<PatternFeatureSet_ptr> empty_vector;
		req_i_j_to_sfs.push_back(empty_vector);

		MentionAndRoleMatchingPattern_ptr mentionRolePattern = _args[i]->castTo<MentionAndRoleMatchingPattern>();
		
		PatternFeatureSet_ptr sfs1 = mentionRolePattern->matchesMentionAndRole(patternMatcher, arg1Sym, rm->getLeftMention(), true);
		PatternFeatureSet_ptr sfs2 = mentionRolePattern->matchesMentionAndRole(patternMatcher, arg2Sym, rm->getRightMention(), true);
		req_i_j_to_sfs[i].push_back(sfs1);
		req_i_j_to_sfs[i].push_back(sfs2);

		ValueMentionAndRoleMatchingPattern_ptr valueMentionRolePattern = _args[i]->castTo<ValueMentionAndRoleMatchingPattern>();
		PatternFeatureSet_ptr sfs3 = valueMentionRolePattern->matchesValueMentionAndRole(patternMatcher, rm->getTimeRole(), rm->getTimeArgument());		
		req_i_j_to_sfs[i].push_back(sfs3);
		
		// if we fail at a non-optional argument, might as well bail now
		if (sfs1 == 0 && sfs2 == 0 && sfs3 == 0) {
			ArgumentPattern_ptr argPattern = boost::dynamic_pointer_cast<ArgumentPattern>(_args[i]);
			if (argPattern && !argPattern->isOptional()) {
				return PatternFeatureSet_ptr();
			}
		}
	}

	std::vector<std::vector<PatternFeatureSet_ptr> > opt_i_j_to_sfs;
	for(size_t i = 0; i < _optArgs.size(); i++) {
		std::vector<PatternFeatureSet_ptr> empty_vector;
		opt_i_j_to_sfs.push_back(empty_vector);

		MentionAndRoleMatchingPattern_ptr mentionRolePattern = _optArgs[i]->castTo<MentionAndRoleMatchingPattern>();

		PatternFeatureSet_ptr sfs1 = mentionRolePattern->matchesMentionAndRole(patternMatcher, arg1Sym, rm->getLeftMention(), true);
		PatternFeatureSet_ptr sfs2 = mentionRolePattern->matchesMentionAndRole(patternMatcher, arg2Sym, rm->getRightMention(), true);
		opt_i_j_to_sfs[i].push_back(sfs1);
		opt_i_j_to_sfs[i].push_back(sfs2);

		ValueMentionAndRoleMatchingPattern_ptr valueMentionRolePattern = _optArgs[i]->castTo<ValueMentionAndRoleMatchingPattern>();
		PatternFeatureSet_ptr sfs3 = valueMentionRolePattern->matchesValueMentionAndRole(patternMatcher, rm->getTimeRole(), rm->getTimeArgument());
		opt_i_j_to_sfs[i].push_back(sfs3);
	}

	PatternFeatureSet_ptr allFeatures = boost::make_shared<PatternFeatureSet>();
	// Add a feature for the return value (if we have one)
	if (getReturn())
		allFeatures->addFeature(boost::make_shared<RelMentionReturnPFeature>(shared_from_this(), rm, sent_no, patternMatcher->getActiveLanguageVariant()));
	// Add a feature for the pattern itself
	allFeatures->addFeature(boost::make_shared<RelMentionPFeature>(shared_from_this(), rm, sent_no, patternMatcher->getActiveLanguageVariant()));
	// Add a feature for the ID (if we have one)
	addID(allFeatures);
	// Set the feature set's scores.
	// Add features from the required arguments.
	return fillAllFeatures(req_i_j_to_sfs, opt_i_j_to_sfs, allFeatures, _args, _optArgs);
}
