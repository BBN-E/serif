// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

/**
 * Implementation of the primary object of Machine Reading
 * relation finding. Initialized as a model in a Serif stage handler.
 *
 * @file PredFinder.h
 * @author nward@bbn.com
 * @date 2012.01.31
 **/

#pragma once

#include "Generic/theories/DocTheory.h"
#include "Generic/common/bsp_declare.h"
#include "PredFinder/elf/ElfDocument.h"
#include "PredFinder/macros/ReadingMacroSet.h"

#include <set>
#include <string>
#include <vector>

BSP_DECLARE(NumberConverter);
BSP_DECLARE(TemporalNormalizer);
BSP_DECLARE(ElfInference);
BSP_DECLARE(PredFinderCounts);
BSP_DECLARE(PatternSet);
BSP_DECLARE(Pattern);
BSP_DECLARE(LearnItPattern);
BSP_DECLARE(LearnIt2Matcher)
BSP_DECLARE(TemporalAttributeAdder)

/**
 * Contains one initialized configuration for finding
 * relations in documents.
 *
 * @author nward@bbn.com
 * @date 2011.01.26
 **/
class PredFinder {
public:
	PredFinder();
	~PredFinder();

	void run(DocTheory* doc_theory);

private:
	/**
	 * The current ontology domain.
	 **/
	std::wstring _domain_prefix;

	/**
	 * The directory that will be used for all of this
	 * configuration's output.
	 **/
	std::wstring _output_dir;

	/**
	 * Name of a file that lists XDoc IDs to be mapped to bound URIs.
	 **/
	std::string _failed_xdoc;

	/**
	 * An initialized inference engine.
	 **/
	ElfInference_ptr _elf_inference;

	/**
	 * The set of loaded ELF, presumably partner S-ELF
	 * intended for merging.
	 **/
	ElfDocumentTypeMap _elf_to_merge;

	/**
	 * The loaded LearnIt patterns that will be applied.
	 **/
	std::vector<LearnItPattern_ptr> _learnit_patterns;

	/**
	 * The objects which apply LearnIt2 patterns.
	 **/
	std::vector<LearnIt2Matcher_ptr> _learnit2_matchers;

	/**
	 * The loaded manual patterns that will be applied.
	 **/
	std::vector<PatternSet_ptr> _manual_patterns;

	/**
	 * The loaded macros that will be applied after
	 * pattern matching.
	 **/
	ReadingMacroSetSortedSet _reading_macros;

	/**
	 * Whether or not output printing should be sorted.
	 **/
	bool _sort_output;

	/**
	 * Whether or not unmerged S-ELF and R-ELF should be written.
	 **/
	bool _save_unmerged_elf;

	/**
	 * The optional end stage of the run; should be one of P-ELF, S-ELF, or R-ELF.
	 * Default is R-ELF.
	 **/
	std::string _end_stage;

	/**
	 * Domain-specific individual clustering.
	 **/
	std::vector<std::wstring> _coreferent_types;

	/**
	 * Contains only one element: L"nfl:NFLGameTeamSummaryScore"
	 **/
	std::vector<std::wstring> _gtss_types;

	/**
	 * Temporal attribute adder
	 **/
	TemporalAttributeAdder_ptr _tempAttrAdder;

	/**
	 * Relation/individual counts.
	 **/
	PredFinderCounts_ptr _counter;

	/**
	 * Handle processing of number-containing text.
	 **/
	NumberConverter_ptr _number_converter;

	/**
	 * Handle processing of date-containing text.
	 **/
	TemporalNormalizer_ptr _temporal_normalizer;

private:
	ElfDocument_ptr processPElfDoc(const DocTheory* dist_doc);
	ElfDocument_ptr createPElfDoc(const DocTheory* dist_doc);
	ElfDocument_ptr processSElfDoc(const DocTheory* dist_doc, const ElfDocument_ptr pelf_doc, size_t & self_doc_count);
	ElfDocument_ptr processMergedSElfDoc(const DocTheory* dist_doc, const ElfDocument_ptr self_doc, size_t self_doc_count);

	std::vector<ElfDocument_ptr> get_elf_docs(const std::wstring & docid, const std::wstring & elf_type, const std::wstring & doc_source = L"");

	void load_learnit();
	void load_temporal();
	void load_manual_patterns();
	void load_reading_macros();

	bool validatePatterns();
	static void printInfoAboutPatterns(const ElfRelationMultisetSortedByName & rel_from_li_patt_set, const ElfRelationMultisetSortedByName & rel_from_man_patt_set);

	static void readLearnItPatternDatabase(const boost::filesystem::path & db_file, std::vector<LearnItPattern_ptr>& patterns);

	static void readManualPatterns(const std::string& path, std::vector<PatternSet_ptr>& patterns);
	static void readManualPatternSet(const boost::filesystem::path & pattern_file, std::vector<PatternSet_ptr>& patterns);
	static void readManualPatternsFromList(const boost::filesystem::path & list_file, std::vector<PatternSet_ptr>& patterns);
	static void readManualPatternsFromDir(const boost::filesystem::path & dir, std::vector<PatternSet_ptr>& patterns);

	static void readSuperfiltersFromDir(const boost::filesystem::path & dir, std::vector<std::string>& files);

	static ElfDocumentTypeMap readInputElfDocuments(const std::string & path);

	static std::string get_opt_param(const std::string & param_name);
	static std::vector<std::string> get_opt_string_vector_param(const std::string & param_name);
	static std::string get_required_param(const std::string & param_name);
};
