/**
 * Class representing an ElfIndividual type declaration
 * as used in RetypeOperator and GenerateIndividualOperator.
 *
 * @file IndividualParameter.h
 * @author nward@bbn.com
 * @date 2010.10.05
 **/

#pragma once

#include "Generic/common/Sexp.h"
#include "ReadingMacroParameter.h"
#include "boost/shared_ptr.hpp"
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(ElfDocument);
BSP_DECLARE(ElfRelation);
BSP_DECLARE(ElfIndividual);

/**
 * Represents one individual, either as a match condition
 * in some macro operator, or as the output ElfRelation
 * name in a RelationParameter.
 *
 * @author nward@bbn.com
 * @date 2010.10.06
 **/
class IndividualParameter : public ReadingMacroParameter {
public:
	IndividualParameter(Sexp* sexp);
	IndividualParameter(std::wstring individual_type) : ReadingMacroParameter(ReadingMacroParameter::INDIVIDUAL_SYM), _individual_type(individual_type), _individual_value(L"") { }

	/**
	 * Inlined accessor to the individual parameter's
	 * individual type.
	 *
	 * @return The value of _individual_type.
	 *
	 * @author nward@bbn.com
	 * @date 2010.11.01
	 **/
	std::wstring get_type(void) const { return _individual_type; }

	/**
	 * Inlined accessor to the individual parameter's
	 * individual value, which may be empty.
	 *
	 * @return The value of _individual_value.
	 *
	 * @author nward@bbn.com
	 * @date 2010.11.01
	 **/
	std::wstring get_value(void) const { return _individual_value; }

	ElfIndividual_ptr to_elf_individual(ElfDocument_ptr document, ElfRelation_ptr reference_relation);

private:
	/**
	 * The ontology type to match against/generate.
	 **/
	std::wstring _individual_type;

	/**
	 * The ontology value to generate, if any.
	 **/
	std::wstring _individual_value;
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.10.05
 **/
typedef boost::shared_ptr<IndividualParameter> IndividualParameter_ptr;
