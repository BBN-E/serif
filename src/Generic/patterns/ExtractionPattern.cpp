// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/ExtractionPattern.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/patterns/ShortcutPattern.h"
#include "Generic/patterns/ArgumentPattern.h"
#include "Generic/patterns/ScoringFactory.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/EventMention.h"
#include <boost/lexical_cast.hpp>
#include "Generic/common/Sexp.h"

// Private symbols
namespace {
	Symbol fulltypeSym = Symbol(L"fulltype");
	Symbol basetypeSym = Symbol(L"basetype");
	Symbol blockSym = Symbol(L"block_args");
	Symbol argsSym = Symbol(L"args");
	Symbol optArgSym = Symbol(L"opt_args");
	Symbol relationSym = Symbol(L"relation");
}

ExtractionPattern::ExtractionPattern(Sexp *sexp, const Symbol::HashSet &entityLabels, const PatternWordSetMap& wordSets)
{
	int nkids = sexp->getNumChildren();
	if (nkids < 2) {
		std::ostringstream ostr;
		ostr << "too few children (" << nkids << ") in ExtractionPattern";
		throwError(sexp, ostr.str().c_str());
	}
}

bool ExtractionPattern::initializeFromSubexpression(Sexp *childSexp, const Symbol::HashSet &entityLabels, 
													const PatternWordSetMap& wordSets) 
{
	Symbol constraintType = childSexp->getFirstChild()->getValue();
	if (Pattern::initializeFromSubexpression(childSexp, entityLabels, wordSets)) {
		return true;
	} else if (constraintType == fulltypeSym) {
		int n_types = childSexp->getNumChildren() - 1;
		for (int j = 0; j < n_types; j++)
			_types.push_back(childSexp->getNthChild(j+1)->getValue());
	} else if (constraintType == basetypeSym) {
		throwError(childSexp, "base type matching not yet implemented for ExtractionPattern");
	} else if (constraintType == blockSym) {
		int n_blocked_args = childSexp->getNumChildren() - 1;
		for (int j=0; j < n_blocked_args; j++)
			_blockedArgs.push_back(parseSexp(childSexp->getNthChild(j+1), entityLabels, wordSets));
	} else if (constraintType == argsSym) {
		int n_args = childSexp->getNumChildren() - 1;
		for (int j = 0; j < n_args; j++)
			_args.push_back(parseSexp(childSexp->getNthChild(j+1), entityLabels, wordSets));
	} else if (constraintType == optArgSym) {
		int n_opt_args = childSexp->getNumChildren() - 1;
		for (int j = 0; j < n_opt_args; j++)
			_optArgs.push_back(parseSexp(childSexp->getNthChild(j+1), entityLabels, wordSets));
	} else {
		logFailureToInitializeFromChildSexp(childSexp);		
		return false;
	}

	return true;
}

Pattern_ptr ExtractionPattern::replaceShortcuts(const SymbolToPatternMap &refPatterns) {
	for (size_t i=0; i<_blockedArgs.size(); ++i)
		replaceShortcut<ArgumentPattern>(_blockedArgs[i], refPatterns);
	for (size_t i=0; i<_args.size(); ++i)
		replaceShortcut<ArgumentMatchingPattern>(_args[i], refPatterns);
	for (size_t i=0; i<_optArgs.size(); ++i)
		replaceShortcut<ArgumentMatchingPattern>(_optArgs[i], refPatterns);

	//move any arguments that are optional to the list of optional arguments
	std::vector<Pattern_ptr> tempArgs;
	for(size_t i = 0; i < _args.size(); ++i) {
		ArgumentPattern_ptr pat = boost::dynamic_pointer_cast<ArgumentPattern>(_args[i]);
		if (pat && pat->isOptional()) {
			_optArgs.push_back(_args[i]);
		} else {
			tempArgs.push_back(_args[i]);
		}
	}
	_args.resize(0);
	std::copy(tempArgs.begin(), tempArgs.end(), std::back_inserter(_args));

	return shared_from_this();
}

bool ExtractionPattern::matchesType(Symbol type) const {
	if (_types.size() == 0) {
		return true;
	} else {
		return (std::find(_types.begin(), _types.end(), type) != _types.end());
	}
}

void ExtractionPattern::dump(std::ostream &out, int indent) const {
	for (int i = 0; i < indent; i++) out << " ";
	out << "ExtractionPattern: ";
	if (!getID().is_null()) out << getID();
	out << std::endl;

	if (!_types.empty()) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  types = {";
		for (size_t j = 0; j < _types.size(); j++)
			out << (j==0?"":" ") << _types[j];
		out << "}\n";	
	}

	if (!_blockedArgs.empty()) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  blocked_args = {\n";
		for (size_t j = 0; j < _blockedArgs.size(); j++)
			_blockedArgs[j]->dump(out, indent+4);		
		for (int i = 0; i < indent; i++) out << " ";
		out << "}\n";
	}

	if (!_args.empty()) {
		for (int i = 0; i < indent; i++) out << " ";
		out << "  args = {\n";
		for (size_t j = 0; j < _args.size(); j++)
			_args[j]->dump(out, indent+4);		
		for (int i = 0; i < indent; i++) out << " ";
		out << "}\n";
	}

	if(!_optArgs.empty()) {
		for (int i = 0; i < indent; i++) out << " ";
		out << " args = {\n";
		for (size_t j = 0; j < _optArgs.size(); j++)
			_optArgs[j]->dump(out, indent+4);
		for (int i = 0; i < indent; i++) out << " ";
		out << "}\n";
	}
}

/**
 * Redefinition of parent class's virtual method that returns a vector of 
 * pointers to PatternReturn objects for a given pattern.
 *
 * @param pointer to vector of pointers to PatternReturn objects for a given pattern
 *
 * @author afrankel@bbn.com
 * @date 2010.10.20
 **/
void ExtractionPattern::getReturns(PatternReturnVecSeq & output) const {
	Pattern::getReturns(output);
	for (size_t i = 0; i < _args.size(); ++i) {
		if (_args[i]) {
			_args[i]->getReturns(output);
		}
	}
	for (size_t i = 0; i < _optArgs.size(); ++i) {
		if(_optArgs[i]) {
			_optArgs[i]->getReturns(output);
		}
	}
}













