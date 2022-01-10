// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include <boost/lexical_cast.hpp>

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Sexp.h"
#include "Generic/patterns/ArgumentPattern.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/ShortcutPattern.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/Argument.h"

// Private symbols
namespace {
	Symbol argumentSym = Symbol(L"argument");
	Symbol roleSym = Symbol(L"role");
	Symbol optionalSym = Symbol(L"OPT");
	Symbol fallThroughSym = Symbol(L"allow_fall_through");
}



ArgumentPattern::ArgumentPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets):
_is_optional(false) 
{
	int nkids = sexp->getNumChildren();
	if (nkids < 2)
		throwError(sexp, "too few children in ArgumentPattern");		 
	if (sexp->getNthChild(0)->getValue() != argumentSym)
		throwError(sexp, "ArgumentPattern must start with 'argument'");

	initializeFromSexp(sexp, entityLabels, wordSets);
}

bool ArgumentPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, 
												  const PatternWordSetMap& wordSets) {
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (Pattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (constraintType == roleSym) {
		int n_roles = childSexp->getNumChildren() - 1;
		for (int j = 0; j < n_roles; j++) {
			// Correct common roles in patterns so that they're properly angle bracketed
			Symbol role = childSexp->getNthChild(j+1)->getValue();
			if (role == Symbol(L"ref"))
				role = Argument::REF_ROLE;
			else if (role == Symbol(L"sub"))
				role = Argument::SUB_ROLE;
			else if (role == Symbol(L"obj"))
				role = Argument::OBJ_ROLE;
			else if (role == Symbol(L"iobj"))
				role = Argument::IOBJ_ROLE;
			else if (role == Symbol(L"poss"))
				role = Argument::POSS_ROLE;
			else if (role == Symbol(L"temp"))
				role = Argument::TEMP_ROLE;
			else if (role == Symbol(L"loc"))
				role = Argument::LOC_ROLE;
			else if (role == Symbol(L"member"))
				role = Argument::MEMBER_ROLE;
			else if (role == Symbol(L"unknown"))
				role = Argument::UNKNOWN_ROLE;
			_roles.push_back(role);
		}
		return true;
	} else if (constraintType == fallThroughSym) {
		int n_roles = childSexp->getNumChildren() - 1;
		for (int j = 0; j < n_roles; j++)
			_fallThroughRoles.push_back(childSexp->getNthChild(j+1)->getValue());
		return true;
	} else {
		if (_pattern != 0)
			throwError(childSexp, "more than one pattern argument found by ArgumentPattern::initializeFromSubexpression()");
		_pattern = parseSexp(childSexp, entityLabels, wordSets);
		return true;
	}
}

bool ArgumentPattern::initializeFromAtom(Sexp *childSexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets) {
	if (childSexp->getValue() == optionalSym) {
		_is_optional = true;
	} else if (_pattern != 0) {
		throwError(childSexp, "more than one pattern argument found by ArgumentPattern::initializeFromAtom()");
	} else {
		_pattern = parseSexp(childSexp, entityLabels, wordSets);
	}
	return true;
}

void ArgumentPattern::disallowCapitalizedRoles() const {
	for (size_t i = 0; i < _roles.size(); i++) {
		std::wstring str = _roles[i].to_string();
		for (size_t j = 0; j < str.length(); j++) {
			if (iswupper(str[j])) {
				std::stringstream error;
				error << "Upper-case character in role element: " << _roles[i].to_debug_string() << "\n";
				dump(error);
				error << "\n";
				throw UnexpectedInputException("ArgumentPattern::disallowCapitalizedRoles()", error.str().c_str());
			}
		}
	}
}

PatternFeatureSet_ptr ArgumentPattern::matchesMentionAndRole(PatternMatcher_ptr patternMatcher, Symbol role, const Mention *mention, bool fall_through_children) {
	if (!matchesRole(role)) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because role '" << role << "' not in pattern\n";
		return PatternFeatureSet_ptr();
	}

	// if mention==0, this was a value mention, so it can't match an argument with a _pattern
	if (!_pattern)
		return makeEmptyFeatureSet();
	else if (mention == 0)
		return PatternFeatureSet_ptr();

	PatternFeatureSet_ptr sfs;
	if (PropMatchingPattern_ptr pat = boost::dynamic_pointer_cast<PropMatchingPattern>(_pattern)) {
		int sent_no = mention->getSentenceNumber();
		Proposition *defProp = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getPropositionSet()->getDefinition(mention->getIndex());
		if (defProp==0) {
			SessionLogger::dbg("BRANDY") << getDebugID() << " Mention " << mention->getUID() << " has no definitional prop\n";
			return PatternFeatureSet_ptr();
		}
		sfs = pat->matchesProp(patternMatcher, sent_no, defProp, fall_through_children, PropStatusManager_ptr());
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning the result of matchesProp(patternMatcher, " << sent_no << ", defProp, " << fall_through_children << ", PropStatusManager_ptr())\n";
	} else if (MentionMatchingPattern_ptr pat = boost::dynamic_pointer_cast<MentionMatchingPattern>(_pattern)) {
		sfs = pat->matchesMention(patternMatcher, mention, fall_through_children);
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning the result of matchesMention(patternMatcher, " << mention->getUID() << ", " << fall_through_children << ")\n";
	} else {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because ArgumentPattern contains a ValueMentionPattern\n";
		return PatternFeatureSet_ptr(); // _pattern is a ValueMentionPattern.
	}

	if (sfs && sfs->getScore() == UNSPECIFIED_SCORE)
		sfs->setScore(getScore());

	return sfs;
}
PatternFeatureSet_ptr ArgumentPattern::matchesValueMentionAndRole(PatternMatcher_ptr patternMatcher, Symbol role, const ValueMention *valueMention) {
	if (!matchesRole(role))
		return PatternFeatureSet_ptr();

	// if valueMention==0, this was a mention, so it can't match an argument with a _pattern
	if (!_pattern)
		return makeEmptyFeatureSet();
	else if (valueMention == 0)
		return PatternFeatureSet_ptr();

	if (ValueMentionMatchingPattern_ptr pat = boost::dynamic_pointer_cast<ValueMentionMatchingPattern>(_pattern)) {
		PatternFeatureSet_ptr sfs = pat->matchesValueMention(patternMatcher, valueMention);
		if (sfs && sfs->getScore() == UNSPECIFIED_SCORE)
			sfs->setScore(getScore());
		return sfs;
	} else {
		return PatternFeatureSet_ptr();
	}
}

bool ArgumentPattern::allowFallThroughToChildren() const {
	if (ArgumentValueMatchingPattern_ptr pat = boost::dynamic_pointer_cast<ArgumentValueMatchingPattern>(_pattern)) {
		return pat->allowFallThroughToChildren();
	} else return true;
}

PatternFeatureSet_ptr ArgumentPattern::matchesArgument(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children) {		
	// Backs off to nested/fall-through arguments only if the "real" argument can't match.
	if (!matchesRole(arg->getRoleSym())) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Role did not match, checking nested arguments.\n";
		return matchesNestedArgument(patternMatcher, sent_no, arg, fall_through_children);
	} else if (!_pattern) {
		SessionLogger::dbg("BRANDY") << getDebugID() << " Returning empty set because ArgumentPattern::_pattern was undefined.\n";
		return makeEmptyFeatureSet();
	} else if (ArgumentValueMatchingPattern_ptr pat = boost::dynamic_pointer_cast<ArgumentValueMatchingPattern>(_pattern)) {
		PatternFeatureSet_ptr sfs = pat->matchesArgumentValue(patternMatcher, sent_no, arg, fall_through_children, PropStatusManager_ptr());
		if (sfs && sfs->getScore() == UNSPECIFIED_SCORE)
			sfs->setScore(getScore());
		if (sfs) {
			SessionLogger::dbg("BRANDY") << getDebugID() << " Returning the result of matchesArgumentValue(patternMatcher, " << sent_no << ", arg, " << fall_through_children << ", PropStatusManager_ptr())\n";
			return sfs;
		}
	}
		
	return matchesNestedArgument(patternMatcher, sent_no, arg, fall_through_children);
}

PatternFeatureSet_ptr ArgumentPattern::matchesNestedArgument(PatternMatcher_ptr patternMatcher, int sent_no, const Argument *arg, bool fall_through_children) {
	if (_fallThroughRoles.size() == 0)
		return PatternFeatureSet_ptr();
	if (std::find(_fallThroughRoles.begin(), _fallThroughRoles.end(), arg->getRoleSym()) == _fallThroughRoles.end())
		return PatternFeatureSet_ptr();
	// NOTE: This will clump together any and all matches it finds in the same PatternFeatureSet. 
	//       I can't think of any better way to implement it.
	//       This is kind of analogous to what happens if we fall through a set and match both children...

	// Find the proposition in which the nested argument will exist
	const Proposition *propToMatch = 0;
	if (arg->getType() == Argument::PROPOSITION_ARG)
		propToMatch = arg->getProposition();
	else if (arg->getType() == Argument::MENTION_ARG)		
		propToMatch = patternMatcher->getDocTheory()->getSentenceTheory(sent_no)->getPropositionSet()->getDefinition(arg->getMentionIndex());

	// If there is no such proposition, we've failed
	if (propToMatch == 0)
		return PatternFeatureSet_ptr();

	PatternFeatureSet_ptr sfs = makeEmptyFeatureSet();
	bool found_some_match = false;
	for (int a = 0; a < propToMatch->getNArgs(); a++) {
		Argument *nestedArg = propToMatch->getArg(a);
		if (matchesRole(nestedArg->getRoleSym())) {					
			if (_pattern) {
				// this should be safe since we checked it in replaceShortcuts, but just in case not...
				ArgumentValueMatchingPattern_ptr pat = boost::dynamic_pointer_cast<ArgumentValueMatchingPattern>(_pattern);
				if (pat) {
					PatternFeatureSet_ptr temp = pat->matchesArgumentValue(patternMatcher, sent_no, nestedArg, fall_through_children, PropStatusManager_ptr());
					if (temp) {
						found_some_match = true;
						sfs->addFeatures(temp);
					}
				}
			} else found_some_match = true; // no _pattern, so no _pattern match necessary					
		}
	}
	if (sfs->getScore() == UNSPECIFIED_SCORE)
		sfs->setScore(getScore());
	if (found_some_match)
		return sfs;
	else return PatternFeatureSet_ptr();
}
	
Pattern_ptr ArgumentPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {
	replaceShortcut<Pattern>(_pattern, refPatterns);
	if (_pattern) {
		ArgumentValueMatchingPattern_ptr pat = boost::dynamic_pointer_cast<ArgumentValueMatchingPattern>(_pattern);
		if (!pat) {
			std::stringstream err;
			err << "ERROR: The pattern inside an ArgumentPattern must be an ArgumentValueMatchingPattern.\n";
			err << _pattern->toDebugString(0) << "\n";
			throw UnexpectedInputException("ArgumentPattern::replaceShortcuts", err.str().c_str());
		}
	}

	return shared_from_this();
}

bool ArgumentPattern::matchesRole(Symbol role) const {
	if (_roles.size() == 0)
		return true;

	return std::find(_roles.begin(), _roles.end(), role) != _roles.end();
}

void ArgumentPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "ArgumentPattern: ";
	if (!getID().is_null()) out << getID();
	out << std::endl;
	for (int i = 0; i < indent; i++) out << " ";
	out << "  Score: " << _score << "\n";

	if (!_roles.empty()) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  roles = {";
		for (size_t j = 0; j < _roles.size(); j++)
			out << (j==0?"":" ") << _roles[j];
		out << "}\n";	
	}

	if (_pattern) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  pattern =\n";
		_pattern->dump(out, indent+2);
	}
}

void ArgumentPattern::getReturns(PatternReturnVecSeq & output) const {
	Pattern::getReturns(output);
	if (_pattern)
		_pattern->getReturns(output);
}
