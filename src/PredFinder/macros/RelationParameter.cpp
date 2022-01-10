/**
 * Class representing an ElfRelation declaration
 * as used in all subclasses of ReadingMacroOperator.
 *
 * @file RelationParameter.cpp
 * @author nward@bbn.com
 * @date 2010.10.06
 **/

#include "Generic/common/leak_detection.h"
#include "ReadingMacroOperator.h"
#include "RelationParameter.h"
#include "RoleParameter.h"
#include "PredFinder/elf/ElfDocument.h"
#include "PredFinder/elf/ElfRelation.h"
#include "boost/foreach.hpp"
#include "boost/make_shared.hpp"

/**
 * Reads the relation parameter in some operator
 * context from the specified S-expression.
 *
 * @param sexp The read S-expression.
 * @param shortcuts A map storing parameters/operators
 * by shortcut name; used to look up predicate/role
 * shortcuts which can be used in this parameter.
 *
 * @author nward@bbn.com
 * @date 2010.10.06
 **/
RelationParameter::RelationParameter(Sexp* sexp, ReadingMacroExpressionShortcutMap shortcuts) : ReadingMacroParameter(sexp) {
	// Local error messages
	std::stringstream error;

	// Make sure we have an S-expression worth reading
	if (sexp->getNumChildren() <  3) {
		error << "Ill-formed relation parameter: need at least 3 top-level nodes; got " << sexp->getNumChildren();
		error << " " << sexp->to_debug_string();
		throw UnexpectedInputException("RelationParameter::RelationParameter(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
	}

	// Make sure this is a relation expression
	if (sexp->getFirstChild()->isList() || sexp->getFirstChild()->getValue() !=  ReadingMacroParameter::RELATION_SYM) {
		error << "Ill-formed relation parameter: need relation expression; got ";
		error << " " << sexp->getFirstChild()->to_debug_string();
		throw UnexpectedInputException("RelationParameter::RelationParameter(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
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

	// Read the role subexpressions (at least one, probably not more than two)
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

	// Make sure we had at least one role
	if (_roles.size() < 1) {
		error << "Ill-formed relation parameter: need at least 1 role subexpression; got ";
		error << " " << sexp->to_debug_string();
		throw UnexpectedInputException("RelationParameter::RelationParameter(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
	}
}

/**
 * Comparison method; makes sure that the relation's
 * roles would all be consumed. Does not check the
 * predicate, since by convention that is used for output.
 *
 * @param relation The relation to match against.
 * @param ignore_unknown A flag that if true considers
 * this relation parameter to match a relation even if
 * the relation contains roles not contained in this parameter.
 * @param allow_individual A flag that if true allows the use
 * of the special "individual" role name indicating that the role
 * refers to a generated individual. Default false.
 *
 * @return True if all non-optional roles are present.
 *
 * @author nward@bbn.com
 * @date 2010.10.15
 **/
bool RelationParameter::matches(ElfRelation_ptr relation, bool ignore_unknown, bool allow_individual) {
	// Loop over this relation parameter's role parameters, checking each one
	size_t matched_roles = 0;
	BOOST_FOREACH(RoleParameter_ptr role, _roles) {
		// Ignore roles that refer to a generated individual if allowed
		if (allow_individual && role->get_input_name() == L"individual")
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
	}

	// Make sure we matched everything (that is the relation didn't have unknown roles)
	if (ignore_unknown || matched_roles == relation->get_args().size())
		return true;

	// No match
	return false;
}

/**
 * Using the roles defined in this parameter, extract
 * (and possibly rename) the matching roles in the
 * reference relation, generating a new relation
 * with deep copies of the matching arguments. Any roles
 * referring to the individual will get a deep copy of the
 * generated individual. If necessary, generates skolem
 * arguments.
 *
 * @param docid The ID of the document that will contain
 * this generated relation.
 * @param reference_relation The input relation from
 * which arguments are filtered.
 * @param individual_parameter Empty in a split context,
 * or the generated individual/value in a generate-individual
 * context.
 * @return A new relation containing the copied and
 * generated arguments.
 *
 * @author nward@bbn.com
 * @date 2010.11.01
 **/
ElfRelation_ptr RelationParameter::to_elf_relation(ElfDocument_ptr document, ElfRelation_ptr reference_relation, IndividualParameter_ptr individual_parameter) {
	// Generate a hash value for the reference relation (used by skolems)
	boost::hash<ElfRelation_ptr> relation_hasher;
	size_t relation_hash = relation_hasher(reference_relation);

	// Loop over this relation parameter's role parameters, copying each one
	std::vector<ElfRelationArg_ptr> output_args;
	BOOST_FOREACH(RoleParameter_ptr role, _roles) {
		// Check if this role refers to a generated individual
		if (role->get_input_name() == L"individual") {
			if (individual_parameter.get() != NULL) {
				// Generated individual or specified value?
				ElfRelationArg_ptr output_arg;
				if (individual_parameter->get_value() == L"") {
					// Generate an individual that will bridge this matching relation
					ElfIndividual_ptr generated_individual = individual_parameter->to_elf_individual(document, reference_relation);
					output_arg = boost::make_shared<ElfRelationArg>(role->get_output_name(), generated_individual);
				} else {
					// Generate an arg with the specified type and value
					output_arg = boost::make_shared<ElfRelationArg>(role->get_output_name(), individual_parameter->get_type(), individual_parameter->get_value(), reference_relation->get_text(), reference_relation->get_start(), reference_relation->get_end());
				}
				output_args.push_back(output_arg);
			} else {
				// Shouldn't happen
				throw UnexpectedInputException("RelationParameter::to_elf_relation(ElfDocument_ptr, ElfRelation_ptr, IndividualParameter_ptr)", "Input role 'individual' used without individual parameter");
			}
		} else if (!role->is_delete()) { // If the role contained a "(delete)" atom, don't pass it through.
			// Get the arg for this role, if any
			ElfRelationArg_ptr input_arg = reference_relation->get_arg(role->get_input_name());
			if (input_arg.get() != NULL) {
				// Deep copy the argument and rename it
				ElfRelationArg_ptr output_arg = boost::make_shared<ElfRelationArg>(input_arg);
				output_arg->set_role(role->get_output_name());
				output_args.push_back(output_arg);
			} else if (role->get_skolem_type() != L"") {
				// Generate a skolem argument type and value for this role
				//   Wasn't present in the reference relation, and the
				//   role parameter defines a skolem type
				std::wstringstream value;
				value << document->replace_uri_prefix(role->get_skolem_type()) << "-" << document->get_id() << "-" << relation_hash;
				ElfRelationArg_ptr skolem_arg = boost::make_shared<ElfRelationArg>(role->get_output_name(), role->get_skolem_type(), value.str());
				output_args.push_back(skolem_arg);
			}
		}
	}

	// Return a new relation containing the mapped arguments
	ElfRelation_ptr relation = boost::make_shared<ElfRelation>(_predicate->get_name(), output_args, reference_relation->get_text(), reference_relation->get_start(), reference_relation->get_end(), reference_relation->get_confidence(), reference_relation->get_score_group());
	relation->set_source(reference_relation->get_source());
	return relation;
}
