// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/ShortcutPattern.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/ArgumentPattern.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/GenericPFeature.h"
#include "Generic/patterns/features/TopLevelPFeature.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include <boost/lexical_cast.hpp>
#include <sstream>

const Symbol Pattern::scoreSym(L"score");
const Symbol Pattern::scoreGroupSym(L"score_group");
const Symbol Pattern::scoreFnSym(L"score-fn");
const Symbol Pattern::shortcutSym(L"shortcut");
const Symbol Pattern::idSym(L"id");
const Symbol Pattern::returnSym(L"return");
const Symbol Pattern::topLevelReturnSym(L"toplevel-return");
const Symbol Pattern::singleMatchOverrideSym(L"SINGLE_MATCH");
const float Pattern::UNSPECIFIED_SCORE = -1;
const int Pattern::UNSPECIFIED_SCORE_GROUP = -1;

#include "Generic/patterns/ArgumentPattern.h"
#include "Generic/patterns/CombinationPattern.h"
#include "Generic/patterns/DocumentPattern.h"
#include "Generic/patterns/EventPattern.h"
#include "Generic/patterns/IntersectionPattern.h"
#include "Generic/patterns/MentionPattern.h"
#include "Generic/patterns/NegationPattern.h"
#include "Generic/patterns/ParseNodePattern.h"
#include "Generic/patterns/PropPattern.h"
#include "Generic/patterns/QuotationPattern.h"
#include "Generic/patterns/RegexPattern.h"
#include "Generic/patterns/RelationPattern.h"
#include "Generic/patterns/TextPattern.h"
#include "Generic/patterns/TopicPattern.h"
#include "Generic/patterns/UnionPattern.h"
#include "Generic/patterns/ValueMentionPattern.h"

namespace { // Private symbols
	Symbol all_of_sym(L"all-of");
	Symbol any_of_sym(L"any-of");
	Symbol none_of_sym(L"none-of");
	Symbol argument_sym(L"argument");
	Symbol doc_sym(L"doc");
	Symbol event_sym(L"event");
	Symbol intersection_sym(L"intersection");
	Symbol mention_sym(L"mention");
	Symbol negation_sym(L"negation");
	Symbol parse_node_sym(L"parse-node");
	Symbol vprop_sym(L"vprop");
	Symbol nprop_sym(L"nprop");
	Symbol mprop_sym(L"mprop");
	Symbol cprop_sym(L"cprop");
	Symbol sprop_sym(L"sprop");
	Symbol anyprop_sym(L"anyprop");
	Symbol quotation_sym(L"quotation");
	Symbol regex_sym(L"regex");
	Symbol relation_sym(L"relation");
	Symbol text_sym(L"text");
	Symbol topic_sym(L"topic");
	Symbol union_sym(L"union");
	Symbol value_sym(L"value");
	Symbol language_sym(L"language");
	Symbol variant_sym(L"variation");
}

bool Pattern::debug_props = false;

Pattern::Pattern(): 
_score(UNSPECIFIED_SCORE), _score_group(UNSPECIFIED_SCORE_GROUP), _scoringFunction(ScoringFactory::scoreMax), _return(),
_toplevel_return(false), _shortcut(), _id(), _force_single_match_sentence(false)
{ 
	debug_props = ParamReader::isParamTrue("debug_props");
}

Pattern::Pattern(boost::shared_ptr<Pattern> pat): 
_score(pat->_score), _score_group(pat->_score_group), _scoringFunction(pat->_scoringFunction), _return(pat->_return),
_toplevel_return(pat->_toplevel_return), _shortcut(pat->_shortcut), _id(pat->_id), _force_single_match_sentence(pat->_force_single_match_sentence)
{ 
	debug_props = ParamReader::isParamTrue("debug_props");
}

void Pattern::initializeFromSexp(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets)
{
	//static unsigned int dbg_counter = 0;
	int nkids = sexp->getNumChildren();
	if (nkids < 1)
		throwError(sexp, "too few children in Pattern");

	for (int i = 1; i < nkids; i++) {
		Sexp *childSexp = sexp->getNthChild(i);
		if (childSexp->isAtom()) {
			//SessionLogger::dbg("is_atom_0") << dbg_counter++ << " " << childSexp->to_debug_string();
			if (!initializeFromAtom(childSexp, entityLabels, wordSets)) {
				std::ostringstream ostr;
				ostr << "initializeFromAtom did not recognize atom element in Pattern: " << childSexp->to_debug_string();
				throwError(sexp, ostr.str().c_str());
			}
		} else if (childSexp->isList()) {
			//SessionLogger::dbg("is_list_0") << dbg_counter++ << " " << childSexp->to_debug_string();
			if (!initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
				std::ostringstream ostr;
				ostr << "initializeFromSubexpression did not recognize list element in Pattern: " << childSexp->to_debug_string();
				throwError(sexp, ostr.str().c_str());
			}
		} else {
			std::ostringstream ostr;
			ostr << "unexpected subexpression type (not list or atom) in Pattern: " << childSexp->to_debug_string();
			throwError(sexp, ostr.str().c_str());
		}
	}
}

bool Pattern::initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) { 
	Symbol atom = childSexp->getValue();
	if (atom == singleMatchOverrideSym) {
		_force_single_match_sentence = true;
		return true;
	} 
	return false; 
}

bool Pattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (constraintType == scoreSym) {
		if (childSexp->getNumChildren() != 2)
			throwError(childSexp, "score sexp should be of length 2");
		_score = boost::lexical_cast<float>(childSexp->getSecondChild()->getValue().to_string());
	} else if (constraintType == scoreGroupSym) {		
		if (childSexp->getNumChildren() != 2)
			throwError(childSexp, "score group sexp should be of length 2");
		_score_group = boost::lexical_cast<int>(childSexp->getSecondChild()->getValue().to_string());
	} else if (constraintType == scoreFnSym) {
		if (childSexp->getNumChildren() != 2)
			throwError(childSexp, "scoring function sexp should be of length 2");
		_scoringFunction = ScoringFactory::getScoringFunction(childSexp->getSecondChild()->getValue());
	} else if (constraintType == idSym) {
		if (childSexp->getNumChildren() != 2)
			throwError(childSexp, "id constraints can only have one token, as in (id <my_id>).");
		setID(childSexp->getSecondChild()->getValue());
	} else if (constraintType == shortcutSym) {
		if (childSexp->getNumChildren() != 2)
			throwError(childSexp, "shortcut sexp should be of length 2");
		setShortcut(childSexp->getSecondChild()->getValue());
	} else if (constraintType == returnSym) {
		if (_return != boost::shared_ptr<PatternReturn>()) 
			throwError(childSexp, "Only one return sexp per pattern may be declared.");
		_return = boost::make_shared<PatternReturn>(childSexp);
	} else if (constraintType == topLevelReturnSym) {
		_return = boost::make_shared<PatternReturn>(childSexp);
		_toplevel_return = true;
	} else {
		//SessionLogger::dbg("init_from_subex_0") << "Could not handle subexpression: " << childSexp->to_debug_string();
		return false; // childSexp was not handled here; probably needs to be handled by the subclass of Pattern
	}
	return true; // childSexp was handled.
}

Pattern_ptr Pattern::parseSexp(Sexp *sexp, const Symbol::HashSet &entityLabels,
									 const PatternWordSetMap& wordSets,
									 bool allow_shortcut_label) 
{
	if (sexp->isAtom()) {
		return boost::make_shared<ShortcutPattern>(sexp->getValue());
	}
	Symbol patternType = sexp->getFirstChild()->getValue();
	Pattern_ptr result;
	if (patternType == all_of_sym || patternType == any_of_sym || patternType == none_of_sym) { 
		return boost::make_shared<CombinationPattern>(sexp, entityLabels, wordSets);
	} else if (patternType == argument_sym) 
		result = boost::make_shared<ArgumentPattern>(sexp, entityLabels, wordSets);
	else if (patternType == doc_sym) 
		result = boost::make_shared<DocumentPattern>(sexp, entityLabels, wordSets);
	else if (patternType == event_sym) 
		result = boost::make_shared<EventPattern>(sexp, entityLabels, wordSets);
	else if (patternType == intersection_sym) 
		result = boost::make_shared<IntersectionPattern>(sexp, entityLabels, wordSets);
	else if (patternType == mention_sym) 
		result = boost::make_shared<MentionPattern>(sexp, entityLabels, wordSets);
	else if (patternType == negation_sym) 
		result = boost::make_shared<NegationPattern>(sexp, entityLabels, wordSets);
	else if (patternType == parse_node_sym) 
		result = boost::make_shared<ParseNodePattern>(sexp, entityLabels, wordSets);
	else if (patternType == vprop_sym) 
		result = boost::make_shared<PropPattern>(sexp, entityLabels, wordSets);
	else if (patternType == nprop_sym) 
		result = boost::make_shared<PropPattern>(sexp, entityLabels, wordSets);
	else if (patternType == mprop_sym) 
		result = boost::make_shared<PropPattern>(sexp, entityLabels, wordSets);
	else if (patternType == cprop_sym) 
		result = boost::make_shared<PropPattern>(sexp, entityLabels, wordSets);
	else if (patternType == sprop_sym)
		result = boost::make_shared<PropPattern>(sexp, entityLabels, wordSets);
	else if (patternType == anyprop_sym)
		result = boost::make_shared<PropPattern>(sexp, entityLabels, wordSets);
	else if (patternType == quotation_sym) 
		result = boost::make_shared<QuotationPattern>(sexp, entityLabels, wordSets);
	else if (patternType == regex_sym) 
		result = boost::make_shared<RegexPattern>(sexp, entityLabels, wordSets);
	else if (patternType == relation_sym) 
		result = boost::make_shared<RelationPattern>(sexp, entityLabels, wordSets);
	else if (patternType == text_sym) 
		result = boost::make_shared<TextPattern>(sexp, entityLabels, wordSets);
	else if (patternType == topic_sym) 
		result = boost::make_shared<TopicPattern>(sexp, entityLabels, wordSets);
	else if (patternType == union_sym) 
		result = boost::make_shared<UnionPattern>(sexp, entityLabels, wordSets);
	else if (patternType == value_sym) 
		result = boost::make_shared<ValueMentionPattern>(sexp, entityLabels, wordSets);
	else if (_additionalPatternTypeFactories().find(patternType) != _additionalPatternTypeFactories().end()) {
		PatternFactory_ptr factory = _additionalPatternTypeFactories()[patternType];
		result = factory->buildPattern(sexp, entityLabels, wordSets);
	} else {
		Symbol::HashMap<Pattern::PatternFactory_ptr>& q = _additionalPatternTypeFactories();
		std::ostringstream ostr;
		ostr << "unrecognized pattern type '" << patternType << "' ";
		throwErrorStatic(sexp, ostr.str().c_str()); // can't call throwError() because we're in a static method
	}
	if (!allow_shortcut_label && (!result->getShortcut().is_null()) &&
		!boost::dynamic_pointer_cast<ShortcutPattern_ptr>(result)) {
		std::ostringstream ostr;
		ostr << "non-reference pattern has shortcut label <" << result->getShortcut() << ">";
		throwErrorStatic(sexp, ostr.str().c_str()); // can't call throwError() because we're in a static method
	}
	//SessionLogger::dbg("patt_type_0") << "Pattern type: " << patternType;
	return result;
}

std::string Pattern::getDebugID() const {
	if (debug_props) {
		if (!getID().is_null()) {
			return getID().to_debug_string();
		} else if (!getShortcut().is_null()) {
			return getShortcut().to_debug_string();
		} else {
			std::ostringstream ostr;
			ostr << "[" << typeName() << "]";
			return ostr.str();
		}
	} else {
		return "enable debug_props parameter to debug props!";
	} 
}

Symbol Pattern::getReturnLabelFromPattern() const {
	return _return ? _return->getLabel() : Symbol();
}

bool Pattern::returnLabelIsEmpty() const {
	return _return ? _return->hasEmptyLabel() : true;
}

size_t Pattern::getNReturnValuePairs() const { 
	return _return ? _return->getNValuePairs() : 0; 
}

// to be used from a non-static method
void Pattern::throwError(const Sexp *sexp, const char *reason) {
	std::stringstream error;
	error << "Parser error for " << typeName() << ": " << reason << ": " << sexp->to_debug_string();
	throw UnexpectedInputException("Pattern::throwError()", error.str().c_str());
}

// to be used from a static method
void Pattern::throwErrorStatic(const Sexp *sexp, const char *reason) {
	std::stringstream error;
	error << "Parser error : " << reason << ": " << sexp->to_debug_string();
	throw UnexpectedInputException("Pattern::throwError()", error.str().c_str());
}

void Pattern::setShortcut(const Symbol& shortcut) {
	if (!_shortcut.is_null()) {
		std::ostringstream ostr;
		ostr << "Attempt to assign shortcut name <" << shortcut << "> to pattern "
			<< "when shortcut name <" << _shortcut << "> was already assigned.";
		throw UnexpectedInputException("Pattern::setShortcut", ostr.str().c_str());
	}
	_shortcut = shortcut;
}

void Pattern::setID(const Symbol& id) {
	if (!_id.is_null()) {
		std::ostringstream ostr;
		ostr << "Attempt to assign ID name <" << id << "> to pattern "
			<< "when ID name <" << _id << "> was already assigned.";
		throw UnexpectedInputException("Pattern::setId", ostr.str().c_str());
	}
	_id = id;
	//std::cerr << "ID: " << _id << std::endl;
}

void Pattern::getReturns(PatternReturnVecSeq & output) const {
	if (getReturn()) {
		std::ostringstream ostr;
		_return->dump(ostr, /*indent=*/ 0);
		SessionLogger::dbg("dump_pr_ret_0") << "Return value for <" << getDebugID() << ">: " << ostr.str();
		if (output.empty()) {
			PatternReturnVec v0;
			v0.push_back(_return);
			output.push_back(v0);
		}
		else {
			output.back().push_back(_return);
		}
	} else {
		SessionLogger::dbg("dump_pr_nret_0") << "No return value for <" << getDebugID() << ">";
	}
	return;
}

PatternFeatureSet_ptr Pattern::makeEmptyFeatureSet() {
	PatternFeatureSet_ptr pfs = boost::make_shared<PatternFeatureSet>();
	pfs->setScore(_score);
	pfs->addFeature(boost::make_shared<GenericPFeature>(shared_from_this(), -1, LanguageVariant::getLanguageVariant()));
	addID(pfs);
	return pfs;
}

void Pattern::addID(PatternFeatureSet_ptr pfs) {
	if (!getID().is_null())
		pfs->addFeature(boost::make_shared<TopLevelPFeature>(shared_from_this(), LanguageVariant::getLanguageVariant()));
}


PatternFeatureSet_ptr Pattern::fillAllFeatures(std::vector<std::vector<PatternFeatureSet_ptr> > req_i_j_to_sfs, std::vector<std::vector<PatternFeatureSet_ptr> > opt_i_j_to_sfs,
													PatternFeatureSet_ptr allFeatures, 
													std::vector<Pattern_ptr> reqArgPatterns, std::vector<Pattern_ptr> optArgPatterns,
													bool require_one_to_one_argument_mapping,
													bool require_all_arguments_to_match_some_pattern,
													bool allow_many_to_many_mapping) const {

	//make a big list of Argument Patterns, putting the required ones last so the recursion in getBestScore will work
	std::vector<Pattern_ptr> allArgPatterns;
	std::copy(optArgPatterns.begin(), optArgPatterns.end(), std::back_inserter(allArgPatterns));
	std::copy(reqArgPatterns.begin(), reqArgPatterns.end(), std::back_inserter(allArgPatterns));

	//do the same with the i_j_to_sfs vectors
	std::vector<std::vector<PatternFeatureSet_ptr> > all_i_j_to_sfs;
	std::copy(opt_i_j_to_sfs.begin(), opt_i_j_to_sfs.end(), std::back_inserter(all_i_j_to_sfs));
	std::copy(req_i_j_to_sfs.begin(), req_i_j_to_sfs.end(), std::back_inserter(all_i_j_to_sfs));

	int max_i = static_cast<int>(all_i_j_to_sfs.size());

	// It is entirely possible that we have an empty i_j_to_sfs.
	// If this is the case, let's just return the features we have so far.
	if (max_i == 0) {
		return allFeatures;
	}

	/*
	Calls getBestScore to find a matching from i's to j's if one exists.
	Then greedily adds additional matches to i's, if there are j's left over that still fit.
	*/
	int max_j = static_cast<int>(all_i_j_to_sfs[0].size());
	std::set<int> empty_set;

	//get the best pairing of argument patterns to arguments - see the function for more info
	std::pair<std::pair<int,float>,std::vector<int> > result = getBestScore(all_i_j_to_sfs, static_cast<int>(allArgPatterns.size()), max_j, 
		empty_set, allArgPatterns, static_cast<int>(optArgPatterns.size()), allow_many_to_many_mapping);
	std::vector<int> ordered_j_indices = result.second;
	// if it found a match with a non-zero score
	if (result.first.second > 0) {
		std::vector<float> scores;
		std::set<int> used_j_indices; 

		// Start with the matching found by getBestScore
		for (int i = 0; i < max_i; i++) {
			int j = ordered_j_indices[i];
			// j will be -1 if there was no match for this argument (i.e. it was optional)
			if (j != -1) {
				PatternFeatureSet_ptr sfs = all_i_j_to_sfs[i][j];
				allFeatures->addFeatures(sfs);
				scores.push_back(sfs->getScore());
				used_j_indices.insert(j);
			}
		}
		
		// Greedily add additional matches
		if (!require_one_to_one_argument_mapping) {
			for (int i = 0; i < max_i; i++) {
				for (int j = 0; j < max_j; j++) {
					if (used_j_indices.find(j) != used_j_indices.end() && !allow_many_to_many_mapping) {
						continue;
					}
					PatternFeatureSet_ptr sfs = all_i_j_to_sfs[i][j];
					if (sfs == 0) {
						continue;
					}
					float score = sfs->getScore();
					if (score == 0) {
						continue;
					}
					allFeatures->addFeatures(sfs);
					scores.push_back(sfs->getScore());
					used_j_indices.insert(j);
				}
			}
		}

		if (require_all_arguments_to_match_some_pattern) {
			for (int j = 0; j < max_j; j++) {
				if (used_j_indices.find(j) == used_j_indices.end()) {
					return PatternFeatureSet_ptr();
				}
			}			
		}

		// combine scores from arguments according to scoring function
		allFeatures->setScore(_scoringFunction(scores, _score));
		return allFeatures;
	} else {
		return PatternFeatureSet_ptr();
	}
}

std::pair<std::pair<int,float>,std::vector<int> > Pattern::getBestScore(std::vector<std::vector<PatternFeatureSet_ptr> > i_j_to_sfs, 
															  int max_i, int max_j, std::set<int> used_j_indices, 
															  std::vector<Pattern_ptr> argPatterns, int numOptArgsAtFront,
															  bool allow_many_to_many_mapping) const
{
	/*
	i is the index into the pattern arguments that must be satisfied.
	j is the index into the arguments of the current prop, event, or relation pattern.
	This function tries all possible matchings of i to j (requiring that each i has a unique match unless allow_many_to_many_mapping=true).

	i_j_to_sfs[i][j] contains a sfs for (i,j)
	We will look at (max_i - 1) and then recurse by decreasing max_i
	During the recursion, we will also keep track of which j_indices have already been used.

	We return a <int,float> that represents the number of patterns matched and the score of those matches, plus
	a vector of ordered_j_indices, where ordered_j_indices[i] gives us the corresponding chosen j.

	The argPatterns vector MUST be arranged such that the optional argument come first, then the required arguments.
	The int "numOptArgsAtFront" specifices how many optional arguments there are, and they must all be at the beginning of the vector.

	*/
    float perfect_score = 1;
	std::vector<int> best_ordered_j_indices;
	if(max_i == 0) return std::make_pair(std::make_pair(0, perfect_score), best_ordered_j_indices);
	int i = max_i - 1;
	float best_score = 0;
	int best_matches = 0;
	for (int j = 0; j < max_j; j++) {
		//check to make sure this j hasn't been used already
		if (used_j_indices.find(j) != used_j_indices.end() && !allow_many_to_many_mapping){
			continue;
		}
		//get the feature set for this i-j pair
		PatternFeatureSet_ptr sfs = i_j_to_sfs[i][j];
		//skip the argument (j) if it's empty, meaning it did not match
		if (sfs == 0){
			continue;
		}
		//get the score of this pattern-argument pairing
		float score = sfs->getScore();
		//skip this pairing if the score is 0
		if (score == 0) {
			continue;
		}
		if (score == -1) { // should this be UNSPECIFIED_SCORE
			score = (float).01; // -1 means we don't have a score, but it's still something to keep.
		}

		//we have now used this j
		used_j_indices.insert(j);
		//keep going with the recursion
		std::pair<std::pair<int,float>,std::vector<int> > result = getBestScore(i_j_to_sfs, i, max_j, used_j_indices, argPatterns, numOptArgsAtFront, allow_many_to_many_mapping);
		used_j_indices.erase(j);

		//evaluate the results if this argument pattern is paired with this prop/relation/event argument
		int matches_subproblem = result.first.first;
		int cumulative_matches = matches_subproblem + 1;
		float score_subproblem = result.first.second;		
		float cumulative_score = score * score_subproblem;

		//add the j index to the vector in the correct place, so the ordering is preserved.
		std::vector<int> ordered_j_indices_subproblem = result.second;
		ordered_j_indices_subproblem.push_back(j);

		if (cumulative_matches > best_matches || 
			(cumulative_matches == best_matches && cumulative_score > best_score))
		{
			best_score = cumulative_score;
			best_matches = cumulative_matches;
			best_ordered_j_indices = ordered_j_indices_subproblem;
		}
	}	

	ArgumentMatchingPattern_ptr argPattern = boost::dynamic_pointer_cast<ArgumentMatchingPattern>(argPatterns[i]);
	//if we have an optional argument, try running without that optional argument taking any j, and see if the score is better
	//we tell if the argument is optional if the i is less than the number of optional arguments, since the optional arguments should be at the beginning of the vector.
	if (argPattern && i < numOptArgsAtFront) {
		std::pair<std::pair<int,float>,std::vector<int> > result = getBestScore(i_j_to_sfs, i, max_j, used_j_indices, argPatterns, numOptArgsAtFront, allow_many_to_many_mapping);
		int matches_subproblem = result.first.first;
		float score_subproblem = result.first.second;
		std::vector<int> ordered_j_indices_subproblem = result.second;
		ordered_j_indices_subproblem.push_back(-1);	// indicates no match
		
		// nothing cumulative because this is the situation where the argPattern doesn't match
		if (matches_subproblem > best_matches ||
			(matches_subproblem == best_matches && score_subproblem > best_score))
		{
			best_matches = matches_subproblem;
			best_score = score_subproblem;
			best_ordered_j_indices = ordered_j_indices_subproblem;
		}
	}

	return std::make_pair(std::make_pair(best_matches,best_score), best_ordered_j_indices);
}

void Pattern::reportBadShortcutType(Pattern_ptr shortcut, Pattern_ptr replacement) {
	std::ostringstream err;
	if (boost::dynamic_pointer_cast<ShortcutPattern>(shortcut)) {
		err << "Error replacing shortcut " << shortcut->getShortcut();
		err << ": reference pattern does not have the correct type.";
		err << "\nReference pattern:\n";
		replacement->dump(err);
	} else {
		err << "Child pattern has unexpected type:\n";
		shortcut->dump(err);
	}
	err << "\nOriginating pattern:\n";
	dump(err);
	throw UnexpectedInputException("ShortcutPattern::shortcutReplacement()", err.str().c_str());
}

void Pattern::logFailureToInitializeFromChildSexp(const Sexp * childSexp) {
	SessionLogger::err("init_from_subex_0") << "Could not initialize " << typeName() << " subpattern from child sexp: " 
		<< childSexp->to_debug_string();
}

void Pattern::logFailureToInitializeFromAtom(const Sexp * atomSexp) {
	SessionLogger::err("init_from_atom_0") << "Could not initialize " << typeName() << " subpattern from atom: " 
		<< atomSexp->to_debug_string();
}

/**
 * Used by IntersectionPattern and UnionPattern.
 *
 * @return First valid ID.
 *
 * @author afrankel@bbn.com
 * @date 2011.08.08
 **/
Symbol Pattern::getFirstValidIDForCompositePattern(const std::vector<Pattern_ptr> & pattern_vec) const {
	size_t n_patterns = pattern_vec.size();
	for (size_t i = 0; i < n_patterns; i++) {
		if (!pattern_vec[i]->getID().is_null()) {
			return pattern_vec[i]->getID();
		}
	}
	return getID(); // probably invalid, but it can be checked by the caller
}

std::string Pattern::toDebugString(int indent) const {
	std::ostringstream ostr;
	dump(ostr, indent);
	return ostr.str();
}

Symbol::HashMap<Pattern::PatternFactory_ptr>& Pattern::_additionalPatternTypeFactories() {
	static Symbol::HashMap<PatternFactory_ptr> patternTypeFactories;
	return patternTypeFactories;
}
void Pattern::ensurePatternTypeIsNotRegistered(Symbol typeName) {
	if (_additionalPatternTypeFactories().find(typeName) != _additionalPatternTypeFactories().end())
		throw UnexpectedInputException("Pattern::ensurePatternTypeIsNotRegistered",
			"A pattern type with this name has already been registered: ", typeName.to_debug_string());
}

Symbol Pattern::createMultiWordSymbol(Sexp *sexp) {
	if (!sexp->isList())
		throwError(sexp, "Expected a list of symbols");
	int nkids = sexp->getNumChildren();
	std::wstringstream str;
	for (int j=0; j<nkids; ++j) {
		Sexp *child = sexp->getNthChild(j);
		if (!child->isAtom()) {
			throwError(sexp, "Expected a list of symbols");
		}
		if (j != 0)
			str << L" ";
		str << child->getValue();		
	}
	return Symbol(str.str());
}

void Pattern::fillListOfWords(Sexp *sexp, const PatternWordSetMap& wordSets, 
							      std::set<Symbol>& listOfWords, bool skipFirstChild,
								  bool errorOnUnknownWordSetSymbol) 
{
	// Allow multi-word symbols ala:
	//  (foo France Belgium (United Kingdom) Germany) --> [France, Belgium, United Kingdom, Germany]
	// This will not be useful in most contexts, but when the set of words
	//   is being used for actor names, it's essential

	if (!sexp->isList())
		throwError(sexp, "Expected a list of symbols");
	int nkids = sexp->getNumChildren();
	for (int j=(skipFirstChild?1:0); j<nkids; ++j) {
		Sexp *child = sexp->getNthChild(j);
		Symbol sym;
		if (!child->isAtom()) {
			sym = createMultiWordSymbol(child);
		} else sym = child->getValue();
		
		if (PatternWordSet::isWordSetSymbol(sym)) {
			PatternWordSetMap::const_iterator it = wordSets.find(sym);
			if (it != wordSets.end()) {
				const PatternWordSet_ptr wordSet = (*it).second;
				for (int word=0; word<wordSet->getNWords(); ++word)
					listOfWords.insert(wordSet->getNthWord(word));
			} else if (errorOnUnknownWordSetSymbol) {
				throwError(sexp, "Unrecognized PatternWordSet name in word list");
			} else {
				listOfWords.insert(sym);
			}
		} else {
			listOfWords.insert(sym);
		}
	}
}

void Pattern::fillListOfSymbols(Sexp *sexp, std::set<Symbol>& listOfSymbols, bool skipFirstChild) {
	if (!sexp->isList())
		throwError(sexp, "Expected a list of symbols");
	int nkids = sexp->getNumChildren();
	for (int j=(skipFirstChild?1:0); j<nkids; ++j) {
		Sexp *child = sexp->getNthChild(j);
		if (!child->isAtom())
			throwError(sexp, "Expected a list of symbols");
		listOfSymbols.insert(child->getValue());
	}
}

void Pattern::fillListOfWordsWithWildcards(Sexp *sexp, const PatternWordSetMap& wordSets, 
									 std::set<Symbol>& words, std::set<Symbol>& wordPrefixes) {
	int nkids = sexp->getNumChildren() - 1;
	for (int j = 0; j < nkids; j++) {
		Symbol sym = sexp->getNthChild(j+1)->getValue();

		if (PatternWordSet::isWordSetSymbol(sym)) {
			PatternWordSetMap::const_iterator it = wordSets.find(sym);
			if (it == wordSets.end()) {
				std::ostringstream err;
				err << "Unrecognized PatternWordSet name in PropPattern: " << sym.to_debug_string();
				throw UnexpectedInputException("PropPattern::fillPredicateArray()", err.str().c_str());
			}
			const PatternWordSet_ptr wordSet = (*it).second;
			for (int word=0; word<wordSet->getNWords(); ++word)
				addToWordArrayWithWildcards(wordSet->getNthWord(word), words, wordPrefixes);
		} else {
			addToWordArrayWithWildcards(sym, words, wordPrefixes);
		}
	}
}

void Pattern::addToWordArrayWithWildcards(const Symbol &element, std::set<Symbol>& words, std::set<Symbol>& wordPrefixes) {
	std::wstring element_str = element.to_string();
	if (element_str[element_str.length()-1] == L'*') {
		Symbol prefix(element_str.substr(0, element_str.length()-1));
		wordPrefixes.insert(prefix);
	} else {
		words.insert(element);
	}
}

bool Pattern::wordMatchesWithPrefix(Symbol word, std::set<Symbol>& words, std::set<Symbol>& wordPrefixes, bool return_true_if_empty) {	
	if (words.size() == 0 && wordPrefixes.size() == 0) {
		return return_true_if_empty;
	} else if (word.is_null()) {  // If argument is null, it is not valid
		return false;
	} else if (words.find(word) != words.end()) { // Argument is valid if it matches straight-up
 		return true;
	} else { // Argument is valid if it matches one of our predicate prefixes
		std::wstring arg_as_string(word.to_string());
		BOOST_FOREACH(Symbol s, wordPrefixes) {
			if (arg_as_string.find(s.to_string()) == 0) {
				return true;
			}
		}
	}
	return false;
}

bool LanguageVariantSwitchingPattern::initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	if (!_languageVariant)
		_languageVariant = LanguageVariant::getLanguageVariant();
	return Pattern::initializeFromAtom(childSexp, entityLabels, wordSets);
}

bool LanguageVariantSwitchingPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (!_languageVariant)
		_languageVariant = LanguageVariant::getLanguageVariant();
	
	if (Pattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else {
		if (constraintType == language_sym) {
			if (childSexp->getNumChildren() != 2)
				throwError(childSexp, "language sexp should be of length 2");
			if (childSexp->getSecondChild()->getValue() == LanguageVariant::languageVariantAnySym)
				_languageVariant = LanguageVariant::getLanguageVariantAny();
			else 
				_languageVariant = LanguageVariant::getLanguageVariant(childSexp->getSecondChild()->getValue(), _languageVariant->getVariant());
		} else if (constraintType == variant_sym) {
			if (childSexp->getNumChildren() != 2)
				throwError(childSexp, "variant sexp should be of length 2");
			if (childSexp->getSecondChild()->getValue() != LanguageVariant::languageVariantAnySym)
				_languageVariant = LanguageVariant::getLanguageVariant(_languageVariant->getLanguage(), childSexp->getSecondChild()->getValue());
		} else {
			//SessionLogger::dbg("init_from_subex_0") << "Could not handle subexpression: " << childSexp->to_debug_string();
			return false; // childSexp was not handled here; probably needs to be handled by the subclass of Pattern
		}
	}
	return true; // childSexp was handled.
}
