/**
 * Class that generates a bridging individual
 * for an input ElfRelation and associates it with
 * roles in each of the output ElfRelations.
 *
 * @file GenerateIndividualOperator.h
 * @author nward@bbn.com
 * @date 2010.10.07
 **/

#pragma once

#include "Generic/common/Sexp.h"
#include "ReadingMacroOperator.h"
#include "PredicateParameter.h"
#include "IndividualParameter.h"
#include "RelationParameter.h"
#include "boost/shared_ptr.hpp"
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(ElfDocument);
BSP_DECLARE(ElfRelation);

/**
 * Performs a 1-to-many split on an input ElfRelation,
 * with an ontology-typed generated individual bridging
 * the output ElfRelations. In some contexts a single
 * ElfRelation with added generated individual will be
 * created.
 *
 * @author nward@bbn.com
 * @date 2010.10.07
 **/
class GenerateIndividualOperator : public ReadingMacroOperator {
public:
	GenerateIndividualOperator(Sexp* sexp, ReadingMacroExpressionShortcutMap shortcuts);

	bool matches(ElfRelation_ptr relation) const;

	void apply(ElfDocument_ptr document) const;

	Symbol get_operator_symbol() const { return ReadingMacroOperator::GENERATE_INDIVIDUAL_SYM; }

	void retrieve_predicates_generated(std::set<std::wstring> & predicate_names) const;

private:
	/**
	 * The ontology predicate to match against.
	 **/
	PredicateParameter_ptr _predicate;

	/**
	 * The ontology individual to generate.
	 **/
	IndividualParameter_ptr _individual;

	/**
	 * The relations to generate (with role names/individual
	 * placeholder to match and generate).
	 **/
	std::set<RelationParameter_ptr> _relations;
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.10.07
 **/
typedef boost::shared_ptr<GenerateIndividualOperator> GenerateIndividualOperator_ptr;
