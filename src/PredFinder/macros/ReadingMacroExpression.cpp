/**
 * Abstract class interface encapsulating functionality
 * in common between ReadingMacroOperators and
 * ReadingMacroParameters.
 *
 * @file ReadingMacroExpression.cpp
 * @author nward@bbn.com
 * @date 2010.10.05
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/common/UnexpectedInputException.h"
#include "ReadingMacroExpression.h"
#include "ReadingMacroParameter.h"
#include "IndividualParameter.h"
#include "PredicateParameter.h"
#include "RelationParameter.h"
#include "RoleParameter.h"
#include "ReadingMacroOperator.h"
#include "BinarizeOperator.h"
#include "CatchOperator.h"
#include "DeleteOperator.h"
#include "GenerateIndividualOperator.h"
#include "MakeTripleOperator.h"
#include "RenameOperator.h"
#include "SplitOperator.h"
#include "boost/make_shared.hpp"

/**
 * Superclass constructor, shouldn't be invoked directly.
 * Reads the S-expression just enough to extract the
 * expression name and optionally the shortcut name.
 *
 * @param sexp The read S-expression.
 *
 * @author nward@bbn.com
 * @date 2010.10.05
 **/
ReadingMacroExpression::ReadingMacroExpression(const Sexp* sexp) {
	// Local error messages
	std::stringstream error;

	// Make sure we have an S-expression worth reading
	if (sexp->getNumChildren() < 1) {
		error << "Ill-formed reading macro expression: need at least 1 top-level node; got " << sexp->getNumChildren();
		error << " (" << sexp->to_debug_string() << ")";
		throw UnexpectedInputException("ReadingMacroExpression::ReadingMacroExpression()", error.str().c_str());
	}

	// Get the expression name
	if (sexp->getFirstChild()->isAtom())
		_expression_name = sexp->getFirstChild()->getValue();
	else {
		error << "Ill-formed reading macro expression: need expression name atom; got ";
		error << " (" << sexp->getFirstChild()->to_debug_string() << ")";
		throw UnexpectedInputException("ReadingMacroExpression::ReadingMacroExpression()", error.str().c_str());
	}

	// Check if this is a shortcut definition
	if (sexp->getNumChildren() >= 2)
		if (sexp->getSecondChild()->isList())
			if (sexp->getSecondChild()->getNumChildren() == 2)
				if (sexp->getSecondChild()->getFirstChild()->isAtom() && sexp->getSecondChild()->getFirstChild()->getValue() == Symbol(L"shortcut"))
					_shortcut_name = sexp->getSecondChild()->getSecondChild()->getValue();
}

/**
 * Determines the appropriate subclass constructor to use
 * and returns the expression object constructed from
 * the specified S-expression.
 *
 * @param sexp The read S-expression.
 * @param shortcuts A map of read shortcuts that can be
 * referenced by shortcuts in the specified expression.
 * @return The constructed expression object
 *
 * @author nward@bbn.com
 * @date 2010.10.05
 **/
ReadingMacroExpression_ptr ReadingMacroExpression::from_s_expression(Sexp* sexp, ReadingMacroExpressionShortcutMap shortcuts) {
	// Local error messages
	std::stringstream error;

	// Make sure we have an S-expression worth reading
	if (sexp->getNumChildren() < 1) {
		error << "Ill-formed reading macro expression: need at least 1 top-level node; got " << sexp->getNumChildren();
		error << " (" << sexp->to_debug_string() << ")";
		throw UnexpectedInputException("ReadingMacroExpression::from_s_expression(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
	}

	// Get the expression name for determining subclass
	Symbol expression_name;
	if (sexp->getFirstChild()->isAtom())
		expression_name = sexp->getFirstChild()->getValue();
	else {
		error << "Ill-formed reading macro expression: need expression name atom; got ";
		error << " (" << sexp->getFirstChild()->to_debug_string() << ")";
		throw UnexpectedInputException("ReadingMacroExpression::from_s_expression(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
	}

	// Determine which expression type this is and construct the appropriate subclass
	if (expression_name == ReadingMacroParameter::INDIVIDUAL_SYM) {
		return boost::make_shared<IndividualParameter>(sexp);
	} else if (expression_name == ReadingMacroParameter::PREDICATE_SYM) {
		return boost::make_shared<PredicateParameter>(sexp);
	} else if (expression_name == ReadingMacroParameter::RELATION_SYM) {
		return boost::make_shared<RelationParameter>(sexp, shortcuts);
	} else if (expression_name == ReadingMacroParameter::ROLE_SYM) {
		return boost::make_shared<RoleParameter>(sexp);
	} else if (expression_name == ReadingMacroOperator::BINARIZE_SYM) {
		return boost::make_shared<BinarizeOperator>(sexp);
	} else if (expression_name == ReadingMacroOperator::CATCH_SYM) {
		return boost::make_shared<CatchOperator>(sexp, shortcuts);
	} else if (expression_name == ReadingMacroOperator::DELETE_SYM) {
		return boost::make_shared<DeleteOperator>(sexp, shortcuts);
	} else if (expression_name == ReadingMacroOperator::GENERATE_INDIVIDUAL_SYM) {
		return boost::make_shared<GenerateIndividualOperator>(sexp, shortcuts);
	} else if (expression_name == ReadingMacroOperator::MAKE_TRIPLE_SYM) {
		return boost::make_shared<MakeTripleOperator>(sexp);
	} else if (expression_name == ReadingMacroOperator::RENAME_SYM) {
		return boost::make_shared<RenameOperator>(sexp, shortcuts);
	} else if (expression_name == ReadingMacroOperator::SPLIT_SYM) {
		return boost::make_shared<SplitOperator>(sexp, shortcuts);
	} else {
		std::stringstream error;
		error << "Invalid macro expression '" << expression_name << "'";
		throw UnexpectedInputException(
			"ReadingMacroExpression::from_s_expression(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
	}
}

/**
 * Looks up the specified shortcut name in the shortcuts map
 * and returns the matching expression. Optionally restricts
 * the expression subclass.
 *
 * Abstraction to reduce duplicated code in expression subclass
 * S-expression constructors.
 *
 * @param shortcuts A map of read shortcuts that we're searching.
 * @param shortcut_name The shortcut to find by name.
 * @param expression_name Optional restriction on the shortcut
 * expression type, to handle shortcut collisions.
 * @return The matching shortcut expression, if any; otherwise
 * throws an exception.
 *
 * @author nward@bbn.com
 * @date 2010.10.07
 **/
ReadingMacroExpression_ptr ReadingMacroExpression::from_shortcut(const ReadingMacroExpressionShortcutMap shortcuts, const Sexp* containing_sexp, const Symbol& shortcut_name, const Symbol& expression_name) {
	// Local error messages
	std::stringstream error;

	// Get the referenced shortcut, if present
	ReadingMacroExpressionShortcutMap::const_iterator shortcut_i = shortcuts.find(shortcut_name);
	if (shortcut_i != shortcuts.end()) {
		// Make sure the shortcut is a relation parameter
		if (expression_name == Symbol() || shortcut_i->second->get_expression() == expression_name) {
			return shortcut_i->second;
		} else {
			error << "Ill-formed macro expression: need " << expression_name << " shortcut; got ";
			error << shortcut_i->second->get_expression() << " for shortcut " << shortcut_name;
			error << " in " << containing_sexp->to_debug_string();
			throw UnexpectedInputException("ReadingMacroExpression::from_shortcut(ReadingMacroExpressionShortcutMap,Sexp*,Symbol,Symbol)", error.str().c_str());
		}
	} else {
		error << "Ill-formed macro expression: shortcut '" << shortcut_name << "' not defined; used";
		error << " in " << containing_sexp->to_debug_string();
		throw UnexpectedInputException("ReadingMacroExpression::from_shortcut(ReadingMacroExpressionShortcutMap,Sexp*,Symbol,Symbol)", error.str().c_str());
	}
}
