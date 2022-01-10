/**
 * Class that converts an N-ary relation into
 * an individual that is the subject of a relation
 * for each of its args, named by role. Intended for
 * use with N-ary ontologies like the agent ontology
 * for the MR-KBP task.
 *
 * @file BinarizeOperator.h
 * @author nward@bbn.com
 * @date 2011.08.16
 **/

#pragma once

#include "Generic/common/Sexp.h"
#include "ReadingMacroOperator.h"
#include "PredicateParameter.h"
#include "IndividualParameter.h"
#include "boost/shared_ptr.hpp"
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(ElfDocument);
BSP_DECLARE(ElfRelation);

/**
 * Performs a 1-to-many split on an input ElfRelation,
 * with an ontology-typed generated individual bridging
 * the output ElfRelations. At least two output relations,
 * expressing the subject and object of a binary input
 * relation, should be generated. Output RelationParameters
 * are determined dynamically based on the args of of the
 * input relation, so this operator cannot be statically
 * analyzed.
 *
 * @author nward@bbn.com
 * @date 2011.08.16
 **/
class BinarizeOperator : public ReadingMacroOperator {
public:
	BinarizeOperator(const Sexp* sexp);

	bool matches(ElfRelation_ptr relation) const { return _predicate->matches(relation); };

	void apply(ElfDocument_ptr document) const;

	Symbol get_operator_symbol() const { return ReadingMacroOperator::BINARIZE_SYM; }

	// Returns nothing because without a loaded ontology we don't know what predicates will be returned
	void retrieve_predicates_generated(std::set<std::wstring> & predicate_names) const { };
    virtual ~BinarizeOperator() {}

private:
	/**
	 * The ontology predicate to match against.
	 **/
	PredicateParameter_ptr _predicate;

	/**
	 * The ontology individual to generate; its type
	 * is determined directly from the input predicate.
	 **/
	IndividualParameter_ptr _individual;
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2011.08.16
 **/
typedef boost::shared_ptr<BinarizeOperator> BinarizeOperator_ptr;
