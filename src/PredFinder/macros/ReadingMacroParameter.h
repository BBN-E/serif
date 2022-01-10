/**
 * Abstract class interface (with factory methods) for
 * parameters used in ReadingMacroOperators.
 *
 * @file ReadingMacroParameter.h
 * @author nward@bbn.com
 * @date 2010.10.05
 **/

#pragma once

#include "Generic/common/Sexp.h"
#include "Generic/common/Symbol.h"
#include "ReadingMacroExpression.h"
#include "boost/shared_ptr.hpp"

class ReadingMacroParameter : public ReadingMacroExpression {
public:
	ReadingMacroParameter(Sexp* sexp) : ReadingMacroExpression(sexp) { }
	ReadingMacroParameter(Symbol expression_name) : ReadingMacroExpression(expression_name) { }

	static Symbol INDIVIDUAL_SYM;
	static Symbol PREDICATE_SYM;
	static Symbol RELATION_SYM;
	static Symbol ROLE_SYM;
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.10.05
 **/
typedef boost::shared_ptr<ReadingMacroParameter> ReadingMacroParameter_ptr;
