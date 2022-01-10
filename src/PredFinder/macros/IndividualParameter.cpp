/**
 * Class representing an ElfIndividual type declaration
 * as used in RetypeOperator and GenerateIndividualOperator.
 *
 * @file IndividualParameter.cpp
 * @author nward@bbn.com
 * @date 2010.10.05
 **/

#include "Generic/common/leak_detection.h"
#include "IndividualParameter.h"
#include "PredFinder/elf/ElfDocument.h"
#include "PredFinder/elf/ElfRelation.h"
#include "PredFinder/elf/ElfDescriptor.h"

/**
 * Reads the individual parameter in some operator
 * context from the specified S-expression.
 *
 * @param sexp The read S-expression.
 *
 * @author nward@bbn.com
 * @date 2010.10.05
 **/
IndividualParameter::IndividualParameter(Sexp* sexp) : ReadingMacroParameter(sexp) {
	// Local error messages
	std::stringstream error;

	// Make sure we have an S-expression worth reading
	if (sexp->getNumChildren() < 2 || sexp->getNumChildren() > 4) {
		error << "Ill-formed individual parameter: need 2-4 top-level nodes; got " << sexp->getNumChildren();
		error << " " << sexp->to_debug_string();
		throw UnexpectedInputException("IndividualParameter::IndividualParameter(Sexp*)", error.str().c_str());
	}

	// Make sure this is an individual expression
	if (sexp->getFirstChild()->isList() || sexp->getFirstChild()->getValue() !=  ReadingMacroParameter::INDIVIDUAL_SYM) {
		error << "Ill-formed individual parameter: need individual expression; got ";
		error << " " << sexp->getFirstChild()->to_debug_string();
		throw UnexpectedInputException("IndividualParameter::IndividualParameter(Sexp*)", error.str().c_str());
	}

	// Check if we need to start reading subexpressions after a shortcut expression
	int child = 1;
	if (_shortcut_name != Symbol())
		child++;

	// Make sure we have a type specification
	if (child >= sexp->getNumChildren()) {
		error << "Ill-formed individual parameter: need 1-2 top-level nodes after shortcut; got none";
		error << " " << sexp->to_debug_string();
		throw UnexpectedInputException("IndividualParameter::IndividualParameter(Sexp*)", error.str().c_str());
	}

	// Get the type specification
	if (sexp->getNthChild(child)->isAtom())
		_individual_type = sexp->getNthChild(child)->getValue().to_string();
	else {
		error << "Ill-formed individual parameter: need ontology type atom; got ";
		error << " " << sexp->getNthChild(sexp->getNumChildren() - 1)->to_debug_string();
		throw UnexpectedInputException("IndividualParameter::IndividualParameter(Sexp*)", error.str().c_str());
	}

	// Get the optional value specification
	child++;
	if (child < sexp->getNumChildren()) {
		if (sexp->getNthChild(child)->isAtom())
			_individual_value = sexp->getNthChild(child)->getValue().to_string();
		else {
			error << "Ill-formed individual parameter: need ontology type atom; got ";
			error << " " << sexp->getNthChild(sexp->getNumChildren() - 1)->to_debug_string();
			throw UnexpectedInputException("IndividualParameter::IndividualParameter(Sexp*)", error.str().c_str());
		}
	} else
		_individual_value = L"";
}

/**
 * Use the text and offsets from a reference relation
 * to generate an individual for this parameter's type.
 *
 * @param docid The ID of the document that will contain
 * this generated individual.
 * @param reference_relation The input relation from
 * which type provenance is determined.
 * @return A new individual containing the copied provenance.
 *
 * @author nward@bbn.com
 * @date 2010.10.18
 **/
ElfIndividual_ptr IndividualParameter::to_elf_individual(ElfDocument_ptr document, ElfRelation_ptr reference_relation) {
	// Generate a new unique identifier for this individual
	boost::hash<ElfRelation_ptr> relation_hasher;
	std::wstringstream id;
	id << document->replace_uri_prefix(_individual_type) << "-" << document->get_id() << "-" << relation_hasher(reference_relation);

	// Return a new individual containing the provenance of the relation that produced it
	ElfType_ptr individual_type = boost::make_shared<ElfType>(_individual_type, reference_relation->get_text(), reference_relation->get_start(), reference_relation->get_end());
	ElfString_ptr individual_desc = boost::make_shared<ElfDescriptor>(reference_relation->get_text(), -1.0, reference_relation->get_text(), reference_relation->get_start(), reference_relation->get_end());
	ElfIndividual_ptr individual = boost::make_shared<ElfIndividual>(id.str(), individual_desc, individual_type);
	return individual;
}
