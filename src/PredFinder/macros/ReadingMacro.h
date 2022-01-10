/**
 * Represents one multi-stage macro rule that operates
 * over ElfDocuments to transform them using bridging
 * ontologies.
 *
 * @file ReadingMacro.h
 * @author nward@bbn.com
 * @date 2010.10.05
 **/

#pragma once

#include "Generic/common/Sexp.h"
#include "Generic/common/Symbol.h"
#include "ReadingMacroExpression.h"
#include "ReadingMacroOperator.h"
#include "boost/shared_ptr.hpp"
#include "Generic/common/bsp_declare.h"
#include <map>
#include <vector>

BSP_DECLARE(ElfDocument);
BSP_DECLARE(ElfRelation);

/**
 * Operators in stage lookup table.
 *
 * @author nward@bbn.com
 * @date 2010.10.07
 **/
typedef std::map<Symbol,std::vector<ReadingMacroOperator_ptr> > ReadingMacroStageMap;

/**
 * A single multi-stage macro rule that matches
 * a particular set of ElfRelations and generates
 * output ElfRelations/ElfIndividuals by applying
 * ReadingMacroOperators in order.
 *
 * @author nward@bbn.com
 * @date 2010.10.07
 **/
class ReadingMacro {
public:
	ReadingMacro(Sexp* sexp, ReadingMacroExpressionShortcutMap shortcuts);

	/**
	 * Inlined accessor for the macro ID
	 *
	 * @return Current value of _id;
	 * @author afrankel@bbn.com
	 * @date 2010.11.03
	 **/
	std::wstring get_id() const { return _id; }

	bool matches(Symbol stage_name, ElfRelation_ptr relation) const;

	void apply(Symbol stage_name, ElfDocument_ptr document) const;

	const ReadingMacroStageMap & get_stage_map() const {return _stages;}
	
	bool operator<(const ReadingMacro& rhs) const { return _id < rhs._id; }
	bool operator==(const ReadingMacro& rhs) const { return _id == rhs._id; }

	static Symbol RULE_SYM;
	static Symbol ID_SYM;
	static Symbol STAGE_SYM;

	static Symbol STAGE_BBN_SYM;
	static Symbol STAGE_ERUDITE_SYM;

private:
	/**
	 * The unique identifier for this macro rule,
	 * used in logging/debugging and to generate
	 * values for the source attribute in output
	 * ElfRelations.
	 **/
	std::wstring _id;

	/**
	 * The collection of operators, an ordered list
	 * for each stage, stored by stage name.
	 **/
	ReadingMacroStageMap _stages;

	void insert_operator(Symbol stage_name, ReadingMacroOperator_ptr macro_operator);
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.10.05
 **/
typedef boost::shared_ptr<ReadingMacro> ReadingMacro_ptr;

/**
 * std::set sorting for ReadingMacros
 *
 * @author afrankel@bbn.com
 * @date 2010.11.03
 **/
class ReadingMacroPtrSortCriterion {
public:
	bool operator() (const ReadingMacro_ptr& p1, const ReadingMacro_ptr& p2) const {
		return (*p1 < *p2);
	}
};
