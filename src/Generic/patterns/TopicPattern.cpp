// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/patterns/TopicPattern.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/features/TopicPFeature.h"
#include "Generic/patterns/features/CoveredPropNodePFeature.h"
#include <boost/lexical_cast.hpp>
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Argument.h"
#include "Generic/common/Sexp.h"

// Private symbols
namespace {
	Symbol querySlotSym(L"query_slot");
	Symbol thresholdSym(L"threshold");
	Symbol contextSym(L"context");
	Symbol backwardRangeSym(L"backward_range");
	Symbol forwardRangeSym(L"forward_range");
	Symbol matchStrategySym(L"strategy");
	Symbol scoreTypeSym(L"score_type");
	Symbol minNodeSym(L"min_node_count");
	Symbol maxNodeSym(L"max_node_count");
}

TopicPattern::TopicPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets)
: _querySlot(), _threshold(-1), _context_size(1), _forward_range(0), _backward_range(0), 
  _match_strategy(FULL), _use_real_scores(false), _min_node_count(-1), _max_node_count(-1)
{
	initializeFromSexp(sexp, entityLabels, wordSets);

	if (_querySlot.is_null())
		throwError(sexp, "query slot not set in TopicPattern");
	if (_threshold == -1)
		throwError(sexp, "threshold not set in TopicPattern");
	if (_context_size != 1 && (_forward_range > 0 || _backward_range > 0))
		throwError(sexp, "forward/backward range only allowed when context==1");

}

bool TopicPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (Pattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (constraintType == contextSym) {
		_context_size = boost::lexical_cast<int>(childSexp->getSecondChild()->getValue().to_string());
	} else if (constraintType == forwardRangeSym) {
		_forward_range = boost::lexical_cast<int>(childSexp->getSecondChild()->getValue().to_string());
	} else if (constraintType == backwardRangeSym) {
		_backward_range = boost::lexical_cast<int>(childSexp->getSecondChild()->getValue().to_string());
	} else if (constraintType == querySlotSym) {
		_querySlot = childSexp->getSecondChild()->getValue();
	} else if (constraintType == scoreTypeSym) {
		if (childSexp->getSecondChild()->getValue() == Symbol(L"real"))
			_use_real_scores = true;
		else throwError(childSexp, "score_type must be 'real' (only option right now)");
	} else if (constraintType == matchStrategySym) {
		if (childSexp->getSecondChild()->getValue() == Symbol(L"full")) {
			_match_strategy = FULL;
		} else if (childSexp->getSecondChild()->getValue() == Symbol(L"edge")) {
			_match_strategy = EDGE;
		} else if (childSexp->getSecondChild()->getValue() == Symbol(L"subtree")) { // Backwards compatibility
			_match_strategy = EDGE;
		} else if (childSexp->getSecondChild()->getValue() == Symbol(L"node")) {
			_match_strategy = NODE;
		} else if (childSexp->getSecondChild()->getValue() == Symbol(L"decision_tree")) {
			_match_strategy = DECISION_TREE;
		} else {
			throwError(childSexp, "relevance match_type must be 'full', 'edge', 'node', or 'decision_tree'");
		}
	} else if (constraintType == thresholdSym) {
		_threshold = boost::lexical_cast<float>(childSexp->getSecondChild()->getValue().to_string());
	} else if (constraintType == minNodeSym) {
		_min_node_count = boost::lexical_cast<int>(childSexp->getSecondChild()->getValue().to_string());
	} else if (constraintType == maxNodeSym) {
		_max_node_count = boost::lexical_cast<int>(childSexp->getSecondChild()->getValue().to_string());
	} else {
		logFailureToInitializeFromChildSexp(childSexp);		
		return false;
	}
	return true;
}

PatternFeatureSet_ptr TopicPattern::matchesProp(PatternMatcher_ptr patternMatcher, int sent_no, const Proposition* prop, bool fall_through_children, PropStatusManager_ptr statusOverrides)
{
	// fall_through_children & statusOverrides are not relevant here

	float score = 0;
	const AbstractSlot *slot = patternMatcher->getSlot(_querySlot);
	if (slot == 0)
		return PatternFeatureSet_ptr();
	boost::shared_ptr<PropMatch> propMatch = boost::shared_ptr<PropMatch>();
	if (_match_strategy == FULL) {
		score = patternMatcher->getFullScore(slot, sent_no, prop);
		propMatch = slot->getMatcher(AbstractSlot::FULL);
	} else if (_match_strategy == EDGE) {
		score = patternMatcher->getEdgeScore(slot, sent_no, prop);
		propMatch = slot->getMatcher(AbstractSlot::EDGE);
	} else {
		SessionLogger::warn("illegal_pattern_use") << "TopicPattern should not be called on a Proposition with match strategy 'node'!\n";
		dump(std::cerr);
	}		
	if (score > _threshold && satisfiesQueryConstraints(propMatch))
		return makePatternFeatureSet(propMatch, sent_no, patternMatcher->getActiveLanguageVariant(), score);
	else return PatternFeatureSet_ptr();
}

PatternFeatureSet_ptr TopicPattern::matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides) {
	if (arg->getType() == Argument::MENTION_ARG) {
		MentionSet *mSet = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getMentionSet();
		return matchesMention(patternMatcher, arg->getMention(mSet), fall_through_children);
	}
	return PatternFeatureSet_ptr();
}

PatternFeatureSet_ptr TopicPattern::matchesMention(PatternMatcher_ptr patternMatcher, const Mention *mention, bool fall_through_children)
{
	// fall_through_children is not relevant here

	float score = 0;
	const AbstractSlot *slot = patternMatcher->getSlot(_querySlot);
	if (slot == 0)
		return PatternFeatureSet_ptr();
	boost::shared_ptr<PropMatch> propMatch = boost::shared_ptr<PropMatch>();
	if (_match_strategy == FULL) {
		score = patternMatcher->getFullScore(slot, mention->getSentenceNumber(), mention);
		propMatch = slot->getMatcher(AbstractSlot::FULL);
	} else if (_match_strategy == EDGE) {
		score = patternMatcher->getEdgeScore(slot, mention->getSentenceNumber(), mention);
		propMatch = slot->getMatcher(AbstractSlot::EDGE);
	} else {
		SessionLogger::warn("illegal_pattern_use") << "TopicPattern should not be called on a Mention with match strategy 'node'!\n";
		dump(std::cerr);
	}		
	if (score > _threshold && satisfiesQueryConstraints(propMatch))
		return makePatternFeatureSet(propMatch, mention->getSentenceNumber(), patternMatcher->getActiveLanguageVariant(), score, mention);
	else return PatternFeatureSet_ptr();
}

std::vector<PatternFeatureSet_ptr> TopicPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) {

	// no need to override _force_single_match_sentence, since it already does so

	std::vector<PatternFeatureSet_ptr> results;
	PatternFeatureSet_ptr result = matchesSentence(patternMatcher, sTheory, debug);
	if (result.get() != 0) {
		results.push_back(result);
	}
	return results;
}	


PatternFeatureSet_ptr TopicPattern::matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) { 
	int sentno = sTheory->getTokenSequence()->getSentenceNumber();

	std::vector<PatternFeatureSet_ptr> matches;

	// These can match using backward/forward range, and this causes failures elsewhere
	if (sTheory->getTokenSequence()->getNTokens() == 0)
		return PatternFeatureSet_ptr();
			
	float sentence_relevance = 0;
	const AbstractSlot *slot = patternMatcher->getSlot(_querySlot);
	if (slot == 0)
		return PatternFeatureSet_ptr();
	boost::shared_ptr<PropMatch> propMatch = boost::shared_ptr<PropMatch>();
	if (_match_strategy == DECISION_TREE) {
		sentence_relevance = patternMatcher->getDecisionTreeScore(slot, sentno);
		// we just get the full/edge/node for this sentence, though surrounding sentences might be why we found this
		matches.push_back(patternMatcher->getFullFeatures(slot, sentno));
		matches.push_back(patternMatcher->getEdgeFeatures(slot, sentno));
		matches.push_back(patternMatcher->getNodeFeatures(slot, sentno));
		propMatch = slot->getMatcher(AbstractSlot::NODE);
	} else if (_match_strategy == FULL) {
		sentence_relevance = patternMatcher->getFullScore(slot, sentno, _context_size);
		matches.push_back(patternMatcher->getFullFeatures(slot, sentno));
		propMatch = slot->getMatcher(AbstractSlot::FULL);
	} else if (_match_strategy == EDGE) {
		sentence_relevance = patternMatcher->getEdgeScore(slot, sentno, _context_size);
		matches.push_back(patternMatcher->getEdgeFeatures(slot, sentno));
		propMatch = slot->getMatcher(AbstractSlot::EDGE);
	} else if (_match_strategy == NODE) {
		sentence_relevance = patternMatcher->getNodeScore(slot, sentno, _context_size);
		matches.push_back(patternMatcher->getNodeFeatures(slot, sentno));
		propMatch = slot->getMatcher(AbstractSlot::NODE);
	} else {
		throw UnexpectedInputException("TopicPattern::matchesSentence()", "_match_strategy has invalid value");
	}

	if (!satisfiesQueryConstraints(propMatch))
		return PatternFeatureSet_ptr();

	if (sentence_relevance > _threshold) {
		return makePatternFeatureSet(matches, sentno, patternMatcher->getActiveLanguageVariant(), sentence_relevance);
	} else {
		matches.clear();
	}
		
	// only allow forward and backward range if context == 1
	if (_context_size == 1) {
		for (int f = 1; f <= _forward_range; f++) {
			if (sentno + f < patternMatcher->getDocTheory()->getNSentences()) {
				if (_match_strategy == FULL) {
					sentence_relevance = patternMatcher->getFullScore(slot, sentno+f, _context_size);
					matches.push_back(patternMatcher->getFullFeatures(slot, sentno+f));
				} else if (_match_strategy == EDGE) {
					sentence_relevance = patternMatcher->getEdgeScore(slot, sentno+f, _context_size);
					matches.push_back(patternMatcher->getEdgeFeatures(slot, sentno+f));
				} else if (_match_strategy == NODE) {
					sentence_relevance = patternMatcher->getNodeScore(slot, sentno+f, _context_size);
					matches.push_back(patternMatcher->getNodeFeatures(slot, sentno+f));
				} else {
					throw UnexpectedInputException("TopicPattern::matchesSentence()", "_match_strategy has invalid value");
				}
				if (sentence_relevance > _threshold)
					return makePatternFeatureSet(matches, sentno, patternMatcher->getActiveLanguageVariant(), sentence_relevance);
				else matches.clear();
			} else break;
		}

		for (int b = 1; b <= _backward_range; b++) {
			if (sentno - b >= 0) {
				if (_match_strategy == FULL) {
					sentence_relevance = patternMatcher->getFullScore(slot, sentno-b, _context_size);
					matches.push_back(patternMatcher->getFullFeatures(slot, sentno-b));
				} else if (_match_strategy == EDGE) {
					sentence_relevance = patternMatcher->getEdgeScore(slot, sentno-b, _context_size);
					matches.push_back(patternMatcher->getEdgeFeatures(slot, sentno-b));
				} else if (_match_strategy == NODE) {
					sentence_relevance = patternMatcher->getNodeScore(slot, sentno-b, _context_size);
					matches.push_back(patternMatcher->getNodeFeatures(slot, sentno-b));
				} else {
					throw UnexpectedInputException("TopicPattern::matchesSentence()", "_match_strategy has invalid value");
				}
				if (sentence_relevance > _threshold)
					return makePatternFeatureSet(matches, sentno, patternMatcher->getActiveLanguageVariant(), sentence_relevance);
				else matches.clear();
			} else break;		
		}
	}
	return PatternFeatureSet_ptr();
}

PatternFeatureSet_ptr TopicPattern::makePatternFeatureSet(boost::shared_ptr<PropMatch> propMatch, int sentno, const LanguageVariant_ptr& languageVariant, float confidence, const Mention* mention) {
	PatternFeatureSet_ptr sfs = makeCorePatternFeatureSet(sentno, languageVariant, confidence, mention);

	// Add features that indicate which prop-nodes were involved in this match
	/*  THIS DOESN'T WORK FOR FULL/EDGE ANYWAY
	if (propMatch) {
		AbstractSlot::MatchType matchType = AbstractSlot::FULL;
		if (_match_strategy == EDGE)
			matchType = AbstractSlot::EDGE;
		else if (_match_strategy == NODE)
			matchType = AbstractSlot::NODE;
		for (size_t i = 0; i < propMatch->getMatchCovered().size(); i++) {
			// We don't return the query predicate here, since this is never a node match anyway
			if (propMatch->getMatchCovered().at(i))
				sfs->addFeature(boost::make_shared<CoveredPropNodePFeature>(matchType, propMatch->getMatchCovering().at(i), 
				propMatch->getMatchCovered().at(i), _querySlot, Symbol(L"NULL"), int(i), int(propMatch->getMatchCovered().size())));
		}
	}*/

	return sfs;

}

PatternFeatureSet_ptr TopicPattern::makePatternFeatureSet(std::vector<PatternFeatureSet_ptr> matches, int sentno, const LanguageVariant_ptr& languageVariant, float confidence) {
	PatternFeatureSet_ptr sfs = makeCorePatternFeatureSet(sentno, languageVariant, confidence);
	BOOST_FOREACH(PatternFeatureSet_ptr match, matches) {
		if (match)
			sfs->addFeatures(match);
	}
	return sfs;
}

PatternFeatureSet_ptr TopicPattern::makeCorePatternFeatureSet(int sentno, const LanguageVariant_ptr& languageVariant, float confidence, const Mention* mention) {
	PatternFeatureSet_ptr sfs = boost::make_shared<PatternFeatureSet>();
	// Add a feature for the return value (if we have one)
	if (getReturn())
		sfs->addFeature(boost::make_shared<TopicReturnPFeature>(shared_from_this(), sentno, confidence, _querySlot, mention, languageVariant));
	// Add a feature for the pattern itself
	sfs->addFeature(boost::make_shared<TopicPFeature>(shared_from_this(), sentno, confidence, _querySlot, mention, languageVariant));
	// Add a feature for the ID (if we have one)
	addID(sfs);
	// Initialize our score.
	sfs->setScore(useRealScores() ? confidence : getScore());
	
	return sfs;
}

bool TopicPattern::satisfiesQueryConstraints(boost::shared_ptr<PropMatch> propMatch) {
	if (_min_node_count != -1 || _max_node_count != -1) {
		int num_nodes = propMatch->getNumNodes();
		if (num_nodes < _min_node_count || (_max_node_count != -1 && num_nodes > _max_node_count))
			return false;
	}
	return true;
}

void TopicPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "TopicPattern = (query_slot=" << _querySlot << ", ";
	out << "threshold=" << _threshold << ", ";
	out << "match strategy=";
	if (_match_strategy == FULL)
		out << "full, ";
	else if (_match_strategy == EDGE)
		out << "edge, ";
	else if (_match_strategy == NODE)
		out << "node, ";
	out << "forward_range=" << _forward_range << ", ";
	out << "backward_range=" << _backward_range << ", ";
	out << "context_size=" << _context_size << ")\n";
}

