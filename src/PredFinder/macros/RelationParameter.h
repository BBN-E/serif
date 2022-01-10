/**
 * Class representing an ElfRelation declaration
 * as used in all subclasses of ReadingMacroOperator.
 *
 * @file RelationParameter.h
 * @author nward@bbn.com
 * @date 2010.10.06
 **/

#pragma once

#include "Generic/common/Sexp.h"
#include "ReadingMacroParameter.h"
#include "PredicateParameter.h"
#include "RoleParameter.h"
#include "IndividualParameter.h"
#include "boost/shared_ptr.hpp"
#include <set>
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(ElfDocument);
BSP_DECLARE(ElfRelation);
BSP_DECLARE(RelationParameter);
/**
 * Represents one ElfRelation used as one of the outputs
 * of a ReadingMacroOperator, containing one PredicateParameter
 * and one or more RoleParameters (depending on operator context).
 *
 * @author nward@bbn.com
 * @date 2010.10.06
 **/
class RelationParameter : public ReadingMacroParameter {
public:
	RelationParameter(Sexp* sexp, ReadingMacroExpressionShortcutMap shortcuts);
	RelationParameter(PredicateParameter_ptr predicate, std::set<RoleParameter_ptr> roles) : ReadingMacroParameter(ReadingMacroParameter::RELATION_SYM), _predicate(predicate) { _roles.insert(roles.begin(), roles.end()); }

	/**
	 * Inlined accessor to the relation parameters's predicate parameter.
	 *
	 * @return The value of _predicate.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.13
	 **/
	PredicateParameter_ptr get_predicate(void) const { return _predicate; }

	/**
	 * Inlined accessor to the relation parameters's role parameters.
	 *
	 * @return The value of _roles.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.13
	 **/
	std::set<RoleParameter_ptr> get_roles(void) const { return _roles; }

	bool matches(ElfRelation_ptr relation, bool ignore_unknown = false, bool allow_individual = false);

	ElfRelation_ptr to_elf_relation(ElfDocument_ptr document, ElfRelation_ptr reference_relation, IndividualParameter_ptr individual_parameter);

private:
	/**
	 * The ontology predicate to generate.
	 **/
	PredicateParameter_ptr _predicate;

	/**
	 * The predicate argument roles to match/generate.
	 **/
	std::set<RoleParameter_ptr> _roles;
};

