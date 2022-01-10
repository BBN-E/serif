/**
 * Handles information that affects multiple documents (e.g., XDoc IDs).
 *
 * @file ElfMultiDoc.h
 * @author afrankel@bbn.com
 * @date 2011.05.31
 **/

#pragma once

#include "Generic/common/Symbol.h"
#include "Generic/common/Offset.h" // for EDTOffset
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/RelationSet.h" //for sub-GPE info
#include "Generic/theories/Relation.h" //for sub-GPE info
#include "PredFinder/common/ContainerTypes.h" // for StringReplacementMap, ElfIndividualMap, SetOfStrings, etc.
#include "PredFinder/db/PlayerDB.h" // for PlayerResultMap
#include "PredFinder/db/LocationDB.h"
#include "PredFinder/elf/ElfDocument.h"

#include "XDocIdType.h"
#include <string>
#include <map>
#include <set>
#include <vector>
class SynNode;
class DocTheory;
class Entity;
class Mention;
class TemporalNormalizer;
class PlayerDB;
BSP_DECLARE(PlayerDB);
BSP_DECLARE(TemporalNormalizer);

/**
 * Defines a simple string to string dictionary,
 * intended for use in a DocIdAndTypeToSRMMap for containing
 * pairs of string replacements to be performed. Now declared in ContainerTypes.h.
 *
 * @author nward@bbn.com
 * @date 2010.06.14
 **/
//typedef std::map<std::wstring, std::wstring> StringReplacementMap;

/**
 * Handles information that affects multiple documents (e.g., XDoc IDs).
 *
 * @author afrankel@bbn.com
 * @date 2011.05.31
 **/
class ElfMultiDoc {
protected:
	// TYPEDEFS

	/**
	 * Defines a nested lookup table that maps a document ID and
	 * a type string to a map of value string replacements.
	 *
	 * Used indirectly by the ElfRelationArg constructor to determine whether
	 * an ElfEntity should be generated, or if an ontology
	 * individual URI should be used instead.
	 *
	 * @author nward@bbn.com
	 * @date 2010.06.14
	 **/
	typedef std::map<std::pair<std::wstring, std::wstring>, StringReplacementMap> DocIdAndTypeToSRMMap;
	// corpus-level
	typedef std::map<std::wstring, StringReplacementMap> TypeToSRMMap;

    // "X2B" = XDoc ID to bound URI
    typedef std::map<XDocIdType, std::wstring> X2BMap;
	typedef std::pair<Symbol, std::wstring> EntityTypeAndNamePair;
	typedef std::pair<Symbol, XDocIdType> EntityTypeAndXdocIdPair;
	/**
	 * Stores cross-document cluster URIs by ACE entity type
	 * and name string.
	 *
	 * @author mfreedma@bbn.com
	 * @date 2010.08.29
	 **/
	typedef std::map<EntityTypeAndNamePair, std::wstring> EntityTypeAndNamePairToBoundUriMap;
	typedef std::map<EntityTypeAndNamePair, XDocIdType> EntityTypeAndNamePairToXDocIdMap;

	/**
	 * Stores name strings by ACE entity type and
	 * cross-document cluster (XDoc) URI.
	 *
	 * @author nward@bbn.com
	 * @date 2010.08.29
	 **/
	typedef std::map<EntityTypeAndXdocIdPair, std::vector<std::wstring> > EntityTypeAndXdocIdPairToNamesMap;

	typedef std::pair<int, int> CountAndGreatestLengthPair;
	/**
	 * Maps from a potential XDoc ID to a pair indicating (a) the number of times it has been encountered
	 * and (b) the greatest length (in terms of words) for a corresponding mention head found so far for the ID.
	 *
	 * @author mfreedma@bbn.com
	 * @date 2010.08.29
	 **/
	typedef std::map<XDocIdType, CountAndGreatestLengthPair> PotentialXdocIdToCountAndGreatestLengthMap;

    typedef std::map<int, std::set<XDocIdType> > IntToSetOfXDocIdsMap;
    typedef std::map<XDocIdType, std::set<int> > XDocIdToSetOfIntsMap;

public:
	// STATIC METHODS

	// INITIALIZATION
	static void initialize(const std::wstring & domain_prefix); // call this method prior to reading any files

	// Read the map of equivalent name strings (e.g., equiv_name_cluster_output.txt).  This should be called 
	// BEFORE init_type_w_or_wo_docid_to_srm_map() so that the SRM map can be expanded with the equivalent names.
	// Sample lines:
	//		1	GPE	soviet, soviet union, ussr, 
	//		2	GPE	china, chinese, people s republic of china, prc, sino, 
	static void load_xdoc_maps(const std::wstring & filename);
	// Read the list (e.g., problematic_cluster_output.txt) of XDoc IDs to be mapped to bound URIs,
	// with contents like the following: 
	//		u.s. military	NONE	ic:U.S._military
	//		izzedine al-qassam brigades	8832	ic:Izz_el-Deen_al-Qassam_organization
	//		al-qaeda	103	ic:al-Qaeda
	//		al-aqsa martyr's brigade	723	ic:Al-Aqsa_Martyr_s_Brigade
	//		abdul-majid al-khoei	192	ic:Abdul-Majid_al-Khoei
	//		king fahd	3178	ic:King_Fahd
	// and internally fills a static StringReplacementMap that maps from XDoc IDs to bound URIs such as the following:
	//		[5](("bbn:xdoc-103","ic:al-Qaeda"),("bbn:xdoc-192","ic:Abdul-Majid_al-Khoei"),("bbn:xdoc-3178","ic:King_Fahd"),
	//		("bbn:xdoc-723","ic:Al-Aqsa_Martyr_s_Brigade"),("bbn:xdoc-8832","ic:Izz_el-Deen_al-Qassam_organization"))
	// Reads same file read by EIUtils::load_xdoc_failures(), but this method looks at the ordinary lines
	// (i.e., the ones NOT containing the strings "NONE" or "MILITARY").
	static void load_mr_xdoc_output_file(const std::string& filename);

	// For each line read from a file specified by the ParamReader parameter "arg_map" (e.g., nfl.arg_map.tdl.txt), produces a mapping from a (docid, type) pair to a triplet of (name, URI) pairs
	// mapping from original, lowercased, or uppercased versions of the name (e.g., "cardinals") to a URI 
	// (e.g., "nfl:ArizonaCardinals").
	static void init_type_w_or_wo_docid_to_srm_map(const std::string & path);

	struct BoundTypeURI {
		BoundTypeURI(const std::wstring& type, const std::wstring& uri) :
			type(type), uri(uri) {}
		std::wstring type;
		std::wstring uri;
	};
	typedef std::vector<BoundTypeURI> BoundTypeURIs;

	struct BoundURIMapping {
		BoundURIMapping(const std::wstring& type, const std::wstring& str,
				const std::wstring& uri) : str(str), uriType(type, uri) {}
		std::wstring str;
		BoundTypeURI uriType;
	};
	
	typedef std::vector<BoundURIMapping> TitleBoundURIMap;
	static void load_title_bound_uris(const std::string& title_bound_uris);
	static void process_title_bound_uri_line(const std::vector<std::wstring>& tokens);
	static BoundTypeURIs title_bound_uri(const std::wstring& curType, const std::wstring& lookup);
	static BoundTypeURIs title_bound_uri_internal(const std::wstring& curType, const std::wstring& lookup);
	
	// Initialize a previously produced SQL DB with roster information for teams over a number of seasons.
	// ParamReader Parameter: "world_knowledge_db" (e.g. players.db)
	static void init_nfl_player_db(const std::string & path);
	static void init_location_db(const std::string & path);
	// Top-level method. 
	static void addGPERelationsToElfDoc(ElfDocument_ptr elfDoc, const DocTheory* doc_theory);
	// Top-level method. Updates _eid_to_xdoc_ids, _eid_to_bound_uris, _eid_to_bound_uris_merged.
	// Reads _xdoc_id_to_bound_uri, which must have already been set by load_mr_xdoc_output_file().
	static void generate_entity_and_id_maps(const DocTheory* doc_theory);
	// Fills _name_docid_to_bound_uris, _name_to_bound_uris, and _bound_uri_to_types. 
	// See method definition for more detail.
	static void generate_typeless_maps();

	static std::wstring get_ontology_domain() {return _ontology_domain;}

	// HIGHER-LEVEL
	// Dump methods
	static void dump_all(); // dump all nontrivial maps
	static void dump_eid_maps();
	static void dump_non_eid_maps();

	// Replace appropriate XDoc cluster IDs with URIs from the loaded argument value map.
	// Called after generate_typeless_maps() by PredFinder.
	static void replace_xdoc_ids_with_mapped_args();
	// Adds/updates the entity-id-to-xdoc_id and entity-id-to-bound-uri mappings for the entity ID.
	// Makes sure that all entity IDs in other_ids point to the specified primary XDoc ID.
	// Called by EIUtils::attemptNewswireLeadFix().
	static void merge_entities_in_map(const DocTheory* doc_theory, int primary_eid, const std::set<int> & other_eids);

	// LOWER-LEVEL
	// Retrieval methods
	static std::wstring get_mapped_arg(const std::wstring & best_name, const std::wstring & type_string, 
		const DocTheory* doc_theory, const Entity* entity, std::wstring & bound_uri_out, XDocIdType & xdoc_id_out);
	// always returns either a bound URI or an empty string, never an XDoc ID
	static std::wstring get_mapped_arg_for_nfl_team(const std::wstring & best_name, const std::wstring & type_string, 
											const DocTheory* doc_theory, const Entity* entity);
	static std::wstring find_best_text_for_mention(const DocTheory* doc_theory, const Mention* mention, 
		std::wstring & mention_type, EDTOffset& start_offset, EDTOffset& end_offset);
	// Retrieves the types associated with a bound URI.
	static SetOfStrings get_types_for_bound_uri(const std::wstring & id);
	static XDocIdType get_xdoc_id(const Symbol & entity_type, const std::wstring & name_string);
	static XDocIdType get_xdoc_id_and_entity_type(const std::wstring & name_string, Symbol& matched_entity_type);
	static std::vector<std::wstring> get_other_names(const Symbol & entity_type, const std::wstring & name_string);
	static std::wstring find_bound_uri_from_name_w_or_wo_docid(const std::wstring & s, const std::wstring & docid);
	static std::wstring find_bound_uri_from_name_and_docid(const std::wstring & s, const std::wstring & docid);
	static std::wstring find_bound_uri_from_name(const std::wstring & s);
	// Never used
	//static std::vector<std::wstring> get_strings_from_cluster(const EntityTypeAndXdocIdPairToNamesMap::key_type & cluster);
	static std::set<XDocIdType> get_xdoc_ids_from_eid(int entity_id); //_eid_to_xdoc_ids
	// Not currently called
	static SetOfStrings get_bound_uris_from_eid(int entity_id); //_eid_to_bound_uris
	static PlayerResultMap get_teams_for_player(const std::wstring & player, int season);
	// Once we change over to a scheme where the caller decides which URI is best, we could
	// get rid of this method. All methods that use the return value from this method as their own return value
	// would then need to be made void.
	static std::wstring best_uri_from_bound_uri_and_xdoc_id(const std::wstring & bound_uri, const XDocIdType xdoc_id) {
		if (!bound_uri.empty()) {
			return bound_uri;
		} else {
			return xdoc_id.as_string();
		}
	}
	// Return the current value of _next_special_xdoc_id_to_assign and decrement it so the next call
	// will get the next lower value.
	static int get_next_special_xdoc_id_to_assign() {
		return (_next_special_xdoc_id_to_assign--);}

	static void output_location_db_counts();

	static TemporalNormalizer_ptr temporal_normalizer;

private:
	// STATIC METHODS
	// This method is called by ElfMultiDoc::initialize(ontology_domain), so there's no need to call it directly.
	static void set_ontology_domain(const std::wstring & ontology_domain) {_ontology_domain = ontology_domain;}
	static std::wstring find_bound_uri_from_name_and_docid_in_specified_map(const std::wstring & s, const std::wstring & docid, 
		const StringPairToSetOfStringsMap& name_docid_to_bound_uris);
	static std::wstring find_bound_uri_from_name_in_specified_map(const std::wstring & s, 
		const StringToSetOfStringsMap& name_to_bound_uris);
	static bool get_mapped_arg_from_doc_level_uri_assignment(const std::wstring & best_name, const std::wstring & type_string, 
											const DocTheory* doc_theory, const Entity* entity,
											std::wstring & bound_uri_out, XDocIdType & xdoc_id_out);
	static std::wstring get_mapped_arg_when_no_srm_entry_found(const std::wstring & best_name, const std::wstring & type_string, 
											const DocTheory* doc_theory, const Entity* entity, XDocIdType & xdoc_id_out);

	// methods called (directly or indirectly) by generate_entity_and_id_maps()
	static IntToSetOfStringsMap get_xdoc_conflicts(const DocTheory* doc_theory);
	static int add_bound_uris_for_entity_w_or_wo_docid(const Entity* entity, const std::wstring & name_string, 
		const std::wstring & docid, IntToSetOfStringsMap & eid_to_bound_uris,
		StringToSetOfIntsMap & bound_uri_to_eids, const DocTheory* doc_theory);
	static int add_bound_uris_for_entity_w_docid(const Entity* entity, const std::wstring & name_string, 
		const std::wstring & docid, IntToSetOfStringsMap & eid_to_bound_uris,
		StringToSetOfIntsMap & bound_uri_to_eids, const DocTheory* doc_theory);
	static int add_bound_uris_for_entity_wo_docid(const Entity* entity, const std::wstring & name_string, 
		IntToSetOfStringsMap & eid_to_bound_uri,
		StringToSetOfIntsMap & bound_uri_to_eids, const DocTheory* doc_theory);
	static int add_eid_to_bound_uri_mappings(const Entity* entity, const SetOfStrings & uris,
		IntToSetOfStringsMap & eid_to_bound_uris,
		StringToSetOfIntsMap & bound_uri_to_eids, const DocTheory* doc_theory);
	static void transfer_to_best_xdoc_id(const Entity* entity, const IntToSetOfXDocIdsMap & eid_to_xdoc_id_current, 
		const XDocIdToSetOfIntsMap& xdoc_id_to_eid_current,  
		IntToSetOfXDocIdsMap & eid_to_xdoc_id_final,
		XDocIdToSetOfIntsMap& xdoc_id_to_eid_final,
		const DocTheory* doc_theory, bool allow_multiple_entities_to_share_id = false);
	static void transfer_to_best_bound_uri(const Entity* entity, const IntToSetOfStringsMap & eid_to_bound_uri_current, 
		const StringToSetOfIntsMap & bound_uri_to_eid_current,  
		IntToSetOfStringsMap & eid_to_bound_uri_final,
		StringToSetOfIntsMap & bound_uri_to_eid_final,
		const DocTheory* doc_theory, bool allow_multiple_entities_to_share_id = false);

	static void handle_found_xdoc_id_to_eid(const XDocIdType & possible_id, const DocTheory* doc_theory, 
		const Entity* entity, const XDocIdType & best_xdoc_id, 
		IntToSetOfXDocIdsMap& eid_to_xdoc_id_final,
		XDocIdToSetOfIntsMap& xdoc_id_to_eid_final, bool allow_multiple_entities_to_share_id);
	static void handle_found_bound_uri_to_eid(const std::wstring & possible_bound_uri, const DocTheory* doc_theory, 
											 const Entity* entity, const std::wstring & best_bound_uri,
											 IntToSetOfStringsMap& eid_to_bound_uri_final,
											 StringToSetOfIntsMap& bound_uri_to_eid_final,
											 bool allow_multiple_entities_to_share_id);
	static void handle_multiple_other_entities_for_xdoc_id(const std::set<int> & other_entities, const XDocIdType & possible_id, 
		const DocTheory* doc_theory, const Entity* entity, const XDocIdType & best_xdoc_id,
		IntToSetOfXDocIdsMap & eid_to_xdoc_id_final, 
		XDocIdToSetOfIntsMap & xdoc_id_to_eid_final);
	static void handle_multiple_other_entities_for_bound_uri(const std::set<int> & other_entities, const std::wstring & possible_id, 
		const DocTheory* doc_theory, const Entity* entity, const std::wstring & best_bound_uri,
		IntToSetOfStringsMap & eid_to_bound_uri_final, 
		StringToSetOfIntsMap & bound_uri_to_eid_final);
	static void add_xdoc_bound_entities(const Entity *entity, IntToSetOfStringsMap & eid_to_bound_uri_all, 
		StringToSetOfIntsMap & bound_uri_to_eids_all, 
		const DocTheory* doc_theory, const std::wstring & best_name_mention_first, int & n_added);
	//get the different possible entity names from xdoc info
	static std::set<std::wstring> get_xdoc_entity_names(const Entity *entity, const DocTheory* doc_theory);
	static void generate_entity_and_bound_uri_maps(const DocTheory* doc_theory, 
		const IntToSetOfStringsMap & eid_to_alignable_strings,
		const IntToSetOfStringsMap & multi_match_entities_ids);
	static void add_to_bound_uri_tables(const Entity* entity, 
		const std::wstring & bound_uri, 
		IntToSetOfStringsMap & eid_to_ids_current, 
		StringToSetOfIntsMap & id_to_eids_current);
	static void add_to_xdoc_id_tables(const Entity* entity, const XDocIdType & xdoc_id, 
		IntToSetOfXDocIdsMap & eid_to_ids_current, 
		XDocIdToSetOfIntsMap & id_to_eids_current);
	// methods called (directly or indirectly) by get_mapped_arg
	static void collect_potential_xdoc_ids(const EntitySet * entity_set, const Entity * entity, const Symbol & entity_type, 
		PotentialXdocIdToCountAndGreatestLengthMap & out_id_map);
	static XDocIdType get_best_potential_xdoc_id(const Entity* entity, 
		const PotentialXdocIdToCountAndGreatestLengthMap & potential_ids, const XDocIdType & best_id_so_far);

	static bool is_valid_ontology_type_for_entity_type(const Entity* entity, SetOfStrings types, 
												   const DocTheory* doc_theory);
	// called (directly or indirectly) by load_xdoc_maps()
	static void store_entity_type_and_name_to_xdoc_id(const std::vector<std::wstring> & trimmed_names, 
										  Symbol entity_type, XDocIdType & xdoc_id);
	static bool update_map_from_cluster_split(const std::vector<SetOfStrings > & cluster_split,
												Symbol entity_type, const XDocIdType & xdoc_id);
	static bool find_and_handle_conflicts(const std::vector<std::wstring> & trimmed_names, const XDocIdType & xdoc_id, 
											Symbol entity_type,	SetOfStrings & conflict_names);
	static bool handle_conflicts(const std::vector<std::wstring> & trimmed_names, const XDocIdType & xdoc_id, 
		Symbol entity_type, const SetOfStrings & conflict_names);
	static SetOfStrings get_possible_acronyms(const std::wstring & s);
	static SetOfStrings get_possible_acronyms_from_syn_node(const SynNode* n);
	static int new_xdoc_id_from_split_index(int orig_xdoc_id, int split_index);
	// misc methods
	static void print_info_for_entity_with_multiple_xdoc_ids(const Entity* entity, const DocTheory* doc_theory, 
															   const std::set<XDocIdType> & ids);
	static bool has_nationality_individual(const DocTheory* doc_theory, const Entity* entity);
	static void update_replacement_id(int entity_type_i, const std::wstring & lookup_id, const std::wstring & boundString,
									 const XDocIdType & xdocID, XDocIdType & replacementXDocID);
	static bool ok_to_link(const XDocIdType & new_xdoc_id, const XDocIdType & xdoc_id,
							 const std::wstring & normalized_best_string, const std::wstring & name_string,
							 const Entity* entity, const SynNode* head, const SynNode* bestHead,
							 const SetOfStrings & do_not_link_names);
	// Special disambiguation methods
	static std::wstring get_best_name_for_bush(const std::wstring & best_name, const Entity* entity, const DocTheory* doc_theory,
		std::wstring & bound_uri_out, XDocIdType & xdoc_id_out);
    // Always returns a bound URI or empty string, never an XDoc ID
	static std::wstring get_best_name_for_ny_nfl_team(const DocTheory* doc_theory);
	static void process_srm_map_line_w_docid(const std::vector<std::wstring> & line_tokens);
	static void process_srm_map_line_wo_docid(const std::vector<std::wstring> & line_tokens);

	// Not currently used
	//static void unused_method(const DocTheory* doc_theory, const IntToSetOfStringsMap & eid_to_alignable_strings);

	// STATIC DATA MEMBERS
	static std::wstring _ontology_domain;
	static PlayerDB_ptr _nfl_player_db;
	static LocationDB_ptr _location_db;
	static int _max_xdoc_id_in_file;
	// Used to allocate XDoc IDs that are neither in the original cluster file nor assigned to split clusters.
	// In order to minimize the possibility of a clash, this value starts at INT_MAX-1 and is decremented 
	// (within ElfMultiDoc) every time a new one is assigned.
	static int _next_special_xdoc_id_to_assign;
	static SetOfStrings _org_subtypes;
	static SetOfStrings _gpe_loc_subtypes;
	static SetOfStrings _wea_subtypes;
	static SetOfStrings _per_subtypes;
	static SetOfStrings _fac_subtypes;
	static SetOfStrings _per_gpe_org_subtypes;
	static SetOfStrings _gpe_org_subtypes;
	//static SetOfStrings _any_subtypes;
	static bool _has_location_db;

	/**
	* uri --> ontology types
	* Generated by generate_typeless_maps()
	* Bound URIs ARE NOT cleared between documents
	*/
	static bool _initialized_entity_type_and_name_to_xdoc_id;
	static EntityTypeAndNamePairToBoundUriMap _entity_type_and_name_to_bound_uri;
	static EntityTypeAndNamePairToXDocIdMap _entity_type_and_name_to_xdoc_id;
	static EntityTypeAndXdocIdPairToNamesMap _entity_type_and_xdoc_id_to_names;
	static StringToSetOfStringsMap _bound_uri_to_types;
	static X2BMap _xdoc_id_to_bound_uri;

	static TitleBoundURIMap _title_bound_uris;

	// A map to an SRM (StringReplacementMap) is referred to elsewhere as an arg_map.
	static DocIdAndTypeToSRMMap _docid_type_to_srm; // per-document mapping
	static TypeToSRMMap _type_to_srm; // per-corpus mapping
	// (normalized name, docid) -> bound_uri; generated by generate_typeless_maps()
	static StringPairToSetOfStringsMap _name_docid_to_bound_uris;
	static StringToSetOfStringsMap _name_to_bound_uris;
	/********************************************************/
	/**
	* Entity ID --> XDoc IDs.  Generated (and cleared) by generate_entity_and_id_maps()
	* By convention, only allow one XDoc for each Entity ID
	* MUST CLEAR FOR EVERY DOCUMENT
	* Evil Hack: for NFL, only NFLTeams get 'bound URIs', Locations/GPEs/Etc. will get XDocids
	*/
	static IntToSetOfXDocIdsMap _eid_to_xdoc_ids;	
	/**
	* Entity ID --> Bound IDs.  Generated (and cleared) by generate_entity_and_id_maps()
	* By convention, only allow one bound URI for each Entity ID
	* MUST CLEAR FOR EVERY DOCUMENT
	*/
	static IntToSetOfStringsMap _eid_to_bound_uris; 
	/**
	* Entity ID --> Bound IDs.  Generated (and cleared) by generate_entity_and_id_maps()
	* By convention, only allow one BoundId for each Entity ID
	* Entities may map to the same bound URI
	* MUST CLEAR FOR EVERY DOCUMENT
	*/
	static IntToSetOfStringsMap _eid_to_bound_uris_merged; 
	/********************************************************/
};

