/**
 * Parallel implementation of ElfDocument object based on Python
 * implementation in ELF.py. Uses Xerces-C++ for XML writing.
 *
 * @file ElfDocument.h
 * @author nward@bbn.com
 * @date 2010.06.16
 **/

#pragma once

#include "Generic/common/bsp_declare.h"
#include "PredFinder/common/ContainerTypes.h" // for StringReplacementMap
#include "ElfRelation.h" // for ElfRelationMap

#include "xercesc/dom/DOM.hpp"

#include <string>
#include <vector>
#include <set>
#include "boost/make_shared.hpp"

// Forward declarations to use shared pointer for class in class
BSP_DECLARE(ElfDocument);
BSP_DECLARE(ElfRelation);
BSP_DECLARE(ElfRelationArg);
BSP_DECLARE(ElfIndividual);
BSP_DECLARE(ElfInference);
BSP_DECLARE(PatternSet);
BSP_DECLARE(Pattern);
BSP_DECLARE(EIDocData);
BSP_DECLARE(LearnItPattern);
BSP_DECLARE(LearnIt2Matcher);
BSP_DECLARE(TemporalAttributeAdder)

/**
 * Contains one document, which may have any number of relations
 * and their associated entities and metadata. Used to produce all
 * of the pattern matches on a given Serif Document and convert it
 * into a <doc> XML element.
 *
 * @author nward@bbn.com
 * @date 2010.05.14
 **/
class ElfDocument {
public:
	// docData is the return value from a previous call to ElfInference::prepareDocumentForInference(), 
	// invoked on the same Serif document
	ElfDocument(EIDocData_ptr docData, const ElfInference_ptr elfInference, 
		const std::vector<LearnItPattern_ptr> & learnit_patterns = std::vector<LearnItPattern_ptr>(), 
		const std::vector<PatternSet_ptr> & manual_patterns = std::vector<PatternSet_ptr>(),
		const std::vector<LearnIt2Matcher_ptr> & learnit2_matchers = std::vector<LearnIt2Matcher_ptr>(),
		TemporalAttributeAdder_ptr temporalAttributeAdder = TemporalAttributeAdder_ptr());
	ElfDocument(const std::string & input_file);
	ElfDocument(const ElfDocument_ptr other);
	ElfDocument(const DocTheory* doc_theory, const std::vector<ElfDocument_ptr> & others);

	/**
	 * Inlined accessor to get the documents's unique identifier.
	 *
	 * @return This documents's globally-unique string _id.
	 *
	 * @author nward@bbn.com
	 * @date 2010.08.31
	 **/
	std::wstring get_id(void) const { return _id; }

	/**
	 * Inlined accessor to get the documents's source.
	 *
	 * @return This documents's generating site _source
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.19
	 **/
	std::wstring get_source(void) const { return _source; }

	/**
	 * Inlined accessor to get the documents's contents.
	 *
	 * @return This documents's ELF level.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.08
	 **/
	std::wstring get_contents(void) const { return _contents; }

	/**
	 * Inlined mutator to set the documents's contents.
	 *
	 * @param contents This documents's new ELF level.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.08
	 **/
	void set_contents(const std::wstring & contents) { _contents = contents; }

	/**
	 * Inlined accessor to get the ElfRelations in this document.
	 *
	 * @return The _relations std::set.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.13
	 **/
	std::set<ElfRelation_ptr> get_relations(void) const { return _relations; }

	/**
	 * Inlined mutator to add ElfRelations to this document.
	 *
	 * @param relations A std::vector of relations to add.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.13
	 **/
	void add_relations(const std::set<ElfRelation_ptr> & relations) { _relations.insert(relations.begin(), relations.end()); }

	/**
	 * Inlined accessor to count the number of ElfRelations
	 * in this document.
	 *
	 * @return The length of the _relations std::vector.
	 *
	 * @author nward@bbn.com
	 * @date 2010.05.14
	 **/
	int get_n_relations(void) const { return _relations.size(); }

	/**
	 * Inlined accessor to count the number of ElfIndividuals
	 * in this document.
	 *
	 * @return The key count of the _individuals std::map.
	 *
	 * @author nward@bbn.com
	 * @date 2010.06.24
	 **/
	int get_n_individuals(void) const { return _individuals.size(); }

	void remove_relations(const std::set<ElfRelation_ptr> & relations_to_remove);
	void remove_individuals(const ElfIndividualSet & individuals_to_remove);

	ElfIndividualSet get_individuals_by_type(const std::wstring & search_type = L"") const;
	std::vector<ElfIndividualSet> get_individuals_by_type_and_sentence(const DocTheory* doc_theory, const std::wstring & search_type = L"") const;
	ElfIndividual_ptr get_individual_by_generated_uri(const std::wstring & generated_uri) const;
	ElfIndividualSet get_merged_individuals_by_type(const std::wstring & search_type = L"") const;
	ElfIndividual_ptr get_merged_individual_by_uri(const std::wstring & uri) const;
	ElfRelationMap get_relations_by_individual(const ElfIndividual_ptr search_individual) const;
	ElfRelationMap get_relations_by_arg(const ElfRelationArg_ptr search_arg) const;
	bool individual_in_relation(const ElfIndividual_ptr individual, 
			const std::wstring& relation_type);

	void insert_individual(const ElfIndividual_ptr individual);

	void split_duplicate_role_relations();
	void convert_individuals_to_relations(const std::wstring& domain_prefix);

	std::wstring replace_uri_prefix(const std::wstring & uri);

	void do_document_level_individual_coreference(const std::wstring& domain_prefix, 
		const std::vector<std::wstring> & coreferent_types = std::vector<std::wstring>());
	void do_sentence_level_individual_coreference(const DocTheory* doc_theory,
		const std::wstring& domain_prefix, const std::vector<std::wstring> & coreferent_types = std::vector<std::wstring>());

	void replace_individual_uris(const ElfIndividualUriMap & individual_uri_map);
	void add_individual_coref_uris(const ElfIndividualUriMap & individual_uri_map);

	void dump_individuals_by_type(const std::wstring & output_dir, 
		const std::vector<std::wstring> & types = std::vector<std::wstring>()) const;

    xercesc::DOMDocument* to_xml(void) const;
	void to_file(const std::wstring & output_dir) const;

protected:
	ElfIndividualSet get_merged_individuals(void) const;
	void addTemporalAttributes(TemporalAttributeAdder_ptr temporalAttributeAdder,
		ElfRelation_ptr rel, EIDocData_ptr docData, int sent_index);

private:
	/**
	 * The corpus-unique document identifier, loaded from
	 * a distill-doc in a Serif DistillationDoc object.
	 **/
	std::wstring _id;

	/**
	 * The current version of the ELF schema implemented by this
	 * document object hierarchy.
	 **/
	std::wstring _version;

	/**
	 * The executable or script that produced this particular
	 * ELF document; currently only PredFinder uses these objects.
	 **/
	std::wstring _source;

	/**
	 * The semantic contents of the various relation, entity, and
	 * argument fields in this document; currently PredFinder
	 * always produces P-ELF output.
	 **/
	std::wstring _contents;

	/**
	 * All of the relations in this document.
	 **/
	std::set<ElfRelation_ptr> _relations;

	/**
	 * Uniquified cache of all of the individuals either used in
	 * relation arguments or otherwise found in this document,
	 * stored by ElfIndividual ID.
	 **/
	ElfIndividualSet _individuals;
};

/**
 * Defines a simple string to ElfDocument dictionary,
 * intended for use in PredFinder for reading existing
 * ELF. Keys should be the document's ID.
 *
 * @author nward@bbn.com
 * @date 2010.08.31
 **/
typedef std::map<std::wstring, ElfDocument_ptr> ElfDocumentMap;

/**
 * Defines a simple string to ElfDocumentMap dictionary,
 * intended for use in PredFinder for reading existing
 * ELF. Keys should be the document's ELF type and source.
 *
 * @author nward@bbn.com
 * @date 2010.10.25
 **/
typedef std::map<std::pair<std::wstring, std::wstring>, ElfDocumentMap> ElfDocumentTypeMap;
