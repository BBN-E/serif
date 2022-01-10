/**
 * Class that rearranges the ElfRelationArgs of
 * an input ElfRelation into multiple output
 * ElfRelations.
 *
 * @file SplitOperator.h
 * @author nward@bbn.com
 * @date 2010.10.07
 **/

#pragma once

#include "Generic/common/Sexp.h"
#include "ReadingMacroOperator.h"
#include "PredicateParameter.h"
#include "RelationParameter.h"
#include "boost/shared_ptr.hpp"
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(ElfDocument);
BSP_DECLARE(ElfRelation);

/**
 * Performs a 1-to-many split on an input ElfRelation,
 * typically used to convert an N-ary relation into
 * multiple binary relations.
 *
 * @author nward@bbn.com
 * @date 2010.10.07
 **/
class SplitOperator : public ReadingMacroOperator {
public:
	SplitOperator(Sexp* sexp, ReadingMacroExpressionShortcutMap shortcuts);

	bool matches(ElfRelation_ptr relation) const;

	void apply(ElfDocument_ptr document) const;

	Symbol get_operator_symbol() const { return ReadingMacroOperator::SPLIT_SYM; }

	void retrieve_predicates_generated(std::set<std::wstring> & predicate_names) const;

private:
	/**
	 * The ontology predicate to match against.
	 **/
	PredicateParameter_ptr _predicate;

	/**
	 * The relations to generate (with role names
	 * to match and generate).
	 **/
	std::set<RelationParameter_ptr> _relations;

	/**
	 * Whether or not the input relation should be
	 * preserved in the output.
	 **/
	bool _preserve_input_relation;
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.10.07
 **/
typedef boost::shared_ptr<SplitOperator> SplitOperator_ptr;
