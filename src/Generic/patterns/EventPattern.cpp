// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/patterns/EventPattern.h"
#include "Generic/patterns/ShortcutPattern.h"
#include "Generic/patterns/ArgumentPattern.h"
#include "Generic/patterns/ScoringFactory.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/features/EventMentionPFeature.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/EventMention.h"
#include "Generic/common/Sexp.h"

// Private symbols
namespace {
	Symbol eventSym = Symbol(L"event");
	Symbol anchorSym = Symbol(L"anchor");
	Symbol ignoreSym = Symbol(L"IGNORE");
}

EventPattern::EventPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets)
: ExtractionPattern(sexp, entityLabels, wordSets)
{
	if (sexp->getNthChild(0)->getValue() != eventSym) {
		std::ostringstream ostr;
		ostr << "Unexpected pattern type sym for EventPattern: " << sexp->getNthChild(0)->getValue().to_debug_string();
		throwError(sexp, ostr.str().c_str());
	}
	initializeFromSexp(sexp, entityLabels, wordSets);
}

Symbol EventPattern::getPatternTypeSym() {
	return eventSym;
}

bool EventPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	if (childSexp->getFirstChild()->getValue() == anchorSym) {
		_anchorPattern = parseSexp(childSexp->getSecondChild(), entityLabels, wordSets);
		return true;
	} else {
		return ExtractionPattern::initializeFromSubexpression(childSexp, entityLabels, wordSets);
	}
}

Pattern_ptr EventPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {
	replaceShortcut<Pattern>(_anchorPattern, refPatterns);
	ExtractionPattern::replaceShortcuts(refPatterns);
	return shared_from_this();
}


PatternFeatureSet_ptr EventPattern::matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) {
	int sent_no = sTheory->getTokenSequence()->getSentenceNumber();
	std::vector<float> scores;
	int n_items = sTheory->getEventMentionSet()->getNEventMentions();

	PatternFeatureSet_ptr allSentenceMatches = boost::make_shared<PatternFeatureSet>();
	bool matched = false;		
	for (int i = 0; i < n_items; i++) {
		PatternFeatureSet_ptr sfs;
		sfs = matchesEventMention(patternMatcher, sent_no, sTheory->getEventMentionSet()->getEventMention(i));
		if (sfs) {
			//SessionLogger::dbg("ev_ment_0") << "Matched event mention " << i << " of " << n_items << " with score " << sfs->getScore();
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

std::vector<PatternFeatureSet_ptr> EventPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *debug) {
	std::vector<PatternFeatureSet_ptr> return_vector;

	if (_force_single_match_sentence) {
		PatternFeatureSet_ptr result = matchesSentence(patternMatcher, sTheory, debug);
		if (result.get() != 0) {
			return_vector.push_back(result);
		}
		return return_vector;
	}

	int sent_no = sTheory->getTokenSequence()->getSentenceNumber();
	int n_items = sTheory->getEventMentionSet()->getNEventMentions();

	for (int i = 0; i < n_items; i++) {
		PatternFeatureSet_ptr sfs;
		sfs = matchesEventMention(patternMatcher, sent_no, sTheory->getEventMentionSet()->getEventMention(i));
		if (sfs) {
			return_vector.push_back(sfs);
		}
	}
	return return_vector;
}

PatternFeatureSet_ptr EventPattern::matchesEventMention(PatternMatcher_ptr patternMatcher, int sent_no, const EventMention *vm) {
	if (!matchesType(vm->getEventType()) || vm->getEventType() == ignoreSym)
		return PatternFeatureSet_ptr();

	// If anchor (trigger) pattern is specified, match it, trying first to use 
	// the event mention's anchor proposition, and failing that, use the event 
	// mention's anchor node.
	PatternFeatureSet_ptr anchorSFS;
	if (_anchorPattern != 0) {
		if (PropMatchingPattern_ptr pattern = boost::dynamic_pointer_cast<PropMatchingPattern>(_anchorPattern)) {
			if (vm->getAnchorProp() != 0)
				anchorSFS = pattern->matchesProp(patternMatcher, sent_no, vm->getAnchorProp(), false, PropStatusManager_ptr()); // don't fall through sets
		} else if (ParseNodeMatchingPattern_ptr pattern = boost::dynamic_pointer_cast<ParseNodeMatchingPattern>(_anchorPattern)) {
			if (vm->getAnchorNode() != 0)
				anchorSFS = pattern->matchesParseNode(patternMatcher, sent_no, vm->getAnchorNode());
		}
		if (!anchorSFS) return PatternFeatureSet_ptr();
	} 

	// these don't count for scoring, because if they fire, we aren't matching!
	for (size_t i = 0; i < _blockedArgs.size(); i++) {
		ArgumentPattern_ptr blockedArgPattern = _blockedArgs[i]->castTo<ArgumentPattern>();
		for (int j = 0; j < vm->getNArgs(); j++) {
			PatternFeatureSet_ptr sfs = blockedArgPattern->matchesMentionAndRole(patternMatcher, vm->getNthArgRole(j), vm->getNthArgMention(j), true);
			if (sfs != 0) return PatternFeatureSet_ptr();
		}
		for (int j = 0; j < vm->getNValueArgs(); j++) {
			PatternFeatureSet_ptr sfs = blockedArgPattern->matchesValueMentionAndRole(patternMatcher, vm->getNthArgValueRole(j), vm->getNthArgValueMention(j));
			if (sfs != 0) return PatternFeatureSet_ptr();
		}
	}

	// Try every matching from pattern argument (i) to this argument or value argument (j).
	// Take the one with the highest score if it's greater than 0.
	std::vector<std::vector<PatternFeatureSet_ptr> > req_i_j_to_sfs;
	for (size_t i = 0; i < _args.size(); i++) {		
		MentionAndRoleMatchingPattern_ptr mentionRolePattern = _args[i]->castTo<MentionAndRoleMatchingPattern>();
		ValueMentionAndRoleMatchingPattern_ptr valueMentionRolePattern = _args[i]->castTo<ValueMentionAndRoleMatchingPattern>();
		std::vector<PatternFeatureSet_ptr> empty_vector;
		req_i_j_to_sfs.push_back(empty_vector);
		bool matched = false;
		if (mentionRolePattern) {
			for (int j = 0; j < vm->getNArgs(); j++) {
				PatternFeatureSet_ptr sfs = mentionRolePattern->matchesMentionAndRole(patternMatcher, vm->getNthArgRole(j), vm->getNthArgMention(j), true);
				req_i_j_to_sfs[i].push_back(sfs);
				if (sfs != 0) matched = true;
			}
		}
		if (valueMentionRolePattern) {
			for (int j = 0; j < vm->getNValueArgs(); j++) {
				PatternFeatureSet_ptr sfs = valueMentionRolePattern->matchesValueMentionAndRole(patternMatcher, vm->getNthArgValueRole(j), vm->getNthArgValueMention(j));
				req_i_j_to_sfs[i].push_back(sfs);
				if (sfs != 0) matched = true;
			}
		}
		// if we fail at a non-optional argument, might as well bail now
		if (!matched) {
			ArgumentPattern_ptr argPattern = boost::dynamic_pointer_cast<ArgumentPattern>(_args[i]);
			if (argPattern && !argPattern->isOptional()) {
				return PatternFeatureSet_ptr();
			}
		}
	}

	std::vector<std::vector<PatternFeatureSet_ptr> > opt_i_j_to_sfs;
	for(size_t i = 0; i < _optArgs.size(); i++) {
		MentionAndRoleMatchingPattern_ptr mentionRolePattern = _optArgs[i]->castTo<MentionAndRoleMatchingPattern>();
		ValueMentionAndRoleMatchingPattern_ptr valueMentionRolePattern = _optArgs[i]->castTo<ValueMentionAndRoleMatchingPattern>();
		std::vector<PatternFeatureSet_ptr> empty_vector;
		opt_i_j_to_sfs.push_back(empty_vector);
		bool matched = false;
		if (mentionRolePattern) {
			for(int j = 0; j < vm->getNArgs(); j++) {
				PatternFeatureSet_ptr sfs = mentionRolePattern->matchesMentionAndRole(patternMatcher, vm->getNthArgRole(j), vm->getNthArgMention(j), true);
				opt_i_j_to_sfs[i].push_back(sfs);
				if (sfs != 0) matched = true;
			}
		}
		if(valueMentionRolePattern) {
			for(int j = 0; j < vm->getNValueArgs(); j++)  {
				PatternFeatureSet_ptr sfs = valueMentionRolePattern->matchesValueMentionAndRole(patternMatcher, vm->getNthArgValueRole(j), vm->getNthArgValueMention(j));
				opt_i_j_to_sfs[i].push_back(sfs);
				if(sfs!=0) matched = true;
			}
		}
	}
	

	PatternFeatureSet_ptr allFeatures = boost::make_shared<PatternFeatureSet>();
	// Add a feature for the return value (if we have one)
	if (getReturn())
		allFeatures->addFeature(boost::make_shared<EventMentionReturnPFeature>(shared_from_this(), vm, sent_no, patternMatcher->getActiveLanguageVariant()));
	// Add a feature for the pattern itself
	allFeatures->addFeature(boost::make_shared<EventMentionPFeature>(shared_from_this(), vm, sent_no, patternMatcher->getActiveLanguageVariant()));
	// Add a feature for the ID (if we have one)
	addID(allFeatures);
	// Set the feature set's scores.
	allFeatures->setScore(getScore());
	// Add features from the anchor.
	if (anchorSFS)
		allFeatures->addFeatures(anchorSFS);
	// Add features from the required arguments.
	return fillAllFeatures(req_i_j_to_sfs, opt_i_j_to_sfs, allFeatures, _args, _optArgs);
}

PatternFeatureSet_ptr EventPattern::matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides) {
	if (arg->getType() == Argument::MENTION_ARG) {
		MentionSet *mSet = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getMentionSet();
		const Mention *ment = arg->getMention(mSet);
		return matchesMention(patternMatcher, ment, fall_through_children);
	} else if (arg->getType() == Argument::PROPOSITION_ARG) {
		EventMention *vm = getVMFromNode(patternMatcher, arg->getProposition()->getPredHead(), sent_no);
		if (vm == 0) 
			return PatternFeatureSet_ptr();
		else return matchesEventMention(patternMatcher, sent_no, vm);
	}
	return PatternFeatureSet_ptr();
}

PatternFeatureSet_ptr EventPattern::matchesMention(PatternMatcher_ptr patternMatcher, const Mention *mention, bool fall_through_children) {
	// fall through sets is irrelevant here
	EventMention *vm = getVMFromNode(patternMatcher, mention->getNode(), mention->getSentenceNumber());
	if (vm == 0) 
		return PatternFeatureSet_ptr();
	else return matchesEventMention(patternMatcher, mention->getSentenceNumber(), vm);
}

EventMention *EventPattern::getVMFromNode(PatternMatcher_ptr patternMatcher, const SynNode *node, int sent_no) {
	EventMentionSet *vmSet = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getEventMentionSet();
	for (int i = 0; i < vmSet->getNEventMentions(); i++) {
		if (vmSet->getEventMention(i)->getAnchorNode()->getHeadPreterm() == node->getHeadPreterm())
			return vmSet->getEventMention(i);
	}
	return 0;
}









