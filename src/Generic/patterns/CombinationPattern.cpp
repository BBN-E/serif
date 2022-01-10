// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include <boost/foreach.hpp>
#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/patterns/CombinationPattern.h"
#include "Generic/patterns/ArgumentPattern.h"
#include "Generic/patterns/MentionPattern.h"
#include "Generic/patterns/PropPattern.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/PropStatusManager.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/MentionPFeature.h"
#include "Generic/patterns/features/PropPFeature.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/Sexp.h"

namespace { // Private symbols
	Symbol anyOfSym(L"any-of");
	Symbol allOfSym(L"all-of");
	Symbol noneOfSym(L"none-of");
	Symbol membersSym(L"members");
	Symbol greedySym(L"GREEDY");
}

CombinationPattern::CombinationPattern(Sexp *sexp, const Symbol::HashSet &entityLabels,
									   const PatternWordSetMap& wordSets):
_is_greedy(false), _combination_type(CombinationType(-1))
{
	// Read the sexp.
	initializeFromSexp(sexp, entityLabels, wordSets);
	// Set our combination type.
	if (sexp->getFirstChild()->getValue() == anyOfSym) {
		_combination_type = ANY_OF;
	} else if (sexp->getFirstChild()->getValue() == allOfSym) {
		_combination_type = ALL_OF;
	} else if (sexp->getFirstChild()->getValue() == noneOfSym) {
		_combination_type = NONE_OF;
	} else {
		throwError(sexp, "CombinationPattern must be all-of or any-of or none-of");
	}
	// Sanity checks
	if (_patternList.size() == 0)
		throwError(sexp, "no member patterns specified in CombinationPattern");
	if (_is_greedy && _combination_type == ALL_OF)
		throwError(sexp, "GREEDY should not be used with ALL_OF patterns");
	if (_is_greedy && _combination_type == NONE_OF)
		throwError(sexp, "GREEDY should not be used with 'not' patterns");
	
}

bool CombinationPattern::initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol atom = childSexp->getValue();
	if (Pattern::initializeFromAtom(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (atom == greedySym) {
		_is_greedy = true; 
		return true;
	} else {
		logFailureToInitializeFromAtom(childSexp);
		return false;
	}
}

bool CombinationPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels,
													  const PatternWordSetMap& wordSets)
{
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (Pattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (constraintType == membersSym) {
		int n_patterns = childSexp->getNumChildren() - 1;
		for (int j = 0; j < n_patterns; j++)
			_patternList.push_back(parseSexp(childSexp->getNthChild(j+1), entityLabels, wordSets));
		return true;
	} else if (PropStatusManager::isPropStatusManagerSym(constraintType)) {
		if (_patternStatusOverrides)
			throwError(childSexp, "Cannot specify more than one psm (PropStatusManager) per pattern");
		_patternStatusOverrides = boost::make_shared<PropStatusManager>(childSexp);
		return true;
	} else {
		logFailureToInitializeFromChildSexp(childSexp);
		return false;
	}
}

namespace { // Private helper method.
	template<typename T>
	size_t numPatternsWithType(const std::vector<Pattern_ptr>& patterns) {
		size_t num = 0;
		BOOST_FOREACH(Pattern_ptr pattern, patterns) {
			if (boost::dynamic_pointer_cast<T>(pattern))
				++num;
		}
		return num;
	}
}

Pattern_ptr CombinationPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {

	// First replace shortcuts in our subpattern list.
	for (size_t i = 0; i < _patternList.size(); ++i) {
		replaceShortcut<Pattern>(_patternList[i], refPatterns);
	}
	// Then convert this combination pattern to an appropriate subclass, 
	// based on the types of its subpatterns.
	size_t num_prop = numPatternsWithType<PropMatchingPattern>(_patternList);
	size_t num_event = numPatternsWithType<EventMentionMatchingPattern>(_patternList);
	size_t num_rel = numPatternsWithType<RelMentionMatchingPattern>(_patternList);
	size_t num_mention = numPatternsWithType<MentionMatchingPattern>(_patternList);
	size_t num_argument = numPatternsWithType<ArgumentMatchingPattern>(_patternList);
	size_t num_argval = numPatternsWithType<ArgumentValueMatchingPattern>(_patternList);
	size_t num_parsenode = numPatternsWithType<ParseNodeMatchingPattern>(_patternList);
	CombinationPattern_ptr thisPattern = shared_from_this()->castTo<CombinationPattern>();
	if (num_prop == _patternList.size()) {
		return boost::make_shared<PropCombinationPattern>(thisPattern);
	} else if (num_event == _patternList.size()) {
		return boost::make_shared<EventCombinationPattern>(thisPattern);
	} else if (num_rel == _patternList.size()) {
		return boost::make_shared<RelationCombinationPattern>(thisPattern);
	} else if (num_mention == _patternList.size()) {
		return boost::make_shared<MentionCombinationPattern>(thisPattern);
	} else if (num_argument == _patternList.size()) {
		return boost::make_shared<ArgumentCombinationPattern>(thisPattern);
	} else if (num_argval == _patternList.size()) {
		return boost::make_shared<ArgumentValueCombinationPattern>(thisPattern);
	} else if (num_parsenode == _patternList.size()) {
		return boost::make_shared<ParseNodeCombinationPattern>(thisPattern);
	} else {
		// Display a friendly error message.
		std::ostringstream err;
		err << "Member patterns for all-of/any-of/none-of must have the same type!\n";
		dump(err);
		err << "  Types for " << _patternList.size() << " member patterns: \n";
		err << "    " << num_prop << " PropMatchingPattern(s)\n";
		err << "    " << num_event << " EventMentionMatchingPattern(s)\n";
		err << "    " << num_rel << " RelMentionMatchingPattern(s)\n";
		err << "    " << num_mention << " MentionMatchingPattern(s)\n";
		err << "    " << num_argument << " ArgumentMatchingPattern(s)\n";
		err << "    " << num_argval << " ArgumentValueMatchingPattern(s)\n";
		err << "    " << num_parsenode << " ParseNodeMatchingPattern(s)\n";
		throw UnexpectedInputException("CombinationPattern::shortcutReplacement", err.str().c_str());
	}
}

CombinationPattern::CombinationPattern(CombinationPattern_ptr pattern)
: Pattern(pattern), _combination_type(pattern->_combination_type), _patternList(pattern->_patternList), 
  _is_greedy(pattern->_is_greedy), _patternStatusOverrides(pattern->_patternStatusOverrides)
{
}

bool CombinationPattern::hasFallThroughRoles() const {
	for (size_t n = 0; n < _patternList.size(); ++n) {
		if (_patternList[n]) {
			ArgumentPattern_ptr pat1 = boost::dynamic_pointer_cast<ArgumentPattern>(_patternList[n]);
			if (pat1 && pat1->hasFallThroughRoles())
				return true;
			CombinationPattern_ptr pat2 = boost::dynamic_pointer_cast<CombinationPattern>(_patternList[n]);
			if (pat2 && pat2->hasFallThroughRoles())
				return true;
		}
	}
	return false;
}

void CombinationPattern::getReturns(PatternReturnVecSeq & output) const {
	Pattern::getReturns(output);
	size_t n_patterns = _patternList.size();
	if (_combination_type == ANY_OF) {
		multiplyReturns(static_cast<int>(n_patterns), output);
	} else if (_combination_type == ALL_OF) {
		for (size_t n = 0; n < n_patterns; ++n) {
			if (_patternList[n])
				_patternList[n]->getReturns(output);
		}
	} else if (_combination_type == NONE_OF) {
		return; // don't keep any return values from child matches
	}
}

/**
 * Virtual method that returns a sequence of PatternReturnVec objects 
 * for a given top-level pattern. It is called only by CombinationPattern,
 * and only in the case of an any-of combination. It produces a cross-product
 * of the pattern returns from the any-of with the pattern returns from the
 * rest of the pattern. If output is nonempty when extendReturns is called,
 * the cross-product will be produced by combining the existing contents with
 * the output from the any-of. The cross-product will be used to fill output,
 * and the previous contents will be deleted. If output is empty when
 * extendReturns is called, output will be filled with a number of 
 * PatternReturnVec objects equal to the number of members of the any-of.
 * Note that a PatternReturnVec object corresponds to the set of args for a
 * single relation.
 * @see http://wiki.d4m.bbn.com/wiki/PatternReturn_Hierarchy for a description of the
 * PatternReturn hierarchy.
 *
 * @param sequence of PatternReturnVec objects for a given pattern
 *
 * @author afrankel@bbn.com
 * @date 2011.03.22
 **/
void CombinationPattern::multiplyReturns(int pattern_count, PatternReturnVecSeq & output) const {
    // Find the indices of the subpatterns of the top-level pattern that are non-null and have returns,
    // and store them in a vector of ints.
	std::vector< PatternReturnVecSeq > new_seqs(pattern_count);
	std::vector< int > patterns_w_returns;
	for (int i = 0; i < pattern_count; ++i) {
		if (_patternList[i]) {
			// These are "new" because they weren't previously contained in output.
			_patternList[i]->getReturns(new_seqs[i]); 
		}
		if (!new_seqs[i].empty()) {
			patterns_w_returns.push_back(i);
		}
	}

	size_t n_returns_size = patterns_w_returns.size();
	size_t output_orig_size = output.size();
	if (n_returns_size != 0) {
		if (output_orig_size == 0) {
            // There were no PatternReturns in *output when we started,
            // so simply push back one PatternReturnVec for each
            // subpattern that has a return.
			BOOST_FOREACH(int pattern_index, patterns_w_returns) {
				BOOST_FOREACH(PatternReturnVec new_vec, new_seqs[pattern_index]) {
					output.push_back(new_vec);
				}
			}
		}
		else {
            // There were already PatternReturnVec objs in output when we started.
            // For each subpattern that has a return, step through each
            // existing PatternReturnVec (in output), create a new PatternReturnVec                  
			// from it, append the newly retrieved PatternReturns to it, and
            // push this PatternReturnVec onto the back of output.
            // For now, we'll keep the old PatternReturnVec objects at the front of output.
			PatternReturnVecSeq::iterator iter = output.begin();
			for (size_t i = 0; i < output_orig_size; ++i) {
				BOOST_FOREACH(int pattern_index, patterns_w_returns) {
					BOOST_FOREACH(PatternReturnVec new_vec, new_seqs[pattern_index]) {
						PatternReturnVec v0(*iter);
						BOOST_FOREACH(PatternReturn_ptr new_ptr, new_vec) {
							v0.push_back(new_ptr);
						}
						output.push_back(v0);
					}
				}
				++iter;
			}
            // Now delete the old PatternReturnVec objects. Since the underlying data type 
			// (PatternReturnVecSeq) is a list rather than a vector, this step does not require copying
			// or reallocation.
			for (size_t i = 0; i < output_orig_size; ++i) {
				output.pop_front();
			}
		}
	}
	return;
}

std::vector<PatternFeatureSet_ptr> CombinationPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out) {
	throw InternalInconsistencyException("CombinationPattern::multiMatchesSentence",
		"Do not call any match methods until after calling replaceShortcuts()!");
}

PatternFeatureSet_ptr CombinationPattern::matchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out) {
	std::vector<PatternFeatureSet_ptr> matches = multiMatchesSentence(patternMatcher, sTheory, out);
	if (!matches.empty()) {
		PatternFeatureSet_ptr allSentenceMatches = makeEmptyFeatureSet();
		std::vector<float> scores;
		BOOST_FOREACH(PatternFeatureSet_ptr sfs, matches) {
			allSentenceMatches->addFeatures(sfs);
			scores.push_back(sfs->getScore());
		}
		allSentenceMatches->setScore(ScoringFactory::scoreMax(scores, Pattern::UNSPECIFIED_SCORE));
		return allSentenceMatches;
	} else {
		return PatternFeatureSet_ptr();
	}
}

namespace {
	const char* combinationTypeName(CombinationPattern::CombinationType t) {
		if (t == CombinationPattern::ALL_OF) return "ALL_OF";
		else if (t == CombinationPattern::ANY_OF) return "ANY_OF";
		else if (t == CombinationPattern::NONE_OF) return "NONE_OF";
		else return "UNKNOWN";
	}
}

// These seem identical to me for 1 type and 2 types, so I added one for 3 types...

template<typename Matcher, typename ARG1_TYPE>
PatternFeatureSet_ptr CombinationPattern::matchesWith(PatternMatcher_ptr patternMatcher, ARG1_TYPE arg1) {
	std::vector<float> scores;
	PatternFeatureSet_ptr allPatterns = makeEmptyFeatureSet();
	for (size_t i = 0; i < _patternList.size(); i++) {
		PatternFeatureSet_ptr sfs = Matcher::match(_patternList[i], patternMatcher, arg1);
		if (sfs) { 
			if (SessionLogger::dbg_or_msg_enabled("BRANDY"))
				SessionLogger::dbg("BRANDY") << getDebugID() << " " << combinationTypeName(_combination_type)
					<< " member " << i << " of " << _patternList.size() << " has score " << sfs->getScore() << "\n";
			if (_combination_type == NONE_OF)
				return PatternFeatureSet_ptr();
			allPatterns->addFeatures(sfs);
			scores.push_back(sfs->getScore());
			if (_combination_type==ANY_OF && _is_greedy) break;
		} else {
			if (SessionLogger::dbg_or_msg_enabled("BRANDY"))
				SessionLogger::dbg("BRANDY") << getDebugID() << " " << combinationTypeName(_combination_type)
					<< " member " << i << " of " << _patternList.size() << " had no match.\n";
			if (_combination_type == ALL_OF)
				return PatternFeatureSet_ptr();
		}
	}
	if (scores.empty() && (_combination_type!=NONE_OF))
		return PatternFeatureSet_ptr();
	allPatterns->setScore(_scoringFunction(scores, _score));
	return allPatterns;
}

template<typename Matcher, typename ARG1_TYPE, typename ARG2_TYPE>
PatternFeatureSet_ptr CombinationPattern::matchesWith(PatternMatcher_ptr patternMatcher, ARG1_TYPE arg1, ARG2_TYPE arg2) {
	std::vector<float> scores;
	PatternFeatureSet_ptr allPatterns = makeEmptyFeatureSet();
	for (size_t i = 0; i < _patternList.size(); i++) {
		PatternFeatureSet_ptr sfs = Matcher::match(_patternList[i], patternMatcher, arg1, arg2);
		if (sfs) {
			if (SessionLogger::dbg_or_msg_enabled("BRANDY"))
				SessionLogger::dbg("BRANDY") << getDebugID() << " " << combinationTypeName(_combination_type)
					<< " member " << i << " of " << _patternList.size() << " (" << _patternList[i]->getDebugID() << ")"
					<< " has score " << sfs->getScore() << "\n";
			if (_combination_type == NONE_OF)
				return PatternFeatureSet_ptr();
			allPatterns->addFeatures(sfs);
			scores.push_back(sfs->getScore());
			if (_combination_type==ANY_OF && _is_greedy) { 
				break;
			}
		} else {
			if (SessionLogger::dbg_or_msg_enabled("BRANDY"))
				SessionLogger::dbg("BRANDY") << getDebugID() << " " << combinationTypeName(_combination_type)
					<< " member " << i << " of " << _patternList.size() << " (" << _patternList[i]->getDebugID() << ") has no match.\n";
			if (_combination_type == ALL_OF)
				return PatternFeatureSet_ptr();
		}
	}
	if (scores.empty() && (_combination_type!=NONE_OF))
		return PatternFeatureSet_ptr();
	allPatterns->setScore(_scoringFunction(scores, _score));
	return allPatterns;
}

template<typename Matcher, typename ARG1_TYPE, typename ARG2_TYPE, typename ARG3_TYPE>
PatternFeatureSet_ptr CombinationPattern::matchesWith(PatternMatcher_ptr patternMatcher, ARG1_TYPE arg1, ARG2_TYPE arg2, ARG3_TYPE arg3) {
	std::vector<float> scores;
	PatternFeatureSet_ptr allPatterns = makeEmptyFeatureSet();
	for (size_t i = 0; i < _patternList.size(); i++) {
		PatternFeatureSet_ptr sfs = Matcher::match(_patternList[i], patternMatcher, arg1, arg2, arg3);
		if (sfs) {
			if (SessionLogger::dbg_or_msg_enabled("BRANDY"))
				SessionLogger::dbg("BRANDY") << getDebugID() << " " << combinationTypeName(_combination_type)
					<< " member " << i << " of " << _patternList.size() << " (" << _patternList[i]->getDebugID() << ")"
					<< " has score " << sfs->getScore() << "\n";
			if (_combination_type == NONE_OF)
				return PatternFeatureSet_ptr();
			allPatterns->addFeatures(sfs);
			scores.push_back(sfs->getScore());
			if (_combination_type==ANY_OF && _is_greedy) { 
				break;
			}
		} else {
			if (SessionLogger::dbg_or_msg_enabled("BRANDY"))
				SessionLogger::dbg("BRANDY") << getDebugID() << " " << combinationTypeName(_combination_type)
					<< " member " << i << " of " << _patternList.size() << " (" << _patternList[i]->getDebugID() << ") has no match.\n";
			if (_combination_type == ALL_OF)
				return PatternFeatureSet_ptr();
		}
	}
	if (scores.empty() && (_combination_type!=NONE_OF))
		return PatternFeatureSet_ptr();
	allPatterns->setScore(_scoringFunction(scores, _score));
	return allPatterns;
}

void CombinationPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "CombinationPattern [" << combinationTypeName(_combination_type) << "]: ";
	if (!getID().is_null()) out << getID() << " ";
	out << std::endl;
	for (size_t i = 0; i < _patternList.size(); i++) {
		_patternList[i]->dump(out, indent+2);
	}
}

/*********************************************************************
 * Matcher Classes for CombinationPattern::matchWith() template
 *********************************************************************/
namespace {
	struct ArgMatcher {
		static PatternFeatureSet_ptr match(Pattern_ptr pattern, PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg)
		{ return pattern->castTo<ArgumentMatchingPattern>()->matchesArgument(patternMatcher, sent_no, arg, true); }
	};
	struct ArgMatcherNoFT {
		static PatternFeatureSet_ptr match(Pattern_ptr pattern, PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg)
		{ return pattern->castTo<ArgumentMatchingPattern>()->matchesArgument(patternMatcher, sent_no, arg, false); }
	};
	struct ArgValueMatcher {
		static PatternFeatureSet_ptr match(Pattern_ptr pattern, PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, PropStatusManager_ptr statusOverrides)
		{ return pattern->castTo<ArgumentValueMatchingPattern>()->matchesArgumentValue(patternMatcher, sent_no, arg, true, statusOverrides); }
	};
	struct ArgValueMatcherNoFT {
		static PatternFeatureSet_ptr match(Pattern_ptr pattern, PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, PropStatusManager_ptr statusOverrides)
		{ return pattern->castTo<ArgumentValueMatchingPattern>()->matchesArgumentValue(patternMatcher, sent_no, arg, false, statusOverrides); }
	};
	struct EventMentionMatcher {
		static PatternFeatureSet_ptr match(Pattern_ptr pattern, PatternMatcher_ptr patternMatcher, int sent_no, const EventMention *em)
		{ return pattern->castTo<EventMentionMatchingPattern>()->matchesEventMention(patternMatcher, sent_no, em); }
	};
	struct MentionMatcher {
		static PatternFeatureSet_ptr match(Pattern_ptr pattern, PatternMatcher_ptr patternMatcher, const Mention *mention)
		{ return pattern->castTo<MentionMatchingPattern>()->matchesMention(patternMatcher, mention, true); }
	};
	struct MentionMatcherNoFT {
		static PatternFeatureSet_ptr match(Pattern_ptr pattern, PatternMatcher_ptr patternMatcher, const Mention *mention)
		{ return pattern->castTo<MentionMatchingPattern>()->matchesMention(patternMatcher, mention, false); }
	};
	struct PropMatcher {
		static PatternFeatureSet_ptr match(Pattern_ptr pattern, PatternMatcher_ptr patternMatcher, int sent_no, const Proposition *prop, PropStatusManager_ptr statusOverrides)
		{ return pattern->castTo<PropMatchingPattern>()->matchesProp(patternMatcher, sent_no, prop, true, statusOverrides); }
	};
	struct PropMatcherNoFT {
		static PatternFeatureSet_ptr match(Pattern_ptr pattern, PatternMatcher_ptr patternMatcher, int sent_no, const Proposition *prop, PropStatusManager_ptr statusOverrides)
		{ return pattern->castTo<PropMatchingPattern>()->matchesProp(patternMatcher, sent_no, prop, false, statusOverrides); }
	};
	struct RelMentionMatcher {
		static PatternFeatureSet_ptr match(Pattern_ptr pattern, PatternMatcher_ptr patternMatcher, int sent_no, const RelMention *rm)
		{ return pattern->castTo<RelMentionMatchingPattern>()->matchesRelMention(patternMatcher, sent_no, rm); }
	};
	struct ParseNodeMatcher {
		static PatternFeatureSet_ptr match(Pattern_ptr pattern, PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node)
		{ return pattern->castTo<ParseNodeMatchingPattern>()->matchesParseNode(patternMatcher, sent_no, node); }
	};
	struct MentionAndRoleMatcher {
		static PatternFeatureSet_ptr match(Pattern_ptr pattern, PatternMatcher_ptr patternMatcher, Symbol role, const Mention *mention)
		{ return pattern->castTo<MentionAndRoleMatchingPattern>()->matchesMentionAndRole(patternMatcher, role, mention, true); }
	};
	struct MentionAndRoleMatcherNoFT {
		static PatternFeatureSet_ptr match(Pattern_ptr pattern, PatternMatcher_ptr patternMatcher, Symbol role, const Mention *mention)
		{ return pattern->castTo<MentionAndRoleMatchingPattern>()->matchesMentionAndRole(patternMatcher, role, mention, false); }
	};
	struct ValueMentionAndRoleMatcher {
		static PatternFeatureSet_ptr match(Pattern_ptr pattern, PatternMatcher_ptr patternMatcher, Symbol role, const ValueMention *valueMention)
		{ return pattern->castTo<ValueMentionAndRoleMatchingPattern>()->matchesValueMentionAndRole(patternMatcher, role, valueMention); }
	};
}

/*********************************************************************
 * PropCombinationPattern
 *********************************************************************/

std::vector<PatternFeatureSet_ptr> PropCombinationPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out) {
	std::vector<PatternFeatureSet_ptr> return_vector;

	int n_items = sTheory->getPropositionSet()->getNPropositions();
	int sent_no = sTheory->getTokenSequence()->getSentenceNumber();	
	for (int i = 0; i < n_items; i++) {
		const Proposition *prop = sTheory->getPropositionSet()->getProposition(i);
		if (PatternFeatureSet_ptr sfs = matchesProp(patternMatcher, sent_no, prop, false, _patternStatusOverrides)) // don't fall through children; they'll get their own chance
			return_vector.push_back(sfs);
	} 
	return return_vector;
}

PatternFeatureSet_ptr PropCombinationPattern::matchesProp(PatternMatcher_ptr patternMatcher, int sent_no, const Proposition *prop, bool fall_through_children, PropStatusManager_ptr statusOverrides) {

	// Use the highest-up override we can
	// If this is coming from the same pattern (e.g. for a fall-through), then
	//   these are the same, so it's all good
	PropStatusManager_ptr statusToUse = statusOverrides;
	if (!statusOverrides)
		statusToUse = _patternStatusOverrides;

	if (prop->getPredType() == Proposition::COMP_PRED || prop->getPredType() == Proposition::SET_PRED) {

		// First, does this prop match on its own, with no fall-through?
		// This could happen if one or more of the patterns being matched is a sprop or cprop
		PatternFeatureSet_ptr sfs = matchesWith<PropMatcherNoFT>(patternMatcher, sent_no, prop, statusToUse);
		if (sfs || !fall_through_children)
			return sfs;

		// Now, match against each child individually
		PatternFeatureSet_ptr allFeatures = boost::make_shared<PatternFeatureSet>();
		std::vector<float> scores;
		bool matched = false;
		for (int mem = 0; mem < prop->getNArgs(); mem++) {
			const Argument* arg = prop->getArg(mem);
			if (arg->getRoleSym() == Argument::MEMBER_ROLE) {
				const Proposition *memberProp = 0;
				if (arg->getType() == Argument::MENTION_ARG) {
					memberProp = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getPropositionSet()->getDefinition(arg->getMentionIndex());
				} else if (arg->getType() == Argument::PROPOSITION_ARG) {
					memberProp = arg->getProposition();
				}
				if (memberProp) {
					PatternFeatureSet_ptr sfs = matchesProp(patternMatcher, sent_no, memberProp, fall_through_children, statusToUse);
					if (sfs) {
						allFeatures->addFeatures(sfs);
						scores.push_back(sfs->getScore());
						matched = true;
					}
				}
			}
		}
		if (matched) {
			allFeatures->setScore(ScoringFactory::scoreMax(scores, Pattern::UNSPECIFIED_SCORE));
			return allFeatures;
		}	

	} else {

		// Just try and match this as-is
		PatternFeatureSet_ptr result;
		if (_combination_type == ALL_OF || !fall_through_children)
			result = matchesWith<PropMatcherNoFT>(patternMatcher, sent_no, prop, statusToUse);
		else result = matchesWith<PropMatcher>(patternMatcher, sent_no, prop, statusToUse);

		if (result)
			return result;

		// But if we can't, let's also try matching the partitive child of this prop
		if (fall_through_children && prop->getNArgs() > 0) {
			Argument *arg = prop->getArg(0);
			if (arg->getRoleSym() == Argument::REF_ROLE && arg->getType() == Argument::MENTION_ARG) {
				const Mention *refMention = arg->getMention(patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getMentionSet());
				if (refMention->getMentionType() == Mention::PART && refMention->getChild() != 0) {
					const Proposition *partitiveProp = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getPropositionSet()->getDefinition(refMention->getChild()->getIndex());					
					if (partitiveProp)
						return matchesProp(patternMatcher, sent_no, partitiveProp, fall_through_children, statusToUse);
				}
			}
		}
	}

	return PatternFeatureSet_ptr();
}

PatternFeatureSet_ptr PropCombinationPattern::matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides) {
	// punt to matchesProp() function since this handles fall-through correctly

	// Don't fall through if this is a <ref> argument (this is an implicit all-of with a proposition, typically)
	if (arg->getRoleSym() == Argument::REF_ROLE)
		fall_through_children = false;

	const Proposition *propToMatch = 0;
	if (arg->getType() == Argument::PROPOSITION_ARG) {
		propToMatch = arg->getProposition();
	} else if (arg->getType() == Argument::MENTION_ARG) {
		propToMatch = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getPropositionSet()->getDefinition(arg->getMentionIndex());
	}
	if (propToMatch)
		return matchesProp(patternMatcher, sent_no, propToMatch, fall_through_children, statusOverrides);

	return PatternFeatureSet_ptr();
}

bool PropCombinationPattern::allowFallThroughToChildren() const {
	for (size_t n = 0; n < _patternList.size(); ++n) {
		if (_patternList[n]) {
			PropMatchingPattern_ptr pat1 = boost::dynamic_pointer_cast<PropMatchingPattern>(_patternList[n]);
			if (pat1 && !pat1->allowFallThroughToChildren())
				return false;
		}
	}
	return true;
}

/*********************************************************************
 * EventCombinationPattern
 *********************************************************************/

std::vector<PatternFeatureSet_ptr> EventCombinationPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out) {
	std::vector<PatternFeatureSet_ptr> return_vector;
	int n_items = sTheory->getEventMentionSet()->getNEventMentions();
	int sent_no = sTheory->getTokenSequence()->getSentenceNumber();	
	for (int i = 0; i < n_items; i++) {
		const EventMention *em = sTheory->getEventMentionSet()->getEventMention(i);
		if (PatternFeatureSet_ptr sfs = matchesEventMention(patternMatcher, sent_no, em))
			return_vector.push_back(sfs);
	}
	return return_vector;
}

PatternFeatureSet_ptr EventCombinationPattern::matchesEventMention(PatternMatcher_ptr patternMatcher, int sent_no, const EventMention *em) {
	return matchesWith<EventMentionMatcher>(patternMatcher, sent_no, em);
}
PatternFeatureSet_ptr EventCombinationPattern::matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides) {
	if (_combination_type == ALL_OF || !fall_through_children || arg->getRoleSym() == Argument::REF_ROLE)
		return matchesWith<ArgValueMatcherNoFT>(patternMatcher, sent_no, arg, statusOverrides);
	else return matchesWith<ArgValueMatcher>(patternMatcher, sent_no, arg, statusOverrides);
}
PatternFeatureSet_ptr EventCombinationPattern::matchesMention(PatternMatcher_ptr patternMatcher, const Mention *mention, bool fall_through_children) {
	if (_combination_type == ALL_OF || !fall_through_children)
		return matchesWith<MentionMatcherNoFT>(patternMatcher, mention);
	else return matchesWith<MentionMatcher>(patternMatcher, mention);
}

bool EventCombinationPattern::allowFallThroughToChildren() const { return true; } 

/*********************************************************************
 * RelationCombinationPattern
 *********************************************************************/

std::vector<PatternFeatureSet_ptr> RelationCombinationPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out) {
	std::vector<PatternFeatureSet_ptr> return_vector;
	int n_items = sTheory->getRelMentionSet()->getNRelMentions();
	int sent_no = sTheory->getTokenSequence()->getSentenceNumber();	
	for (int i = 0; i < n_items; i++) {
		const RelMention *rm = sTheory->getRelMentionSet()->getRelMention(i);
		if (PatternFeatureSet_ptr sfs = matchesRelMention(patternMatcher, sent_no, rm))
			return_vector.push_back(sfs);
	}
	return return_vector;
}

PatternFeatureSet_ptr RelationCombinationPattern::matchesRelMention(PatternMatcher_ptr patternMatcher, int sent_no, const RelMention *rm) {
	return matchesWith<RelMentionMatcher>(patternMatcher, sent_no, rm);
}

/*********************************************************************
 * MentionCombinationPattern
 *********************************************************************/

std::vector<PatternFeatureSet_ptr> MentionCombinationPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out) {
	std::vector<PatternFeatureSet_ptr> return_vector;
	int n_items = sTheory->getMentionSet()->getNMentions();
	int sent_no = sTheory->getTokenSequence()->getSentenceNumber();	
	for (int i = 0; i < n_items; i++) {
		const Mention *mention = sTheory->getMentionSet()->getMention(i);
		if (PatternFeatureSet_ptr sfs = matchesMention(patternMatcher, mention, false)) // don't fall through children; they'll get their own chance
			return_vector.push_back(sfs);
	}
	return return_vector;
}

PatternFeatureSet_ptr MentionCombinationPattern::matchesMention(PatternMatcher_ptr patternMatcher, const Mention *mention, bool fall_through_children) {

	if (!allowFallThroughToChildren())
		fall_through_children = false;

	if (mention->getMentionType() == Mention::LIST || mention->getMentionType() == Mention::PART) {

		// First see if this matches without fall-through
		PatternFeatureSet_ptr sfs = matchesWith<MentionMatcherNoFT>(patternMatcher, mention);
		if (sfs)
			return sfs;

		if (fall_through_children) {
		
			// Now, match against each child individually
			PatternFeatureSet_ptr allFeatures = boost::make_shared<PatternFeatureSet>();
			std::vector<float> scores;
			const Mention *child = mention->getChild();
			bool matched = false;
			while (child != 0) {
				if (PatternFeatureSet_ptr sfs = matchesMention(patternMatcher, child, fall_through_children)) {
					allFeatures->addFeatures(sfs);
					scores.push_back(sfs->getScore());
					matched = true;
				}
				child = child->getNext();
			}
			if (matched) {
				allFeatures->setScore(ScoringFactory::scoreMax(scores, Pattern::UNSPECIFIED_SCORE));
				return allFeatures;
			}			

		}

	} else {
		// Don't allow fall-through if this is an ALL_OF or if the caller doesn't want us to fall through		
		if (_combination_type == ALL_OF || !fall_through_children)
			return matchesWith<MentionMatcherNoFT>(patternMatcher, mention);
		else return matchesWith<MentionMatcher>(patternMatcher, mention);
	}

	return PatternFeatureSet_ptr();
}
PatternFeatureSet_ptr MentionCombinationPattern::matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides) {
	// punt to matchesMention() function since this handles fall-through correctly

	// Don't fall through if this is a <ref> argument (this is an implicit all-of with a proposition, typically)
	if (arg->getRoleSym() == Argument::REF_ROLE)
		fall_through_children = false;

	if (arg->getType() == Argument::MENTION_ARG) {
		MentionSet *mSet = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getMentionSet();
		const Mention *mention = arg->getMention(mSet);
		return matchesMention(patternMatcher, mention, fall_through_children);
	} 
	return PatternFeatureSet_ptr();
}
PatternFeatureSet_ptr MentionCombinationPattern::matchesParseNode(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node) {
	return matchesWith<ParseNodeMatcher>(patternMatcher, sent_no, node);
}

bool MentionCombinationPattern::allowFallThroughToChildren() const {
	for (size_t n = 0; n < _patternList.size(); ++n) {
		if (_patternList[n]) {
			MentionMatchingPattern_ptr pat1 = boost::dynamic_pointer_cast<MentionMatchingPattern>(_patternList[n]);
			if (pat1 && !pat1->allowFallThroughToChildren())
				return false;
		}
	}
	return true;
}

/*********************************************************************
 * ArgumentCombinationPattern
 *********************************************************************/

std::vector<PatternFeatureSet_ptr> ArgumentCombinationPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out) {
	std::stringstream msg;
	msg << "ArgumentCombinationPattern may not be matched against a sentence: ";
	msg << toDebugString(0) << "\n";
	throw UnexpectedInputException("ArgumentCombinationPattern::multiMatchesSentence", msg.str().c_str());
}
PatternFeatureSet_ptr ArgumentCombinationPattern::matchesArgument(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children) {
	if (_combination_type == ALL_OF || !fall_through_children || arg->getRoleSym() == Argument::REF_ROLE)
		return matchesWith<ArgMatcherNoFT>(patternMatcher, sent_no, arg);
	else return matchesWith<ArgMatcher>(patternMatcher, sent_no, arg);
}
PatternFeatureSet_ptr ArgumentCombinationPattern::matchesMentionAndRole(PatternMatcher_ptr patternMatcher, Symbol role, const Mention *mention, bool fall_through_children) {

	// Don't fall through if this is a <ref> argument (this is an implicit all-of with a proposition, typically)
	if (role == Argument::REF_ROLE || !allowFallThroughToChildren())
		fall_through_children = false;

	if (mention->getMentionType() == Mention::LIST || mention->getMentionType() == Mention::PART) {

		// First see if this matches without fall-through
		PatternFeatureSet_ptr sfs = matchesWith<MentionAndRoleMatcherNoFT>(patternMatcher, role, mention);
		if (sfs)
			return sfs;

		if (fall_through_children) {
			// Now, match against each child individually
			PatternFeatureSet_ptr allFeatures = boost::make_shared<PatternFeatureSet>();
			std::vector<float> scores;
			const Mention *child = mention->getChild();
			bool matched = false;
			while (child != 0) {
				if (PatternFeatureSet_ptr sfs = matchesMentionAndRole(patternMatcher, role, child, fall_through_children)) {
					allFeatures->addFeatures(sfs);
					scores.push_back(sfs->getScore());
					matched = true;
				}
				child = child->getNext();
			}
			if (matched) {
				allFeatures->setScore(ScoringFactory::scoreMax(scores, Pattern::UNSPECIFIED_SCORE));
				return allFeatures;
			}			
		}

	} else {
		// Don't allow fall-through if this is an ALL_OF or if the caller doesn't want us to fall through		
		if (_combination_type == ALL_OF || !fall_through_children)
			return matchesWith<MentionAndRoleMatcherNoFT>(patternMatcher, role, mention);
		else return matchesWith<MentionAndRoleMatcher>(patternMatcher, role, mention);
	}

	return PatternFeatureSet_ptr();
}
PatternFeatureSet_ptr ArgumentCombinationPattern::matchesValueMentionAndRole(PatternMatcher_ptr patternMatcher, Symbol role, const ValueMention *valueMention) {
	return matchesWith<ValueMentionAndRoleMatcher>(patternMatcher, role, valueMention);
}

bool ArgumentCombinationPattern::allowFallThroughToChildren() const {
	for (size_t n = 0; n < _patternList.size(); ++n) {
		if (_patternList[n]) {
			ArgumentMatchingPattern_ptr pat1 = boost::dynamic_pointer_cast<ArgumentMatchingPattern>(_patternList[n]);
			if (pat1 && !pat1->allowFallThroughToChildren())
				return false;
		}
	}
	return true;
}

/*********************************************************************
 * ArgumentValueCombinationPattern
 *********************************************************************/

std::vector<PatternFeatureSet_ptr> ArgumentValueCombinationPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out) {
	// Run any child combination patterns over the whole sentence
	std::vector< std::vector<PatternFeatureSet_ptr> > submatches;
	submatches.reserve(_patternList.size());
	BOOST_FOREACH(Pattern_ptr pattern, _patternList) {
		boost::shared_ptr<ArgumentValueCombinationPattern> argValueComboPattern = boost::dynamic_pointer_cast<ArgumentValueCombinationPattern>(pattern);
		if (argValueComboPattern.get())
			submatches.push_back(argValueComboPattern->multiMatchesSentence(patternMatcher, sTheory, out));
		else
			submatches.push_back(std::vector<PatternFeatureSet_ptr>());
	}

	// Run patterns over mentions
	std::vector<PatternFeatureSet_ptr> matches;
	MentionSet* mentions = sTheory->getMentionSet();
	if (mentions != NULL) {
		for (int m = 0; m < mentions->getNMentions(); m++) {
			// Get the nprop for this mention, if any
			Proposition* definitionProposition = NULL;
			PropositionSet* props = sTheory->getPropositionSet();
			if (props != NULL)
				definitionProposition = props->getDefinition(m);

			// Get pattern matches on this mention
			std::vector<PatternFeatureSet_ptr> mentionMatches;
			for (size_t p = 0; p < _patternList.size(); p++) {
				// Determine the type of the subpattern
				Pattern_ptr pattern = _patternList[p];
				PatternFeatureSet_ptr match;
				MentionPattern_ptr mentionPattern = boost::dynamic_pointer_cast<MentionPattern>(pattern);
				boost::shared_ptr<MentionCombinationPattern> mentionComboPattern = boost::dynamic_pointer_cast<MentionCombinationPattern>(pattern);
				PropPattern_ptr propPattern = boost::dynamic_pointer_cast<PropPattern>(pattern);
				boost::shared_ptr<PropCombinationPattern> propComboPattern = boost::dynamic_pointer_cast<PropCombinationPattern>(pattern);
				boost::shared_ptr<ArgumentValueCombinationPattern> argValueComboPattern = boost::dynamic_pointer_cast<ArgumentValueCombinationPattern>(pattern);
				if (mentionPattern.get()) {
					match = mentionPattern->matchesMention(patternMatcher, mentions->getMention(m), false);
				} else if (mentionComboPattern.get()) {
					match = mentionComboPattern->matchesMention(patternMatcher, mentions->getMention(m), false);
				} else if (propPattern.get()) {
					if (definitionProposition != NULL) {
						match = propPattern->matchesProp(patternMatcher, sTheory->getSentNumber(), definitionProposition, false, PropStatusManager_ptr());
					}
				} else if (propComboPattern.get()) {
					if (definitionProposition != NULL) {
						match = propComboPattern->matchesProp(patternMatcher, sTheory->getSentNumber(), definitionProposition, false, PropStatusManager_ptr());
					}
				} else if (argValueComboPattern.get()) {
					// If this combo pattern matched the sentence, see if it matched this mention or prop and pass it up
					std::vector<PatternFeatureSet_ptr> comboMatches = submatches[p];
					for (size_t c = 0; c < comboMatches.size(); c++) {
						PatternFeatureSet_ptr comboMatch = comboMatches[c];
						bool done = false;
						for (size_t f = 0; f < comboMatch->getNFeatures(); f++) {
							MentionPFeature_ptr mentionFeature = boost::dynamic_pointer_cast<MentionPFeature>(comboMatch->getFeature(f));
							PropPFeature_ptr propFeature = boost::dynamic_pointer_cast<PropPFeature>(comboMatch->getFeature(f));
							if (mentionFeature.get()) {
								if (mentionFeature->getMention()->getIndex() == m) {
									done = true;
								}
							} else if (propFeature.get()) {
								if (definitionProposition != NULL && propFeature->getProp()->getIndex() == definitionProposition->getIndex()) {
									done = true;
								}
							}
							if (done)
								break;
						}
						if (done) {
							match = comboMatch;
							break;
						}
					}
				} else {
					// Not a valid subpattern in this context
					std::stringstream msg;
					msg << "an all-of ArgumentValueCombinationPattern matching a sentence can only contain MentionPatterns, PropPatterns, or combinations thereof: ";
					msg << toDebugString(0) << "\n";
					throw UnexpectedInputException("ArgumentValueCombinationPattern::multiMatchesSentence", msg.str().c_str());
				}
				if (match.get())
					mentionMatches.push_back(match);
			}

			// Make sure that all subpatterns produced a match
			if (mentionMatches.size() == _patternList.size() && _combination_type == ALL_OF) {
				// Combine the matches
				PatternFeatureSet_ptr combinedMatch = makeEmptyFeatureSet();
				BOOST_FOREACH(PatternFeatureSet_ptr mentionMatch, mentionMatches) {
					combinedMatch->addFeatures(mentionMatch);
				}
				matches.push_back(combinedMatch);
			} else if (_combination_type == ANY_OF) {
				// Just use the matches
				matches.insert(matches.end(), mentionMatches.begin(), mentionMatches.end());
			}
		}
	}

	// Done
	return matches;
}

PatternFeatureSet_ptr ArgumentValueCombinationPattern::matchesChildren(PatternMatcher_ptr patternMatcher, int sent_no, 
																	   const Proposition *compoundProp, bool fall_through_children, 
																	   Symbol role, PropStatusManager_ptr statusOverridesToUse) 
{
	if (compoundProp == 0)
		return PatternFeatureSet_ptr();

	PatternFeatureSet_ptr allFeatures = boost::make_shared<PatternFeatureSet>();
	std::vector<float> scores;
	bool matched = false;

	for (int mem = 0; mem < compoundProp->getNArgs(); mem++) {
		const Argument* childArg = compoundProp->getArg(mem);
		if (childArg->getRoleSym() == role) {
			bool has_cycle = compoundProp->hasCycle(patternMatcher->getDocTheory()->getSentenceTheory(sent_no), childArg);
			PatternFeatureSet_ptr sfs = matchesArgumentValue(patternMatcher, sent_no, childArg, fall_through_children && !has_cycle, statusOverridesToUse);
			if (sfs) {
				allFeatures->addFeatures(sfs);
				scores.push_back(sfs->getScore());
				matched = true;
			}
		}
	}
	if (matched) {
		allFeatures->setScore(ScoringFactory::scoreMax(scores, Pattern::UNSPECIFIED_SCORE));
		return allFeatures;
	}	
	return PatternFeatureSet_ptr();
}

PatternFeatureSet_ptr ArgumentValueCombinationPattern::matchesArgumentValue(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children, PropStatusManager_ptr statusOverrides) {

	// Use the highest-up override we can
	// If this is coming from the same pattern (e.g. for a fall-through), then
	//   these are the same, so it's all good
	PropStatusManager_ptr statusToUse = statusOverrides;
	if (!statusOverrides)
		statusToUse = _patternStatusOverrides;

	// Don't fall through if this is a <ref> argument (this is an implicit all-of with a proposition, typically)
	if (arg->getRoleSym() == Argument::REF_ROLE || !allowFallThroughToChildren())
		fall_through_children = false;

	if (arg->getType() == Argument::PROPOSITION_ARG) {
		const Proposition *prop = arg->getProposition();

		if (prop->getPredType() == Proposition::COMP_PRED || prop->getPredType() == Proposition::SET_PRED) {

			// First, does this prop match on its own, with no fall-through?
			// This could happen if one or more of the patterns being matched is a sprop or cprop
			PatternFeatureSet_ptr sfs = matchesWith<ArgValueMatcherNoFT>(patternMatcher, sent_no, arg, statusToUse);
			if (sfs || !fall_through_children)
				return sfs;

			// If not, match against each child individually & return combined result
			return matchesChildren(patternMatcher, sent_no, prop, fall_through_children, Argument::MEMBER_ROLE, statusToUse);

		} else {
			// Just try and match this as-is
			PatternFeatureSet_ptr result;
			if (_combination_type == ALL_OF || !fall_through_children)
				result = matchesWith<ArgValueMatcherNoFT>(patternMatcher, sent_no, arg, statusToUse);
			else result = matchesWith<ArgValueMatcher>(patternMatcher, sent_no, arg, statusToUse);

			if (result)
				return result;

			// But if we can't, let's also try matching the partitive child of this prop
			if (fall_through_children) {
				if (prop->getPredType() == Proposition::NOUN_PRED && prop->getNArgs() > 0) {
					Argument *arg = prop->getArg(0);
					if (arg->getRoleSym() == Argument::REF_ROLE && arg->getType() == Argument::MENTION_ARG) {
						const Mention *refMention = arg->getMention(patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getMentionSet());
						if (refMention->getMentionType() == Mention::PART && refMention->getChild() != 0) {
							return matchesChildren(patternMatcher, sent_no, prop, fall_through_children, Symbol(L"of"), statusToUse);
						}
					}
				}
			}
		}

	} else if (arg->getType() == Argument::MENTION_ARG) {
		
		MentionSet *mSet = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getMentionSet();
		const Mention *mention = arg->getMention(mSet);
		
		if (mention->getMentionType() == Mention::LIST || mention->getMentionType() == Mention::PART) {

			// First see if this matches without fall-through
			PatternFeatureSet_ptr sfs = matchesWith<ArgValueMatcherNoFT>(patternMatcher, sent_no, arg, statusToUse);
			if (sfs || !fall_through_children)
				return sfs;

			// If not, match against each child individually & return combined result
			// We need these to be arguments, so go get the definitional prop and grab them that way
			const Proposition *defProp = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getPropositionSet()->getDefinition(arg->getMentionIndex());			
			if (mention->getMentionType() == Mention::LIST)
				return matchesChildren(patternMatcher, sent_no, defProp, fall_through_children, Argument::MEMBER_ROLE, statusToUse);
			else if (mention->getMentionType() == Mention::PART)
				return matchesChildren(patternMatcher, sent_no, defProp, fall_through_children, Symbol(L"of"), statusToUse);


		} else {
			// Don't allow fall-through if this is an ALL_OF or if the caller doesn't want us to fall through		
			if (_combination_type == ALL_OF || !fall_through_children)
				return matchesWith<ArgValueMatcherNoFT>(patternMatcher, sent_no, arg, statusToUse);
			else return matchesWith<ArgValueMatcher>(patternMatcher, sent_no, arg, statusToUse);
		}		
	}
	
	return PatternFeatureSet_ptr();
}

bool ArgumentValueCombinationPattern::allowFallThroughToChildren() const {
	for (size_t n = 0; n < _patternList.size(); ++n) {
		if (_patternList[n]) {
			ArgumentValueMatchingPattern_ptr pat1 = boost::dynamic_pointer_cast<ArgumentValueMatchingPattern>(_patternList[n]);
			if (pat1 && !pat1->allowFallThroughToChildren())
				return false;
		}
	}
	return true;
}


/*********************************************************************
 * ParseNodeCombinationPattern
 *********************************************************************/

std::vector<PatternFeatureSet_ptr> ParseNodeCombinationPattern::multiMatchesSentence(PatternMatcher_ptr patternMatcher, SentenceTheory *sTheory, UTF8OutputStream *out) {

	// Only matches nonterminal nodes, because that's the easy access function we have in SynNode

	std::vector<PatternFeatureSet_ptr> return_vector;
	int sent_no = sTheory->getTokenSequence()->getSentenceNumber();	
	const SynNode* node = sTheory->getPrimaryParse()->getRoot();

	std::vector<const SynNode*> nodes;
	node->getAllNonterminalNodes(nodes);

	BOOST_FOREACH(const SynNode* child, nodes) {
		if (PatternFeatureSet_ptr sfs = matchesParseNode(patternMatcher, sent_no, child))
			return_vector.push_back(sfs);
	}
	return return_vector;

}
PatternFeatureSet_ptr ParseNodeCombinationPattern::matchesParseNode(PatternMatcher_ptr patternMatcher, int sent_no, const SynNode *node) {
	return matchesWith<ParseNodeMatcher>(patternMatcher, sent_no, node);
}

/**
 * Redefinition of parent class's virtual method.
 *
 * @return First valid ID.
 *
 * @author afrankel@bbn.com
 * @date 2010.10.25
 **/
Symbol CombinationPattern::getFirstValidID() const {
	return getFirstValidIDForCompositePattern(_patternList);
}
