/**
 * Class representing an ElfRelationArg role declaration
 * as used in RelationParameters.
 *
 * @file RoleParameter.cpp
 * @author nward@bbn.com
 * @date 2010.10.06
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/common/UnexpectedInputException.h"
#include "RoleParameter.h"

Symbol RoleParameter::DELETE_SYM(L"delete");
Symbol RoleParameter::OPTIONAL_SYM(L"optional");
Symbol RoleParameter::SKOLEM_SYM(L"skolem");

/**
 * Reads the role parameter in some operator
 * context from the specified S-expression.
 *
 * @param sexp The read S-expression.
 *
 * @author nward@bbn.com
 * @date 2010.10.06
 **/
RoleParameter::RoleParameter(Sexp* sexp) : ReadingMacroParameter(sexp) {
	// Local error messages
	std::stringstream error;

	// Make sure we have an S-expression worth reading
	if (sexp->getNumChildren() < 2 || sexp->getNumChildren() > 5) {
		error << "Ill-formed role parameter: need 2-5 top-level nodes; got " << sexp->getNumChildren();
		error << " " << sexp->to_debug_string();
		throw UnexpectedInputException("RoleParameter::RoleParameter(Sexp*)", error.str().c_str());
	}

	// Make sure this is a role expression
	if (sexp->getFirstChild()->isList() || sexp->getFirstChild()->getValue() !=  ReadingMacroParameter::ROLE_SYM) {
		error << "Ill-formed role parameter: need role expression; got";
		error << " " << sexp->getFirstChild()->to_debug_string();
		throw UnexpectedInputException("RoleParameter::RoleParameter(Sexp*)", error.str().c_str());
	}

	// Check if we need to start reading subexpressions after a shortcut expression
	int child = 1;
	if (_shortcut_name != Symbol())
		child++;

	// Get the input role name specification
	if (sexp->getNthChild(child)->isAtom()) {
		_input_role_name = sexp->getNthChild(child)->getValue().to_string();
	} else {
		error << "Ill-formed role parameter: need predicate input role atom; got ";
		error << " " << sexp->getNthChild(child)->to_debug_string();
		throw UnexpectedInputException("RoleParameter::RoleParameter(Sexp*)", error.str().c_str());
	}

	// Not optional unless we see otherwise
	_optional = false;
	// Do not delete unless we see otherwise
	_delete = false;

	// Loop over the remaining children
	//   This reduces duplicated code, but makes the if checks for each "slot" kinda weird
	for (child++; child < sexp->getNumChildren(); child++) {
		// Check if we're looking for an output role
		if ((child == sexp->getNumChildren() - 2 && sexp->getNthChild(sexp->getNumChildren() - 1)->isList()) ||
			(child == sexp->getNumChildren() - 1 && sexp->getNthChild(sexp->getNumChildren() - 1)->isAtom())) {
			if (sexp->getNthChild(child)->isAtom()) {
				_output_role_name = sexp->getNthChild(child)->getValue().to_string();
			} else {
				error << "Ill-formed role parameter: need predicate output role atom; got ";
				error << " " << sexp->getNthChild(child)->to_debug_string();
				throw UnexpectedInputException("RoleParameter::RoleParameter(Sexp*)", error.str().c_str());
			}
		} else if (child == sexp->getNumChildren() - 1 && sexp->getNthChild(child)->isList()) {
			// Make sure this is a valid 'optional'/'delete'/'skolem' type specification
			Sexp* skolem_sexp = sexp->getNthChild(child);
			bool is_valid(false);
			if (skolem_sexp->getNumChildren() == 1 && skolem_sexp->getFirstChild()->isAtom()) {
				if (skolem_sexp->getFirstChild()->getValue() == OPTIONAL_SYM) {
					// Input role is optional
					_optional = true;
					is_valid = true;
				} else if (skolem_sexp->getFirstChild()->getValue() == DELETE_SYM) {
					_delete = true;
					is_valid = true;
				}
			} else if (skolem_sexp->getNumChildren() == 2 && skolem_sexp->getFirstChild()->isAtom() && skolem_sexp->getFirstChild()->getValue() == SKOLEM_SYM) {
				if (skolem_sexp->getSecondChild()->isAtom()) {
					// Generate skolem with specified type if input role not present
					_optional = true;
					_skolem_type = skolem_sexp->getSecondChild()->getValue().to_string();
					is_valid = true;
				} else {
					error << "Ill-formed role parameter: need skolem type atom; got ";
					error << " " << skolem_sexp->to_debug_string();
					throw UnexpectedInputException("RoleParameter::RoleParameter(Sexp*)", error.str().c_str());
				}
			} 
			if (!is_valid) {
				error << "Ill-formed role parameter: need 1-node 'optional'/'delete' or 2-node 'skolem' expression; "
				      << "got " << skolem_sexp->getNumChildren();
				error << " " << skolem_sexp->to_debug_string();
				throw UnexpectedInputException("RoleParameter::RoleParameter(Sexp*)", error.str().c_str());
			}
		} else {
			error << "Ill-formed role parameter: need predicate output role atom and/or 'optional'/'delete'/'skolem' expression; got ";
			error << " " << sexp->getNthChild(child)->to_debug_string();
			throw UnexpectedInputException("RoleParameter::RoleParameter(Sexp*)", error.str().c_str());
		}
	}

	// Set the output role to identity if there was no rename
	if (_output_role_name == L"")
		_output_role_name = _input_role_name;
}
