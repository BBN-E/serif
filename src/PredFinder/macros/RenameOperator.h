/**
 * Class that performs a rename operation on a
 * single ElfRelation's predicate and roles.
 *
 * @file RenameOperator.h
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
 * Performs a 1-to-1 rename of an input ElfRelation,
 * producing an output ElfRelation with possibly
 * different predicate and role names.
 *
 * @author nward@bbn.com
 * @date 2010.10.07
 **/
class RenameOperator : public ReadingMacroOperator {
public:
	RenameOperator(Sexp* sexp, ReadingMacroExpressionShortcutMap shortcuts);
	RenameOperator(Sexp* sexp) : ReadingMacroOperator(sexp) { }

	bool matches(ElfRelation_ptr relation) const;

	void apply(ElfDocument_ptr document) const;

	Symbol get_operator_symbol() const { return ReadingMacroOperator::RENAME_SYM; }

	virtual void retrieve_predicates_generated(std::set<std::wstring> & predicate_names) const;

protected:
	/**
	 * The ontology predicate to match against.
	 **/
	PredicateParameter_ptr _predicate;

	/**
	 * The relation to generate (with role names to match
	 * and generate).
	 **/
	RelationParameter_ptr _relation;
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.10.07
 **/
typedef boost::shared_ptr<RenameOperator> RenameOperator_ptr;
