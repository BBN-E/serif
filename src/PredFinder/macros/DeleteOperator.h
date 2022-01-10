/**
 * Class that Deletes relations that we use as input during ElfInference
 * and we don't want to pass on past P-ELF.
 *
 * @file DeleteOperator.h
 * @author nward@bbn.com
 * @date 2011.11.30
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
 * Deletes relations that we use as input during ElfInference
 * and we don't want to pass on past P-ELF.
 *
 * @author nward@bbn.com
 * @date 2011.11.30
 **/
class DeleteOperator : public ReadingMacroOperator {
public:
	DeleteOperator(Sexp* sexp, ReadingMacroExpressionShortcutMap shortcuts);

	bool matches(ElfRelation_ptr relation) const;

	void apply(ElfDocument_ptr document) const;

	Symbol get_operator_symbol() const { return ReadingMacroOperator::DELETE_SYM; }

	// Does nothing because the Delete operator never generates relations.
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
 * @author nward@bbn.com
 * @date 2011.11.30
 **/
typedef boost::shared_ptr<DeleteOperator> DeleteOperator_ptr;
