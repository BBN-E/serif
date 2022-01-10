/**
 * Class representing an ElfRelationArg role declaration
 * as used in RelationParameters.
 *
 * @file RoleParameter.h
 * @author nward@bbn.com
 * @date 2010.10.06
 **/

#pragma once

#include "Generic/common/Sexp.h"
#include "ReadingMacroParameter.h"
#include "boost/shared_ptr.hpp"

/**
 * Represents one role, either as a match condition
 * in some macro operator, or as the output
 * name in a RelationParameter.
 *
 * @author nward@bbn.com
 * @date 2010.10.06
 **/
class RoleParameter : public ReadingMacroParameter {
public:
	RoleParameter(Sexp* sexp);
	RoleParameter(std::wstring input_role_name, std::wstring output_role_name) : ReadingMacroParameter(ReadingMacroParameter::ROLE_SYM), _input_role_name(input_role_name), _output_role_name(output_role_name), _optional(false), _delete(false), _skolem_type(L"") { }

	/**
	 * Inlined accessor to the role parameters's input role name.
	 *
	 * @return The value of _input_role_name.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.13
	 **/
	std::wstring get_input_name(void) const { return _input_role_name; }

	/**
	 * Inlined accessor to the role parameters's output role name,
	 * which may be identical to the input role name.
	 *
	 * @return The value of _output_role_name.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.13
	 **/
	std::wstring get_output_name(void) const { return _output_role_name; }

	/**
	 * Inlined accessor to check if this role is optional.
	 *
	 * @return The value of _optional, which is also always
	 * true if _skolem_type is defined.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.15
	 **/
	bool is_optional(void) const { return _optional; }

	/**
	 * Inlined accessor to check whether this role should be deleted.
	 *
	 * @return The value of _delete.
	 *
	 * @author afrankel@bbn.com
	 * @date 2011.04.04
	 **/
	bool is_delete(void) const { return _delete; }

	/**
	 * Inlined accessor to get this role's optional skolem
	 * type assertion.
	 *
	 * @return The value of _skolem_type.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.19
	 **/
	std::wstring get_skolem_type(void) const { return _skolem_type; }

	static Symbol DELETE_SYM;
	static Symbol OPTIONAL_SYM;
	static Symbol SKOLEM_SYM;

private:
	/**
	 * The predicate argument role to match against.
	 **/
	std::wstring _input_role_name;

	/**
	 * The predicate argument role to emit. (May be
	 * identical to the input in some contexts.)
	 **/
	std::wstring _output_role_name;

	/**
	 * Whether or not an argument with this role is
	 * optional in the context of its containing
	 * RelationParameter; true if _skolem_type is
	 * defined. Input ElfRelations missing this role
	 * can still match.
	 **/
	bool _optional;

	/**
	 * Whether or not an argument with this role should
	 * be deleted in the context of its containing
	 * RelationParameter.
	 **/
	bool _delete;

	/**
	 * The type of the skolem individual that should
	 * be generated if an argument with this role
	 * is missing in an otherwise-matching input
	 * ElfRelation.
	 **/
	std::wstring _skolem_type;
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.10.06
 **/
typedef boost::shared_ptr<RoleParameter> RoleParameter_ptr;
