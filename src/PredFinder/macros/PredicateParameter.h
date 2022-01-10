/**
 * Class representing an ElfRelation name declaration
 * as used in all subclasses of ReadingMacroOperator.
 *
 * @file PredicateParameter.h
 * @author nward@bbn.com
 * @date 2010.10.06
 **/

#pragma once

#include "Generic/common/Sexp.h"
#include "ReadingMacroParameter.h"
#include "boost/shared_ptr.hpp"
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(ElfRelation);

/**
 * Represents one predicate, either as a match condition
 * in some macro operator, or as the output ElfRelation
 * name in a RelationParameter.
 *
 * @author nward@bbn.com
 * @date 2010.10.06
 **/
class PredicateParameter : public ReadingMacroParameter {
public:
	PredicateParameter(Sexp* sexp);
	PredicateParameter(std::wstring predicate_name) : ReadingMacroParameter(ReadingMacroParameter::PREDICATE_SYM), _predicate_name(predicate_name) { }

	/**
	 * Inlined accessor to the predicate parameters's predicate name.
	 *
	 * @return The value of _predicate_name.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.13
	 **/
	std::wstring get_name(void) const { return _predicate_name; }

	/**
	 * Comparison method; makes sure the predicate name
	 * matches the relation name.
	 *
	 * @return True if the strings are identical.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.15
	 **/
	bool matches(ElfRelation_ptr relation);

private:
	/**
	 * The ontology predicate to match against/generate.
	 **/
	std::wstring _predicate_name;
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.10.06
 **/
typedef boost::shared_ptr<PredicateParameter> PredicateParameter_ptr;
