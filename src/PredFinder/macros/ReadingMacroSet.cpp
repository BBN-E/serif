/**
 * Contains one set of macro rules, presumably associated with
 * a specific predicate, read from either a standalone file or
 * contained within a pattern file.
 *
 * @file ReadingMacroSet.cpp
 * @author nward@bbn.com
 * @date 2010.10.05
 **/

#include "Generic/common/leak_detection.h"
#pragma warning(disable: 4996)
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/BoostUtil.h"
#include "ReadingMacroSet.h"
#include "ReadingMacro.h"
#include "boost/foreach.hpp"
#include "boost/make_shared.hpp"
#include "boost/algorithm/string.hpp"
#include "PredFinder/elf/ElfDocument.h"
#include "PredFinder/elf/ElfRelation.h"
#include <iostream>
#include <boost/scoped_ptr.hpp>
/**
 * Stores a domain prefix, probably set on the
 * command line in PredicationFinder.cpp. Used
 * as a default in certain namespace operations.
 *
 * @author nward@bbn.com
 * @date 2010.10.27
 **/
std::wstring ReadingMacroSet::domain_prefix;

/**
 * Contains one set of related macro rules, typically
 * associated with a particular private extraction
 * predicate (i.e. found in P-ELF, valid in the private
 * catalog). Handles shortcut mapping for convenient
 * macro writing.
 *
 * @param sexp The read S-expression.
 *
 * @author nward@bbn.com
 * @date 2010.10.05
 **/
ReadingMacroSet::ReadingMacroSet(Sexp* sexp) {
	// Local error messages
	std::stringstream error;

	// Make sure we have an S-expression worth reading
	if (sexp->getNumChildren() < 2) {
		error << "Ill-formed reading macro set: need at least 2 top-level nodes; got " << sexp->getNumChildren();
		error << " " << sexp->to_debug_string();
		throw UnexpectedInputException("ReadingMacroSet::ReadingMacroSet(Sexp*)", error.str().c_str());
	}

	// Get the macro set name
	_macro_set_name = sexp->getFirstChild()->getValue();

	// Loop over the child S-expressions, looking for macro-relevant expressions
	//   Allows us to embed macros in pattern files if necessary
	Sexp* references_sexp = NULL;
	Sexp* macros_sexp = NULL;
	for (int j = 1; j < sexp->getNumChildren(); j++) {
		// Check if this child is one we need to read
		Sexp* child_sexp = sexp->getNthChild(j);
		if (child_sexp->getFirstChild()->getValue() == Symbol(L"macro-reference")) {
			references_sexp = child_sexp;
		} else if (child_sexp->getFirstChild()->getValue() == Symbol(L"macros")) {
			macros_sexp = child_sexp;
		}
	}

	// Loop over the macro parameter and operator shortcuts, if any, reading them
	if (references_sexp != NULL) {
		for (int r = 1; r < references_sexp->getNumChildren(); r++) {
			// Determine the type of shortcut
			Sexp* reference_sexp = references_sexp->getNthChild(r);

			// Create the appropriate shortcut by type and store it
			ReadingMacroExpression_ptr reference_expression = ReadingMacroExpression::from_s_expression(reference_sexp, _macro_shortcuts);
			if (!reference_expression->is_shortcut()) {
				error << "Ill-formed reading macro set: need shortcut expression in references; got ";
				error << " " << reference_sexp->to_debug_string();
				throw UnexpectedInputException("ReadingMacroSet::ReadingMacroSet(Sexp*)", error.str().c_str());
			}
			_macro_shortcuts.insert(ReadingMacroExpressionShortcutMap::value_type(reference_expression->get_shortcut(), reference_expression));
		}
	}

	// Loop over the macro rules, if any, reading them
	if (macros_sexp != NULL) {
		for (int m = 1; m < macros_sexp->getNumChildren(); m++) {
			ReadingMacro_ptr ptr(boost::make_shared<ReadingMacro>(macros_sexp->getNthChild(m), _macro_shortcuts));
			if (_macros.find(ptr) != _macros.end()) {
				error << "Ill-formed reading macro set: id previously encountered: ";
				error << ptr->get_id();
				throw UnexpectedInputException("ReadingMacroSet::ReadingMacroSet(Sexp*)", error.str().c_str());
			}
			else {
				_macros.insert(ptr);
			}
		}
	}
}

/**
 * Takes an input relation and checks if it is
 * consumed by at least one macro's operator in
 * the specified stage.
 *
 * @param stage_name The stage for which we're
 * looking for a rule match.
 * @param relation The ElfRelation being checked
 * for possible consumption.
 *
 * @author nward@bbn.com
 * @date 2010.11.10
 **/
bool ReadingMacroSet::matches(Symbol stage_name, ElfRelation_ptr relation) const {
	// Check each macro defined in this set in turn
	BOOST_FOREACH(ReadingMacro_ptr macro, _macros) {
		if (macro->matches(stage_name, relation))
			return true;
	}

	// No match
	return false;
}

/**
 * Takes an input document and applies each macro
 * defined in this macro set for the specified stage.
 * If no rules match for that stage, the result is
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
void ReadingMacroSet::apply(Symbol stage_name, ElfDocument_ptr document) const {
	// Apply each macro defined in this set in turn
	BOOST_FOREACH(ReadingMacro_ptr macro, _macros) {
		macro->apply(stage_name, document);
	}
}

/**
 * Returns true if the macro set contains at least one stage with at least one operator with the given Symbol.
 *
 * @param operator_symbol A Symbol defined by ReadingMacroOperator referring to the given operator.
 * @return True if the macro set contains at least one operator with the given Symbol, false otherwise.
 *
 * @example bool contains_catch = rms.has_operator(ReadingMacroOperator::CATCH_SYM);
 *
 * @author afrankel@bbn.com
 * @date 2011.07.14
 **/
bool ReadingMacroSet::has_operator(Symbol operator_symbol) const {
	bool found(false);
	BOOST_FOREACH(ReadingMacro_ptr macro, _macros) {
		const ReadingMacroStageMap & stage_map = macro->get_stage_map();
		// typedef std::map<Symbol,std::vector<ReadingMacroOperator_ptr>> ReadingMacroStageMap;
		BOOST_FOREACH(ReadingMacroStageMap::value_type map_entry, stage_map) {
			BOOST_FOREACH(ReadingMacroOperator_ptr op, map_entry.second) {
				if (op->get_operator_symbol() == operator_symbol) {
                    return true;
                }
			}
		}
	}
	return false;
}

/**
 * Finds the predicate names of all relations generated in the given stage and inserts them into a set.
 *
 * @param pred_names Set of names into which the predicates of all relations generated in the given stage 
 * will be inserted.
 * @param stage_symbol Symbol representing the stage (e.g., ReadingMacro::STAGE_BBN_SYM for "bbn").
 * 
 * @author afrankel@bbn.com
 * @date 2011.07.14
 **/
void ReadingMacroSet::retrieve_predicates_generated_in_stage(Symbol stage_symbol, std::set<std::wstring> & pred_names) {
	BOOST_FOREACH(ReadingMacro_ptr macro, _macros) {
		const ReadingMacroStageMap & stage_map = macro->get_stage_map();
		ReadingMacroStageMap::const_iterator iter = stage_map.find(stage_symbol);
		if (iter != stage_map.end()) {
			BOOST_FOREACH(ReadingMacroOperator_ptr op, iter->second) {
				op->retrieve_predicates_generated(pred_names);
			}
		}
	}
}

/**
 * Sequentially tries applying all of the macros in
 * the specified set for the stage to the specified
 * document, generating an output document.
 *
 * If the input ELF document does not have the correct
 * contents for the specified stage of macro application,
 * throws an error.
 *
 * @param stage_name The stage for which rules should
 * be applied.
 * @param macro_sets All of the active macro sets.
 * @param document The document to transform.
 * @return The transformed document after running
 * through this stage.
 *
 * @author nward@bbn.com
 * @date 2010.10.08
 **/
ElfDocument_ptr ReadingMacroSet::apply_macros(Symbol stage_name, ReadingMacroSetSortedSet macro_sets, 
											  const ElfDocument_ptr input_document) {
	// Local error messages
	std::stringstream error;

	// Deep copy the input document so we don't overwrite anything unintentionally
	ElfDocument_ptr document = boost::make_shared<ElfDocument>(input_document);

	// Make sure the stage name is valid
	if (stage_name == ReadingMacro::STAGE_BBN_SYM) {
		if (document->get_contents() == L"P-ELF") {
			// This is a P-ELF -> S-ELF transformation
			document->set_contents(L"S-ELF");
		} else {
			error << "ELF contents '" << document->get_contents() << "' not valid input for stage '";
			error << stage_name << "', expected 'P-ELF'";
			throw UnexpectedInputException("ReadingMacroSet::apply(Symbol,ElfDocument_ptr)", error.str().c_str());
		}
	} else if (stage_name == ReadingMacro::STAGE_ERUDITE_SYM) {
		if (document->get_contents() == L"S-ELF") {
			// This is a S-ELF -> R-ELF transformation
			document->set_contents(L"R-ELF");
		} else {
			error << "ELF contents '" << document->get_contents() << "' not valid input for stage '";
			error << stage_name << "', expected 'S-ELF'";
			throw UnexpectedInputException("ReadingMacroSet::apply(Symbol,ElfDocument_ptr)", error.str().c_str());
		}
	} else {
		error << "Cannot apply macro set: bad stage name '" << stage_name << "'";
		throw UnexpectedInputException("ReadingMacroSet::apply(Symbol,ElfDocument_ptr)", error.str().c_str());
	}

	// If we're generating S-ELF, split relations that contain multiple args with the same role
	//   This is done before macro processing, in case duplicate roles interfere with macro matching
	if (stage_name == ReadingMacro::STAGE_BBN_SYM)
		document->split_duplicate_role_relations();

	// Try invoking each macro for this stage on this document and collect the results
	BOOST_FOREACH(ReadingMacroSet_ptr macro_set, macro_sets) {
		// Apply this macro set to the copy in-place
		macro_set->apply(stage_name, document);
	}

	// For some domains, cluster macro-generated individuals
	if (stage_name == ReadingMacro::STAGE_ERUDITE_SYM && ReadingMacroSet::domain_prefix == L"ic") {
		// Running the clustering engine is the easiest way to merge these as opposed to trying
		// to generate the individual's URI based on a predicate-specific value role
		std::vector<std::wstring> date_individuals;
		date_individuals.push_back(L"ic:Date");
		document->do_document_level_individual_coreference(ReadingMacroSet::domain_prefix, date_individuals);
		std::vector<std::wstring> count_individuals;
		count_individuals.push_back(L"ic:Count");
		document->do_document_level_individual_coreference(ReadingMacroSet::domain_prefix, count_individuals);
	}

	// If we're generating R-ELF, convert individuals to relations
	if (stage_name == ReadingMacro::STAGE_ERUDITE_SYM)
		document->convert_individuals_to_relations(ReadingMacroSet::domain_prefix);

	// Return the converted document
	return document;
}

/**
 * Reads the S-expression in the specified file
 * and constructs a ReadingMacroSet from it.
 *
 * @param filename The path to a pattern/macro file.
 * @return A shared pointer to the read ReadingMacroSet
 * 
 * @author nward@bbn.com
 * @date 2010.10.05
 **/
ReadingMacroSet_ptr ReadingMacroSet::read_from_file(const boost::filesystem::path & filename) {
	// Load the macro set
	SessionLogger::info("rm_set_file_0") << "  Reading macro set " << filename.string()	<< "...\n";
	boost::scoped_ptr<UTF8InputStream> macro_stream_scoped_ptr(UTF8InputStream::build(filename.string().c_str()));
	UTF8InputStream& macro_stream(*macro_stream_scoped_ptr);
	Sexp* macro_sexp = _new Sexp(macro_stream, true);
	return boost::make_shared<ReadingMacroSet>(macro_sexp);
}

/**
 * Recursively reads a collection of ReadingMacroSets
 * from a directory.
 *
 * @param dirname The path to a directory of macro
 * files, to be read recursively
 * @return A set of shared pointers, one for each
 * macro set read.
 * 
 * @author nward@bbn.com
 * @date 2010.10.22
 **/
ReadingMacroSetSortedSet ReadingMacroSet::read_from_dir(const boost::filesystem::path & dir) {
	// Read each item from this directory in turn
	ReadingMacroSetSortedSet macro_sets;
	boost::filesystem::directory_iterator end;
	SessionLogger::info("rm_set_dir_0") << "Reading macros dir " << dir.string() << "...\n";
	for (boost::filesystem::directory_iterator dir_item(dir); dir_item != end; dir_item++) {
		if (boost::filesystem::is_directory(*dir_item)) {
			// Recurse
			ReadingMacroSetSortedSet recursed_macro_sets = read_from_dir(*dir_item);
			macro_sets.insert(recursed_macro_sets.begin(), recursed_macro_sets.end());
		} else {
			// Filter based on file suffix
			if (boost::algorithm::ends_with(BOOST_FILESYSTEM_DIR_ITERATOR_GET_FILENAME(dir_item), ".sexp")) {
				ReadingMacroSet_ptr new_macro_set(read_from_file(*dir_item));
				BOOST_FOREACH(ReadingMacroSet_ptr macro_set, macro_sets) {
					BOOST_FOREACH(ReadingMacro_ptr macro, macro_set->_macros) {
						if (new_macro_set->_macros.find(macro) != new_macro_set->_macros.end()) {
							std::stringstream error;
							error << "Ill-formed reading macro set: id previously encountered: ";
							error << macro->get_id();
							throw UnexpectedInputException("ReadingMacroSet::read_from_dir()", error.str().c_str());
						}
					}
				}
				macro_sets.insert(new_macro_set);
			}
		}
	}
	return macro_sets;
}

/**
 * Recursively reads a collection of ReadingMacroSets
 * from a directory or a single ReadingMacroSet file.
 *
 * @param path The path to a directory or file
 * containing one or more macro sets.
 * @return A set of shared pointers, one for each
 * macro set read.
 * 
 * @author nward@bbn.com
 * @date 2010.10.22
 **/
ReadingMacroSetSortedSet ReadingMacroSet::read(const std::string & path) {
	// Get the full path
	boost::filesystem::path full_path = boost::filesystem::system_complete(boost::filesystem::path(path));

	// Make sure input exists
	std::stringstream error;
	if (!boost::filesystem::exists(full_path)) {
		error << "Specified macro set input (" << full_path << ") doesn't exist";
		throw std::runtime_error(error.str().c_str());
	}

	// Read
	if (boost::filesystem::is_directory(full_path)) {
		return read_from_dir(full_path);
	} else if (boost::filesystem::is_regular_file(full_path)) {
		ReadingMacroSetSortedSet macro_sets;
		macro_sets.insert(read_from_file(full_path));
		return macro_sets;
	} else {
		error << "Macro set input (" << full_path << ") is not a file or directory";
		throw std::runtime_error(error.str().c_str());
	}
}
