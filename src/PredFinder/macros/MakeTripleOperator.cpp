/**
 * Class that provides a convenience constructor for
 * a common make-triple operation: generating RDF triples
 * from punned ElfRelations.
 *
 * @file MakeTripleOperator.h
 * @author nward@bbn.com
 * @date 2010.10.27
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/common/UnexpectedInputException.h"
#include "MakeTripleOperator.h"
#include "ReadingMacroSet.h"
#include "boost/make_shared.hpp"

/**
 * Reads the make-triple operator in some macro
 * context from the specified S-expression.
 *
 * @param sexp The read S-expression.
 * @param shortcuts A map storing parameters/operators
 * by shortcut name; used to look up predicate/relation
 * shortcuts which can be used in this operator.
 *
 * @author nward@bbn.com
 * @date 2010.10.27
 **/
MakeTripleOperator::MakeTripleOperator(Sexp* sexp) : RenameOperator(sexp) {
	// Local error messages
	std::stringstream error;

	// Make sure we have an S-expression worth reading
	if (sexp->getNumChildren() < 4 || sexp->getNumChildren() > 5) {
		error << "Ill-formed make-triple operator: need 4-5 top-level nodes; got " << sexp->getNumChildren();
		error << " " << sexp->to_debug_string();
		throw UnexpectedInputException("MakeTripleOperator::MakeTripleOperator(Sexp*)", error.str().c_str());
	}

	// Make sure this is a make-triple expression
	if (sexp->getFirstChild()->isList() || sexp->getFirstChild()->getValue() !=  ReadingMacroOperator::MAKE_TRIPLE_SYM) {
		error << "Ill-formed make-triple operator: need make-triple expression; got ";
		error << " " << sexp->getFirstChild()->to_debug_string();
		throw UnexpectedInputException("MakeTripleOperator::MakeTripleOperator(Sexp*)", error.str().c_str());
	}

	// Check if we need to start reading subexpressions after a shortcut expression
	int child = 1;
	if (_shortcut_name != Symbol())
		child++;

	// Get the required input predicate name
	if (sexp->getNthChild(child)->isAtom()) {
		_predicate = boost::make_shared<PredicateParameter>(std::wstring(sexp->getNthChild(child)->getValue().to_string()));
	} else {
		error << "Ill-formed make-triple operator: need input predicate name; got ";
		error << " " << sexp->getNthChild(child)->to_debug_string();
		throw UnexpectedInputException("MakeTripleOperator::MakeTripleOperator(Sexp*)", error.str().c_str());
	}

	// Get required input subject role name
	std::set<RoleParameter_ptr> roles;
	child++;
	if (sexp->getNthChild(child)->isAtom()) {
		roles.insert(boost::make_shared<RoleParameter>(std::wstring(sexp->getNthChild(child)->getValue().to_string()), L"rdf:subject"));
	} else {
		error << "Ill-formed make-triple operator: need input subject role name; got ";
		error << " " << sexp->getNthChild(child)->to_debug_string();
		throw UnexpectedInputException("MakeTripleOperator::MakeTripleOperator(Sexp*)", error.str().c_str());
	}

	// Get required input object role name
	child++;
	if (sexp->getNthChild(child)->isAtom()) {
		roles.insert(boost::make_shared<RoleParameter>(std::wstring(sexp->getNthChild(child)->getValue().to_string()), L"rdf:object"));
	} else {
		error << "Ill-formed make-triple operator: need input object role name; got ";
		error << " " << sexp->getNthChild(child)->to_debug_string();
		throw UnexpectedInputException("MakeTripleOperator::MakeTripleOperator(Sexp*)", error.str().c_str());
	}

	// Create the output predicate to be used in the RenameOperator's RelationParameter
	std::wstring output_predicate_name = _predicate->get_name();
	output_predicate_name = ReadingMacroSet::domain_prefix + L":" + output_predicate_name.substr(output_predicate_name.find_first_of(L":") + 1);
	PredicateParameter_ptr output_predicate = boost::make_shared<PredicateParameter>(output_predicate_name);

	// Create the output relation from the positional subject/object roles and output predicate
	_relation = boost::make_shared<RelationParameter>(output_predicate, roles);
}
