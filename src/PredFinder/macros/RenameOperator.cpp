/**
 * Class that performs a rename operation on a
 * single ElfRelation's rename and roles.
 *
 * @file RenameOperator.cpp
 * @author nward@bbn.com
 * @date 2010.10.07
 **/

#include "Generic/common/leak_detection.h"
#include "RenameOperator.h"
#include "boost/make_shared.hpp"
#include "PredFinder/elf/ElfDocument.h"
#include "PredFinder/elf/ElfRelation.h"
#include "boost/foreach.hpp"

/**
 * Reads the rename operator in some macro
 * context from the specified S-expression.
 *
 * @param sexp The read S-expression.
 * @param shortcuts A map storing parameters/operators
 * by shortcut name; used to look up predicate/relation
 * shortcuts which can be used in this operator.
 *
 * @author nward@bbn.com
 * @date 2010.10.06
 **/
RenameOperator::RenameOperator(Sexp* sexp, ReadingMacroExpressionShortcutMap shortcuts) : ReadingMacroOperator(sexp) {
	// Local error messages
	std::stringstream error;

	// Make sure we have an S-expression worth reading
	if (sexp->getNumChildren() < 3 || sexp->getNumChildren() > 4) {
		error << "Ill-formed rename operator: need 3-4 top-level nodes; got " << sexp->getNumChildren();
		error << " " << sexp->to_debug_string();
		throw UnexpectedInputException("RenameOperator::RenameOperator(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
	}

	// Make sure this is a rename expression
	if (sexp->getFirstChild()->isList() || sexp->getFirstChild()->getValue() !=  ReadingMacroOperator::RENAME_SYM) {
		error << "Ill-formed rename operator: need rename expression; got ";
		error << " " << sexp->getFirstChild()->to_debug_string();
		throw UnexpectedInputException("RenameOperator::RenameOperator(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
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

	// Get the required relation subexpression
	child++;
	if (sexp->getNthChild(child)->isAtom()) {
		// Get the referenced relation shortcut
		ReadingMacroExpression_ptr expression = ReadingMacroExpression::from_shortcut(shortcuts, sexp, sexp->getNthChild(child)->getValue(), ReadingMacroParameter::RELATION_SYM);
		_relation = boost::static_pointer_cast<RelationParameter>(expression);
	} else {
		// Read the relation expression
		_relation = boost::make_shared<RelationParameter>(sexp->getNthChild(child), shortcuts);
	}
}


/**
 * Implements ReadingMacroOperator::matches.
 *
 * Checks that the specified relation can be
 * consumed by this operator.
 *
 * @param relation The ElfRelation being checked
 * for possible consumption.
 *
 * @author nward@bbn.com
 * @date 2010.11.10
 **/
bool RenameOperator::matches(ElfRelation_ptr relation) const {
	// Make sure this is a matching relation by predicate and roles
	return (_predicate->matches(relation) && _relation->matches(relation));
}

/**
 * Implements ReadingMacroOperator::apply.
 *
 * Renames any matching ElfRelations in this
 * document, replacing the match with the
 * renamed version. If there are no matches,
 * the result is a no-op and the document is
 * unchanged.
 *
 * @param document The ELF document, which will be
 * transformed in-place.
 *
 * @author nward@bbn.com
 * @date 2010.10.08
 **/
void RenameOperator::apply(ElfDocument_ptr document) const {
	// Loop through the document's relations, checking for matching relations
	std::set<ElfRelation_ptr> relations = document->get_relations();
	BOOST_FOREACH(ElfRelation_ptr relation, relations) {
		// Make sure this is a matching relation
		if (matches(relation)) {
			// Replace the predicate
			relation->set_name(_relation->get_predicate()->get_name());
			relation->add_source(_source_id);

			// Replace each matching role
			BOOST_FOREACH(RoleParameter_ptr role, _relation->get_roles()) {
				// Get the arg for this role, if any (optionality may allow match without it)
				ElfRelationArg_ptr arg = relation->get_arg(role->get_input_name());
				if (arg.get() != NULL) {
					if (role->is_delete()) {
						relation->remove_argument(arg);
					} else {
						arg->set_role(role->get_output_name());
					}
				}
			}
		}
	}
}

/**
 * Implements ReadingMacroOperator::retrieve_predicates_generated().
 *
 * @author afrankel@bbn.com
 * @date 2011.07.14
 **/
void RenameOperator::retrieve_predicates_generated(std::set<std::wstring> & predicate_names) const {
	std::wstring pred_name = _relation->get_predicate()->get_name();
	predicate_names.insert(pred_name);
}
