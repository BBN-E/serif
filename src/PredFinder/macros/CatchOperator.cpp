/**
 * Operator for catching relations that should have been transformed by ElfInference
 * and should not have entered P-ELF in their original form.
 *
 * @file CatchOperator.cpp
 * @author afrankel@bbn.com
 * @date 2011.03.14
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "CatchOperator.h"
#include "boost/foreach.hpp"
#include "boost/make_shared.hpp"
#include "PredFinder/elf/ElfDocument.h"
#include "PredFinder/elf/ElfRelation.h"

/**
 * Reads the catch operator in some macro
 * context from the specified S-expression.
 *
 * @param sexp The read S-expression.
 *
 * @author afrankel@bbn.com
 * @date 2011.03.14
 **/
CatchOperator::CatchOperator(Sexp* sexp, ReadingMacroExpressionShortcutMap shortcuts) : ReadingMacroOperator(sexp) {
	// Local error messages
	std::stringstream error;

	// Make sure we have an S-expression worth reading
	if (sexp->getNumChildren() < 2) {
		error << "Ill-formed catch operator: needs 2+ top-level nodes; got " << sexp->getNumChildren();
		error << " " << sexp->to_debug_string();
		throw UnexpectedInputException("CatchOperator::CatchOperator(Sexp*)", error.str().c_str());
	}

	// Make sure this is a catch expression
	if (sexp->getFirstChild()->isList() || sexp->getFirstChild()->getValue() !=  ReadingMacroOperator::CATCH_SYM) {
		error << "Ill-formed catch operator: need catch expression; got ";
		error << " " << sexp->getFirstChild()->to_debug_string();
		throw UnexpectedInputException("CatchOperator::CatchOperator(Sexp*)", error.str().c_str());
	}

	// Check if we need to start reading subexpressions after a shortcut expression
	int child = 1;
	if (_shortcut_name != Symbol())
		child++;

	// Get the required predicate subexpression
	if (sexp->getNthChild(child)->isAtom()) {
		// Get the referenced predicate shortcut
		ReadingMacroExpression_ptr expression = ReadingMacroExpression::from_shortcut(shortcuts, sexp, sexp->getNthChild(child)->getValue(), ReadingMacroParameter::PREDICATE_SYM);
		_predicate = boost::static_pointer_cast<PredicateParameter>(expression);
	} else {
		// Read the predicate expression
		_predicate = boost::make_shared<PredicateParameter>(sexp->getNthChild(child));
	}

	// Read the role subexpressions (zero or more)
	for (child++; child < sexp->getNumChildren(); child++) {
		if (sexp->getNthChild(child)->isAtom()) {
			// Get the referenced predicate shortcut
			ReadingMacroExpression_ptr expression = ReadingMacroExpression::from_shortcut(shortcuts, sexp, sexp->getNthChild(child)->getValue(), ReadingMacroParameter::ROLE_SYM);
			_roles.insert(boost::static_pointer_cast<RoleParameter>(expression));
		} else {
			// Read the role expression
			_roles.insert(boost::make_shared<RoleParameter>(sexp->getNthChild(child)));
		}
	}
}


/**
 * Implements ReadingMacroOperator::matches.
 *
 * Checks that the specified relation matches 
 * this operator.
 *
 * @param relation The ElfRelation being checked
 * for possible consumption.
 *
 * @author afrankel@bbn.com
 * @date 2011.03.14
 **/
bool CatchOperator::matches(ElfRelation_ptr relation) const {
	// Make sure this is a matching relation by predicate
	if (_predicate->matches(relation)) {
		// If no role parameters were specified, just match since the predicate matched
		if (_roles.size() == 0)
			return true;

		// Loop over this catch operator's role parameters, checking each one
		size_t matched_roles = 0;
		BOOST_FOREACH(RoleParameter_ptr role, _roles) {
			// Ignore roles that refer to a generated individual (shouldn't be used in this operator)
			if (role->get_input_name() == L"individual")
				continue;

			// Get the arg for this role, if any
			ElfRelationArg_ptr arg = relation->get_arg(role->get_input_name());
			if (arg.get() != NULL) {
				// Count this as a match
				matched_roles++;
			} else if (!role->is_optional()) {
				// Required role not found, no match
				return false;
			}
			// We do want roles marked 'delete' to be matched, but we don't need to do anything with them.
		}

		// Make sure we matched everything (that is, the relation didn't have unknown roles)
		if (matched_roles == relation->get_args().size())
			return true;
	}

	// No match
	return false;
}

/**
 * Implements ReadingMacroOperator::apply.
 *
 * Here we issue a message (warning or error), since by the point at which
 * we call this, we should not be encountering any relations that trigger a "catch" macro
 * operator.
 *
 * @param document The ELF document, which will be unchanged.
 *
 * @author afrankel@bbn.com
 * @date 2011.03.14
 **/
void CatchOperator::apply(ElfDocument_ptr document) const {
	// Loop through the document's relations, checking for matching relations
	std::set<ElfRelation_ptr> relations = document->get_relations();
	BOOST_FOREACH(ElfRelation_ptr relation, relations) {
		// Make sure this is a matching relation
		if (matches(relation)) {
			SessionLogger::warn("LEARNIT") << "Encountered relation <" 
				<< _predicate->get_name() << "> "
				<< "(" << _source_id << "), which should have been transformed by ElfInference.";
		}
	}
}
