/**
 * Class that generates a bridging individual
 * for an input ElfRelation and associates it with
 * roles in each of the output ElfRelations.
 *
 * @file SplitOperator.cpp
 * @author nward@bbn.com
 * @date 2010.10.07
 **/

#include "Generic/common/leak_detection.h"
#include "SplitOperator.h"
#include "boost/make_shared.hpp"
#include "boost/foreach.hpp"
#include "PredFinder/elf/ElfDocument.h"
#include "PredFinder/elf/ElfRelation.h"

/**
 * Reads the split operator in some macro
 * context from the specified S-expression.
 *
 * @param sexp The read S-expression.
 * @param shortcuts A map storing parameters/operators
 * by shortcut name; used to look up predicate/individual/relation
 * shortcuts which can be used in this operator.
 *
 * @author nward@bbn.com
 * @date 2010.10.07
 **/
SplitOperator::SplitOperator(Sexp* sexp, ReadingMacroExpressionShortcutMap shortcuts) : ReadingMacroOperator(sexp) {
	// Local error messages
	std::stringstream error;

	// Make sure we have an S-expression worth reading
	if (sexp->getNumChildren() < 4) {
		error << "Ill-formed split operator: need at least 4 top-level nodes; got " << sexp->getNumChildren();
		error << " " << sexp->to_debug_string();
		throw UnexpectedInputException("SplitOperator::SplitOperator(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
	}

	// Make sure this is a generate individual expression
	if (sexp->getFirstChild()->isList() || sexp->getFirstChild()->getValue() !=  ReadingMacroOperator::SPLIT_SYM) {
		error << "Ill-formed split operator: need split expression; got ";
		error << " " << sexp->getFirstChild()->to_debug_string();
		throw UnexpectedInputException("SplitOperator::SplitOperator(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
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

	// Read the relation subexpressions (at least one, probably not more than two)
	_preserve_input_relation = false;
	for (child++; child < sexp->getNumChildren(); child++) {
		// Get this child
		Sexp* relation_sexp = sexp->getNthChild(child);
		if (relation_sexp->isAtom()) {
			// Get the referenced relation shortcut
			ReadingMacroExpression_ptr expression = ReadingMacroExpression::from_shortcut(shortcuts, sexp, relation_sexp->getValue(), ReadingMacroParameter::RELATION_SYM);
			_relations.insert(boost::static_pointer_cast<RelationParameter>(expression));
		} else {
			// Check if this is the input-relation flag
			if (relation_sexp->getNumChildren() == 1 && relation_sexp->getFirstChild()->isAtom() && relation_sexp->getFirstChild()->getValue() == Symbol(L"input-relation")) {
				_preserve_input_relation = true;
			} else {
				// Read the relation expression
				_relations.insert(boost::make_shared<RelationParameter>(relation_sexp, shortcuts));
			}
		}
	}

	// Make sure we had at least one relation
	if ((!_preserve_input_relation && _relations.size() < 2) || (_preserve_input_relation && _relations.size() < 1)) {
		error << "Ill-formed split operator: need at least 2 relation subexpression; got ";
		error << " " << sexp->to_debug_string();
		throw UnexpectedInputException("SplitOperator::SplitOperator(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
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
bool SplitOperator::matches(ElfRelation_ptr relation) const {
	// Make sure this is a matching relation by predicate
	if (_predicate->matches(relation)) {
		// Make sure the input roles are all consumed
		bool roles_match = true;
		BOOST_FOREACH(RelationParameter_ptr relation_parameter, _relations) {
			if (!relation_parameter->matches(relation, true)) {
				roles_match = false;
				break;
			}
		}

		// Done
		return roles_match;
	} else {
		// No match
		return false;
	}
}

/**
 * Implements ReadingMacroOperator::apply.
 *
 * Splits any matching ElfRelations in this
 * document, replacing the match with the
 * resulting ElfRelations. If there are no
 * matches, the result is a no-op and the
 * document is unchanged.
 *
 * @param document The ELF document, which will be
 * transformed in-place.
 *
 * @author nward@bbn.com
 * @date 2010.10.08
 **/
void SplitOperator::apply(ElfDocument_ptr document) const {
	// Loop through the document's relations, checking for matching relations
	std::set<ElfRelation_ptr> relations = document->get_relations();
	std::set<ElfRelation_ptr> relations_to_remove;
	BOOST_FOREACH(ElfRelation_ptr relation, relations) {
		// Make sure this is a matching relation
		if (matches(relation)) {
			// Generate each of the relations from the split
			IndividualParameter_ptr null_individual;
			std::set<ElfRelation_ptr> split_relations;
			BOOST_FOREACH(RelationParameter_ptr relation_parameter, _relations) {
				ElfRelation_ptr split_relation = relation_parameter->to_elf_relation(document, relation, null_individual);
				split_relation->add_source(_source_id);
				if (split_relation->get_args().size() >= 2)
					split_relations.insert(split_relation);
			}
			document->add_relations(split_relations);
		
			// Mark the original relation that we split for removal,
			// unless it was marked for preservation.
			//   Don't want to invalidate our iterators
			if (!_preserve_input_relation)
				relations_to_remove.insert(relation);
		}
	}

	// Loop through the relations we split and remove them from the document
	document->remove_relations(relations_to_remove);
}

/**
 * Implements ReadingMacroOperator::retrieve_predicates_generated().
 *
 * @author afrankel@bbn.com
 * @date 2011.07.14
 **/
void SplitOperator::retrieve_predicates_generated(std::set<std::wstring> & predicate_names) const {
	BOOST_FOREACH(RelationParameter_ptr rel, _relations) {
		std::wstring pred_name = rel->get_predicate()->get_name();
		predicate_names.insert(pred_name);
	}
}
