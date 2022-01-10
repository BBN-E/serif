/**
 * Contains one set of macro rules, presumably associated with
 * a specific predicate, read from either a standalone file or
 * contained within a pattern file.
 *
 * @file ReadingMacroSet.h
 * @author nward@bbn.com
 * @date 2010.10.05
 **/

#pragma once

#include "Generic/common/Sexp.h"
#include "Generic/common/Symbol.h"
#include <set>
#include "ReadingMacro.h"
#include "ReadingMacroOperator.h"
#include "ReadingMacroParameter.h"
#include "boost/shared_ptr.hpp"
#include "boost/filesystem/path.hpp"
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(ElfDocument);
BSP_DECLARE(ElfRelation);
BSP_DECLARE(ReadingMacro);
// Forward declaration to use shared pointer in class
class ReadingMacroSet;

// Forward declaration to use sorted set in class
class ReadingMacroSetPtrSortCriterion;

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.10.05
 **/
typedef boost::shared_ptr<ReadingMacroSet> ReadingMacroSet_ptr;

/**
 * Sorted set of reading macros, so they get applied in a
 * consistent order.
 *
 * @author nward@bbn.com
 * @date 2011.05.23
 **/
typedef std::set<ReadingMacroSet_ptr, ReadingMacroSetPtrSortCriterion> ReadingMacroSetSortedSet;

/**
 * Contains one set of macro rules (e.g., eru:attendedSchool) and the methods necessary to check that 
 * an ElfDocument's contents are correct for the specified stage of macro application and, if so, apply them.
 **/
class ReadingMacroSet {
public:
	ReadingMacroSet(Sexp* sexp);

	/**
	 * Inlined accessor for the macro set name.
	 *
	 * @return Current value of _macro_set_name;
	 * @author nward@bbn.com
	 * @date 2010.10.05
	 **/
	Symbol get_macro_set_name() const { return _macro_set_name; }

	bool matches(Symbol stage_name, ElfRelation_ptr relation) const;

	void apply(Symbol stage_name, ElfDocument_ptr document) const;

	bool has_operator(Symbol operator_symbol) const;

	void retrieve_predicates_generated_in_stage(Symbol stage_symbol, std::set<std::wstring> & pred_names);

	static ElfDocument_ptr apply_macros(Symbol stage_name, ReadingMacroSetSortedSet macro_sets, 
		const ElfDocument_ptr input_document);

	static ReadingMacroSet_ptr read_from_file(const boost::filesystem::path & filename);
	static ReadingMacroSetSortedSet read_from_dir(const boost::filesystem::path & dir);
	static ReadingMacroSetSortedSet read(const std::string & path);

	static std::wstring domain_prefix;

private:
	/**
	 * The name of this macro set; used for self-documentation,
	 * not macro matching.
	 *
	 * @see PatternSet::_patternSetName
	 **/
	Symbol _macro_set_name;

	/**
	 * All of the parameter and operator shortcuts used in this macro set.
	 **/
	ReadingMacroExpressionShortcutMap _macro_shortcuts;

	/**
	 * The staged macro rules that can be executed by this macro set.
     * Sort by rule ID.
	 **/
	std::set<ReadingMacro_ptr, ReadingMacroPtrSortCriterion> _macros;
};

/**
 * std::set sorting for ReadingMacroSets
 *
 * @author nward@bbn.com
 * @date 2011.05.23
 **/
class ReadingMacroSetPtrSortCriterion {
public:
	bool operator() (const ReadingMacroSet_ptr& p1, const ReadingMacroSet_ptr& p2) const {
		return (p1->get_macro_set_name() < p2->get_macro_set_name());
	}
};
