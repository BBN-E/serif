/**
 * Abstract class interface (with factory methods) for
 * parameters used in ReadingMacroOperators.
 *
 * @file ReadingMacroOperator.cpp
 * @author nward@bbn.com
 * @date 2010.10.07
 **/

#include "Generic/common/leak_detection.h"
#include "ReadingMacroOperator.h"

Symbol ReadingMacroOperator::BINARIZE_SYM(L"binarize");
Symbol ReadingMacroOperator::CATCH_SYM(L"catch");
Symbol ReadingMacroOperator::DELETE_SYM(L"delete");
Symbol ReadingMacroOperator::GENERATE_INDIVIDUAL_SYM(L"generate-individual");
Symbol ReadingMacroOperator::MAKE_TRIPLE_SYM(L"make-triple");
Symbol ReadingMacroOperator::RENAME_SYM(L"rename");
Symbol ReadingMacroOperator::RETYPE_SYM(L"retype");
Symbol ReadingMacroOperator::SPLIT_SYM(L"split");

/**
 * Updates the _source_id of this operator
 * with a new string generated from (presumably)
 * its parent macro.
 *
 * @param macro_id The unique identifier of this
 * operator's parent macro.
 * @param macro_stage The parent stage of this
 * operator in its parent macro.
 *
 * @author nward@bbn.com
 * @date 2010.11.05
 **/
void ReadingMacroOperator::generate_source_id(const std::wstring & macro_id, Symbol macro_stage) {
	// Generate a new source ID URI
	std::wstringstream source_id;
	std::wstring stage_name = macro_stage.to_string();
	source_id << stage_name.substr(0, 3) << L":macro-" << macro_id << "-" << _expression_name.to_string();

	// Add a shortcut reference, if any
	if (_shortcut_name != Symbol()) {
		std::wstring shortcut_name = _shortcut_name.to_string();
		std::transform(shortcut_name.begin(), shortcut_name.end(), shortcut_name.begin(), tolower);
		source_id << "-" << shortcut_name;
	}

	// Done
	_source_id = source_id.str();
}
