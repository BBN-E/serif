/**
 * Abstract class interface (with factory methods) for
 * parameters used in ReadingMacro rules.
 *
 * @file ReadingMacroOperator.h
 * @author nward@bbn.com
 * @date 2010.10.05
 **/

#pragma once

#include "Generic/common/Sexp.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/bsp_declare.h"
#include "ReadingMacroExpression.h"
#include "boost/shared_ptr.hpp"

BSP_DECLARE(ElfDocument);
BSP_DECLARE(ElfRelation);

class ReadingMacroOperator : public ReadingMacroExpression {
public:
	ReadingMacroOperator(const Sexp* sexp) : ReadingMacroExpression(sexp) { _source_id = L"bbn:macro-" + std::wstring(_expression_name.to_string()); }
	ReadingMacroOperator(Symbol expression_name) : ReadingMacroExpression(expression_name) { _source_id = L"bbn:macro-" + std::wstring(_expression_name.to_string()); }

	void generate_source_id(const std::wstring & macro_id, Symbol macro_stage);

	virtual bool matches(ElfRelation_ptr relation) const = 0;

	virtual void apply(ElfDocument_ptr document) const = 0;

	/**
	 * Method that returns the symbol corresponding to the operator type (e.g., ReadingMacroOperator::CATCH_SYM
	 * for CatchOperator).
	 *
	 * @author afrankel@bbn.com
	 * @date 2011.07.14
	 **/
	virtual Symbol get_operator_symbol() const = 0;

	/**
	 * Method that retrieves the names of all predicates produced by the ReadingMacroOperator and inserts them into
	 * the output set. Some operators (e.g., CatchOperator) do not produce predicates, so
	 * their version of this method does nothing.
	 *
	 * @param predicate_names Names of predicates produced by the ReadingMacroOperator will be inserted
	 * into this set.
	 * @author afrankel@bbn.com
	 * @date 2011.07.14
	 **/
	virtual void retrieve_predicates_generated(std::set<std::wstring> & predicate_names) const = 0;
    virtual ~ReadingMacroOperator() {}
    
	static Symbol BINARIZE_SYM;
	static Symbol CATCH_SYM;
	static Symbol DELETE_SYM;
	static Symbol GENERATE_INDIVIDUAL_SYM;
	static Symbol MAKE_TRIPLE_SYM;
	static Symbol RENAME_SYM;
	static Symbol RETYPE_SYM;
	static Symbol SPLIT_SYM;

protected:
	/**
	 * A unique identifier for this operator, used
	 * when logging source metadata for a transformed
	 * relation.
	 **/
	std::wstring _source_id;
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.10.05
 **/
typedef boost::shared_ptr<ReadingMacroOperator> ReadingMacroOperator_ptr;
