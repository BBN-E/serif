// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternSet.h"
#include "Generic/patterns/EntityLabelPattern.h"
#include "Generic/patterns/ShortcutPattern.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/patterns/RegexPattern.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/TopLevelPFeature.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include "Generic/common/Sexp.h"

namespace { // private symbols
	const Symbol WORDSETS_SYM(L"wordsets");
	const Symbol ENTITYLABELS_SYM(L"entitylabels");
	const Symbol REFERENCE_SYM(L"reference");
	const Symbol TOPLEVEL_SYM(L"toplevel");
	const Symbol DOCLEVEL_SYM(L"doclevel");
	const Symbol BACKOFF_SYM(L"backoff");
	const Symbol OPTIONS_SYM(L"options");
}

PatternSet::PatternSet(): _keep_all_features(false), _entityLabels(4), _do_not_block_patterns(false)
{
	_single_match_for_ment_prop = ParamReader::getOptionalTrueFalseParamWithDefaultVal("independent_props_ments_sentence_snippets", false);
}

PatternSet::PatternSet(Pattern_ptr pattern) : _keep_all_features(false), _entityLabels(4), _do_not_block_patterns(false)
{
    _single_match_for_ment_prop = ParamReader::getOptionalTrueFalseParamWithDefaultVal("independent_props_ments_sentence_snippets", false);
    _topLevelPatterns.push_back(pattern);
}

PatternSet::PatternSet(const char* filename, bool encrypted): _keep_all_features(false), _entityLabels(4), _do_not_block_patterns(false)
{
	Sexp sexp(filename, true, true, true, true, std::vector<std::string>(), encrypted);
	initializeFromSexp(&sexp);
}


PatternSet::PatternSet(Sexp *sexp): _keep_all_features(false), _entityLabels(4), _do_not_block_patterns(false)
{
	initializeFromSexp(sexp);
}

void PatternSet::initializeFromSexp(Sexp *callers_sexp) {
	_single_match_for_ment_prop = ParamReader::getOptionalTrueFalseParamWithDefaultVal("independent_props_ments_sentence_snippets", false);

	Sexp *sexp = callers_sexp;
	if (sexp->getNumChildren() == 1 && sexp->getFirstChild()->isList()){
		// peel off the extra wrapper layer; it may have held includes or sets or other macros
		sexp = sexp->getFirstChild();
	}
	// Get the name.
	if (!sexp->getFirstChild()->isAtom()) {
		throw UnexpectedInputException("PatternSet::PatternSet",
			"PatternSet Sexp should begin with a pattern set name.");
	}
	_patternSetName = sexp->getFirstChild()->getValue();

	// Process the remaining children, and collect them into lists.  
	// We no longer require a strict ordering for these children, 
	// and we now allow children to be repeated.
	int num_children = sexp->getNumChildren();
	std::vector<Sexp*> wordsetsSexps;
	std::vector<Sexp*> entitylabelsSexps;
	std::vector<Sexp*> referenceSexps;
	std::vector<Sexp*> toplevelSexps;
	std::vector<Sexp*> doclevelSexps;
	std::vector<Sexp*> backoffSexps;
	std::vector<Sexp*> optionsSexps;
	for (int i=1; i<num_children; ++i) {
		Sexp *child = sexp->getNthChild(i);
		if ((!child->isList()) || (child->getNumChildren()==0) ||
			(!child->getFirstChild()->isAtom())) {
			std::stringstream error;
			error << "Unexpected child of pattern set: " << child->to_debug_string();
			throw UnexpectedInputException("PatternSet::PatternSet()", error.str().c_str());
		}
		Symbol childType = child->getFirstChild()->getValue();
		if (childType == WORDSETS_SYM) 
			wordsetsSexps.push_back(child);
		else if (childType == ENTITYLABELS_SYM) 
			entitylabelsSexps.push_back(child);
		else if (childType == REFERENCE_SYM) 
			referenceSexps.push_back(child);
		else if (childType == TOPLEVEL_SYM) 
			toplevelSexps.push_back(child);
		else if (childType == DOCLEVEL_SYM) 
			doclevelSexps.push_back(child);
		else if (childType == BACKOFF_SYM) 
			backoffSexps.push_back(child);
		else if (childType == OPTIONS_SYM) 
			optionsSexps.push_back(child);
		else {
			std::stringstream error;
			error << "Unexpected child of pattern set: " << child->to_debug_string();
			throw UnexpectedInputException("PatternSet::PatternSet()", error.str().c_str());
		}
	}

	// Create the word set map.
	BOOST_FOREACH(Sexp *wordSetSexp, wordsetsSexps) {
		int n_wordsets = wordSetSexp->getNumChildren() - 1;
		for (int j = 0; j < n_wordsets; j++) {
			PatternWordSet_ptr wordSet = boost::make_shared<PatternWordSet>(wordSetSexp->getNthChild(j+1), _wordSets);
			if (_wordSets.find(wordSet->getLabel()) != _wordSets.end()) {
				std::stringstream error;
				error << "Duplicate wordset label: " << wordSet->getLabel();
				throw UnexpectedInputException("PatternSet::PatternSet()", error.str().c_str());
			}
			_wordSets[wordSet->getLabel()] = wordSet;
		}
	}

	// Create the entity label patterns.
	BOOST_FOREACH(Sexp *entityLabelSexp, entitylabelsSexps) {
		int n_entity_label_patterns = entityLabelSexp->getNumChildren() - 1;
		for (int j = 0; j < n_entity_label_patterns; j++) {
			EntityLabelPattern_ptr entityLabelPattern = boost::make_shared<EntityLabelPattern>(
				entityLabelSexp->getNthChild(j+1), _entityLabels, _wordSets);
			
			// Even though we don't have any reference patterns yet, we call replaceShortcuts
			// because we need CombinationPatterns to update their type correctly.
			if (entityLabelPattern->hasPattern()) {
				entityLabelPattern->replaceShortcuts(_referencePatterns);
			}
			_entityLabelPatterns.push_back(entityLabelPattern);
			_entityLabels.insert(entityLabelPattern->getLabel());
		}
	}

	// Create the reference patterns (shortcuts).  Note that this is the only
	// place where we allow patterns to contain shortcut labels.
	BOOST_FOREACH(Sexp *refSexp, referenceSexps) {
		int n_ref_patterns = refSexp->getNumChildren() - 1;
		for (int j = 0; j < n_ref_patterns; j++) {
			Sexp* refPatSexp = refSexp->getNthChild(j+1);
			// Create the reference pattern.
			Pattern_ptr refPattern = Pattern::parseSexp(
				refPatSexp, _entityLabels, _wordSets, true);
			// Get the shortcut name and make sure it's valid.
			Symbol shortcut = refPattern->getShortcut();
			if (shortcut.is_null())
				throwParseError(refPatSexp, "Reference patterns must have shortcut names");
			if (_referencePatterns.find(shortcut) != _referencePatterns.end())
				throwParseError(refPatSexp, "Duplicate shortcut name");
			// Resolve any shortcuts in the reference pattern.  Reference patterns
			// are only allowed to reference patterns that precede them in the list.
			try {
				refPattern = refPattern->replaceShortcuts(_referencePatterns);	
			} catch (UnexpectedInputException& e) {
				std::ostringstream err;
				err << "\n\nIn reference pattern: " << refPatSexp->to_debug_string() << "\n\n";
				err << e.getMessage();
				throw UnexpectedInputException("PatternSet::initializeFromSexp", err.str().c_str());
			}
			// Add the reference pattern to _referencePatterns.  Note that we do this
			// after we replace shortcuts in refPattern -- no self-reference allowed!
			_referencePatterns[shortcut] = refPattern;
		}
	}

	// Create the top-level patterns.
	BOOST_FOREACH(Sexp *topLevelSexp, toplevelSexps)
		loadPatterns(topLevelSexp, _topLevelPatterns);

	// Create the document patterns.
	BOOST_FOREACH(Sexp *docPatternsSexp, doclevelSexps)
		loadPatterns(docPatternsSexp, _docPatterns);

	// Create the backoff level vector.
	BOOST_FOREACH(Sexp* backoffSexp, backoffSexps) {
		int n_backoff_levels = backoffSexp->getNumChildren() - 1;
		for (int j = 0; j < n_backoff_levels; j++) {		
			_backoffLevels.push_back(backoffSexp->getNthChild(j+1)->getValue());
		}
	}
	
	// Process options.
	BOOST_FOREACH(Sexp* optionsSexp, optionsSexps) {
		for (int i = 1; i < optionsSexp->getNumChildren(); ++i) {
			Sexp *childSexp = optionsSexp->getNthChild(i);
			if (childSexp->isAtom()) {
				if (childSexp->getValue() == Symbol(L"KEEP_ALL_FEATURES")) {
					// This should only be set to true if the patterns are engineered to have
					// all the features kept (talk to Liz if you don't know what this means)
					_keep_all_features = true;
				} else {
					throwParseError(childSexp, "Unrecognized atom element in option list");
				}
			} else if (childSexp->isList()) {
				Symbol optionType = childSexp->getFirstChild()->getValue();
				if (optionType == Symbol(L"blocked_proposition_types")) {
					for (int j=0; j<childSexp->getNumChildren()-1; ++j) {
						Symbol blockedType = childSexp->getNthChild(j+1)->getValue();
						if (blockedType == Symbol(L"null")  || blockedType == Symbol(L"none")) {
							;
						} else {
							_blockedPropositionStatuses.insert(PropositionStatusAttribute::getFromString(blockedType.to_string()));
						}
					}
				} else {
					throwParseError(childSexp, "Unrecognized option in option list");
				}
			} else {
				throwParseError(childSexp, "Unrecognized option in option list");
			}
		} 
	}
}

void PatternSet::loadPatterns(Sexp *sexp, std::vector<Pattern_ptr> &patterns) {
	if (sexp == 0) { return; }
	int num_patterns = sexp->getNumChildren() - 1;
	for (int j = 0; j < num_patterns; j++) {
		// Parse the pattern.
		Sexp* childSexp = sexp->getNthChild(j+1);
		Pattern_ptr pattern = Pattern::parseSexp(childSexp, _entityLabels, _wordSets);

		// Replace shortcuts with the patterns they point to.
		try {
			pattern = pattern->replaceShortcuts(_referencePatterns);
		} catch (UnexpectedInputException& e) {
			std::ostringstream err;
			err << "\n\nIn pattern: " << childSexp->to_debug_string() << "\n\n";
			err << e.getMessage();
			throw UnexpectedInputException("PatternSet::loadPatterns", err.str().c_str());
		}

		// Add the pattern to the list of patterns.
		patterns.push_back(pattern);
	}
}

void PatternSet::throwParseError(const Sexp *sexp, const char *reason) {
	std::stringstream error;
	error << "Parser Error: " << reason << ": " << sexp->to_debug_string();
	throw UnexpectedInputException("Pattern::throwError()", error.str().c_str());
}

Symbol PatternSet::getPatternSetName() const { 
	return _patternSetName; 
}
size_t PatternSet::getNTopLevelPatterns() const { 
	return _topLevelPatterns.size(); 
}
Pattern_ptr PatternSet::getNthTopLevelPattern(size_t n) const { 
	return _topLevelPatterns[n]; 
}
size_t PatternSet::getNEntityLabelPatterns() const { 
	return _entityLabelPatterns.size(); 
}
EntityLabelPattern_ptr PatternSet::getNthEntityLabelPattern(size_t n) { 
	return _entityLabelPatterns[n]; 
}
size_t PatternSet::getNDocPatterns() const { 
	return _docPatterns.size(); 
}
Pattern_ptr PatternSet::getNthDocPattern(size_t n) const { 
	return _docPatterns[n]; 
}
int PatternSet::getNBackoffLevels() const { 
	return static_cast<int>(_backoffLevels.size()); 
}

const Symbol::HashSet& PatternSet::getEntityLabelSet() {
	return _entityLabels;
}


Symbol PatternSet::getNthBackoffLevel(int n) const { 
	if (n >= 0 && n < static_cast<int>(_backoffLevels.size())) 
		return _backoffLevels[n];
	else 
		return Symbol(); 
}

void PatternSet::addBlockedPropositionStatus(PropositionStatusAttribute status) {
	_blockedPropositionStatuses.insert(status);
}

bool PatternSet::isBlockedPropositionStatus(PropositionStatusAttribute status) const {
	if (_do_not_block_patterns)
		return false;
	if (_blockedPropositionStatuses.empty()) {
		const std::set<PropositionStatusAttribute> &defaultBlockedPropositionStatuses = getDefaultBlockedPropositionStatuses();
		return defaultBlockedPropositionStatuses.find(status) != defaultBlockedPropositionStatuses.end();
	} else {
		return _blockedPropositionStatuses.find(status) != _blockedPropositionStatuses.end();
	}
}

const std::set<PropositionStatusAttribute> &PatternSet::getDefaultBlockedPropositionStatuses() {
	static bool initialized = false;
	static std::set<PropositionStatusAttribute> blockedTypes;
	if (!initialized) {
		initialized = true;
		std::vector<std::wstring> types = ParamReader::getWStringVectorParam("blocked_proposition_types");
		if (types.empty()) {
			ParamReader::getRequiredParam("blocked_proposition_types"); // Make sure it was defined.
		}
		BOOST_FOREACH(std::wstring type, types) {
			if (boost::iequals(type, L"none") || boost::iequals(type, L"null")) {
				;
			} else {
				blockedTypes.insert(PropositionStatusAttribute::getFromString(type.c_str()));
			}
		}
	}
	return blockedTypes;
}

/**
 * Uses a PatternSet to build an IDToPatternReturnVecSeqMap, 
 * which maps a pattern id name to a PatternReturnVecSeqMap, which stores
 * any of the possible bundles of information used to represent all the args
 * for a given top-level pattern. Usually, there is only one such bundle, but
 * if the pattern contains an any-of, there will be more. If it contains multiple
 * any-ofs, there could be many more. 
 *
 * @param output_map IDToPatternReturnVecSeqMap to be filled with non-tbd PatternReturns if no tbd pattern 
 * is encountered.
 * @param output_tbd_predicates Names of tbd predicates
 * derived from the "tbd" ReturnPatterns encountered (other than "tbd agent" and "tbd patient").
 * @param dump Determines whether pattern output (which is verbose)
 * should be dumped.
 *
 * @see http://wiki.d4m.bbn.com/wiki/PatternReturn_Hierarchy
 *
 * @author afrankel@bbn.com
 * @date 2010.10.18
 **/

void PatternSet::getPatternReturns(IDToPatternReturnVecSeqMap & output_map, bool dump) const 
{
	if (dump) {
		SessionLogger::info("ps_0") << "PATTERN: " << getPatternSetName().to_debug_string();
	}
	size_t n_toplevel_patterns = _topLevelPatterns.size();
	for (size_t n = 0; n < n_toplevel_patterns; ++n) {
		if (_topLevelPatterns[n]) {
			if (_topLevelPatterns[n]->getFirstValidID().is_null()) {
				// We're probably dealing with a pattern that produces an individual
				// and hence doesn't need an ID. Just skip the pattern.
				continue;
			}
			if (dump) {
				std::ostringstream ostr;
				ostr << "  ID: " << _topLevelPatterns[n]->getFirstValidID().to_debug_string() << std::endl;
				_topLevelPatterns[n]->dump(ostr, 0);
				SessionLogger::info("ps_1") << ostr.str();
			}
			PatternReturnVecSeq pattern_return_vec_seq;
			_topLevelPatterns[n]->getReturns(pattern_return_vec_seq);
			output_map.insert(make_pair(_topLevelPatterns[n]->getFirstValidID(), pattern_return_vec_seq));
			int size_after(static_cast<int>(output_map.size()));
			if (dump) {
				std::ostringstream ostr;
				dumpPatternReturnVecSeq(pattern_return_vec_seq, ostr, 0);
				SessionLogger::info("ps_2") << ostr.str();
			}
		}
	}
	return;
}

bool PatternSet::validForPreParseMatching() const {
	for (size_t i = 0; i < _topLevelPatterns.size(); i++) {
		Pattern_ptr pattern = _topLevelPatterns[i];
		RegexPattern_ptr rp = boost::dynamic_pointer_cast<RegexPattern>(pattern);
		if (!rp || !rp->containsOnlyTextSubpatterns()) 
			return false;
	}
	return true;
}

