// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

/**
 * Implementation of the primary object of Machine Reading
 * relation finding. Initialized as a model in a Serif stage handler.
 *
 * @file PredFinder.cpp
 * @author nward@bbn.com
 * @date 2012.01.31
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/common/version.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/BoostUtil.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/edt/CorefUtilities.h"
#include "Generic/values/NumberConverter.h"
#include "Generic/values/TemporalNormalizer.h"
#include "Generic/patterns/PatternSet.h"
#include "LearnIt/util/FileUtilities.h"
#include "LearnIt/db/LearnItDB.h"
#include "LearnIt/LearnItPattern.h"
#include "LearnIt/LearnIt2Matcher.h"
#include "Temporal/TemporalDB.h"
#include "Temporal/TemporalAttributeAdderManualAttachment.h"
#include "Temporal/TemporalTrainingDataGenerator.h"
#include "PredFinder.h"
#include "PredFinderCounts.h"
#include "PredFinder/common/ElfMultiDoc.h"
#include "PredFinder/inference/ElfInference.h"
#include "PredFinder/inference/EITbdAdapter.h"
#include "PredFinder/elf/ElfRelationFactory.h"

#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

const char NONE_STR[] = "[none]";

PredFinder::PredFinder() {
	// Allow for dumping pre-cluster ELF for debugging (mostly NFL)
	_save_unmerged_elf = ParamReader::isParamTrue("save_unmerged_elf");

	// We usually run all the way through R-ELF generation
	_end_stage = ParamReader::getParam("predfinder_end_stage", "R-ELF");

	// Xerces doesn't create directories when writing XML
	_output_dir = UnicodeUtil::toUTF16StdString(get_required_param("output_dir"));
	if (_output_dir.empty()) {
		SessionLogger::err("empty_output_dir_0") << "Empty name specified for output dir.\n";
		throw UnexpectedInputException("PredFinder::PredFinder",
				"Empty name specified for output dir");
	}
	try {
		boost::filesystem::create_directories(_output_dir);
	} catch (std::exception & exc) {
		SessionLogger::err("create_dir_0") << "Error in calling create_directories(" << _output_dir << "); " << exc.what() << "\n";
		throw;
	}

	// The domain prefix controls a lot of behavior
	_domain_prefix = UnicodeUtil::toUTF16StdString(get_required_param("domain_prefix"));

	// Set up for xdoc and bound URI lookup
	_failed_xdoc = get_opt_param("failed_xdoc");
	ElfMultiDoc::initialize(_domain_prefix);
	std::string arg_map_path = get_required_param("arg_map");
	std::string string_to_cluster_filename = get_required_param("string_to_cluster_filename");
	std::string title_bound_uris = ParamReader::getParam("title_bound_uris");

	// Handle NFL player -> team lookups
	std::string nfl_player_db;
	if (_domain_prefix == L"nfl") {
		nfl_player_db = get_opt_param("world_knowledge_db"); 
	}
	ElfMultiDoc::init_nfl_player_db(nfl_player_db);

	// Set the GPE URI lookup database
	ElfMultiDoc::init_location_db(get_opt_param("location_db"));

	// Create a list of types we want to corefer, based on the current domain
	if (_domain_prefix == L"nfl") {
		_coreferent_types.push_back(L"nfl:NFLGame");
		_coreferent_types.push_back(L"nfl:NFLRegularSeasonGame");
		_coreferent_types.push_back(L"nfl:NFLPlayoffGame");
		_coreferent_types.push_back(L"nfl:NFLExhibitionGame");
	}
	_gtss_types.push_back(L"nfl:NFLGameTeamSummaryScore");

	// Check if we're validating, limit loading
	bool validate_only = ParamReader::getOptionalTrueFalseParamWithDefaultVal("validate_only", false);
	if (!validate_only) {
		// Read in world knowledge lists.  This controls merging of entities within a document.
		CorefUtilities::initializeWKLists();

		// Read the map of equivalent name strings (e.g., equiv_name_cluster_output.txt).
		// This happens BEFORE init_docid_type_to_srm_map() 
		// so that the srm_map can be expanded with the equivalent names.
		// Sample lines:
		//		1	GPE	soviet, soviet union, ussr, 
		//		2	GPE	china, chinese, people s republic of china, prc, sino, 
		ElfMultiDoc::load_xdoc_maps(UnicodeUtil::toUTF16StdString(string_to_cluster_filename));

		// Read the list of XDoc IDs to be mapped to bound URIs (e.g., problematic_cluster_output.txt).
		// Sample lines:
		// u.s. military	NONE	ic:U.S._military
		// izzedine al-qassam brigades	8832	ic:Izz_el-Deen_al-Qassam_organization
		// al-qaeda	103	ic:al-Qaeda
		// al-aqsa martyr's brigade	723	ic:Al-Aqsa_Martyr_s_Brigade
		// abdul-majid al-khoei	192	ic:Abdul-Majid_al-Khoei
		// king fahd	3178	ic:King_Fahd
		ElfMultiDoc::load_mr_xdoc_output_file(_failed_xdoc);

		// Read the map of argument values to ontology individuals (string replacement pairs).
		// Mappings apply to all docs.
		// Example:
		//	nfl:NFLTeam	Arizona Cardinals	nfl:ArizonaCardinals
		//	nfl:NFLTeam	Cardinals	nfl:ArizonaCardinals
		ElfMultiDoc::init_type_w_or_wo_docid_to_srm_map(arg_map_path);
		ElfMultiDoc::generate_typeless_maps();

		// Replace appropriate XDoc cluster IDs with URIs from the loaded argument value map
		ElfMultiDoc::replace_xdoc_ids_with_mapped_args();

		// Load Title bound URI mapping
		if (!title_bound_uris.empty() && title_bound_uris != NONE_STR) {
			ElfMultiDoc::load_title_bound_uris(title_bound_uris);
		}

		// Read input ELF files, storing by ELF type and source, and then by document ID
		std::vector<std::string> input_elf_dirs = get_opt_string_vector_param("input_elf_dirs");
		BOOST_FOREACH(std::string input_elf_dir, input_elf_dirs) {
			ElfDocumentTypeMap dir_elf_to_merge = readInputElfDocuments(input_elf_dir);
			BOOST_FOREACH(ElfDocumentTypeMap::value_type dir_elf_type, dir_elf_to_merge) {
				std::pair<ElfDocumentTypeMap::iterator, bool> elf_type_insert = 
					_elf_to_merge.insert(ElfDocumentTypeMap::value_type(dir_elf_type.first, ElfDocumentMap()));
				BOOST_FOREACH(ElfDocumentMap::value_type dir_elf_doc, dir_elf_type.second) {
					elf_type_insert.first->second.insert(dir_elf_doc);
				}
			}
		}
	}

	// Set up the order in which ElfInference will run filters
	std::string filter_order = get_opt_param("filter_ordering");
	std::vector<std::string> superfilters;
	std::string superfilter_path = get_opt_param("superfilters");
	if (superfilter_path.length() > 0) {
		boost::filesystem::path superfilter_dir = boost::filesystem::system_complete(boost::filesystem::path(superfilter_path));
		readSuperfiltersFromDir(superfilter_dir, superfilters);
	}

	// Initialize the inference engine.
	try {
		_elf_inference = boost::make_shared<ElfInference>(filter_order, superfilters, _failed_xdoc);
	} catch(std::exception & exc) {
		SessionLogger::err("ElfInference") << "Error intitializing ElfInference. " << exc.what() << std::endl;
		throw;
	}

	// Set up value processing
	_number_converter = NumberConverter_ptr(NumberConverter::build());
	ElfIndividual::number_converter = _number_converter;
	_temporal_normalizer = TemporalNormalizer_ptr(TemporalNormalizer::build());
	ElfMultiDoc::temporal_normalizer = _temporal_normalizer;

	// Control some printing options
	_sort_output = ParamReader::getOptionalTrueFalseParamWithDefaultVal("sort_output", false);
	bool show_ids_for_individuals = ParamReader::getOptionalTrueFalseParamWithDefaultVal("show_ids_for_individuals", true);
	ElfIndividual::set_show_id(show_ids_for_individuals);
	ElfRelationArg::set_show_id(show_ids_for_individuals);

	

	// Initialize the tables in EITbdAdapter. (Calling EITbdAdapter::init() again later will have 
	// no effect.)
	EITbdAdapter::init();

	// Load all of the pattern objects
	load_learnit();
	load_manual_patterns();
	load_reading_macros();
	load_temporal();

	// We often want to run a test with a limited number of LearnIt patterns, manual patterns, or macros,
	// so don't generate an error if validatePatterns returns false.
	bool bRet = validatePatterns();

	// Track counts
	_counter = PredFinderCounts_ptr(PredFinderCounts::getPredFinderCounts(_sort_output));
}

PredFinder::~PredFinder() {
	// Print some summary information across all documents
	ElfMultiDoc::output_location_db_counts();
	_counter->finish();
}

void PredFinder::run(DocTheory* doc_theory) {
	// Start a new counter
	_counter->doc_start(doc_theory);

	// Initialize xdoc/bound URI lookup table for this document
	ElfMultiDoc::generate_entity_and_id_maps(doc_theory);
	if(ParamReader::getRequiredTrueFalseParam("fix_newswire_leads"))
		ElfInference::fixNewswireLeads(doc_theory);

	// If specified, read P-ELF, otherwise generate it using patterns
	ElfDocument_ptr pelf_doc = processPElfDoc(doc_theory);
	_counter->add(pelf_doc, CountRecord::PELF);
	if (_end_stage == "P-ELF") {
		_counter->doc_end();
		return;
	}

	// If specified, read S-ELF, otherwise generate it using macros
	size_t self_doc_count(0);
	ElfDocument_ptr self_doc = processSElfDoc(doc_theory, pelf_doc, self_doc_count);
	_counter->add(self_doc, CountRecord::SELF);
	ElfDocument_ptr merged_self_doc = processMergedSElfDoc(doc_theory, self_doc, self_doc_count);
	_counter->add(merged_self_doc, CountRecord::MERGED_SELF);
	if (_end_stage == "S-ELF") {
		_counter->doc_end();
		return;
	}

	// Generate both BBN and merged R-ELF
	ElfDocument_ptr relf_doc = ReadingMacroSet::apply_macros(ReadingMacro::STAGE_ERUDITE_SYM, _reading_macros, self_doc);
	_counter->add(relf_doc, CountRecord::RELF);
	ElfDocument_ptr merged_relf_doc = ReadingMacroSet::apply_macros(ReadingMacro::STAGE_ERUDITE_SYM, _reading_macros, merged_self_doc);
	_counter->add(merged_relf_doc, CountRecord::MERGED_RELF);
	_counter->doc_end();

	// Write out the R-ELF documents as XML
	if (_save_unmerged_elf)
		relf_doc->to_file(_output_dir + L"/R-ELF");
	merged_relf_doc->to_file(_output_dir + L"/merged-R-ELF");
}

ElfDocument_ptr PredFinder::processPElfDoc(const DocTheory* doc_theory) {
	ElfDocument_ptr pelf_doc;
	std::vector<ElfDocument_ptr> pelf_docs = get_elf_docs(doc_theory->getDocument()->getName().to_string(), L"P-ELF", L"BBN PredFinder");
	if (pelf_docs.size() == 0) {
		pelf_doc = createPElfDoc(doc_theory);
	} else if (pelf_docs.size() == 1) {
		// Just read the P-ELF
		pelf_doc = pelf_docs[0];
	} else {
		std::stringstream error;
		error << "Got more than one BBN P-ELF document for " << doc_theory->getDocument()->getName().to_string();
		throw std::runtime_error(error.str().c_str());
	}
	return pelf_doc;
}

ElfDocument_ptr PredFinder::processSElfDoc(const DocTheory* doc_theory, const ElfDocument_ptr pelf_doc, size_t & self_doc_count) {
	ElfDocument_ptr self_doc;
	std::vector<ElfDocument_ptr> self_docs = get_elf_docs(doc_theory->getDocument()->getName().to_string(), L"S-ELF", L"BBN PredFinder");
	self_doc_count = self_docs.size();
	if (self_doc_count == 0) {
		// Create an S-ELF document from the P-ELF
		self_doc = ReadingMacroSet::apply_macros(ReadingMacro::STAGE_BBN_SYM, _reading_macros, pelf_doc);

		// Do coreference on any generated GTSS individuals in NFL
		if (_domain_prefix == L"nfl") {
			self_doc->do_document_level_individual_coreference(_domain_prefix, _gtss_types);
			if (ParamReader::isParamTrue("dump_individuals")) {
				self_doc->dump_individuals_by_type(_output_dir + L"/individuals-S-ELF", _gtss_types);
			}
		}

		// Write out the document as XML
		if (_save_unmerged_elf)
			self_doc->to_file(_output_dir + L"/S-ELF");
	} else if (self_doc_count == 1) {
		// Just read the S-ELF
		self_doc = self_docs[0];
	} else {
		std::stringstream error;
		error << "Somehow got more than one BBN S-ELF document for " << doc_theory->getDocument()->getName().to_string();
		throw std::runtime_error(error.str().c_str());
	}

	return self_doc;
}

ElfDocument_ptr PredFinder::processMergedSElfDoc(const DocTheory* doc_theory, const ElfDocument_ptr self_doc, size_t self_doc_count) {
	// If any non-BBN S-ELF was specified, merge them in
	ElfDocument_ptr merged_self_doc;
	std::vector<ElfDocument_ptr> self_docs_to_merge = get_elf_docs(doc_theory->getDocument()->getName().to_string(), L"S-ELF");
	if (self_doc_count == 0) {
		// Add the newly generated BBN S-ELF to the merge list
		self_docs_to_merge.push_back(self_doc);
	}
	if (self_docs_to_merge.size() == 1) {
		// Probably just dealing with BBN S-ELF
		merged_self_doc = self_docs_to_merge[0];
		if (self_doc_count == 0) {
			// BBN S-ELF was freshly generated, write it out
			merged_self_doc->to_file(_output_dir + L"/merged-S-ELF");
		}
	} else if (self_docs_to_merge.size() > 1) {
		// Merge all of the S-ELF
		merged_self_doc = boost::make_shared<ElfDocument>(doc_theory, self_docs_to_merge);

		EIDocData_ptr docData = _elf_inference->prepareDocumentForInference(doc_theory);
		_elf_inference->do_partner_inference(docData, merged_self_doc);

		// Rerun coreference on the merged S-ELF
		merged_self_doc->do_document_level_individual_coreference(_domain_prefix, _coreferent_types);

		// Do coreference on any generated GTSS individuals in NFL
		if (_domain_prefix == L"nfl") {
			merged_self_doc->do_document_level_individual_coreference(_domain_prefix, _gtss_types);
			if (ParamReader::isParamTrue("dump_individuals")) {
				merged_self_doc->dump_individuals_by_type(_output_dir + L"/individuals-merged-S-ELF", _gtss_types);
			}
		}

		// Write out the document as XML
		merged_self_doc->to_file(_output_dir + L"/merged-S-ELF");
	} else {
		std::stringstream error;
		error << "Somehow got no S-ELF documents for " << doc_theory->getDocument()->getName().to_string();
		throw std::runtime_error(error.str().c_str());
	}
	return merged_self_doc;
}

ElfDocument_ptr PredFinder::createPElfDoc(const DocTheory* doc_theory) {
	ElfDocument_ptr pelf_doc;
	EIDocData_ptr docData = _elf_inference->prepareDocumentForInference(doc_theory);

	// Create a P-ELF document from this Serif document
	pelf_doc = boost::make_shared<ElfDocument>(docData, _elf_inference, 
		_learnit_patterns, _manual_patterns, _learnit2_matchers, _tempAttrAdder);

	// Do inference on the ELF document
	_elf_inference->do_inference(docData, pelf_doc);

	// Do coreference on the unmerged unmacroed P-ELF
	if (ParamReader::isParamTrue("dump_unclustered_elf")) {
		pelf_doc->to_file(_output_dir + L"/unclustered-P-ELF");
	}
	if (ParamReader::isParamTrue("do_sentence_level_individual_coref")) {
		pelf_doc->do_sentence_level_individual_coreference(doc_theory, _domain_prefix, _coreferent_types);
		if (ParamReader::isParamTrue("dump_unclustered_elf")) {
			pelf_doc->to_file(_output_dir + L"/sent-clustered-P-ELF");
		}
	}
	pelf_doc->do_document_level_individual_coreference(_domain_prefix, _coreferent_types);
	if (ParamReader::isParamTrue("dump_individuals")) {
		pelf_doc->dump_individuals_by_type(_output_dir + L"/individuals-P-ELF", _coreferent_types);
	}

	// Add in GPE relations
	ElfMultiDoc::addGPERelationsToElfDoc(pelf_doc, doc_theory);

	// Write out the document as XML
	pelf_doc->to_file(_output_dir + L"/P-ELF");
	return pelf_doc;
}

std::vector<ElfDocument_ptr> PredFinder::get_elf_docs(const std::wstring & docid, const std::wstring & elf_type, const std::wstring & doc_source) {
	// List of documents in the map matching the search criteria (could be empty)
	std::vector<ElfDocument_ptr> docs;

	// Check if a source filter was specified
	if (doc_source != L"") {
		// Find this particular document in the loaded ELF map
		ElfDocumentTypeMap::iterator elf_doc_map_i = _elf_to_merge.find(ElfDocumentTypeMap::key_type(elf_type, doc_source));
		if (elf_doc_map_i != _elf_to_merge.end()) {
			ElfDocumentMap::iterator elf_doc_i = elf_doc_map_i->second.find(docid);
			if (elf_doc_i != elf_doc_map_i->second.end()) {
				docs.push_back(elf_doc_i->second);
			}
		}
	} else {
		// Loop through all of the type/source combinations and check for a type match, returning all matching documents
		BOOST_FOREACH(ElfDocumentTypeMap::value_type elf_doc_map, _elf_to_merge) {
			if (elf_doc_map.first.first == elf_type) {
				ElfDocumentMap::iterator elf_doc_i = elf_doc_map.second.find(docid);
				if (elf_doc_i != elf_doc_map.second.end()) {
					docs.push_back(elf_doc_i->second);
				}
			}
		}
	}

	// Done
	return docs;
}

void PredFinder::load_learnit() {
	std::string learnit_dbs = get_opt_param("learnit_dbs");
	std::string learnit2_dbs = get_opt_param("learnit2_dbs");

	// Read the databases containing LearnIt patterns, then read in all the active patterns
	if (!learnit_dbs.empty()) {
		std::set<boost::filesystem::path> db_paths = FileUtilities::getLearnItDatabasePaths(learnit_dbs);
		BOOST_FOREACH(boost::filesystem::path path, db_paths) {
			readLearnItPatternDatabase(path, _learnit_patterns);
		}
	}

	if (_learnit_patterns.size() == 0) {
		SessionLogger::warn("no_learnit_0") << "No active LearnIt patterns loaded!\n";
	} else {
		SessionLogger::info("loaded_learnit_0") << "Loaded " << _learnit_patterns.size() 
			<< " active LearnIt pattern(s)\n";
	}

	if (!learnit2_dbs.empty()) {
		std::set<boost::filesystem::path> db_paths = FileUtilities::getLearnItDatabasePaths(learnit2_dbs);
		BOOST_FOREACH(boost::filesystem::path path, db_paths) {
			SessionLogger::info("loaded_learnit_2") << L"Loading LearnIt 2 " << "matcher from " << path.string();
			_learnit2_matchers.push_back(boost::make_shared<LearnIt2Matcher>(boost::make_shared<LearnItDB>(path.string())));
		}
	}

	if (_learnit2_matchers.empty()) {
		SessionLogger::warn("no_li_patt_0") << "No LearnIt2 matchers loaded!" << std::endl;
	} else {
		SessionLogger::info("li_patt_0") << "Loaded " << _learnit2_matchers.size() << " LearnIt2 matchers" << std::endl;
	}
}

void PredFinder::load_temporal() {
	if (ParamReader::hasParam("temporal_db")) {
		if (ParamReader::hasParam("generate_temporal_training")) {
			throw UnexpectedInputException("PredFinder::load_temporal",
				"temporal_db and generate_temporal_training parameters are mutually exclusive");
		}

		std::string p = ParamReader::getParam("temporal_db");
		if (p != "[none]") {
			TemporalDB_ptr temporalDB = boost::make_shared<TemporalDB>(p);
			_tempAttrAdder = boost::make_shared<TemporalAttributeAdderManual>(temporalDB);

			SessionLogger::info("load_temporal_db") << "Loaded temporal database "
				<< "from " << p;
		}
	} else if (ParamReader::isParamTrue("generate_temporal_training_data")) {
		SessionLogger::info("generate_temporal_training") 
			<< "Generating temporal training data";
		_tempAttrAdder = TemporalTrainingDataGenerator::create(
			UnicodeUtil::toUTF8StdString(_output_dir));
	}
}


void PredFinder::load_manual_patterns() {
	// Read the files containing Manual (manual) patterns
	std::vector<std::string> manual_patterns = get_opt_string_vector_param("manual_patterns");
	if (!manual_patterns.empty()) {
		BOOST_FOREACH(std::string manual_pattern, manual_patterns) {
			if (!manual_pattern.empty()) {
				readManualPatterns(manual_pattern, _manual_patterns);
			}
		}
	}
	if (_manual_patterns.size() == 0) {
		SessionLogger::warn("no_man_patt_0") << "No manual Manual patterns loaded!\n";
	} else {
		SessionLogger::info("man_patt_0") << "Loaded " << _manual_patterns.size() << " manual Manual pattern(s)\n";
	}
}

void PredFinder::load_reading_macros() {
	if (_domain_prefix.length() >= 5 && _domain_prefix.substr(0, 5) == L"blitz")
		// Override domain prefix for macros to set output predicate namespace
		ReadingMacroSet::domain_prefix = L"ic";
	else
		ReadingMacroSet::domain_prefix = _domain_prefix;

	// Read the files containing reading macros
	std::string reading_macros = get_opt_param("reading_macros");
	if (!reading_macros.empty()) {
		_reading_macros = ReadingMacroSet::read(reading_macros);
	}
}

/**
 * Validates LearnIt and manual patterns against reading macro sets to make sure that each relation
 * produced by a pattern will be consumed by a macro set, and that each macro set consumes at least
 * one relation.
 * @param force_verbose Derived from the parameter "verbose_pattern_verification". If true, output
 * is highly verbose. To gain finer control of verbosity, set "verbose_pattern_verification" to false
 * and set the individual parameters (e.g., "rel_union") mentioned in the body of this method to true
 * as desired.
 *
 * @author afrankel@bbn.com
 **/
bool PredFinder::validatePatterns() {
	bool validate_manual_patterns = ParamReader::getOptionalTrueFalseParamWithDefaultVal(
		"validate_manual_patterns", /*defaultVal=*/ false);
	bool verbose_pattern_verification = ParamReader::getOptionalTrueFalseParamWithDefaultVal(
		"verbose_pattern_verification", /* defaultVal=*/ false);
	if (!validate_manual_patterns) {
		return false;
	}
	bool bAllRelationsUsed(true);
	bool bAllMacrosUsed(true);
	ElfRelationMultisetSortedByName rel_from_li_patt_set;
	ElfRelationMultisetSortedByName rel_from_man_patt_set;
	std::set<std::wstring> tbd_pattern_set_names;
	std::set<std::wstring> tbd_predicates;
	ElfRelationFactory::fake_relations_from_li_pattern_set(_learnit_patterns, rel_from_li_patt_set);
	ElfRelationFactory::fake_relations_from_man_pattern_set(_manual_patterns, rel_from_man_patt_set, tbd_pattern_set_names,
		tbd_predicates, verbose_pattern_verification);
	ElfRelationMultisetSortedByName filtered_rel_from_man_patt_set;
	BOOST_FOREACH(ElfRelation_ptr rel, rel_from_man_patt_set) {
		std::wstring rel_name = rel->get_name();
		std::set<std::wstring>::const_iterator iter = tbd_pattern_set_names.find(rel_name);
		if (iter == tbd_pattern_set_names.end()) {
			filtered_rel_from_man_patt_set.insert(rel);
		}
	}
	size_t rel_from_man_patt_set_size(filtered_rel_from_man_patt_set.size());
	std::vector<ElfRelation_ptr> rel_from_man_patt_vec(filtered_rel_from_man_patt_set.begin(), filtered_rel_from_man_patt_set.end());
	if (verbose_pattern_verification || SessionLogger::dbg_or_msg_enabled("verbose_pattern_verification")) {
	   printInfoAboutPatterns(rel_from_li_patt_set, filtered_rel_from_man_patt_set);
    }
    // Fill rel_union with the union of relations from LearnIt and manual patterns.
	std::vector<ElfRelation_ptr> rel_union;
	set_union(rel_from_li_patt_set.begin(), rel_from_li_patt_set.end(),
		filtered_rel_from_man_patt_set.begin(), filtered_rel_from_man_patt_set.end(), std::inserter(rel_union, rel_union.end()), 
		ElfRelation_less_than());

    if (verbose_pattern_verification || SessionLogger::dbg_or_msg_enabled("rel_union")) {
		SessionLogger::dbg("rel_union") << "UNION OF RELATIONS FROM LEARNIT AND MANUAL PATTERNS:";
    	BOOST_FOREACH(ElfRelation_ptr rel, rel_union) {
    		std::ostringstream ostr;
    		rel->dump(ostr);
    		SessionLogger::dbg("rel_union") << ostr.str();
    	}
    }
	std::vector<std::wstring> unconsuming_macro_sets;
	std::vector<std::wstring> unconsumed_relations;
	// Determine whether each reading macro consumes at least one relation.
	size_t rel_union_size(rel_union.size());
	std::vector<bool> relation_used(rel_union_size, false);
	std::set<std::wstring> preds_generated_in_bbn_stage;
	BOOST_FOREACH(ReadingMacroSet_ptr reading_macro_set, _reading_macros) {
		bool bUsed(false);
		reading_macro_set->retrieve_predicates_generated_in_stage(ReadingMacro::STAGE_BBN_SYM, preds_generated_in_bbn_stage);
		std::wstring macro_set_name = reading_macro_set->get_macro_set_name().to_string();
		std::set<std::wstring>::const_iterator iter = tbd_predicates.find(macro_set_name);
		if (iter != tbd_predicates.end()) {
			// e.g., we encountered a pattern that returns "tbd bombing", which translates (via EITbdAdapter::getEventName())
			// into "eru:bombingAttackEvent", which matches the macro set name in the macro set file eru_bombingAttackEvent.sexp.
			bUsed = true; 
		} else {
			for (size_t i = 0; i < rel_union_size; ++i) {
				if (reading_macro_set->matches(L"bbn", rel_union[i])) {
					relation_used[i] = true;
					bUsed = true;
				} else if (reading_macro_set->matches(L"erudite", rel_union[i])) {
					relation_used[i] = true;
					bUsed = true;
				}
			}
		}
		// Macros with a "catch" operator are only for catching relations that unexpectedly 
		// slip through. We don't need to validate those macros.
		if (!bUsed && !reading_macro_set->has_operator(ReadingMacroOperator::CATCH_SYM)) {
			std::wstring name(reading_macro_set->get_macro_set_name().to_string());
			unconsuming_macro_sets.push_back(name);
		}
	}
	// Make another pass through unconsuming_macro_sets to weed out the ones that consume relations
	// produced during the earlier ("bbn") stage of macro application.
	std::set<std::wstring> filtered_unconsuming_macro_sets;
	BOOST_FOREACH(std::wstring unconsuming_macro_set, unconsuming_macro_sets) {
		if (preds_generated_in_bbn_stage.find(unconsuming_macro_set) == preds_generated_in_bbn_stage.end()) {
			filtered_unconsuming_macro_sets.insert(unconsuming_macro_set);
		}
	}

	// Determine whether each relation has been used.
	ostringstream unconsumed_rel_info;
	for (size_t i = 0; i < rel_union_size; ++i) {
		if (!relation_used[i]) {
			wstringstream wstr;
			wstr << rel_union[i]->get_name() << " (" << rel_union[i]->get_source() << ")";
			unconsumed_relations.push_back(wstr.str());
			if (unconsumed_relations.size() == 1) { // first item
				unconsumed_rel_info << "Information about relations not consumed by macro sets:\n";
			}
			rel_union[i]->dump(unconsumed_rel_info, /*indent=*/ 0);
			bAllRelationsUsed = false;
		}
	}
	wstringstream unconsuming_macro_info;
	if (!filtered_unconsuming_macro_sets.empty()) {
		bAllMacrosUsed = false;
		unconsuming_macro_info << L"\nMacro sets that consumed no relations:\n";
		BOOST_FOREACH(std::wstring wstr, filtered_unconsuming_macro_sets) {
			unconsuming_macro_info << wstr << endl;
		}
	}
	if (!unconsuming_macro_info.str().empty()) {
		SessionLogger::warn("unconsuming_macro_0") << unconsuming_macro_info.str();
	}
	wstringstream unconsumed_rel_list;
	if (!unconsumed_relations.empty()) {
		unconsumed_rel_list << "\nRelations not consumed by macro sets:\n";
		BOOST_FOREACH(std::wstring wstr, unconsumed_relations) {
			unconsumed_rel_list << wstr << endl;
		}
	}
	if (!unconsumed_rel_info.str().empty()) {
		SessionLogger::warn("unconsumed_rel_0") << unconsumed_rel_info.str();
	}
	if (!unconsumed_rel_list.str().empty()) {
		SessionLogger::warn("unconsumed_rel_list_0") << unconsumed_rel_list.str();
	}
	// Unconsumed relations will cause the function to return false, but 
	// unconsuming macro sets just generate warnings.
	return(bAllRelationsUsed && bAllMacrosUsed); 
}

void PredFinder::printInfoAboutPatterns(const ElfRelationMultisetSortedByName & rel_from_li_patt_set, const ElfRelationMultisetSortedByName & rel_from_man_patt_set) {
	size_t rel_from_li_patt_set_size(rel_from_li_patt_set.size());
	size_t rel_from_man_patt_set_size(rel_from_man_patt_set.size());

	std::set<std::wstring> rel_names_from_li_patt_set;
	std::set<std::wstring> rel_names_from_man_patt_set;

	BOOST_FOREACH(ElfRelation_ptr rel, rel_from_li_patt_set) {
		rel_names_from_li_patt_set.insert(rel->get_name());
	}
	BOOST_FOREACH(ElfRelation_ptr rel, rel_from_man_patt_set) {
		rel_names_from_man_patt_set.insert(rel->get_name());
	}
	size_t max_size = (rel_from_li_patt_set_size > rel_from_man_patt_set_size) ? rel_from_li_patt_set_size : rel_from_man_patt_set_size;
	std::vector<std::wstring> intersec(max_size);
	std::vector<std::wstring> rel_union(rel_from_li_patt_set_size + rel_from_man_patt_set_size);
	std::vector<std::wstring> rels_in_li_only(max_size);
	std::vector<std::wstring> rels_in_man_only(max_size);
	set_intersection(rel_names_from_li_patt_set.begin(), rel_names_from_li_patt_set.end(),
		rel_names_from_man_patt_set.begin(), rel_names_from_man_patt_set.end(), intersec.begin());
	set_union(rel_names_from_li_patt_set.begin(), rel_names_from_li_patt_set.end(),
		rel_names_from_man_patt_set.begin(), rel_names_from_man_patt_set.end(), rel_union.begin());
	set_difference(rel_names_from_li_patt_set.begin(), rel_names_from_li_patt_set.end(),
		rel_names_from_man_patt_set.begin(), rel_names_from_man_patt_set.end(), rels_in_li_only.begin());
	set_difference(rel_names_from_man_patt_set.begin(), rel_names_from_man_patt_set.end(),
		rel_names_from_li_patt_set.begin(), rel_names_from_li_patt_set.end(), rels_in_man_only.begin());

	SessionLogger::info("intersec_rel_0") << "INTERSECTION OF LEARNIT AND MANUAL PATTERNS:\n";
	BOOST_FOREACH(std::wstring rel, intersec) {
		SessionLogger::info("intersec_rel_1") << rel;
	}
	SessionLogger::info("union_rel_0") << "UNION OF LEARNIT AND MANUAL PATTERNS:\n";
	BOOST_FOREACH(std::wstring rel, rel_union) {
		SessionLogger::info("union_rel_1") << rel;
	}
	SessionLogger::info("set_diff_00") << "RELATIONS IN LEARNIT ONLY:\n";
	BOOST_FOREACH(std::wstring rel, rels_in_li_only) {
		SessionLogger::info("set_diff_01") << rel;
	}
	SessionLogger::info("set_diff_10") << "RELATIONS IN MANUAL PATTERNS ONLY:\n";
	BOOST_FOREACH(std::wstring rel, rels_in_man_only) {
		SessionLogger::info("set_diff_11") << rel;
	}
	BOOST_FOREACH(ElfRelation_ptr rel, rel_from_li_patt_set) {
		std::ostringstream ostr;
		rel->dump(ostr);
		SessionLogger::info("rel_from_li_0") << ostr.str();
	}
	BOOST_FOREACH(ElfRelation_ptr rel, rel_from_man_patt_set) {
		std::ostringstream ostr;
		rel->dump(ostr);
		SessionLogger::info("rel_from_man_0") << ostr.str();
	}
}

void PredFinder::readLearnItPatternDatabase(const boost::filesystem::path & db_file, std::vector<LearnItPattern_ptr>& patterns) {
	if (!boost::filesystem::exists(db_file)) {
		std::ostringstream out;
		out << "Specified LearnIt pattern input (" << db_file << ") doesn't exist";
		throw std::runtime_error(out.str().c_str());
	}

	// Load the pattern database
	SessionLogger::info("read_li_3") << "  Reading LearnIt pattern DB " << db_file.string() << "...\n";
	LearnItDB_ptr pattern_db = boost::make_shared<LearnItDB>(db_file.string());

	// Collect all of the non-duplicate active patterns
	BOOST_FOREACH(LearnItPattern_ptr pattern, pattern_db->getPatterns(true)) {
		if (pattern->active()) {
			patterns.push_back(pattern);
		}
	}
}

// Manual patterns are also called manual patterns.
void PredFinder::readManualPatterns(const std::string & path, std::vector<PatternSet_ptr>& patterns) {
	boost::filesystem::path full_path;
	bool is_simple_file(false);
	if (path.empty()) {
		std::ostringstream out;
		out << "Manual pattern input is empty";
		throw std::runtime_error(out.str().c_str());
	}
	if (boost::algorithm::ends_with(path, ".sexp")) {
		is_simple_file = true;
		full_path = boost::filesystem::system_complete(boost::filesystem::path(path));
	}
	else {
		full_path = boost::filesystem::system_complete(boost::filesystem::path(path));
	}
	if (!boost::filesystem::exists(full_path)) {
		std::ostringstream out;
		out << "Specified Manual pattern input (" << full_path << ") doesn't exist";
		throw std::runtime_error(out.str().c_str());
	}

	// Read list of pattern files or walk through a directory of pattern files
	if (is_simple_file) {
		if (!boost::filesystem::is_regular_file(full_path)) {
			std::ostringstream out;
			out << "Manual pattern input (" << full_path << ") is not a regular file";
			throw std::runtime_error(out.str().c_str());
		}
		// Return the pattern read from the specified pattern file
		PredFinder::readManualPatternSet(full_path, patterns);
	} else if (boost::filesystem::is_directory(full_path)) {
		// Read patterns iteratively directory by directory
		PredFinder::readManualPatternsFromDir(full_path, patterns);
	} else {
		// Return all of the patterns read from each pattern file specified in the specified list
		PredFinder::readManualPatternsFromList(full_path, patterns);
	}
}

void PredFinder::readManualPatternSet(const boost::filesystem::path & pattern_file, std::vector<PatternSet_ptr>& patterns) {
	// Load the pattern set
	SessionLogger::info("patt_file_0") << "  Reading Manual pattern file " << pattern_file.string() << "...\n";
	boost::scoped_ptr<UTF8InputStream> pattern_stream_scoped_ptr(UTF8InputStream::build(pattern_file.string().c_str()));
	UTF8InputStream& pattern_stream(*pattern_stream_scoped_ptr);
	Sexp* pattern_sexp = _new Sexp(pattern_stream, true);
	patterns.push_back(boost::make_shared<PatternSet>(pattern_sexp));
}

void PredFinder::readManualPatternsFromList(const boost::filesystem::path & list_file, std::vector<PatternSet_ptr>& patterns) {
	std::string line;
	std::ifstream dbs_wifstream(list_file.string().c_str());

	// Try to load each specified pattern file
	SessionLogger::info("patt_list_0") << "Reading Manual patterns list " << list_file.string() << "...\n";
	while (dbs_wifstream.is_open() && getline(dbs_wifstream, line)) {
		boost::algorithm::trim(line);
		if (line != "" && line[0] != '#') {
			// Clean up the path
			boost::filesystem::path full_path = boost::filesystem::system_complete(boost::filesystem::path(line));
			if (!boost::filesystem::exists(full_path)) {
				std::ostringstream out;
				out << "Manual pattern input (" << full_path << ") doesn't exist.";
				throw std::runtime_error(out.str().c_str());
			}

			// Append any patterns from this file
			PredFinder::readManualPatternSet(full_path, patterns);
		}
	}
}

void PredFinder::readManualPatternsFromDir(const boost::filesystem::path & dir, std::vector<PatternSet_ptr>& patterns) {
	// Read each item from this directory in turn
	boost::filesystem::directory_iterator end;
	SessionLogger::info("patt_dir_0") << "Reading Manual patterns dir " << dir.string() << "...\n";
	for (boost::filesystem::directory_iterator dir_item(dir); dir_item != end; dir_item++) {
		if (boost::filesystem::is_directory(*dir_item)) {
			// Recurse
			PredFinder::readManualPatternsFromDir(*dir_item, patterns);
		} else {
			// Filter based on file suffix
			if (boost::algorithm::ends_with(BOOST_FILESYSTEM_DIR_ITERATOR_GET_FILENAME(dir_item), ".sexp")) {
				PredFinder::readManualPatternSet(*dir_item, patterns);
			}
		}
	}
}

void PredFinder::readSuperfiltersFromDir(const boost::filesystem::path & dir, std::vector<std::string>& files) {
	// Read each item from this directory in turn
	boost::filesystem::directory_iterator end;
	SessionLogger::info("filt_dir_0") << "Reading Superfilters dir " << dir.string() << "...\n";
	for (boost::filesystem::directory_iterator dir_item(dir); dir_item != end; dir_item++) {
		if (boost::filesystem::is_directory(*dir_item)) {
			// Recurse
			PredFinder::readSuperfiltersFromDir(*dir_item, files);
		} else {
			// Filter based on file suffix
			if (boost::algorithm::ends_with(BOOST_FILESYSTEM_DIR_ITERATOR_GET_FILENAME(dir_item), ".sequence")) {
				SessionLogger::info("filt_dir_0") << "Superfilter " << BOOST_FILESYSTEM_DIR_ITERATOR_GET_PATH(dir_item) << "...\n";
				files.push_back(dir_item->path().string());
			}
		}
	}
}

ElfDocumentTypeMap PredFinder::readInputElfDocuments(const std::string & path) {
	// Clean up the path
	if (path.empty()) {
		std::ostringstream out;
		out << "Empty input ELF path";
		throw std::runtime_error(out.str().c_str());
	}
	boost::filesystem::path full_path = boost::filesystem::system_complete(boost::filesystem::path(path));
	if (!boost::filesystem::is_directory(full_path)) {
		std::ostringstream out;
		out << "Specified input ELF directory (" << full_path << ") doesn't exist";
		throw std::runtime_error(out.str().c_str());
	}

	// Filename/docid filter
	boost::regex get_docid("(.*)\\..*elf\\.xml");

	// Read ELF documents from directory
	ElfDocumentTypeMap documents;
	boost::filesystem::directory_iterator end;
	SessionLogger::info("reading_elf_0") << "Reading input ELF dir " << path << "...\n";
	for (boost::filesystem::directory_iterator dir_item(full_path); dir_item != end; dir_item++) {
		if (boost::filesystem::is_regular_file(*dir_item)) {
			SessionLogger::info("reading_elf_1") << BOOST_FILESYSTEM_DIR_ITERATOR_GET_FILENAME(dir_item) << "\n";
			ElfDocument_ptr document = boost::make_shared<ElfDocument>(BOOST_FILESYSTEM_DIR_ITERATOR_GET_PATH(dir_item));
			std::pair<ElfDocumentTypeMap::iterator, bool> document_insert = documents.insert(
				ElfDocumentTypeMap::value_type(ElfDocumentTypeMap::key_type(document->get_contents(), document->get_source()), 
				ElfDocumentMap()));
			document_insert.first->second.insert(ElfDocumentMap::value_type(document->get_id(), document));
		}
	}
	return documents;
}

std::string PredFinder::get_opt_param(const std::string & param_name) {
	try {
		std::string val = ParamReader::getParam(param_name);
		if (val == NONE_STR) {
			return "";
		} 
		return val;
	} catch(std::exception &) {
		SessionLogger::err("param_err_10") << "Error in reading '" << param_name << "'. "
			<< "Note that values cannot be blank; use '" << NONE_STR << "' "
			<< "to indicate that the value assigned to a parameter should be an empty string.";
		throw;
	}
}

std::vector<std::string> PredFinder::get_opt_string_vector_param(const std::string & param_name) {
	std::string value_as_str = ParamReader::getParam(param_name);
	if (value_as_str.find(NONE_STR) != std::string::npos) {
		// NONE_STR was specified as a value (possibly the only value) for 
		// the parameter. Within PredFinder, we're going to use an
		// empty string vector.
		std::vector<std::string> ret;
		return ret;
	} else {
		return ParamReader::getStringVectorParam(param_name.c_str());
	}
}

std::string PredFinder::get_required_param(const std::string & param_name) {
	try {
		std::string val = ParamReader::getRequiredParam(param_name);
		if (val == NONE_STR) {
			return "";
		} 
		return val;
	} catch(std::exception &) {
		SessionLogger::err("param_err_11") << "Error in reading required param '" << param_name << "'. "
			<< "Note that values cannot be blank; use '" << NONE_STR << "' "
			<< "to indicate that the value assigned to a parameter should be an empty string.";
		throw;
	}
}
