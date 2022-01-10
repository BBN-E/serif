/**
 * Abstract class interface encapsulating functionality
 * in common between ReadingMacroOperators and
 * ReadingMacroParameters.
 *
 * @file ReadingMacroExpression.h
 * @author nward@bbn.com
 * @date 2010.10.05
 **/

#pragma once

#include "Generic/common/Sexp.h"
#include "Generic/common/Symbol.h"
#include "boost/shared_ptr.hpp"
#include <map>

// Forward declaration to use shared pointer in class
class ReadingMacroExpression;

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.10.05
 **/
typedef boost::shared_ptr<ReadingMacroExpression> ReadingMacroExpression_ptr;

/**
 * Expression shortcut lookup table.
 *
 * @author nward@bbn.com
 * @date 2010.10.05
 **/
typedef std::map<Symbol, ReadingMacroExpression_ptr> ReadingMacroExpressionShortcutMap;

/**
 * Basic functions and members in common between
 * ReadingMacroOperator and ReadingMacroParameter.
 *
 * @author nward@bbn.com
 * @date 2010.10.05
 **/
class ReadingMacroExpression {
public:
	ReadingMacroExpression(const Sexp* sexp);
	ReadingMacroExpression(Symbol expression_name) : _expression_name(expression_name) { }

	/**
	 * Inlined accessor for retrieving the expression name.
	 *
	 * @return The value of _expression_name.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.06
	 **/
	Symbol get_expression() const { return _expression_name; }

	/**
	 * Inlined accessor for checking if this expression is a shortcut.
	 *
	 * @return True if _shortcut_name is defined, otherwise false.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.05
	 **/
	bool is_shortcut() const { return _shortcut_name != Symbol(); }

	/**
	 * Inlined accessor for retrieving the shortcut name, if any.
	 *
	 * @return The value of _shortcut_name.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.05
	 **/
	Symbol get_shortcut() const { return _shortcut_name; }

	static ReadingMacroExpression_ptr from_s_expression(Sexp* sexp, ReadingMacroExpressionShortcutMap shortcuts);
	static ReadingMacroExpression_ptr from_shortcut(const ReadingMacroExpressionShortcutMap shortcuts, const Sexp* containing_sexp, const Symbol& shortcut_name, const Symbol& expression_name = Symbol());

protected:
	/**
	 * The expression name, whose value is restricted based
	 * on its subclass.
	 **/
	Symbol _expression_name;

	/**
	 * The name if the expression was a shortcut defined in
	 * the references section of a ReadingMacroSet, or empty.
	 **/
	Symbol _shortcut_name;
};
