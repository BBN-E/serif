/**
 * Class representing an ElfRelation name declaration
 * as used in all subclasses of ReadingMacroOperator.
 *
 * @file PredicateParameter.cpp
 * @author nward@bbn.com
 * @date 2010.10.06
 **/

#include "Generic/common/leak_detection.h"
#include "PredicateParameter.h"
#include "PredFinder/elf/ElfDocument.h"
#include "PredFinder/elf/ElfRelation.h"
#pragma warning(disable: 4996)

/**
 * Reads the predicate parameter in some operator
 * context from the specified S-expression.
 *
 * @param sexp The read S-expression.
 *
 * @author nward@bbn.com
 * @date 2010.10.06
 **/
PredicateParameter::PredicateParameter(Sexp* sexp) : ReadingMacroParameter(sexp) {
	// Local error messages
	std::stringstream error;

	// Make sure we have an S-expression worth reading
	if (sexp->getNumChildren() < 2 || sexp->getNumChildren() > 3) {
		error << "Ill-formed predicate parameter: need 2-3 top-level nodes; got " << sexp->getNumChildren();
		error << " " << sexp->to_debug_string();
		throw UnexpectedInputException("PredicateParameter::PredicateParameter(Sexp*)", error.str().c_str());
	}

	// Make sure this is a predicate expression
	if (sexp->getFirstChild()->isList() || sexp->getFirstChild()->getValue() !=  ReadingMacroParameter::PREDICATE_SYM) {
		error << "Ill-formed predicate parameter: need predicate expression; got ";
		error << " " << sexp->getFirstChild()->to_debug_string();
		throw UnexpectedInputException("PredicateParameter::PredicateParameter(Sexp*)", error.str().c_str());
	}

	// Get the name specification
	if (sexp->getNthChild(sexp->getNumChildren() - 1)->isAtom())
		_predicate_name = sexp->getNthChild(sexp->getNumChildren() - 1)->getValue().to_string();
	else {
		error << "Ill-formed predicate parameter: need ontology type atom; got ";
		error << " " << sexp->getNthChild(sexp->getNumChildren() - 1)->to_debug_string();
		throw UnexpectedInputException("PredicateParameter::PredicateParameter(Sexp*)", error.str().c_str());
	}
}

/**
 * Comparison method; makes sure the predicate name
 * matches the relation name.
 *
 * @return True if the strings are identical.
 *
 * @author nward@bbn.com
 * @date 2010.10.15
 **/
bool PredicateParameter::matches(ElfRelation_ptr relation) { return _predicate_name == relation->get_name(); }
