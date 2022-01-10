/**
 * Represents one multi-stage macro rule that operates
 * over ElfDocuments to transform them using bridging
 * ontologies.
 *
 * @file ReadingMacro.cpp
 * @author nward@bbn.com
 * @date 2010.10.08
 **/

#include "Generic/common/leak_detection.h"
#pragma warning(disable: 4996)
#include "boost/foreach.hpp"
#include "ReadingMacro.h"
#include "ReadingMacroOperator.h"
#include "PredFinder/elf/ElfDocument.h"
#include "PredFinder/elf/ElfRelation.h"

Symbol ReadingMacro::RULE_SYM(L"rule");
Symbol ReadingMacro::ID_SYM(L"id");
Symbol ReadingMacro::STAGE_SYM(L"stage");

Symbol ReadingMacro::STAGE_BBN_SYM(L"bbn");
Symbol ReadingMacro::STAGE_ERUDITE_SYM(L"erudite");

/**
 * Reads the macro in some operator
 * context from the specified S-expression.
 *
 * @param sexp The read S-expression.
 *
 * @author nward@bbn.com
 * @date 2010.10.06
 **/
ReadingMacro::ReadingMacro(Sexp* sexp, ReadingMacroExpressionShortcutMap shortcuts) {
	// Local error messages
	std::stringstream error;

	// Make sure we have an S-expression worth reading
	if (sexp->getNumChildren() < 3 || sexp->getNumChildren() > 4) {
		error << "Ill-formed macro rule: need 3-4 top-level nodes; got " << sexp->getNumChildren();
		error << " " << sexp->to_debug_string();
		throw UnexpectedInputException("ReadingMacro::ReadingMacro(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
	}

	// Make sure this is a macro rule
	if (sexp->getFirstChild()->isList() || sexp->getFirstChild()->getValue() !=  ReadingMacro::RULE_SYM) {
		error << "Ill-formed macro rule: need rule; got";
		error << " " << sexp->getFirstChild()->to_debug_string();
		throw UnexpectedInputException("ReadingMacro::ReadingMacro(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
	}

	// Get the macro identifier
	Sexp* id_sexp = sexp->getSecondChild();
	if (id_sexp->isList() && id_sexp->getNumChildren() == 2 && id_sexp->getFirstChild()->isAtom() && id_sexp->getFirstChild()->getValue() == ReadingMacro::ID_SYM) {
		// Check the macro identifier
		if (id_sexp->getSecondChild()->isAtom()) {
			_id = id_sexp->getSecondChild()->getValue().to_string();
		} else {
			error << "Ill-formed macro rule: need rule id; got";
			error << " " << id_sexp->getSecondChild()->to_debug_string();
			throw UnexpectedInputException("ReadingMacro::ReadingMacro(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
		}
	} else {
		error << "Ill-formed macro rule: need id subexpression; got";
		error << " " << id_sexp->to_debug_string();
		throw UnexpectedInputException("ReadingMacro::ReadingMacro(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
	}

	// Get each macro stage
	for (int child = 2; child < sexp->getNumChildren(); child++) {
		Sexp* stage_sexp = sexp->getNthChild(child);
		if (stage_sexp->isList() && stage_sexp->getNumChildren() >= 2 && stage_sexp->getFirstChild()->isAtom() && stage_sexp->getFirstChild()->getValue() == ReadingMacro::STAGE_SYM) {
			// Check the stage name
			if (stage_sexp->getSecondChild()->isAtom()) {
				Symbol stage_name = stage_sexp->getSecondChild()->getValue();
				if (stage_name == ReadingMacro::STAGE_BBN_SYM || stage_name == ReadingMacro::STAGE_ERUDITE_SYM) {
					// Get each macro operator, if any
					for (int stage_child = 2; stage_child < stage_sexp->getNumChildren(); stage_child++) {
						Sexp* operator_sexp = stage_sexp->getNthChild(stage_child);
						ReadingMacroExpression_ptr expression;
						if (operator_sexp->isAtom()) {
							// Get the referenced relation shortcut
							expression = ReadingMacroExpression::from_shortcut(shortcuts, sexp, operator_sexp->getValue());
						} else {
							// Read the operator expression
							expression = ReadingMacroExpression::from_s_expression(operator_sexp, shortcuts);
						}
						ReadingMacroOperator_ptr macro_operator = boost::static_pointer_cast<ReadingMacroOperator>(expression);
						macro_operator->generate_source_id(_id, stage_name);
						insert_operator(stage_name, macro_operator);
					}
				} else {
					error << "Ill-formed macro rule: bad stage name '" << stage_name << "'";
					throw UnexpectedInputException("ReadingMacro::ReadingMacro(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
				}
			} else {
				error << "Ill-formed macro rule: need stage name; got";
				error << " " << stage_sexp->getSecondChild()->to_debug_string();
				throw UnexpectedInputException("ReadingMacro::ReadingMacro(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
			}
		} else {
			error << "Ill-formed macro rule: need stage subexpression; got";
			error << " " << stage_sexp->to_debug_string();
			throw UnexpectedInputException("ReadingMacro::ReadingMacro(Sexp*,ReadingMacroExpressionShortcutMap)", error.str().c_str());
		}
	}
}

/**
 * Takes an input relation and checks if it is
 * consumed by at least one of this macro's operators
 * in the specified stage.
 *
 * @param stage_name The stage for which we're
 * looking for a rule match.
 * @param relation The ElfRelation being checked
 * for possible consumption.
 *
 * @author nward@bbn.com
 * @date 2010.11.10
 **/
bool ReadingMacro::matches(Symbol stage_name, ElfRelation_ptr relation) const {
	// Get the operators for this stage, if any
	ReadingMacroStageMap::const_iterator stage_i = _stages.find(stage_name);
	if (stage_i == _stages.end())
		return false;

	// Check each operator in turn
	BOOST_FOREACH(ReadingMacroOperator_ptr macro_operator, stage_i->second) {
		if (macro_operator->matches(relation))
			return true;
	}

	// No match
	return false;
}

/**
 * Takes an input document and applies each operator
 * defined in this macro for the specified stage.
 * If no operators are defined for that stage, or the
 * operators don't match any relations, the result is
 * a no-op and the document is unchanged.
 *
 * @param stage_name The stage for which rules should
 * be applied.
 * @param document The ELF document, which will be
 * transformed in-place.
 *
 * @author nward@bbn.com
 * @date 2010.10.08
 **/
void ReadingMacro::apply(Symbol stage_name, ElfDocument_ptr document) const {
	// Get the operators for this stage, if any
	ReadingMacroStageMap::const_iterator stage_i = _stages.find(stage_name);
	if (stage_i == _stages.end())
		return;

	// Apply each operator in turn (they may depend on eachother)
	BOOST_FOREACH(ReadingMacroOperator_ptr macro_operator, stage_i->second) {
		macro_operator->apply(document);
	}
}

/**
 * Appends the specified operator to the sequence
 * operations defined for the specified stage.
 *
 * @param stage_name The stage to insert into.
 * @param macro_operator The operator to insert.
 *
 * @author nward@bbn.com
 * @date 2010.10.08
 **/
void ReadingMacro::insert_operator(Symbol stage_name, ReadingMacroOperator_ptr macro_operator) {
	std::pair<ReadingMacroStageMap::iterator, bool> operator_insert = _stages.insert(ReadingMacroStageMap::value_type(stage_name, std::vector<ReadingMacroOperator_ptr>()));
	operator_insert.first->second.push_back(macro_operator);
}
