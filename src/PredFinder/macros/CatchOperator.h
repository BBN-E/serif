/**
 * Class that catches relations that should have been transformed by ElfInference
 * and should not have entered P-ELF in their original form.
 *
 * @file CatchOperator.h
 * @author afrankel@bbn.com
 * @date 2011.03.14
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
 * Catches relations that should have been transformed by ElfInference
 * and should not have entered P-ELF in their original form.
 *
 * @author afrankel@bbn.com
 * @date 2011.03.14
 **/
class CatchOperator : public ReadingMacroOperator {
public:
	CatchOperator(Sexp* sexp, ReadingMacroExpressionShortcutMap shortcuts);

	bool matches(ElfRelation_ptr relation) const;

	void apply(ElfDocument_ptr document) const;

	Symbol get_operator_symbol() const { return ReadingMacroOperator::CATCH_SYM; }

	// Does nothing because the catch operator never generates relations.
	void retrieve_predicates_generated(std::set<std::wstring> & predicate_names) const {}

protected:
	/**
	 * The ontology predicate to match against.
	 **/
	PredicateParameter_ptr _predicate;

	/**
	 * The predicate argument roles to match.
	 **/
	std::set<RoleParameter_ptr> _roles;
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author afrankel@bbn.com
 * @date 2011.03.14
 **/
typedef boost::shared_ptr<CatchOperator> CatchOperator_ptr;
