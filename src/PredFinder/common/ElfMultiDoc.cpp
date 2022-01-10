/**
 * Handles information pertaining to information across multiple documents.
 *
 * @file ElfMultiDoc.cpp
 * @author afrankel@bbn.com
 * @date 2011.05.31
 **/
#pragma warning(disable:4996)

#include "Generic/common/leak_detection.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/foreach_pair.hpp"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/ASCIIUtil.h"
#include "Generic/edt/CorefUtilities.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/Value.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/SynNode.h"
#include "Generic/values/TemporalNormalizer.h"
#include "Generic/common/NationalityRecognizer.h"
#include "Generic/common/WordConstants.h"
#include "LearnIt/MainUtilities.h"
#include "ElfMultiDoc.h"
#include "PredFinder/elf/ElfIndividual.h"
#include "PredFinder/elf/ElfType.h"
#include "boost/bind.hpp"
#include "boost/make_shared.hpp"
#include "boost/lexical_cast.hpp"
#pragma warning(push, 0)
#include "boost/regex.hpp"
#pragma warning(pop)
#include "boost/foreach.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/algorithm/string/trim.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/predicate.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/convenience.hpp"
#include <boost/scoped_ptr.hpp>


const std::wstring PER_GPE_ORG_ENTITY_TYPES[] = { L"PER", L"GPE", L"ORG" };
const size_t PER_GPE_ORG_ENTITY_TYPE_COUNT = sizeof(PER_GPE_ORG_ENTITY_TYPES)/sizeof(std::wstring);

const std::wstring DO_NOT_LINK_BASE_NAMES[] = {
	L"arab", L"muslim", L"islamic", L"un ", L"u n ", L"united nations", L"world bank"};
const size_t DO_NOT_LINK_BASE_NAMES_COUNT = sizeof(DO_NOT_LINK_BASE_NAMES)/sizeof(std::wstring);

const std::wstring GPE_DO_NOT_LINK_BASE_NAMES[] = {
	L"north", L"south", L"east", L"west", L"northern", L"southern", L"eastern", L"western"};
const size_t GPE_DO_NOT_LINK_BASE_NAMES_COUNT = sizeof(GPE_DO_NOT_LINK_BASE_NAMES)/sizeof(std::wstring);

typedef std::pair<std::wstring, const Mention*> name_mention_t;

int ElfMultiDoc::_max_xdoc_id_in_file = 0;
int ElfMultiDoc::_next_special_xdoc_id_to_assign = INT_MAX - 1;

ElfMultiDoc::X2BMap ElfMultiDoc::_xdoc_id_to_bound_uri;
StringPairToSetOfStringsMap ElfMultiDoc::_name_docid_to_bound_uris;
StringToSetOfStringsMap ElfMultiDoc::_name_to_bound_uris;
StringToSetOfStringsMap ElfMultiDoc::_bound_uri_to_types;
ElfMultiDoc::IntToSetOfXDocIdsMap ElfMultiDoc::_eid_to_xdoc_ids;
IntToSetOfStringsMap ElfMultiDoc::_eid_to_bound_uris;
IntToSetOfStringsMap ElfMultiDoc::_eid_to_bound_uris_merged;
std::wstring ElfMultiDoc::_ontology_domain = L"";

ElfMultiDoc::TitleBoundURIMap ElfMultiDoc::_title_bound_uris;

SetOfStrings ElfMultiDoc::_org_subtypes;
SetOfStrings ElfMultiDoc::_gpe_loc_subtypes;
SetOfStrings ElfMultiDoc::_wea_subtypes;
SetOfStrings ElfMultiDoc::_per_subtypes;
SetOfStrings ElfMultiDoc::_fac_subtypes;
SetOfStrings ElfMultiDoc::_per_gpe_org_subtypes;
SetOfStrings ElfMultiDoc::_gpe_org_subtypes;
//SetOfStrings ElfMultiDoc::_any_subtypes;

/**
 * Stores a mapping from entity (ACE) type/name string
 * pairs to best bound URI or XDoc ID. Loaded
 * by ElfMultiDoc::load_xdoc_maps.
 *
 * @author mfreedma@bbn.com
 * @date 2010.08.29
 **/
ElfMultiDoc::EntityTypeAndNamePairToBoundUriMap ElfMultiDoc::_entity_type_and_name_to_bound_uri;
ElfMultiDoc::EntityTypeAndNamePairToXDocIdMap ElfMultiDoc::_entity_type_and_name_to_xdoc_id;

/**
 * Stores a mapping from entity (ACE) type/xdoc URI
 * pairs to clustered names. Loaded
 * by ElfMultiDoc::load_xdoc_maps.
 *
 * @author nward@bbn.com
 * @date 2010.08.29
 **/
ElfMultiDoc::EntityTypeAndXdocIdPairToNamesMap ElfMultiDoc::_entity_type_and_xdoc_id_to_names;

/**
 * Flag indicating whether _entity_type_and_name_to_xdoc_id
 * has been succesfully loaded; we can't just
 * check for emptiness since the map is optional.
 *
 * @author mfreedma@bbn.com
 * @date 2010.08.29
 **/
bool ElfMultiDoc::_initialized_entity_type_and_name_to_xdoc_id = false;

/**
 * Lookup table that maps type strings to value string
 * replacement tables. Used during ElfRelationArg
 * construction to determine if a PatternMatch produces
 * a value that is an ontology individual, or if the
 * Mention should become linked to an ElfEntity by _id.
 *
 * This is a hack that will hopefully be eliminated by
 * better ontology support in the future.
 *
 * @author nward@bbn.com
 * @date 2010.06.14
 **/
ElfMultiDoc::DocIdAndTypeToSRMMap ElfMultiDoc::_docid_type_to_srm;
ElfMultiDoc::TypeToSRMMap ElfMultiDoc::_type_to_srm;

/**
 * Wrapper object for access to cached tables from
 * a sqlite world knowledge database containing
 * NFL player-team associations.
 *
 * @author nward@bbn.com
 * @date 2010.08.17
 **/
PlayerDB_ptr ElfMultiDoc::_nfl_player_db;
LocationDB_ptr ElfMultiDoc::_location_db;
bool ElfMultiDoc::_has_location_db;

/**
 * Used to extract and parse document dates during
 * ElfRelationArg construction, if the special docDate
 * role return value was specified.
 *
 * Treated as a singleton static member; it gets initialized
 * by the PredFinder class.
 *
 * @author nward@bbn.com
 * @date 2010.08.13
 **/
TemporalNormalizer_ptr ElfMultiDoc::temporal_normalizer;

// executed prior to reading any files (world knowledge ("WK") or otherwise)
void ElfMultiDoc::initialize(const std::wstring & domain_prefix) {
	set_ontology_domain(domain_prefix);

	_org_subtypes.insert(L"ic:EducationalInstitution");
	_org_subtypes.insert(L"ic:TerroristOrganization");
	_org_subtypes.insert(L"ic:EducationalInstitution");
	_org_subtypes.insert(L"ic:CommercialOrganization");
	_org_subtypes.insert(L"ic:DemocraticPoliticalOrganization");
	_org_subtypes.insert(L"ic:PoliticalParty");   
	_gpe_loc_subtypes.insert(L"ic:GeopoliticalEntity");
	_gpe_loc_subtypes.insert(L"ic:GeographicalArea");
	_gpe_loc_subtypes.insert(L"ic:Municipality");
	_gpe_loc_subtypes.insert(L"ic:Location");
	_gpe_loc_subtypes.insert(L"ic:NationState");
	_gpe_loc_subtypes.insert(L"ic:StateOrProvince");
	_per_subtypes.insert(L"ic:Person");
	_per_subtypes.insert(L"ic:PersonClass");
	_gpe_org_subtypes.insert(L"ic:HumanOrganization");//usually ORG, but occasional GPE (e.g. EU) slips in
	_gpe_org_subtypes.insert(L"ic:GovernmentOrganization");
	_per_gpe_org_subtypes.insert(L"ic:HumanAgentClass");
	_per_gpe_org_subtypes.insert(L"ic:HumanAgent");
	_per_gpe_org_subtypes.insert(L"ic:PersonGroup");
	_per_gpe_org_subtypes.insert(L"ic:Citizenship");
	_per_gpe_org_subtypes.insert(L"ic:HumanAgentGroup");
	_wea_subtypes.insert(L"ic:BombClass");
	_wea_subtypes.insert(L"ic:WeaponClass");
	_fac_subtypes.insert(L"ic:BuildingClass");
	//any_subtypes.insert(L"ic:PhysicalThingClass");
	//any_subtypes.insert(L"ic:PhysicalThing");
}

std::wstring ElfMultiDoc::find_bound_uri_from_name_w_or_wo_docid(const std::wstring & s, const std::wstring & docid) {
	std::wstring uri = find_bound_uri_from_name_and_docid(s, docid);
	if (uri.empty()) {
		uri = find_bound_uri_from_name(s);
	}
	return uri;
}
/**
 *  See whether there's a mapping from the normalized name and docid to a URI.
 *  If so, return it.  Otherwise, return an empty string.
 **/
std::wstring ElfMultiDoc::find_bound_uri_from_name_and_docid_in_specified_map(const std::wstring & s, 
																			  const std::wstring & docid, 
										const StringPairToSetOfStringsMap& name_docid_to_bound_uris){
	std::wstring normalized_best_string = UnicodeUtil::normalizeTextString(s);
	std::wstring docid_to_search(docid);
	SetOfStrings uris;
	StringPairToSetOfStringsMap::const_iterator val = name_docid_to_bound_uris.find(
		std::pair<std::wstring, std::wstring>(normalized_best_string, docid_to_search));
	if(val != _name_docid_to_bound_uris.end()){
		uris = val->second;
	}
	std::wstring uri;
	size_t uri_count = uris.size();
	if (uri_count != 0) {
		uri = *(uris.begin());
	}
	//SessionLogger::info("LEARNIT") << "name: <" << normalized_best_string << "> and docid: <" << docid_to_search << 
	//	"> yielded bound URI: <" << uri << ">" << std::endl;
	//if (uri_count > 1) {
	//	SessionLogger::info("LEARNIT") << "Warning: returned first URI match for team.";
	//}
	return uri;
}

/**
 *  See whether there's a mapping from the normalized name to a URI.
 *  If so, return it.  Otherwise, return an empty string.
 **/
std::wstring ElfMultiDoc::find_bound_uri_from_name_in_specified_map(const std::wstring & s,
										const StringToSetOfStringsMap& name_to_bound_uris){
	std::wstring normalized_best_string = UnicodeUtil::normalizeTextString(s);
	SetOfStrings uris;
	StringToSetOfStringsMap::const_iterator val = name_to_bound_uris.find(normalized_best_string);
	if(val != name_to_bound_uris.end()){
		uris = val->second;
	}
	std::wstring uri;
	size_t uri_count = uris.size();
	if (uri_count != 0) {
		uri = *(uris.begin());
	}
	//SessionLogger::info("LEARNIT") << "name: <" << normalized_best_string << 
	//	"> yielded bound URI: <" << uri << ">" << std::endl;
	//if (uri_count > 1) {
	//	SessionLogger::info("LEARNIT") << "Warning: returned first URI match for team.";
	//}
	return uri;
}


/**
 * Calls find_bound_uri_from_name_and_docid_in_specified_map() on _name_docid_to_bound_uris.
 **/
std::wstring ElfMultiDoc::find_bound_uri_from_name_and_docid(const std::wstring & s, const std::wstring & docid){
	return find_bound_uri_from_name_and_docid_in_specified_map(s, docid, _name_docid_to_bound_uris);
}

/**
 * Calls find_bound_uri_from_name_in_specified_map() on _name_to_bound_uris.
 **/
std::wstring ElfMultiDoc::find_bound_uri_from_name(const std::wstring & s){
	return find_bound_uri_from_name_in_specified_map(s, _name_to_bound_uris);
}

/**
 * This method disambiguates between the New York Giants and the New York Jets.
 * New York is the only city with more than one NFL team.
 * Since teams are always represented by bound URIs, we don't need to handle XDoc IDs here.
 * @param doc_theory Pointer to the DocTheory (allows us to retrieve information about the doc).
 * @return String representing bound URI (or empty string).
 **/
std::wstring ElfMultiDoc::get_best_name_for_ny_nfl_team(const DocTheory* doc_theory) {
	if(doc_theory == 0)
		return L"";
	const EntitySet* entity_set = doc_theory->getEntitySet();
	bool found_jets = false;
	bool found_giants = false;
	for(int i = 0; i < entity_set->getNEntities(); i++){
		const Entity* entity = entity_set->getEntity(i);
		for(int m = 0; m < entity->getNMentions(); m++){
			const Mention* ment = entity_set->getMention(entity->getMention(m));
			std::wstring ment_str = ment->getNode()->toTextString();
			ment_str = UnicodeUtil::normalizeTextString(ment_str);
			if(!found_jets && ment_str.find(L"jets") != std::wstring::npos){
				found_jets = true;
			}
			if(!found_giants && ment_str.find(L"giants") != std::wstring::npos){
				found_giants = true;
			}
		}
	}
	if(found_jets && !found_giants){
		return find_bound_uri_from_name_w_or_wo_docid(L"jets", doc_theory->getDocument()->getName().to_string());
	}
	if(found_giants && !found_jets){
		return find_bound_uri_from_name_w_or_wo_docid(L"giants", doc_theory->getDocument()->getName().to_string());
	}
	return L"";
}

/**
 * Use date to determine whether it's more likely that "Bush" refers to Papa Bush or Baby Bush.
 **/
std::wstring ElfMultiDoc::get_best_name_for_bush(const std::wstring & best_name, const Entity* entity, const DocTheory* doc_theory,
												 std::wstring & bound_uri_out, XDocIdType & xdoc_id_out) {
	bound_uri_out = L"";
	xdoc_id_out = XDocIdType();
	std::wstring normalized_name_str = UnicodeUtil::normalizeTextString(best_name);
	if(entity == 0)
		return L"";
	if(doc_theory == 0)
		return L"";
	if(normalized_name_str.find(L"bush") == std::wstring::npos)
		return L"";
	std::wstring docid = doc_theory->getDocument()->getName().to_string();
	std::wstring doc_date;
	std::wstring doc_additional_time;
	bool before_2000 = false;
	ElfMultiDoc::temporal_normalizer->normalizeDocumentDate(doc_theory, doc_date, doc_additional_time);
	if(doc_date.size() > 2){
		std::wstring begin = doc_date.substr(0,2);
		if(begin == L"19")
			before_2000 = true;
	}

	XDocIdType w_xid = get_xdoc_id(Symbol(L"PER"), L"george w bush");
	XDocIdType hw_xid = get_xdoc_id(Symbol(L"PER"), L"george h w bush");

	if(!hw_xid.is_valid())
		hw_xid = get_xdoc_id(Symbol(L"PER"), L"george hw bush");

	std::wstring w_bound = find_bound_uri_from_name_w_or_wo_docid(L"george w bush", docid);
	std::wstring hw_bound = find_bound_uri_from_name_w_or_wo_docid(L"george h w bush", docid);
	if(hw_bound == L"")
		hw_bound = find_bound_uri_from_name_w_or_wo_docid(L"george hw bush", docid);

	//if(hw_bound != L"" || w_bound != L"" || (w_xid != L"" && hw_xid != L"")){
	//	SessionLogger::info("LEARNIT")<<"Conflicting George Bushes: "<< "HW: "<<hw_bound<<", "
	//		<<hw_xid<<" W: "<<w_bound<<", "<<w_xid<<" Date: "<<doc_date
	//		<<" "<<normalized_name_str<<std::endl;
	//}
	else{
		return L"";
	}
	bool match_only_bush = false;
	bool match_hw = false;
	bool match_w = false;
	bool match_george = false;
	bool match_unknown = false;
	const EntitySet* entity_set = doc_theory->getEntitySet();
	if(entity != 0){
		for(int i = 0; i < entity->getNMentions(); i++){
			const Mention* mention = entity_set->getMention(entity->getMention(i));
			if(mention->getMentionType() == Mention::NAME && mention->getHead() != 0){
				const SynNode* head = mention->getAtomicHead();
				if(!head)
					continue;
				std::wstring name_str =  head->toTextString();
				name_str = UnicodeUtil::normalizeTextString(name_str);
				if(name_str == L"bush"){
					match_only_bush = true;
					continue;
				}
				if(name_str.find(L"george") != std::wstring::npos){
					match_george = true;
				}
				else{
					match_unknown = true;
				}
				if(name_str.find(L"h w") != std::wstring::npos || name_str.find(L"hw") != std::wstring::npos){
					match_hw = true;
				} 
				else if(name_str.find(L"herbert walker") != std::wstring::npos){
					match_hw = true;
				}
				else if(name_str.find(L" w ") != std::wstring::npos){
					match_w = true;
				}
			}
		}
	}
	else{
		std::wstring name_str = UnicodeUtil::normalizeTextString(best_name);
		if(name_str == L"bush"){
			match_only_bush = true;
		}
		else if(name_str.find(L"george") != std::wstring::npos){
			match_george = true;
		}
		else{
			match_unknown = true;
		}
		if(name_str == L"w"){
			match_w = true;
		}
		else if(name_str.find(L"h w") != std::wstring::npos || name_str.find(L"hw") != std::wstring::npos){
			match_hw = true;
		}
		else if(name_str.find(L"w") != std::wstring::npos){
			match_w = true;
		}	
	}
	if(match_hw){
		bound_uri_out = hw_bound;
		xdoc_id_out = hw_xid;
	} else if(match_w){
		bound_uri_out = w_bound;
		xdoc_id_out = w_xid;
	} else if(match_only_bush && ( match_george || !match_unknown)){
		if(before_2000){
			bound_uri_out = hw_bound;
			xdoc_id_out = hw_xid;
		}
		else{
			bound_uri_out = w_bound;
			xdoc_id_out = w_xid;
		}
	}
	return best_uri_from_bound_uri_and_xdoc_id(bound_uri_out, xdoc_id_out);
}

/**
 * Given a type string and best name string, looks up
 * the name in the loaded argument map (string replacement) table and returns
 * a replacement string if one exists. Replacement string will be 
 * a bound URI (ontology URI) if available, otherwise an XDoc ID if available,
 * otherwise an empty string.
 *
 * @param type_string The ontology type of the named individual.
 * @param best_name The best name string of the individual.
 * @param doc_theory The current document containing the individual.
 * @param entity The entity associated with the individual, if any.
 * @return The replaced bound URI, or XDoc ID, or empty string.
 * 
 * @author nward@bbn.com
 * @date 2010.07.13
 **/
std::wstring ElfMultiDoc::get_mapped_arg(const std::wstring & best_name, const std::wstring & type_string, 
											const DocTheory* doc_theory, const Entity* entity, 
											std::wstring & bound_uri_out, XDocIdType & xdoc_id_out) {
	bound_uri_out = L"";
	xdoc_id_out = XDocIdType(); // INVALID_XDOC_ID;
	std::wstring out_string;
	if (get_mapped_arg_from_doc_level_uri_assignment(best_name, type_string, doc_theory, entity, bound_uri_out, xdoc_id_out)) {
		return best_uri_from_bound_uri_and_xdoc_id(bound_uri_out, xdoc_id_out);
	}
	// The argument map collapses whitespace to avoid row-breaking, so we need to search names similarly
	boost::wregex spaces_re(L"\\s+");
	std::wstring cleaned_best_name = boost::regex_replace(best_name, spaces_re, L" ");

	// Get the document ID for lookups
	std::wstring docid;
	if (doc_theory) {
		docid = std::wstring(doc_theory->getDocument()->getName().to_string());
	}

	// Use cross-doc to get other names for the associated entity, if any
	Symbol entity_type = Symbol(L"NONE");
	if (entity) {
		entity_type = entity->getType().getName();
	}

	std::vector<std::wstring> other_names = get_other_names(entity_type, cleaned_best_name);
	//mrf: XDoc removes punctuation, but the training map does not necessarily.  
	//     Add 'cleaned best name' to other_names to try to help
	//     Also: all else being equal, you'd prefer the id of this instance, so add it to the beginning
    //Note: currently, other_names may contain duplicate strings.
	other_names.insert(other_names.begin(), cleaned_best_name);

	// Check whether the document ID, type, and best name indicate we need to do a replacement
	//   These argument URIs are used to align with annotation; they supersede all other mapped arguments
	DocIdAndTypeToSRMMap::iterator map_i = ElfMultiDoc::_docid_type_to_srm.find(DocIdAndTypeToSRMMap::key_type(docid, type_string));
	if (map_i != _docid_type_to_srm.end()) {
		BOOST_FOREACH(std::wstring other_name, other_names) {
			StringReplacementMap::iterator replace_i = map_i->second.find(other_name);
			if (replace_i != map_i->second.end()) {
				// Convert the best name using the matching argument map
				//SessionLogger::info("LEARNIT") << "Performing docid-specific conversion of <" << other_name << "> "
				//	<< "to <" << replace_i->second << ">" << std::endl;
				bound_uri_out = replace_i->second;
				return bound_uri_out;
			}
		}
	}

	// Check whether the type and best name indicate we need to do a replacement across all docs
	TypeToSRMMap::iterator map_all_i = ElfMultiDoc::_type_to_srm.find(TypeToSRMMap::key_type(type_string));
	if (map_all_i != _type_to_srm.end()) {
		BOOST_FOREACH(std::wstring other_name, other_names) {
			StringReplacementMap::iterator replace_i = map_all_i->second.find(other_name);
			if (replace_i != map_all_i->second.end()) {
                // Sample output: "Performing conversion of <Patriots> to <nfl:NewEnglandPatriots>"
				// Convert the best name using the matching argument map
				//SessionLogger::info("LEARNIT") << "Performing conversion of <" << other_name << "> "
				//	<< "to <" << replace_i->second << ">" << std::endl;
				bound_uri_out = replace_i->second;
				return bound_uri_out;
			}
		}
	}
	return get_mapped_arg_when_no_srm_entry_found(best_name, type_string, doc_theory, entity, xdoc_id_out);
}

// called by get_mapped_arg()
bool ElfMultiDoc::get_mapped_arg_from_doc_level_uri_assignment(const std::wstring & best_name, 
															   const std::wstring & type_string, 
															   const DocTheory* doc_theory, const Entity* entity,
															   std::wstring & bound_uri_out, XDocIdType & xdoc_id_out) {
	bound_uri_out = L"";
	xdoc_id_out = XDocIdType(); // INVALID_XDOC_ID;
	if(ParamReader::getRequiredTrueFalseParam("use_document_level_uri_assignment")){
		if(ParamReader::getRequiredTrueFalseParam("check_george_bush")){
			std::wstring bush_hack = get_best_name_for_bush(best_name, entity, doc_theory, bound_uri_out, xdoc_id_out);
			if (!bush_hack.empty()) {
				return true;
			}
		}
		bool is_nfl_team_type = type_string == L"nfl:NFLTeam";
		if(entity != 0){
			//NFL-- must handle GPE/team metonymy
			if(is_nfl_team_type){
                // Always returns a bound URI
				bound_uri_out = get_mapped_arg_for_nfl_team(best_name, type_string, doc_theory, entity);
				if (!bound_uri_out.empty()) {
					return true;
				}
			}
			else{
				bool ret(false);
				if( _eid_to_bound_uris.find(entity->getID())!= _eid_to_bound_uris.end()){
					bound_uri_out = (*_eid_to_bound_uris[entity->getID()].begin());
					ret = true;
				}
				if(_eid_to_xdoc_ids.find(entity->getID())!= _eid_to_xdoc_ids.end()){
					xdoc_id_out = (*_eid_to_xdoc_ids[entity->getID()].begin());
					ret = true;
				}
				if (ret) {
					return true;
				}
			}
			if(!_eid_to_xdoc_ids.empty()){
				return true;
			}
			SessionLogger::warn("LEARNIT")<<"get_mapped_arg() defaulting to non-document table behavior: "<<best_name<<std::endl;
		}
		else if(is_nfl_team_type){
			std::wstring normalized_best_string = UnicodeUtil::normalizeTextString(best_name);
			if(normalized_best_string == L"new york"){
				// get_best_name_for_ny_nfl_team() always returns a bound URI or empty string, never an XDoc ID
				bound_uri_out = get_best_name_for_ny_nfl_team(doc_theory); 
				return true;
			}
		}
	}
	return false;
}

/**
 * Called by get_mapped_arg() to handle the case where there is no matching bound URI in the string replacement table.
 * Note that the method has no bound_uri_out parameter.
 * @param best_name Best name to match
 * @param type_string Type string
 * @param doc_theory DocTheory (used to retrieve an EntitySet)
 * @param xdoc_id_out XDoc ID found (will be the invalid XDoc ID if none is found)
 * @return String representation of xdoc_id_out, which will be an empty string if XDoc ID represents the invalid XDoc ID.
 **/
std::wstring ElfMultiDoc::get_mapped_arg_when_no_srm_entry_found(const std::wstring & best_name, 
																 const std::wstring & type_string, 
																 const DocTheory* doc_theory, const Entity* entity,
																 XDocIdType & xdoc_id_out) {
	xdoc_id_out = XDocIdType();
	//   Don't necessarily want to restrict ourselves to a specific type string.  What should we do about this?
	//   Note: could add all mapped args to the simpler table, and search all entity types
	if (entity) {
		//MRF: Only do XDoc look-up if there was a name in the entity.  We don't know if best_name is actually a description.
		bool entity_has_name = false;
		const EntitySet* entity_set = doc_theory->getEntitySet();
		for(int i = 0; i < entity->getNMentions(); i++){
			const Mention * ment = entity_set->getMention(entity->getMention(i));
			if((ment != 0) && ment->getMentionType() == Mention::NAME){
				entity_has_name = true;
				break;
			}
		}
		if(!entity_has_name)
			return L"";

		// Get the associated entity's ACE type and try a direct cluster lookup
		Symbol entity_type = entity->getType().getName();
		XDocIdType best_uri = ElfMultiDoc::get_xdoc_id(entity_type, best_name);
		if (best_uri.is_valid()) {
            xdoc_id_out = best_uri;
			return best_uri.as_string();
        }
		// This is an ACE entity; check the name strings of other coreferent mentions.
		// Different mentions in the entity may have different cluster ids.
		// Collect all matches, then resolve. We'll choose the URI that is used most often
		// (highest count) and/or corresponds to the mention head with the greatest 
		// number of words (greatest length).
		PotentialXdocIdToCountAndGreatestLengthMap potential_ids;
		collect_potential_xdoc_ids(entity_set, entity, entity_type, potential_ids);
		xdoc_id_out = get_best_potential_xdoc_id(entity, potential_ids, best_uri);
		return xdoc_id_out.as_string();
	} else {
		// Make sure we're not replacing individual IDs for certain types
		if (type_string != L"ic:Position") {
			// We have an orphan name string (no associated entity), so we might as well try against all types. 
			// Take the first match found.
			Symbol entity_type(L"NONE");
			xdoc_id_out = get_xdoc_id_and_entity_type(best_name, entity_type);
            return xdoc_id_out.as_string();
		}
	}
	// No match
	return L"";
}

// called by get_mapped_arg(); always returns either a bound URI or an empty string
std::wstring ElfMultiDoc::get_mapped_arg_for_nfl_team(const std::wstring & best_name, const std::wstring & type_string, 
											const DocTheory* doc_theory, const Entity* entity) {
	if(_eid_to_bound_uris.size() != 0 && _eid_to_bound_uris.find(entity->getID())!= _eid_to_bound_uris.end()){
		return (*_eid_to_bound_uris[entity->getID()].begin());
	}
	else if(_eid_to_bound_uris_merged.size() != 0 && 
		_eid_to_bound_uris_merged.find(entity->getID())!= _eid_to_bound_uris_merged.end()){
		return (*_eid_to_bound_uris_merged[entity->getID()].begin());
	}
	else{
		//horrible, horrible hack but for NFLTeams we really want to force to the NFL bound uris
		std::wstring docid = std::wstring(doc_theory->getDocument()->getName().to_string());
		std::wstring uri = find_bound_uri_from_name_w_or_wo_docid(best_name, docid);
		if(uri != L""){
			return uri;
		}
		std::wstring name_uri, desc_uri, node_uri, name_str, desc_str, node_str;
		bool found_newyork = false;
		const EntitySet* entity_set = doc_theory->getEntitySet();
		for(int i = 0; i < entity->getNMentions(); i++){
			const Mention* mention = entity_set->getMention(entity->getMention(i));
			if(mention->getMentionType() == Mention::NAME && mention->getHead() != 0){
				std::wstring nstr = mention->getHead()->toTextString();
				std::wstring uri = find_bound_uri_from_name_w_or_wo_docid(nstr, docid);
				if(uri != L"" && name_uri == L""){
					name_str = nstr;
					name_uri = uri;
				} 
				else if(uri == L""){
					std::wstring normalized_best_string = UnicodeUtil::normalizeTextString(nstr);
					if(normalized_best_string == L"new york")
						found_newyork = true;
				}
			}
			if(mention->getMentionType() == Mention::DESC && mention->getHead() != 0){
				std::wstring nstr = mention->getHead()->toTextString();
				std::wstring uri = find_bound_uri_from_name_w_or_wo_docid(nstr, docid);
				if(uri != L"" && desc_uri == L""){
					desc_str = nstr;
					desc_uri = uri;
				}
			}
			if(mention->getNode() != 0){
				std::wstring nstr = mention->getNode()->toTextString();
				std::wstring uri = find_bound_uri_from_name_w_or_wo_docid(nstr, docid);
				if(uri != L"" && node_uri == L""){
					node_str = nstr;
					node_uri = uri;
				}
			}
		}
		if(name_uri != L""){
			//SessionLogger::info("LEARNIT")<<"using a name: "<<best_name<<", "<<name_str<<
			//	"--> n="<<name_uri<<", d="<<desc_uri<<" x="<<node_uri<<std::endl;
			return name_uri;
		}
		else if(desc_uri != L""){
			//SessionLogger::info("LEARNIT")<<"using a desc: "<<best_name<<", "<<desc_str<<
			//	"--> n="<<name_uri<<", d="<<desc_uri<<" x="<<node_uri<<std::endl;
			return desc_uri;
		}
		else if(node_uri != L""){
			//SessionLogger::info("LEARNIT")<<"using a node: "<<best_name<<", "<<node_str<<
			//	"--> n="<<name_uri<<", d="<<desc_uri<<" x="<<node_uri<<std::endl;
			return node_uri;
		}
		else if(found_newyork){
			return get_best_name_for_ny_nfl_team(doc_theory);
		}
		else {
			return L"";
		}
	}
}

/** 
 *  Adds/updates the entity-id-to-xdoc_id and entity-id-to-bound-uri mappings for the entity ID.
 *  Makes sure that all entity IDs in other_ids point to the specified primary XDoc ID.
 *  Called by EIUtils::attemptNewswireLeadFix().
 *
 *  @param doc_theory DocTheory from which to abstract document name
 *  @param primary Primary entity ID
 *  @param other_ids Set of entity IDs that should all point to the primary entity ID
 **/
void ElfMultiDoc::merge_entities_in_map(const DocTheory* doc_theory, int primary_eid, const std::set<int> & other_eids){
	if(_eid_to_xdoc_ids.find(primary_eid) != _eid_to_xdoc_ids.end()) {
		// There is an entry in the entity-id-to-xdoc_id map for this entity ID. Retrieve all the primary xdoc_ids
		// to which the entity ID points. Then make sure that there are entity-id-to-xdoc_id entries pointing
		// from each entity ID in other_ids to each of the primary xdoc_ids (and that any preexisting entries
		// are cleared).
		BOOST_FOREACH(int other_eid, other_eids){
			_eid_to_xdoc_ids[other_eid].clear();
			BOOST_FOREACH(XDocIdType primary_xdoc_id, _eid_to_xdoc_ids[primary_eid]){
				_eid_to_xdoc_ids[other_eid].insert(primary_xdoc_id);
			}
		}
	}
	else {
		// There is no entry in the entity-id-to-xdoc_id map for this entity ID. Create a new 
		// xdoc_id (with a value decremented from INT_MAX-1) that is unlikely to conflict with existing ones.
		// Then make sure that there are new entity-id-to-xdoc_id entries pointing
		// from each entity ID in other_ids to each of the primary xdoc_ids (and that any preexisting entries are cleared).
		XDocIdType new_xdoc_id(get_next_special_xdoc_id_to_assign());
		SessionLogger::info("LEARNIT") << "New XDoc ID: <" << new_xdoc_id << ">" << std::endl;
		_eid_to_xdoc_ids[primary_eid].insert(new_xdoc_id);
		BOOST_FOREACH(int other_eid, other_eids){
			_eid_to_xdoc_ids[other_eid].clear();
			_eid_to_xdoc_ids[other_eid].insert(new_xdoc_id);
		}
	}

	if(_eid_to_bound_uris.find(primary_eid) != _eid_to_bound_uris.end()){
		// There is an entry in the entity-id-to-bound-uri map for this entity ID. Retrieve all the bound URIs
		// to which the entity ID points. Then make sure that there are entity-id-to-bound-uri entries pointing
		// from each entity ID in other_ids to each of the primary bound URIs (and that any preexisting entries
		// are cleared).
		BOOST_FOREACH(int other_eid, other_eids){
			_eid_to_bound_uris[other_eid].clear();
			BOOST_FOREACH(std::wstring primary_bound_uri, _eid_to_bound_uris[primary_eid]){
				_eid_to_bound_uris[other_eid].insert(primary_bound_uri);
			}
		}
	}
}

/**
 * Retrieves the types associated with a bound URI.
 *
 * @param bound_uri The URI for which associated types are to be found.
 * @return Set of types associated with the given bound URI.
 **/
SetOfStrings ElfMultiDoc::get_types_for_bound_uri(const std::wstring & bound_uri){
	SetOfStrings empty_set;
	StringToSetOfStringsMap::iterator iter = _bound_uri_to_types.find(bound_uri);
	if(iter != _bound_uri_to_types.end())
		return (*iter).second;
	else
		return empty_set;
}

/**
 * Traverses a DocIdAndTypeToSRMMap, each element of which maps from a (doc ID, ontology type str)
 * pair to a map of value string replacements, each of which, in turn, maps from a name to a URI. 
 * Also traverses a DocIdAndTypeToSRMMap, each element of which maps from an ontology type str
 * to a map of value string replacements. 
 * Fills three static maps, one of which (_name_docid_to_bound_uris) maps from a (name, doc ID) pair to a bound URI, 
 * one of which (_name_to_bound_uris) maps from a name to a bound URI, 
 * and the third of which (_bound_uri_to_types) maps from a bound URI to a set of types. Both the original name and the 
 * normalized name are used to create mappings in the first two maps (though when the normalized name equals 
 * the original name, only one entry ends up being added).
 * 
 **/
void ElfMultiDoc::generate_typeless_maps(){
    // Loop over document-specific entries
	BOOST_FOREACH(DocIdAndTypeToSRMMap::value_type entry, _docid_type_to_srm) {
		std::wstring doc_id = entry.first.first;
		std::wstring type = entry.first.second;
		StringReplacementMap srm = entry.second;
		BOOST_FOREACH(StringReplacementMap::value_type replace, srm){
			std::wstring name = replace.first;
			std::wstring uri = replace.second;
			_name_docid_to_bound_uris[std::pair<std::wstring, std::wstring>(name, doc_id)].insert(uri);
			//create a normalized, lowercased version of the name and add it
			std::wstring normalized_string = UnicodeUtil::normalizeTextString(name);
			_name_docid_to_bound_uris[std::pair<std::wstring, std::wstring>(normalized_string, doc_id)].insert(uri);
			_bound_uri_to_types[uri].insert(type);
		}
	}
    // Loop over non-document-specific entries
	BOOST_FOREACH(TypeToSRMMap::value_type entry, _type_to_srm) {
		std::wstring type = entry.first;
		StringReplacementMap srm = entry.second;
		BOOST_FOREACH(StringReplacementMap::value_type replace, srm){
			std::wstring name = replace.first;
			std::wstring uri = replace.second;
			_name_to_bound_uris[name].insert(uri);
			//create a normalized, lowercased version of the name and add it
			std::wstring normalized_string = UnicodeUtil::normalizeTextString(name);
			_name_to_bound_uris[normalized_string].insert(uri);
			_bound_uri_to_types[uri].insert(type);
		}
	}
}

/**
 * Given a Mention, determine its type string, offsets, and
 * best text. Derived from SlotFiller::_findBestEntityName
 * but specialized for an ElfRelationArg context. Only used
 * when constructing an ElfRelationArg from a Mention.
 *
 * @param doc_theory The DocTheory containing the extracted
 * Mention, used for determining offsets.
 * @param mention The Mention being extracted.
 * @param type_string Return by reference: a type string
 * derived from Mention::getMentionType().
 * @param start_offset Return by reference: integer start
 * offset of the mention.
 * @param end_offset Return by reference: integer end
 * offset of the mention.
 * @return The best text contents of the mention; a name or description
 * where relevant and available.
 *
 * @author nward@bbn.com
 * @date 2010.06.15
 **/
std::wstring ElfMultiDoc::find_best_text_for_mention(const DocTheory* doc_theory, const Mention* mention, 
											std::wstring& type_string, EDTOffset& start_offset, EDTOffset& end_offset) {
	// Safely determine the start and end offsets
	const SynNode* mention_node = mention->getNode();
	const SynNode* mention_head = mention->getHead();
	Mention::Type mention_type = mention->getMentionType();
	std::wstring text = L"NO_NAME";
	if (mention_node) {
		SentenceTheory* sent_theory = doc_theory->getSentenceTheory(mention->getSentenceNumber());
		if (sent_theory) {
			TokenSequence* token_sequence = sent_theory->getTokenSequence();
			if (token_sequence) {
				// Check the mention type for special handling
				switch (mention_type) {
				case Mention::NAME:
					if (mention_head) {
						start_offset = token_sequence->getToken(mention_head->getStartToken())->getStartEDTOffset();
						end_offset = token_sequence->getToken(mention_head->getEndToken())->getEndEDTOffset();
					}
					break;
				case Mention::APPO:
					if (mention_head && mention_head->hasMention()) {
						const MentionSet* mention_set = sent_theory->getMentionSet();
						Mention* m = mention_set->getMention(mention_head->getMentionIndex());
						mention_type = m->getMentionType();
						start_offset = token_sequence->getToken(mention_head->getStartToken())->getStartEDTOffset();
						end_offset = token_sequence->getToken(mention_head->getEndToken())->getEndEDTOffset();
					}
					break;
				default:
					start_offset = token_sequence->getToken(mention_node->getStartToken())->getStartEDTOffset();
					end_offset = token_sequence->getToken(mention_node->getEndToken())->getEndEDTOffset();
					break;
				}

				// Get the text for the offsets of the mention or the mention head
				LocatedString* located_text = MainUtilities::substringFromEdtOffsets(
					doc_theory->getDocument()->getOriginalText(), start_offset, end_offset);
				text = std::wstring(located_text->toString());
				delete located_text;
			}
		}
	}

	// Construct the complete type string for this mention
	std::wstring entity_type = std::wstring(mention->getEntityType().getName().to_string());
	std::wstring entity_subtype = std::wstring(mention->getEntitySubtype().getName().to_string());
	if (entity_subtype == L"UNDET")
		type_string = std::wstring(L"ace:") + entity_type;
	else
		type_string = std::wstring(L"ace:") + entity_type + std::wstring(L".") + entity_subtype;

	// Done
	return text;
}

/**
 * Reads the specified three-column string to
 * cluster map file, loading ElfMultiDoc::_entity_type_and_name_to_xdoc_id
 * and ElfMultiDoc::_entity_type_and_xdoc_id_to_names. Creates new clusters
 * (with new XDoc IDs) when it determines that the ones read from the file contain 
 * items that should not be lumped together.
 *
 * @param filename The path to the cluster map file.
 *
 * @author mfreedma@bbn.com
 * @date 2010.08.29
 **/
// Sample lines:
//		1	GPE	soviet, soviet union, ussr, 
//		2	GPE	china, chinese, people s republic of china, prc, sino, 
void ElfMultiDoc::load_xdoc_maps(const std::wstring & filename) {
	// Check whether a string to cluster map file was specified
	if (filename.empty()) {
		// We used to allow this, but no more.
		ostringstream ostr;
		ostr << "Value for string_to_cluster_filename must not be an empty string." << endl;
		throw std::runtime_error(ostr.str()); // there's no exception(const char *) constructor outside MSVC
	}

	// Try to load each specified database
	SessionLogger::info("read_eqn_0") << "Reading equivalent names cluster " << filename << "..." << std::endl;
	// Try to load each specified database
	std::wstring line;
    std::wifstream eqnc_wifstream(UnicodeUtil::toUTF8StdString(filename).c_str()); // gcc can't handle a wide filename
	if (!eqnc_wifstream) {
		ostringstream ostr;
		ostr << "Cluster file '" << filename << "' could not be opened." << endl;
		throw std::runtime_error(ostr.str()); // there's no exception(const char *) constructor outside MSVC
	}
	int valid_lines(0);
	_max_xdoc_id_in_file = 0;
	std::vector<std::vector<std::wstring> > triples;
    // Read in each line, splitting on tabs. If a line doesn't contain three tokens, or if the first
    // token on the line (representing an XDoc ID) is not a number, ignore it. Store the pieces in
    // a vector. Also keep track of the largest XDoc ID encountered. This will be stored as    
    // _max_xdoc_id_in_file, which will be used later to calculate new XDoc IDs for
    // new clusters that are created from badly-lumped-together clusters in the original file.
	while (eqnc_wifstream.is_open() && getline(eqnc_wifstream, line)) {
		// Read the tab-delimited table row (where initial hash mark comments out a line)
		boost::algorithm::trim(line);
		if (line != L"" && line[0] != L'#') {
			// Split the line into columns by tab
			std::vector<std::wstring> line_tokens;
			boost::algorithm::split(line_tokens, line, boost::algorithm::is_from_range(L'\t', L'\t'));

			// Check whether this cluster entry is valid
			if (line_tokens.size() != 3) {
				SessionLogger::warn("lxm_bad_line_0") << "ElfMultiDoc::load_xdoc_maps(): ignoring unreadable line: " 
					<< UnicodeUtil::toUTF8StdString(line) << std::endl;
				continue;
			}

			try {
				int i = boost::lexical_cast<int>(line_tokens[0]); // will throw if a bad int is encountered
			} catch(...) {
				SessionLogger::warn("lxm_bad_line_1") << "Ignoring line with invalid string at beginning (should be nonnegative int): " 
					<< line_tokens[0] << endl;
				continue;
			}
			++valid_lines;
			// Cross-doc URI
			std::wstring xdoc_id = L"bbn:xdoc-";
			xdoc_id.append(line_tokens[0]);
			XDocIdType xid(xdoc_id);
			if (xid.as_int() > _max_xdoc_id_in_file) {
				_max_xdoc_id_in_file = xid.as_int();
			}
			line_tokens[0] = xdoc_id;
			triples.push_back(line_tokens);
		} // end if (line != L""...)
	} // end while (eqnc_wifstream...)
	if (valid_lines == 0) {
		ostringstream ostr;
		ostr << "No valid lines were found in " << filename << "." << endl;
		throw std::runtime_error(ostr.str()); // there's no exception(const char *) constructor outside MSVC;
	}
    // Now step through the triples, handling conflicts and creating new clusters (with
    // new XDoc IDs) where necessary.
	BOOST_FOREACH(std::vector<std::wstring> triple, triples) {
		XDocIdType xdoc_id(triple[0]);
		Symbol entity_type(triple[1]); // ACE entity type
		std::wstring all_names(triple[2]); // Comma-separated list of equivalent names
		std::vector<std::wstring> split_names;
		boost::algorithm::split(split_names, all_names, boost::algorithm::is_from_range(L',', L','));
		std::vector<std::wstring> trimmed_names;
		BOOST_FOREACH(std::wstring split_name, split_names) {
			std::wstring name = boost::algorithm::trim_copy(split_name);
			if (!name.empty()) {
				trimmed_names.push_back(name);
			}
		}
		SetOfStrings conflict_names;
		bool added_conflicts = find_and_handle_conflicts(trimmed_names, xdoc_id, entity_type, conflict_names);
		if (!added_conflicts) {
			store_entity_type_and_name_to_xdoc_id(trimmed_names, entity_type, xdoc_id);
		}
	}
	_initialized_entity_type_and_name_to_xdoc_id = true;
}

// called by load_xdoc_maps()
bool ElfMultiDoc::find_and_handle_conflicts(const std::vector<std::wstring> & trimmed_names, const XDocIdType & xdoc_id,
											Symbol entity_type,	SetOfStrings & conflict_names) {
	if (entity_type != Symbol(L"PER") && entity_type != Symbol(L"ORG")) {
		return false;
	}
	bool has_conflict(false);
	size_t name_count(trimmed_names.size());
	std::vector<int> wk_ids(name_count);
	for (size_t i = 0; i < name_count; ++i)
		wk_ids[i] = CorefUtilities::lookUpWKName(trimmed_names[i], entity_type);
	for (size_t i = 0; i < name_count; ++i) {
		if (wk_ids[i] == 0) {
			continue;
		}
		for (size_t j = i+1; j < name_count; ++j) {
			if (trimmed_names[i] == trimmed_names[j]) {
				continue;
			}
			if(wk_ids[j] == 0)
				continue;
			if(wk_ids[i] == -1 && wk_ids[j] == -1){
				has_conflict = true;
				conflict_names.insert(trimmed_names[i]);
				conflict_names.insert(trimmed_names[j]);
				SessionLogger::dbg("conflict_-1") << "Conflict -1: <" <<trimmed_names[i]<<"> <"<<trimmed_names[j]<<"> "<<std::endl;
			}
			if(wk_ids[i] != wk_ids[j]){
				has_conflict = true;
				conflict_names.insert(trimmed_names[i]);
				conflict_names.insert(trimmed_names[j]);
				SessionLogger::dbg("conflict_neq_0")<<"Conflict ID: <"<<trimmed_names[i]<<"> <"<<wk_ids[i]<<"> "
					<< "<"<<trimmed_names[j]<<"> <"<<wk_ids[j]<<"> "<<std::endl;
			}
		}
	} // end loop over trimmed_names

	if(has_conflict){
		return handle_conflicts(trimmed_names, xdoc_id, entity_type, conflict_names);
	} else {
		return false;
	}
}

// called by load_xdoc_maps()
bool ElfMultiDoc::handle_conflicts(const std::vector<std::wstring> & trimmed_names,
								   const XDocIdType & xdoc_id, 
								   Symbol entity_type, 
								   const SetOfStrings & conflict_names) {
	SetOfStrings substring_names;
	if(entity_type == Symbol(L"PER")){
		// If a trimmed name contains a substring that is a conflict name, or vice versa,
		// insert the substring into substring_names.
		BOOST_FOREACH(std::wstring name1, trimmed_names){
			BOOST_FOREACH(std::wstring name2, conflict_names){
				if(name1 == name2)
					continue;
				if(name1.find(name2) != std::wstring::npos){
                    // Sample output: 
                        //substring: adams OF flozell adams
                        //substring: adams OF gaines adams
					//SessionLogger::info("LEARNIT")<<"substring: "<<name2<< " OF "<<name1<<std::endl;
					substring_names.insert(name2);
				}
				if(name2.find(name1) != std::wstring::npos){
					substring_names.insert(name1);
					//SessionLogger::info("LEARNIT")<<"substring: "<<name1<<" OF "<<name2<<std::endl;
				}
			}
		}	
	} // end if(entity_type == Symbol(L"PER")
	std::vector<SetOfStrings> cluster_split;				
	SetOfStrings seeds;	
	SetOfStrings blocked_cluster;	
	BOOST_FOREACH(std::wstring name1, conflict_names){
		// Remove the conflict name from substring_names if it contains a space.
		if(substring_names.find(name1) != substring_names.end()){
			if( name1.find(L" ") == std::wstring::npos)
				continue;
			else{
				substring_names.erase(substring_names.find(name1));
			}
		}
		if(cluster_split.empty()){
			SetOfStrings s;
			s.insert(name1);
			cluster_split.push_back(s);
			seeds.insert(name1);
			//SessionLogger::info("LEARNIT") << "Inserted set containing <" << name1 << "> into cluster_split and seeds." << std::endl;
			continue;
		}
		bool make_new = false;
		int wkid1 = CorefUtilities::lookUpWKName(name1, entity_type);
		bool added = false;
		if(wkid1 == -1){
			make_new = true;
		}
		else{
			SetOfStrings a;
			if(entity_type != Symbol(L"PER")) {
				a = get_possible_acronyms(name1);
			}
			for(std::vector<SetOfStrings>::iterator it = cluster_split.begin(); it != cluster_split.end(); ++it){
				BOOST_FOREACH(std::wstring cname, (*it)){
					int wkid2 = CorefUtilities::lookUpWKName(cname, entity_type);
					if(wkid2 == wkid1 && !added){
						//SessionLogger::info("LEARNIT") << "Inserting <" << name1 << ">" << std::endl; 
						(*it).insert(name1);
						added = true;
						seeds.insert(name1);
					}
					if(entity_type != Symbol(L"PER")){
						SetOfStrings a2 = get_possible_acronyms(cname);
						if(a.find(cname) != a.end() || a2.find(name1) != a2.end()){
							//SessionLogger::info("LEARNIT") << "Inserting <" << name1 << ">" << std::endl; 
							(*it).insert(name1);
							added = true;
							seeds.insert(name1);
						}
					} 
				}						
			}
		}
		if(!added || make_new){
			SetOfStrings s;
			s.insert(name1);
			cluster_split.push_back(s);
			seeds.insert(name1);
		}
	} // end loop over conflict_names
	BOOST_FOREACH(std::wstring name1, trimmed_names){
		if(name1 == L"")
			continue;
		if(substring_names.find(name1) != substring_names.end())
			continue;
		if(seeds.find(name1) != seeds.end())
			continue;	//accounted for this name
		float min_dist = 1000;
		std::wstring best_seed = L"";
		BOOST_FOREACH(std::wstring seed, seeds){
			int dist = CorefUtilities::editDistance(name1, seed);
			float ndist = (float)dist/(std::min(name1.size(), seed.size()));
			if(ndist < min_dist){
				min_dist = ndist;
				best_seed = seed;
				//SessionLogger::info("LEARNIT")<<"Edit Distance: "<<ndist<<" "<<name1<<"  ---> "<<seed<<std::endl;
			}
		}
		if(best_seed != L""){
			if(min_dist > .7){
				//give up on XDoc IDs, too dangerous
			}
			else{
				for(std::vector<SetOfStrings>::iterator it = cluster_split.begin(); 
					it != cluster_split.end(); ++it){
					BOOST_FOREACH(std::wstring cname, (*it)){
						if(cname == best_seed){
							(*it).insert(name1);
						}
					}
				}
			}
		} // end if best_seed nonempty
	} // end for each trimmed name
	//substrings are dangerous here
	BOOST_FOREACH(std::wstring name1, substring_names){
		std::set<int> index_set;
		for(size_t i = 0; i < cluster_split.size(); i++){
			BOOST_FOREACH(std::wstring cname, cluster_split[i]){
				if(cname.find(name1) != std::wstring::npos){
					index_set.insert(i);
				}
			}
		}
		if(index_set.size() == 1)
			cluster_split[(*index_set.begin())].insert(name1);
	}
	//we only ended up with one name, so add others back in
	if(cluster_split.size() == 1){
		BOOST_FOREACH(std::wstring name, trimmed_names){
			if(name != L"")
				cluster_split[0].insert(name);
		}
	}
	return update_map_from_cluster_split(cluster_split, entity_type, xdoc_id);
}

/**
 * Updates _entity_type_and_name_to_xdoc_id using cluster_split.
 * Called by load_xdoc_maps(). Since XDoc clusters often contain
 * strings that don't belong together (e.g., "blackwater usa" and "usfl"), we attempt to 
 * take ones that don't belong and assign new
 * XDoc IDs to them (with values greater than the range of the ones in the file we read in).
 *
 * @param cluster_split ...
 * @param entity_type Symbol denoting, e.g., "PER".
 * @param xdoc_id Cross-doc ID.
 * @return true if cluster_split is nonempty, false otherwise
 **/
bool ElfMultiDoc::update_map_from_cluster_split(const std::vector<SetOfStrings > & cluster_split,
												Symbol entity_type, const XDocIdType & xdoc_id) {
	if(cluster_split.begin() == cluster_split.end()){
		return false;
	}
	//SessionLogger::info("LEARNIT") << std::endl;
	//for(std::vector<setofstrings_t>::const_iterator it = cluster_split.begin(); it != cluster_split.end(); ++it){
        // Sample output:
            // bbn:xdoc-446	(abu musab)(abu musab al zarqawi)(abu musab zarqawi)(abu mussab al zarqawi)(abu mussab zarqawi)(al zarqawi)(zarqawi)
            // bbn:xdoc-446	(salaheddin)(salahuddin)
		//SessionLogger::info("LEARNIT") << xdoc_id << "\t";
		//BOOST_FOREACH(std::wstring cname, (*it)){
		//	SessionLogger::info("LEARNIT") << "(" << cname << ")";
		//}
		//SessionLogger::info("LEARNIT")<<std::endl;
	//}		
	int i = 0;
	for(std::vector<SetOfStrings>::const_iterator it = cluster_split.begin(); it != cluster_split.end(); ++it){
		XDocIdType old_xid(xdoc_id);
		int new_xdoc_id = new_xdoc_id_from_split_index(old_xid.as_int(), i);
		XDocIdType new_xid(new_xdoc_id);
		i++;
		std::vector<std::wstring> curr_names;
		BOOST_FOREACH(std::wstring cname, (*it)){
			if (cname.length() > 0) {
				curr_names.push_back(cname);
				EntityTypeAndNamePairToXDocIdMap::key_type type_name_key = 
					EntityTypeAndNamePairToXDocIdMap::key_type(entity_type, cname);
				if (ElfMultiDoc::_entity_type_and_name_to_xdoc_id.find(type_name_key) != 
					ElfMultiDoc::_entity_type_and_name_to_xdoc_id.end()) {
					SessionLogger::info("LEARNIT") << "ElfMultiDoc::load_xdoc_maps(): Warning, duplicate name: " 
						<< entity_type << ", " << UnicodeUtil::toUTF8StdString(cname) << std::endl;
				} else {
					ElfMultiDoc::_entity_type_and_name_to_xdoc_id[type_name_key] = new_xid;
				}
			}
		}
        // Sample output:
            // Adding: PER bbn:xdoc-9624 abu musab, abu musab al zarqawi, abu musab zarqawi,...
            // Adding: PER bbn:xdoc-18802 salaheddin, salahuddin, 
		//SessionLogger::info("LEARNIT")<<"Adding: " << entity_type << " " << new_xid.as_string() << " ";
		//BOOST_FOREACH(std::wstring cname, curr_names) {
		//	SessionLogger::info("LEARNIT") << cname << ", ";
		//}
		//SessionLogger::info("LEARNIT") << std::endl;
		ElfMultiDoc::_entity_type_and_xdoc_id_to_names.insert(
			EntityTypeAndXdocIdPairToNamesMap::value_type(
				EntityTypeAndXdocIdPairToNamesMap::key_type(entity_type, new_xid.as_string()), curr_names));
	}
	//SessionLogger::info("LEARNIT")<<"--------\n"<<std::endl;	
	return true;	
}

// called by load_xdoc_maps()
void ElfMultiDoc::store_entity_type_and_name_to_xdoc_id(const std::vector<std::wstring> & trimmed_names, 
										  Symbol entity_type, XDocIdType & xdoc_id) {
	// Store the URI by type and name string for each name
	BOOST_FOREACH(std::wstring name_string, trimmed_names) {
		if (name_string.length() > 0) {
			EntityTypeAndNamePairToXDocIdMap::key_type type_name_key = 
				EntityTypeAndNamePairToXDocIdMap::key_type(entity_type, name_string);
			if (ElfMultiDoc::_entity_type_and_name_to_xdoc_id.find(type_name_key) != 
				ElfMultiDoc::_entity_type_and_name_to_xdoc_id.end()) {
				SessionLogger::info("LEARNIT") << "ElfMultiDoc::load_xdoc_maps(): Warning, duplicate name: " 
					<< entity_type << ", " << UnicodeUtil::toUTF8StdString(name_string) << std::endl;
			} else {
				ElfMultiDoc::_entity_type_and_name_to_xdoc_id[type_name_key] = xdoc_id;
			}
		}
	}		
	// Store the name strings by type and URI
	ElfMultiDoc::_entity_type_and_xdoc_id_to_names.insert(EntityTypeAndXdocIdPairToNamesMap::value_type(
		EntityTypeAndXdocIdPairToNamesMap::key_type(entity_type, xdoc_id), trimmed_names));
}

// Top-level method.
// Reads same file read by EIUtils::load_xdoc_failures(), but this method looks at the ordinary lines
// (i.e., the ones NOT containing the strings "NONE" or "MILITARY").
// Loads a file such as problematic_cluster_output.txt, with contents like the following: 
//		u.s. military	NONE	ic:U.S._military
//		izzedine al-qassam brigades	8832	ic:Izz_el-Deen_al-Qassam_organization
//		al-qaeda	103	ic:al-Qaeda
//		al-aqsa martyr's brigade	723	ic:Al-Aqsa_Martyr_s_Brigade
//		abdul-majid al-khoei	192	ic:Abdul-Majid_al-Khoei
//		king fahd	3178	ic:King_Fahd
// and returns a StringReplacementMap that maps from XDoc IDs to bound URIs like the following:
//		[5](("bbn:xdoc-103","ic:al-Qaeda"),("bbn:xdoc-192","ic:Abdul-Majid_al-Khoei"),("bbn:xdoc-3178","ic:King_Fahd"),
//		("bbn:xdoc-723","ic:Al-Aqsa_Martyr_s_Brigade"),("bbn:xdoc-8832","ic:Izz_el-Deen_al-Qassam_organization"))
void ElfMultiDoc::load_mr_xdoc_output_file(const string& filename) {
	if (!filename.empty()) {
		boost::scoped_ptr<UTF8InputStream> instream_scoped_ptr(UTF8InputStream::build(filename.c_str()));
		UTF8InputStream& instream(*instream_scoped_ptr);
		std::wstring line;
		while (getline(instream, line)) {
			std::vector<std::wstring> parts;
			boost::algorithm::split(parts, line, boost::is_any_of("\t"));
			if (parts.size() == 3) {
				std::wstring boundString = parts[0];
				std::wstring xdocID = parts[1];
				std::wstring boundID = parts[2];
				boost::trim(boundID);
				boost::trim(xdocID);
				if (xdocID == L"NONE") {
					continue;
				}
				XDocIdType lookupID(xdocID);
				//SessionLogger::info("LEARNIT")<<"load_mr_xdoc_output_file: " << lookup_id << std::endl;
				XDocIdType replacementID;
				// loop over "PER", "GPE", "ORG"
				for (size_t entity_type_i = 0; entity_type_i < PER_GPE_ORG_ENTITY_TYPE_COUNT; ++entity_type_i) { 
					update_replacement_id(entity_type_i, lookupID.as_string(), boundString, xdocID, replacementID);
				}
				try {
    				if(!replacementID.is_valid()){
						SessionLogger::dbg("insert_0")<<"Inserting <" << lookupID << "> -> <" << boundID
                            << "> into _xdoc_id_to_bound_uri table. " <<std::endl;
						_xdoc_id_to_bound_uri.insert(make_pair(lookupID, boundID)); 
					} else{
						SessionLogger::dbg("insert_1")<<"**inserting: "<<replacementID<<" "<<boundID<<std::endl;
						_xdoc_id_to_bound_uri.insert(make_pair(replacementID, boundID));
					}
				} catch(boost::bad_lexical_cast&) {
					// The xdoc mapping line is not valid; this may or may not be intended.
					SessionLogger::info("LEARNIT")<<"Not inserting: "<<xdocID<<" "<<boundID<<std::endl;
				}
			} else {
				if (!parts.empty() && !line.empty() && !ASCIIUtil::containsNonWhitespace(line)) {
					SessionLogger::warn("LEARNIT") << "load_mr_xdoc_output_file(): ignoring ill-formed line in XDoc failures "
						<< "file (" << filename << "): " << std::endl << line << std::endl;
				}
			}
		}
		instream.close();
	}
}

// called by load_mr_xdoc_output_file()
void ElfMultiDoc::update_replacement_id(int entity_type_i, const std::wstring & lookup_id, const std::wstring & boundString,
									 const XDocIdType & xdocID, XDocIdType & replacementXDocID) {
	const std::wstring entity_type(PER_GPE_ORG_ENTITY_TYPES[entity_type_i]);
	XDocIdType lookup_xid(lookup_id);
	float min_dist = 100;
	int best_cluster = -1;
    // Sample output:
        // ORG: Found: al-qaeda 103
	if(_entity_type_and_xdoc_id_to_names.find(EntityTypeAndXdocIdPairToNamesMap::key_type(entity_type, lookup_id)) 
		!= _entity_type_and_xdoc_id_to_names.end()){
		SessionLogger::dbg("found_ent_id_0") << entity_type << ": Found: " << boundString << " " << xdocID <<std::endl;
	}
	else{
		for(int i =0; i< 5; i++){
			//std::wstringstream newid;
			//newid<<lookup_id<<"-"<<i;
			XDocIdType new_xid;
			try {
				new_xid = new_xdoc_id_from_split_index(lookup_xid.as_int(), i);
			} catch(...) {
				// Invalid XDoc ID (e.g., "bbn:xdoc-NONE")
				continue;
			}
			//SessionLogger::info("LEARNIT") << "Looking up <" << new_xid.as_string() << ">" << std::endl;
			if(_entity_type_and_xdoc_id_to_names.find(EntityTypeAndXdocIdPairToNamesMap::key_type(entity_type, new_xid))  
				!= _entity_type_and_xdoc_id_to_names.end()) {
				std::vector<std::wstring> names = 
					ElfMultiDoc::_entity_type_and_xdoc_id_to_names[EntityTypeAndXdocIdPairToNamesMap::key_type(entity_type, new_xid)];
				BOOST_FOREACH(std::wstring name, names){
                    // We don't hit this point in the IC or NFL dry run corpus.
					float d = (float)CorefUtilities::editDistance(name, boundString)
						/std::min(name.size(), boundString.size());
					SessionLogger::info("edit_dist_0")<<"Edit distance: "<<name<<" ... "<<boundString<<" \t"<<d<<std::endl;
					if(d < min_dist){
						min_dist = d;
						best_cluster = i;
					}
				}
			}
		}
		if(min_dist < .5){
            // We don't hit this code in the IC or NFL dry run corpus.
			int new_xdoc_id = new_xdoc_id_from_split_index(xdocID, best_cluster);
			replacementXDocID = new_xdoc_id;
			SessionLogger::info("LEARNIT") << "New replacement XDoc ID: <" << replacementXDocID << ">" << std::endl;
		}
	}
}

/**
 * Updates the loaded ElfMultiDoc::_entity_type_and_name_to_xdoc_id map
 * using the loaded ElfMultiDoc::_type_to_srm, replacing
 * generated internal XDoc IDs with those bound URIs from
 * the argument map (SRM map) that have a matching name string in 
 * a cluster for some ACE entity type.
 *
 * @author mfreedma@bbn.com
 * @date 2010.08.29
 **/
void ElfMultiDoc::replace_xdoc_ids_with_mapped_args() {
	// Loop through the argument map, looking for matching name strings
	SessionLogger::info("map_xdoc_id_0") << "Mapping xdoc IDs..." << std::endl;
	// Ignore annotated document-specific replacements (stored in _docid_type_to_srm)
	BOOST_FOREACH(TypeToSRMMap::value_type mapped_arg, ElfMultiDoc::_type_to_srm) {
		// Check each replacement name string against the clusters
		//   Should this try to align ontology types with entity types?
		BOOST_FOREACH(StringReplacementMap::value_type string_replace, mapped_arg.second) {
			// Get the name string and replacement URI
			std::wstring name_string = string_replace.first;
			std::wstring uri = string_replace.second;
			
			Symbol entity_type;
			XDocIdType xdoc_id = get_xdoc_id_and_entity_type(name_string, entity_type);

			// If we got a xdoc ID for this name, use its replacement URI for the cluster
			if (xdoc_id.is_valid()) {
				BOOST_FOREACH(EntityTypeAndNamePairToXDocIdMap::value_type mapped_cluster, _entity_type_and_name_to_xdoc_id) {
					if (mapped_cluster.first.first == entity_type && mapped_cluster.second == xdoc_id) {
						//SessionLogger::info("LEARNIT") << "For entry with key (" << entity_type << ", " << mapped_cluster.first.second << "), "
						//	<< " replacing value <" << xdoc_id << "> with <" << uri << ">" << std::endl;
						_entity_type_and_name_to_bound_uri[make_pair(entity_type, name_string)] = uri;
					}
				}
			}
		}
	}
}

/**
 * Dumps a document level map of ACE entity ID --> canonical ID. 
 * Creating this table at the document level (rather than on a per-arg
 * basis) allows enforcing document level constraints on the mapping
 * (e.g., not creating a XDOC id for an entity if that id is shared by a
 * different entity in the document).
 *
 * @author mfreedma@bbn.com
 * @date 2011.01.05
 **/
void dumpStringsForEntity(const Entity* entity, const DocTheory* doc_theory){
	const EntitySet* entities = doc_theory->getEntitySet();
	SessionLogger::info("LEARNIT")<<"Entity: id-"<<entity->getID()<<" "<<entity->getType().getName().to_string()<<std::endl;
	name_mention_t best_name_mention = entity->getBestNameWithSourceMention(doc_theory);
	if(best_name_mention.second != 0 && best_name_mention.second->getMentionType() != Mention::NAME)
		return;	//this is a nameless mention
	std::wstring normalized_best_string = UnicodeUtil::normalizeTextString(best_name_mention.first);
	SessionLogger::info("LEARNIT")<<"   Entity: "<<normalized_best_string<<"      -->";
	for(int j = 0; j < entity->getNMentions(); j++){
		const Mention* mention = entities->getMention(entity->getMention(j));
		if(mention->getMentionType() != Mention::NAME)
			continue;
		const SynNode* head = mention->getAtomicHead();
		if(head){
			std::wstring name_string = UnicodeUtil::normalizeTextString(head->toTextString());
			SessionLogger::info("LEARNIT")<<"("<<name_string<<"), ";
		}
	}
	SessionLogger::info("LEARNIT")<<std::endl;
}

// Called by generate_entity_and_id_maps(). Updates eid_to_xdoc_id_final and xdoc_id_to_eid_final.
// Example of eid_to_xdoc_id_current at beginning of function: there are 22 entries, most of which map from a single
// XDoc ID to a set consisting of a single entity ID. Example: "bbn:xdoc-0" -> 66. Two of the entries map from a 
// single XDoc ID to a set consisting of two entity IDs: "bbn:xdoc-517" -> (0,33) and "bbn:xdoc-520" -> (2,70)	
void ElfMultiDoc::transfer_to_best_xdoc_id(const Entity* entity, const IntToSetOfXDocIdsMap & eid_to_xdoc_id_current, 
									  const XDocIdToSetOfIntsMap & xdoc_id_to_eid_current,  
									  IntToSetOfXDocIdsMap & eid_to_xdoc_id_final,
									  XDocIdToSetOfIntsMap & xdoc_id_to_eid_final,
									  const DocTheory* doc_theory, bool allow_multiple_entities_to_share_id /*= false*/)
{

	XDocIdType best_xdoc_id;
	if(eid_to_xdoc_id_final.find(entity->getID()) != eid_to_xdoc_id_final.end())
		return;
	IntToSetOfXDocIdsMap::const_iterator iter = eid_to_xdoc_id_current.find(entity->getID());
	if(iter == eid_to_xdoc_id_current.end())
		return;
	
	std::set<XDocIdType> ids = (*iter).second;
	if(ids.size() != 1) {
		print_info_for_entity_with_multiple_xdoc_ids(entity, doc_theory, ids);
		return;
	}
	XDocIdType possible_xdoc_id = (*ids.begin());
	if(xdoc_id_to_eid_final.find(possible_xdoc_id) != xdoc_id_to_eid_final.end()){
		handle_found_xdoc_id_to_eid(possible_xdoc_id, doc_theory, entity, best_xdoc_id, eid_to_xdoc_id_final, xdoc_id_to_eid_final,
			allow_multiple_entities_to_share_id);
		return;
	}
	std::set<int> other_entities =(*xdoc_id_to_eid_current.find(possible_xdoc_id)).second;
	if(other_entities.size() == 1){
		eid_to_xdoc_id_final[entity->getID()].insert(possible_xdoc_id);
		xdoc_id_to_eid_final[best_xdoc_id].insert(entity->getID()); // note that best_xdoc_id may point to the invalid XDoc ID (-1)
	}
	else if(allow_multiple_entities_to_share_id){
		eid_to_xdoc_id_final[entity->getID()].insert(possible_xdoc_id);
		xdoc_id_to_eid_final[best_xdoc_id].insert(entity->getID());
	}
	else{
		handle_multiple_other_entities_for_xdoc_id(other_entities, possible_xdoc_id, doc_theory, entity, best_xdoc_id, 
			eid_to_xdoc_id_final, xdoc_id_to_eid_final);
	}
}

// Called by generate_entity_and_id_maps(). Updates eid_to_bound_uri_final and bound_uri_to_eid_final.
void ElfMultiDoc::transfer_to_best_bound_uri(const Entity* entity, const IntToSetOfStringsMap & eid_to_bound_uri_current, 
									  const StringToSetOfIntsMap & bound_uri_to_eid_current,  
									  IntToSetOfStringsMap & eid_to_bound_uri_final,
									  StringToSetOfIntsMap & bound_uri_to_eid_final,
									  const DocTheory* doc_theory, bool allow_multiple_entities_to_share_id /*= false*/)
{

	std::wstring NO_ID = L"-NONE-";
	std::wstring best_bound_uri = NO_ID;
	if(eid_to_bound_uri_final.find(entity->getID()) != eid_to_bound_uri_final.end())
		return;
	IntToSetOfStringsMap::const_iterator iter = eid_to_bound_uri_current.find(entity->getID());
	if(iter == eid_to_bound_uri_current.end())
		return;
	
	SetOfStrings uris = (*iter).second;
	if(uris.size() != 1) {
		// print_info_for_entity_with_multiple_bound_uris(entity, doc_theory, ids);
		return;
	}
	std::wstring possible_bound_uri = (*uris.begin());
	if(bound_uri_to_eid_final.find(possible_bound_uri) != bound_uri_to_eid_final.end()){
		handle_found_bound_uri_to_eid(possible_bound_uri, doc_theory, entity, best_bound_uri, eid_to_bound_uri_final, bound_uri_to_eid_final,
			allow_multiple_entities_to_share_id);
		return;
	}
	std::set<int> other_entities =(*bound_uri_to_eid_current.find(possible_bound_uri)).second;
	if(other_entities.size() == 1){
		eid_to_bound_uri_final[entity->getID()].insert(possible_bound_uri);
		bound_uri_to_eid_final[best_bound_uri].insert(entity->getID());
	}
	else if(allow_multiple_entities_to_share_id){
		eid_to_bound_uri_final[entity->getID()].insert(possible_bound_uri);
		bound_uri_to_eid_final[best_bound_uri].insert(entity->getID());
	}
	else{
		handle_multiple_other_entities_for_bound_uri(other_entities, possible_bound_uri, doc_theory, entity, best_bound_uri, eid_to_bound_uri_final, 
			bound_uri_to_eid_final);
	}
}

// called by transfer_to_best_xdoc_id()
void ElfMultiDoc::handle_found_xdoc_id_to_eid(const XDocIdType & possible_xdoc_id, const DocTheory* doc_theory, 
											 const Entity* entity, const XDocIdType & best_xdoc_id,
											 IntToSetOfXDocIdsMap& eid_to_xdoc_id_final,
											 XDocIdToSetOfIntsMap& xdoc_id_to_eid_final,
											 bool allow_multiple_entities_to_share_id) {
	bool all_match = true;
	name_mention_t best_name_mention = entity->getBestNameWithSourceMention(doc_theory);
	int wk_id = CorefUtilities::lookUpWKName(best_name_mention.second);
    int eid = entity->getID();
	std::set<int> other_entities = xdoc_id_to_eid_final[possible_xdoc_id];
	if(allow_multiple_entities_to_share_id){
		eid_to_xdoc_id_final[eid].insert(possible_xdoc_id);
		xdoc_id_to_eid_final[best_xdoc_id].insert(eid);
		return;
	}
	if(wk_id == -1 || wk_id == 0){
		all_match = false;
	}
	else{					
		BOOST_FOREACH(int id, other_entities){
			name_mention_t oth_best_mention = doc_theory->getEntitySet()->getEntity(id)->getBestNameWithSourceMention(doc_theory);
			int oth_wk_id = CorefUtilities::lookUpWKName(oth_best_mention.second);
			if(oth_wk_id != wk_id)
				all_match = false;
		}
	}
	if(all_match){
		//SessionLogger::info("LEARNIT")<<"A: Allowing FinalID Match: World Knowledge "<<wk_id<<std::endl;
		eid_to_xdoc_id_final[eid].insert(possible_xdoc_id);
		xdoc_id_to_eid_final[best_xdoc_id].insert(eid);
	}
	else{
		//SessionLogger::info("LEARNIT")<<"No link: other final entity has this ID: <" << possible_xdoc_id << ">" << std::endl;
	}
	//dumpStringsForEntity(entity, doc_theory);
	//SessionLogger::info("LEARNIT")<<"OTHERS: "<<std::endl;
	//BOOST_FOREACH(int eid, other_entities){
	//	dumpStringsForEntity(doc_theory->getEntitySet()->getEntity(eid), doc_theory);
	//}
}

// called by transfer_to_best_bound_uri()
void ElfMultiDoc::handle_found_bound_uri_to_eid(const std::wstring & possible_bound_uri, const DocTheory* doc_theory, 
												const Entity* entity, const std::wstring & best_bound_uri,
											 IntToSetOfStringsMap& eid_to_bound_uri_final,
											 StringToSetOfIntsMap& bound_uri_to_eid_final,
											 bool allow_multiple_entities_to_share_id) {
	bool all_match = true;
	name_mention_t best_name_mention = entity->getBestNameWithSourceMention(doc_theory);
	int wk_id = CorefUtilities::lookUpWKName(best_name_mention.second);
    int eid = entity->getID();
	std::set<int> other_entities = bound_uri_to_eid_final[possible_bound_uri];
	if(allow_multiple_entities_to_share_id){
		eid_to_bound_uri_final[eid].insert(possible_bound_uri);
		bound_uri_to_eid_final[best_bound_uri].insert(eid);
		return;
	}
	if(wk_id == -1 || wk_id == 0){
		all_match = false;
	}
	else{					
		BOOST_FOREACH(int id, other_entities){
			name_mention_t oth_best_mention = doc_theory->getEntitySet()->getEntity(id)->getBestNameWithSourceMention(doc_theory);
			int oth_wk_id = CorefUtilities::lookUpWKName(oth_best_mention.second);
			if(oth_wk_id != wk_id)
				all_match = false;
		}
	}
	if(all_match){
		//SessionLogger::info("LEARNIT")<<"A: Allowing FinalID Match: World Knowledge "<<wk_id<<std::endl;
		eid_to_bound_uri_final[eid].insert(possible_bound_uri);
		bound_uri_to_eid_final[best_bound_uri].insert(eid);
	}
	else{
		//SessionLogger::info("LEARNIT")<<"No link: other final entity has this ID: <" << possible_bound_uri << ">" << std::endl;
	}
	//dumpStringsForEntity(entity, doc_theory);
	//SessionLogger::info("LEARNIT")<<"OTHERS: "<<std::endl;
	//BOOST_FOREACH(int eid, other_entities){
	//	dumpStringsForEntity(doc_theory->getEntitySet()->getEntity(eid), doc_theory);
	//}
}

// called by transfer_to_best_xdoc_id()
// If lookupWKName() succeeds, updates eid_to_xdoc_id_final and xdoc_id_to_eid_final with a
// single element each. Otherwise, does nothing.
void ElfMultiDoc::handle_multiple_other_entities_for_xdoc_id(const std::set<int> & other_entities, const XDocIdType & possible_id, 
												 const DocTheory* doc_theory, 
											 const Entity* entity, const XDocIdType & best_xdoc_id,
											 IntToSetOfXDocIdsMap & eid_to_xdoc_id_final,
											 XDocIdToSetOfIntsMap & xdoc_id_to_eid_final) {
	bool all_match = true;
	name_mention_t best_name_mention = entity->getBestNameWithSourceMention(doc_theory);
	int wk_id = CorefUtilities::lookUpWKName(best_name_mention.second);
	if(wk_id == -1 || wk_id == 0){
		all_match = false;
	}
	else{					
		BOOST_FOREACH(int id, other_entities){
			name_mention_t oth_best_mention = doc_theory->getEntitySet()->getEntity(id)->getBestNameWithSourceMention(doc_theory);
			int oth_wk_id = CorefUtilities::lookUpWKName(oth_best_mention.second);
			if(oth_wk_id != wk_id)
				all_match = false;
		}
	}
	if(all_match){
		//SessionLogger::info("LEARNIT")<<"B: Allowing FinalID Match: World Knowledge: "<<wk_id<<std::endl;
		eid_to_xdoc_id_final[entity->getID()].insert(possible_id);
		xdoc_id_to_eid_final[best_xdoc_id].insert(entity->getID());
	} else{
		//SessionLogger::info("LEARNIT")<<"No link: other current entities have this ID: <"<< possible_id << ">" << std::endl;
	}
	//dumpStringsForEntity(entity, doc_theory);
	//SessionLogger::info("LEARNIT")<<"OTHERS: "<<std::endl;
	//BOOST_FOREACH(int eid, other_entities){
	//	dumpStringsForEntity(doc_theory->getEntitySet()->getEntity(eid), doc_theory);
	//}
	return;
}

// called by transfer_to_best_bound_uri()
// If lookupWKName() succeeds, updates eid_to_bound_uri_final and bound_uri_to_eid_final with a
// single element each. Otherwise, does nothing.
void ElfMultiDoc::handle_multiple_other_entities_for_bound_uri(const std::set<int> & other_entities, const std::wstring & possible_id, 
												 const DocTheory* doc_theory, 
											 const Entity* entity, const std::wstring & best_bound_uri,
											 IntToSetOfStringsMap & eid_to_bound_uri_final,
											 StringToSetOfIntsMap & bound_uri_to_eid_final) {
	bool all_match = true;
	name_mention_t best_name_mention = entity->getBestNameWithSourceMention(doc_theory);
	int wk_id = CorefUtilities::lookUpWKName(best_name_mention.second);
	if(wk_id == -1 || wk_id == 0){
		all_match = false;
	}
	else{					
		BOOST_FOREACH(int id, other_entities){
			name_mention_t oth_best_mention = doc_theory->getEntitySet()->getEntity(id)->getBestNameWithSourceMention(doc_theory);
			int oth_wk_id = CorefUtilities::lookUpWKName(oth_best_mention.second);
			if(oth_wk_id != wk_id)
				all_match = false;
		}
	}
	if(all_match){
		//SessionLogger::info("LEARNIT")<<"B: Allowing FinalID Match: World Knowledge: "<<wk_id<<std::endl;
		eid_to_bound_uri_final[entity->getID()].insert(possible_id);
		bound_uri_to_eid_final[best_bound_uri].insert(entity->getID());
	} else{
		//SessionLogger::info("LEARNIT")<<"No link: other current entities have this ID: <"<< possible_id << ">" << std::endl;
	}
	//dumpStringsForEntity(entity, doc_theory);
	//SessionLogger::info("LEARNIT")<<"OTHERS: "<<std::endl;
	//BOOST_FOREACH(int eid, other_entities){
	//	dumpStringsForEntity(doc_theory->getEntitySet()->getEntity(eid), doc_theory);
	//}
	return;
}

// called by transfer_to_best_xdoc_id()
void ElfMultiDoc::print_info_for_entity_with_multiple_xdoc_ids(const Entity* entity, const DocTheory* doc_theory, 
															   const std::set<XDocIdType> & ids) {
	SessionLogger::info("LEARNIT")<<"Preventing link: This entity has multiple XDOC Ids"<<std::endl;
	dumpStringsForEntity(entity, doc_theory);
	SessionLogger::info("LEARNIT")<<"XDOC: "<<std::endl;
	BOOST_FOREACH(XDocIdType id, ids){
		SessionLogger::info("LEARNIT")<<"id: "<<id;
		std::vector<std::wstring> other_names;
		EntityTypeAndXdocIdPairToNamesMap::iterator cluster_i = ElfMultiDoc::_entity_type_and_xdoc_id_to_names.find(
			EntityTypeAndXdocIdPairToNamesMap::key_type(entity->getType().getName(), id));
		if(cluster_i !=  ElfMultiDoc::_entity_type_and_xdoc_id_to_names.end()){
			other_names.insert(other_names.end(), cluster_i->second.begin(), cluster_i->second.end());
			BOOST_FOREACH(std::wstring n, other_names){
				SessionLogger::info("LEARNIT")<<"("<<n<<"), ";
			}
			SessionLogger::info("LEARNIT")<<std::endl;					
		}
	}
	return;
}

// called by add_bound_uris_for_entity()
bool ElfMultiDoc::is_valid_ontology_type_for_entity_type(const Entity* entity, SetOfStrings types, 
												   const DocTheory* doc_theory){
	bool has_gpe_loc_match = false;
	bool has_per_match = false;
	bool has_org_match = false;
	bool has_wea_match = false;
	bool has_fac_match = false;
	bool has_gpe_org_match = false;
	bool has_per_org_gpe_match = false;
	EntityType etype(entity->getType());
	BOOST_FOREACH(std::wstring uri_type, types){
		if(_gpe_loc_subtypes.find(uri_type) != _gpe_loc_subtypes.end())
			has_gpe_loc_match = true;
		if(_gpe_org_subtypes.find(uri_type) != _gpe_org_subtypes.end())
			has_gpe_org_match = true;
		if(_per_subtypes.find(uri_type) != _per_subtypes.end())
			has_per_match = true;
		if(_org_subtypes.find(uri_type) != _org_subtypes.end())
			has_org_match = true;
		if(_wea_subtypes.find(uri_type) != _wea_subtypes.end())
			has_wea_match = true;
		if(_fac_subtypes.find(uri_type) != _fac_subtypes.end())
			has_fac_match = true;
		if(_per_gpe_org_subtypes.find(uri_type) != _per_gpe_org_subtypes.end())
			has_per_org_gpe_match = true;
	}
	if( has_gpe_loc_match && (etype.matchesGPE() || etype.matchesLOC()))
		return true;
	if( has_gpe_org_match && (etype.matchesGPE() || etype.matchesORG()))
		return true;
	if( has_per_match && etype.matchesPER())
		return true;
	if( has_org_match && etype.matchesORG())
		return true;
	if( has_fac_match && etype.matchesFAC())
		return true;
	if( has_wea_match && etype.getName() == Symbol(L"WEA"))
		return true;

	if(!has_gpe_loc_match && !has_gpe_org_match && !has_org_match && !has_per_match && 
		!has_wea_match && !has_fac_match && has_per_org_gpe_match){
		if(etype.matchesPER() || etype.matchesORG() || etype.matchesGPE() || 
			etype.matchesLOC()){
			//SessionLogger::info("LEARNIT")<<"\nOnly Generic types: Linking Liberally "<<uri<<std::endl;
			//BOOST_FOREACH(std::wstring t, types){
			//	SessionLogger::info("LEARNIT")<<t<<std::endl;
			//}
			//dumpStringsForEntity(entity, doc_theory);
			return true;
		}
	}
	if(!etype.matchesPER())
		return true;
	//for people we need to be sure we don't allow 'The American went to Paris' to become the same URI as U.S.
	bool has_nationality_indiv = has_nationality_individual(doc_theory, entity);
	if (has_nationality_indiv) {
		//SessionLogger::info("LEARNIT")<<"\nNot linking entity to URI: Type Mismatch: "<<uri<<std::endl;
		BOOST_FOREACH(std::wstring t, types){
			SessionLogger::info("LEARNIT")<<t<<std::endl;
		}
		dumpStringsForEntity(entity, doc_theory);
		return false;
	}
	return true;
}

// called by is_valid_ontology_type_for_entity_type()
bool ElfMultiDoc::has_nationality_individual(const DocTheory* doc_theory, const Entity* entity) {
	for(int m = 0; m < entity->getNMentions(); m++){
		const Mention* ment = doc_theory->getEntitySet()->getMention(entity->getMention(m));
		if(ment->getMentionType() != Mention::DESC){
			continue;
		}
		const PropositionSet *ps = doc_theory->getSentenceTheory(ment->getSentenceNumber())->getPropositionSet();
		const TokenSequence *ts = doc_theory->getSentenceTheory(ment->getSentenceNumber())->getTokenSequence();

		const Proposition *defProp = ps->getDefinition(ment->getIndex());
		if (defProp == 0 || defProp->getPredType() != Proposition::NOUN_PRED)
			continue;
		Symbol headword = ment->getNode()->getHeadWord();
		if (NationalityRecognizer::isLowercaseNationalityWord(headword)){
			std::wstring fullMentionText = ment->getNode()->toTextString();
			if (NationalityRecognizer::isLowercaseCertainNationalityWord(headword) ||
					fullMentionText.find(L"a ") == 0 ||
					fullMentionText.find(L"an ") == 0 ||
					fullMentionText.find(L"one ") == 0)	{
					return true; //e.g. an American said ...
				} 
			else if (fullMentionText.find(L"the ") == 0) {
				// Eliminate frequent bad construction: "(the GPE) and (GPE noun)"
				if (ment->getNode()->getEndToken() + 1 < ts->getNTokens()) {
					std::wstring next_word = ts->getToken(ment->getNode()->getEndToken() + 1)->getSymbol().to_string();
                    boost::to_lower(next_word);
                    // Note that next_word is never used (and hasn't been since at least r27329); is this an oversight?
				} 
				else{
					return true;
				}
			} 
		}
	}
	return false;
}

/**
 * @example eid: 0 <-> bound_uri: "nfl:NewEnglandPatriots"
 * @example eid: 1 <-> bound_uri: "nfl:NewYorkGiants"
 * @example eid_to_ids_current: [2]((0,[1]("nfl:NewEnglandPatriots")),(1,[1]("nfl:NewYorkGiants")))
 * @example id_to_eids_current: [2](("nfl:NewEnglandPatriots",[1](0)),("nfl:NewYorkGiants",[1](1)))
 *
 **/
void ElfMultiDoc::add_to_bound_uri_tables(const Entity* entity, 
										  const std::wstring & bound_uri, 
										  IntToSetOfStringsMap & eid_to_ids_current, 
										  StringToSetOfIntsMap & id_to_eids_current)
{
	//SessionLogger::info("LEARNIT") << "Mapping <" << entity->getID() << "> to <" << bound_uri << ">" << endl;
	if(!bound_uri.empty()){
		eid_to_ids_current[entity->getID()].insert(bound_uri);
		id_to_eids_current[bound_uri].insert(entity->getID());
	}
}

/**
 * @example eid: 0 <-> xdoc_id: "bbn:xdoc-517"
 * @example eid: 1 <-> xdoc_id: "bbn:xdoc-3910"
 * @example eid_to_ids_current: [2]((0,[1]("bbn:xdoc-517")),(1,[1]("bbn:xdoc-3910")))
 * @example id_to_eids_current: [2](("bbn:xdoc-517",[1](0)),("bbn:xdoc-3910",[1](1)))
 *
 **/
void ElfMultiDoc::add_to_xdoc_id_tables(const Entity* entity, const XDocIdType & xdoc_id, 
										IntToSetOfXDocIdsMap & eid_to_ids_current, 
										XDocIdToSetOfIntsMap & id_to_eids_current)
{
	//SessionLogger::info("LEARNIT") << "Mapping <" << entity->getID() << "> to <" << xdoc_id << ">" << endl;
	if(xdoc_id.is_valid()){
		eid_to_ids_current[entity->getID()].insert(xdoc_id);
		id_to_eids_current[xdoc_id].insert(entity->getID());
	}
}

int ElfMultiDoc::add_bound_uris_for_entity_w_or_wo_docid(const Entity* entity, const std::wstring & name_string, 
												   const std::wstring & docid, 
												   IntToSetOfStringsMap & eid_to_bound_uri,
												   StringToSetOfIntsMap & bound_uri_to_eid, 
												   const DocTheory* doc_theory) {
	int n_added = add_bound_uris_for_entity_w_docid(entity, name_string, docid, eid_to_bound_uri, bound_uri_to_eid, 
		doc_theory);
	if (n_added == 0) {
		n_added = add_bound_uris_for_entity_wo_docid(entity, name_string, eid_to_bound_uri, bound_uri_to_eid, doc_theory);
	}
	return n_added;
}

int ElfMultiDoc::add_bound_uris_for_entity_w_docid(const Entity* entity, const std::wstring & name_string, 
												   const std::wstring & docid, 
												   IntToSetOfStringsMap & eid_to_bound_uri,
												   StringToSetOfIntsMap & bound_uri_to_eid, 
												   const DocTheory* doc_theory)
{
	std::wstring normalized_best_string = UnicodeUtil::normalizeTextString(name_string);
	StringPairToSetOfStringsMap::iterator val = _name_docid_to_bound_uris.find(
		std::pair<std::wstring, std::wstring>(normalized_best_string, docid));
	int n_added = 0;
	if(val != _name_docid_to_bound_uris.end()){
		n_added = add_eid_to_bound_uri_mappings(entity, (*val).second, eid_to_bound_uri, bound_uri_to_eid, doc_theory);
	}
	return n_added;
}

int ElfMultiDoc::add_bound_uris_for_entity_wo_docid(const Entity* entity, const std::wstring & name_string, 
												   IntToSetOfStringsMap & eid_to_bound_uri,
												   StringToSetOfIntsMap & bound_uri_to_eid, 
												   const DocTheory* doc_theory)
{
	std::wstring normalized_best_string = UnicodeUtil::normalizeTextString(name_string);
	int n_added = 0;
	StringToSetOfStringsMap::iterator val = _name_to_bound_uris.find(normalized_best_string);
	if(val != _name_to_bound_uris.end()){
		n_added = add_eid_to_bound_uri_mappings(entity, (*val).second, eid_to_bound_uri, bound_uri_to_eid, doc_theory);
	}
	return n_added;
}

int ElfMultiDoc::add_eid_to_bound_uri_mappings(const Entity* entity, const SetOfStrings & uris,
												   IntToSetOfStringsMap& eid_to_bound_uri,
												   StringToSetOfIntsMap& bound_uri_to_eid, 
												   const DocTheory* doc_theory) {
	int n_added(0);
	BOOST_FOREACH(std::wstring uri, uris){
		//is this entity valid for this uri type
		StringToSetOfStringsMap::iterator types = _bound_uri_to_types.find(uri);
		bool has_valid_type = is_valid_ontology_type_for_entity_type(entity, (*types).second, doc_theory);
		if(has_valid_type){
			add_to_bound_uri_tables(entity, uri, eid_to_bound_uri, bound_uri_to_eid);
			n_added++;
		}
	}
	return n_added;
}

/**
 * Get acronyms that can correspond to a string.
 * @param s String for which acronyms are to be retrieved.
 * @return Set of acronyms to be retrieved.
 * @example get_possible_acronyms("american insurance group") -> ("a i g", "a i g ", "aig")
 **/
SetOfStrings ElfMultiDoc::get_possible_acronyms(const std::wstring & s){
	SetOfStrings acronyms;
	std::vector<std::wstring> words;
	boost::algorithm::split(words, s, boost::algorithm::is_from_range(L' ', L' '));
	std::wstring a1, a2, a3;
	BOOST_FOREACH(std::wstring word, words){
		if(WordConstants::isAcronymStopWord(Symbol(word)))
			continue;
		wchar_t c = word[0];
		a1.push_back(c);
		a2.push_back(c);
		a2.push_back(L' ');
	}
	a2 = UnicodeUtil::normalizeTextString(a2);
	a3 = a2;
	a3.push_back(L' ');
	acronyms.insert(a1);
	acronyms.insert(a2);
	acronyms.insert(a3);
	return acronyms;
}

/**
 * Get acronyms that can correspond to a SynNode.
 * @param n SynNode for which acronyms are to be retrieved.
 * @return Set of acronyms to be retrieved.
 **/
SetOfStrings ElfMultiDoc::get_possible_acronyms_from_syn_node(const SynNode* n){
	SetOfStrings acronyms;
	Symbol ment_symbols[20];
	int n_terms = n->getTerminalSymbols(ment_symbols, 19);
	if(n_terms == 1)
		return acronyms;
	std::wstring a1, a2, a3;
	std::vector<wchar_t> characters;
	for(int i =0; i <n_terms; i++){
		Symbol s = ment_symbols[i];
		if(WordConstants::isAcronymStopWord(s))	
			continue; //don't worry about the rare acronym that includes a stop word
		wchar_t c = s.to_string()[0];
		a1.push_back(c);
		a2.push_back(c);
		if(i !=  (int)characters.size()-1)
			a2.push_back(L'.');	 //don't worry about the rare acronym that ends with a stop word
	}
	a3 = a2;
	a3.push_back(L'.');
	acronyms.insert(a1);
	acronyms.insert(a2);
	acronyms.insert(a3);
	return acronyms;
}

// called by generate_entity_and_id_maps()
IntToSetOfStringsMap ElfMultiDoc::get_xdoc_conflicts(const DocTheory* doc_theory){
	IntToSetOfStringsMap multi_match_entities_ids;
	const EntitySet* entities = doc_theory->getEntitySet();
	// Initialize this set with the const base names defined at the top of the file.
	// Additional names will be inserted, so we initialize it each time we call the method.
	SetOfStrings do_not_link_names(DO_NOT_LINK_BASE_NAMES, 
		DO_NOT_LINK_BASE_NAMES + DO_NOT_LINK_BASE_NAMES_COUNT);
	//get country names that appear in entities
    int entity_count(entities->getNEntities());
	for(int i = 0; i < entity_count; i++){
		const Entity* entity = entities->getEntity(i);
		if(!entity->getType().matchesGPE())
			continue;
		EntitySubtype subtype = entities->guessEntitySubtype(entity);
		if(subtype.getName() != Symbol(L"Nation"))
			continue;
        int mention_count(entity->getNMentions());
		for(int j = 0; j < mention_count; j++){
			const Mention* mention = entities->getMention(entity->getMention(j));
			if(mention->getMentionType() != Mention::NAME)
				continue;
			const SynNode* head = mention->getAtomicHead();
			if(!head)
				continue;
			std::wstring normalized_string = UnicodeUtil::normalizeTextString(head->toTextString());
			do_not_link_names.insert(normalized_string);
		}
	}
	//now loop through entities looking for conflicts
	for(int i = 0; i < entity_count; i++){
		const Entity* entity = entities->getEntity(i);
		name_mention_t best_name_mention = entity->getBestNameWithSourceMention(doc_theory);
		const Mention* bestMention = best_name_mention.second;
		if(bestMention == 0)
			continue;
		if(bestMention->getMentionType() != Mention::NAME)	
			continue;	
		const SynNode* bestHead = best_name_mention.second->getAtomicHead();
		std::wstring normalized_best_string = UnicodeUtil::normalizeTextString(best_name_mention.first);
		XDocIdType xdoc_id = get_xdoc_id(entity->getType().getName(), normalized_best_string);
        int mention_count(entity->getNMentions());
		for(int j = 0; j < mention_count; j++){
			const Mention* mention = entities->getMention(entity->getMention(j));
			if(mention->getMentionType() != Mention::NAME)
				continue;
			const SynNode* head = mention->getAtomicHead();
			if(head){
				std::wstring name_string = UnicodeUtil::normalizeTextString(head->toTextString());
				XDocIdType new_xdoc_id = get_xdoc_id(entity->getType().getName(), name_string);
				if( !entity->getType().matchesPER() || (float)name_string.size()/normalized_best_string.size() > .75){
					bool ok_for_linking = ok_to_link(new_xdoc_id.as_string(), xdoc_id.as_string(), normalized_best_string, name_string,
						entity, head, bestHead, do_not_link_names);
					//Check for Acronyms
					if(!ok_for_linking){
						const Mention* bnm = best_name_mention.second;
						const SynNode* bnm_head = bnm->getAtomicHead();
						if(bnm_head->getNTerminals() == 1){
							SetOfStrings acronyms = get_possible_acronyms_from_syn_node(head);
							Symbol words[5];  
							bnm_head->getTerminalSymbols(words, 4);
							if(acronyms.find(words[0].to_string()) != acronyms.end()){
								ok_for_linking = true;
							}
						}
						else if(head->getNTerminals() == 1){
							SetOfStrings acronyms = get_possible_acronyms_from_syn_node(bnm_head);
							Symbol words[5];  
							head->getTerminalSymbols(words, 4);
							if(acronyms.find(words[0].to_string()) != acronyms.end()){
								ok_for_linking = true;
							}
						}
					}
					if(!ok_for_linking){
						std::wstringstream s;
						s <<new_xdoc_id<<L":"<<name_string;
						multi_match_entities_ids[i].insert(s.str());
						SessionLogger::info("LEARNIT") << "Inserted <" << s.str() << "> into multi_match_entities_ids[" << i << "]" 
							<< std::endl;
						s.clear();
						s <<xdoc_id<<L":"<<normalized_best_string;
						multi_match_entities_ids[i].insert(s.str());	
						SessionLogger::info("LEARNIT") << "Inserted <" << s.str() << "> into multi_match_entities_ids[" << i << "]" 
							<< std::endl;
					}
				} // if !entity->getType().matchesPER()...
			}// if (head)
		} // for j
	} // for i
	return multi_match_entities_ids;
}

// called by get_xdoc_conflicts()
bool ElfMultiDoc::ok_to_link(const XDocIdType & new_xdoc_id, const XDocIdType & xdoc_id,
							 const std::wstring & normalized_best_string, const std::wstring & name_string,
							 const Entity* entity, const SynNode* head, const SynNode* bestHead,
							 const SetOfStrings & do_not_link_names) {
	// Initialize this set with the const base names defined at the top of the file.
	// No additional names will be inserted, so we only have to initialize it once.
	static std::vector<std::wstring> gpe_do_not_link_names(GPE_DO_NOT_LINK_BASE_NAMES, 
		GPE_DO_NOT_LINK_BASE_NAMES + GPE_DO_NOT_LINK_BASE_NAMES_COUNT);
	if((!new_xdoc_id.is_valid() || !xdoc_id.is_valid() || new_xdoc_id == xdoc_id)){ //same XDOC IDs
		return true;
	}
	//everything below allows special case linking
	//The entity might be ok if one name is a substring of the other 
	std::wstring longer(normalized_best_string);
	std::wstring shorter;
	if(name_string.size() > longer.size()){
		shorter = longer;
		longer = name_string;
	}
	else{
		shorter = name_string;
	}
	//hack to match Italy --> Italian, etc. 
	shorter = std::wstring(shorter.begin(), shorter.end()-1);
	//If this is a substring, check for illegal words 
	if(longer.find(shorter) != std::wstring::npos){
		bool found_conflict = false;  //make sure this isn't GPE with an issue
		if(entity->getType().matchesGPE()){
			BOOST_FOREACH(std::wstring w, gpe_do_not_link_names){
				bool f1 = (normalized_best_string.find(w) == std::wstring::npos);
				bool f2 = (name_string.find(w) == std::wstring::npos);
				if(f1 != f2){
					found_conflict = true;
				}
				if((f1 || f2) && (bestHead != 0 && head->getNTerminals() != bestHead->getNTerminals())){
					found_conflict = true;
				}
			}
		}
		else{//check other words
			BOOST_FOREACH(std::wstring w, do_not_link_names){
				bool f1 = (normalized_best_string.find(w) == std::wstring::npos);
				bool f2 = (name_string.find(w) == std::wstring::npos);
				if(f1 != f2){
					found_conflict = true;
				}
			}
		}
		//Substring and no conflict so ok to link
		if(!found_conflict)
			return true;
	}
	return false;
}

// Top-level method. Updates _eid_to_xdoc_ids, _eid_to_bound_uris, _eid_to_bound_uris_merged.
// Reads _xdoc_id_to_bound_uri, which must have already been set by load_mr_xdoc_output_file(), 
// and whose contents will look something like this:
// [5](("bbn:xdoc-103","ic:al-Qaeda"),("bbn:xdoc-192","ic:Abdul-Majid_al-Khoei"),("bbn:xdoc-3178","ic:King_Fahd"),
// ("bbn:xdoc-723","ic:Al-Aqsa_Martyr_s_Brigade"),("bbn:xdoc-8832","ic:Izz_el-Deen_al-Qassam_organization"))
void ElfMultiDoc::generate_entity_and_id_maps(const DocTheory* doc_theory) {
	const EntitySet* entities = doc_theory->getEntitySet();
	std::wstring docid = std::wstring(doc_theory->getDocument()->getName().to_string());
	IntToSetOfXDocIdsMap eid_to_xdoc_id_best;
	IntToSetOfXDocIdsMap eid_to_xdoc_id_all;
	XDocIdToSetOfIntsMap xdoc_id_best_to_eid;
	XDocIdToSetOfIntsMap xdoc_id_all_to_eid;
	//IntToSetOfStringsMap eid_to_xdoc_id_final;
	XDocIdToSetOfIntsMap xdoc_id_to_eid_final;
	IntToSetOfStringsMap eid_to_alignable_strings;
	StringToSetOfIntsMap alignable_strings_to_eid;
	IntToSetOfStringsMap multi_match_entities_ids;

	_eid_to_xdoc_ids.clear();
	_eid_to_bound_uris.clear();
	_eid_to_bound_uris_merged.clear();
	if(ParamReader::getOptionalTrueFalseParamWithDefaultVal("block_within_doc_merged_xdoc", false) 
		&& _ontology_domain != L"nfl" ){
		multi_match_entities_ids = get_xdoc_conflicts(doc_theory);
	}

	for(int i = 0; i < entities->getNEntities(); i++){
		const Entity* entity = entities->getEntity(i);
		name_mention_t best_name_mention = entity->getBestNameWithSourceMention(doc_theory);
		const Mention* bestMention = best_name_mention.second;
		if(bestMention == 0)
			continue;
		if(bestMention->getMentionType() != Mention::NAME)	
			continue;	//this is a nameless mention
		std::wstring normalized_best_string = UnicodeUtil::normalizeTextString(best_name_mention.first);
		XDocIdType xdoc_id = get_xdoc_id(entity->getType().getName(), normalized_best_string);
		if (xdoc_id.is_valid()) {
			add_to_xdoc_id_tables(entity, xdoc_id, eid_to_xdoc_id_best, xdoc_id_best_to_eid);		
			for(int j = 0; j < entity->getNMentions(); j++){
				const Mention* mention = entities->getMention(entity->getMention(j));
				if(mention->getMentionType() != Mention::NAME)
					continue;
				const SynNode* head = mention->getAtomicHead();
				if(head){
					std::wstring name_string = UnicodeUtil::normalizeTextString(head->toTextString());
					XDocIdType new_xdoc_id = get_xdoc_id(entity->getType().getName(), name_string);
					// HACK--> We want to be conservative, so only allow alternatives that are 75% the length 
					// (in characters) of the best name
					if((float)name_string.size()/normalized_best_string.size() > .75){
                        // example: eid: 7 <-> name_string: "Ali Abdallah Saleh"
                        // example: eid: 8 <-> name_string: "AFP" <-> xdoc_id: "bbn:xdoc-69"
						add_to_xdoc_id_tables(entity, new_xdoc_id, eid_to_xdoc_id_all, xdoc_id_all_to_eid);	
						eid_to_alignable_strings[entity->getID()].insert(name_string);
						alignable_strings_to_eid[name_string].insert(entity->getID());
					}
				}
			}
		} else {
			SessionLogger::info("xdi_failed_0") << "get_xdoc_id(" << entity->getType().getName() << ", " << normalized_best_string  << ") "
				<< "failed in generate_entity_and_id_maps()." << std::endl;
		}
	}

	for(int i = 0; i < entities->getNEntities(); i++){
		transfer_to_best_xdoc_id(entities->getEntity(i), eid_to_xdoc_id_best, xdoc_id_best_to_eid, _eid_to_xdoc_ids, xdoc_id_to_eid_final, 
			doc_theory);
	}
    // by the end of the previous call, xdoc_id_to_eid_final contains the following:	
    // [1](("-NONE-",[20](1,3,7,8,12,24,37,43,46,48,49,53,54,55,59,60,62,64,65,66)))
	// where "-NONE-" represents an invalid (uninitialized) XDoc ID.
	for(int i = 0; i < entities->getNEntities(); i++){
		transfer_to_best_xdoc_id(entities->getEntity(i), eid_to_xdoc_id_all, xdoc_id_all_to_eid, _eid_to_xdoc_ids, xdoc_id_to_eid_final, 
			doc_theory);
	}
	generate_entity_and_bound_uri_maps(doc_theory, eid_to_alignable_strings, multi_match_entities_ids);
}

void ElfMultiDoc::output_location_db_counts() {
	if (_location_db) {
		SessionLogger::info("location_db") << L"Sub-gpe relation helped:" << boost::lexical_cast<std::wstring>(_location_db->subgpe_good) << L"\n";
		SessionLogger::info("location_db")<< L"Sub-gpe relation didn't help:" << boost::lexical_cast<std::wstring>(_location_db->subgpe_bad) << L"\n";
		SessionLogger::info("location_db") << L"db lookup ambiguous:" << boost::lexical_cast<std::wstring>(_location_db->lookup_ambig) << L"\n";
		SessionLogger::info("location_db") << L"db lookup unambiguous:" << boost::lexical_cast<std::wstring>(_location_db->lookup_unambig) << L"\n";
		SessionLogger::info("location_db") << L"disambiguated by default:" << boost::lexical_cast<std::wstring>(_location_db->by_default) << L"\n";
		SessionLogger::info("location_db") << L"disambiguated by is_country:" << boost::lexical_cast<std::wstring>(_location_db->by_country) << L"\n";
		SessionLogger::info("location_db") << L"disambiguated by score:" << boost::lexical_cast<std::wstring>(_location_db->by_pop) << L"\n";
		SessionLogger::info("location_db") << L"total:" << boost::lexical_cast<std::wstring>(_location_db->total) << L"\n";
	} else {
		SessionLogger::info("no_loc_db") << "No location db was loaded.";
	}
}

void ElfMultiDoc::addGPERelationsToElfDoc(ElfDocument_ptr elfDoc, const DocTheory* doc_theory) {
	if (_has_location_db) {
		std::wstring dom_pref = ElfMultiDoc::get_ontology_domain()+L":";
		//transform GPEs with uri data to GPE-specs
		ElfIndividualSet individuals = elfDoc->get_individuals_by_type(dom_pref+L"HumanOrganization");
		BOOST_FOREACH(ElfIndividual_ptr ind,individuals) {
			if (ind->get_entity_id() != -1 && doc_theory->getEntitySet()->getEntity(ind->get_entity_id())->getType().matchesGPE() &&
				(_location_db->getEntityUri(ind->get_entity_id()) != L"")) {
					EDTOffset start;
					EDTOffset end;
					ind->get_spanning_offsets(start,end);
					ind->set_type(boost::make_shared<ElfType>(dom_pref+L"GPE-spec",ind->get_type()->get_string(),start,end));
					elfDoc->insert_individual(ind);
			}
		}
		//transform GPEs with uri data to GPE-specs
		individuals = elfDoc->get_individuals_by_type(dom_pref+L"GeopoliticalEntity");
		BOOST_FOREACH(ElfIndividual_ptr ind,individuals) {
			if (_location_db->getEntityUri(ind->get_entity_id()) != L"") {
				EDTOffset start;
				EDTOffset end;
				ind->get_spanning_offsets(start,end);
				ind->set_type(boost::make_shared<ElfType>(dom_pref+L"GPE-spec",ind->get_type()->get_string(),start,end));
				elfDoc->insert_individual(ind);
			}
		}
		//add gazetteer knowledge relations for all GPE-specs
		individuals = elfDoc->get_individuals_by_type(dom_pref+L"GPE-spec");
		BOOST_FOREACH(ElfIndividual_ptr ind,individuals) {
			std::set<ElfRelation_ptr> relations = _location_db->getURIGazetteerRelations(doc_theory, ind, dom_pref);
			if (!relations.empty()) {
				elfDoc->add_relations(relations);
			}
		}
	}
}

// called by generate_entity_and_id_maps(). 
void ElfMultiDoc::generate_entity_and_bound_uri_maps(const DocTheory* doc_theory, 
													const IntToSetOfStringsMap & eid_to_alignable_strings,
													const IntToSetOfStringsMap & multi_match_entities_ids) {
	const EntitySet* entities = doc_theory->getEntitySet();
	std::wstring docid = std::wstring(doc_theory->getDocument()->getName().to_string());
	//StringPairToSetOfStringsMap name_docid_to_bound_uris = r.first;
	//StringToSetOfStringsMap uris_to_types = r.second;
	IntToSetOfStringsMap eid_to_bound_uris_best;
	StringToSetOfIntsMap bound_uri_to_eids_best;
	IntToSetOfStringsMap eid_to_bound_uris_all;
	StringToSetOfIntsMap bound_uri_to_eids_all;
	//IntToSetOfStringsMap eid_to_bound_uris_final;
	StringToSetOfIntsMap bound_uri_to_eids_final;

	//handle GPEs
	std::set<int> leftover; //the bucket for GPEs not found in the gazetteer
	if (_has_location_db) {
		//get sub-GPE relations
		RelationSet* rel_set = doc_theory->getRelationSet();
		std::map<int,int> part_whole_map;
		std::map<int,int> whole_part_map;
		for (int i = 0; i < rel_set->getNRelations(); i++) {
			Relation* rel = rel_set->getRelation(i);
			if (rel->getType() == Symbol(L"PART-WHOLE.Geographical")) {
				part_whole_map[rel->getLeftEntityID()] = rel->getRightEntityID();
				whole_part_map[rel->getRightEntityID()] = rel->getLeftEntityID();
			}
		} 
		//first we load the gpes into the disambiguator
		_location_db->clear();
		for (int i=0;i<entities->getNEntities();i++){
			const Entity* entity = entities->getEntity(i);
			if (entity->getType().matchesGPE() || entity->getType().matchesLOC()) {
				if (part_whole_map.find(i) != part_whole_map.end()) {
					_location_db->addEntity(i,ElfMultiDoc::get_xdoc_entity_names(entity,doc_theory),
						ElfMultiDoc::get_xdoc_entity_names(entities->getEntity(part_whole_map[i]),doc_theory),
						entity->getNMentions(),true);
				} else if (whole_part_map.find(i) != whole_part_map.end()) {
					_location_db->addEntity(i,ElfMultiDoc::get_xdoc_entity_names(entity,doc_theory),
						ElfMultiDoc::get_xdoc_entity_names(entities->getEntity(whole_part_map[i]),doc_theory),
						entity->getNMentions(),false);
				} else {
					_location_db->addEntity(i,ElfMultiDoc::get_xdoc_entity_names(entity,doc_theory),
						std::set<std::wstring>(),entity->getNMentions());
				}
			}
		}
		//now disambiguate
		_location_db->disambiguate(entities);
		//now we can associate the correct URIs
		int n_gpe_added = 0;
		for (int i=0;i<entities->getNEntities();i++){
			const Entity* entity = entities->getEntity(i);
			if (entity->getType().matchesGPE() || entity->getType().matchesLOC()) {
				std::wstring uri = _location_db->getEntityUri(i);
				if (uri != L"") {
					add_to_bound_uri_tables(entity, ElfMultiDoc::get_ontology_domain()+L":"+uri, eid_to_bound_uris_best, bound_uri_to_eids_best);
					n_gpe_added++;
				}
				else {
					//move to standard mapping below
					leftover.insert(i);
				}
			}
		}
	}

	//URI mapping for non-GPEs
	for(int i = 0; i < entities->getNEntities(); i++){
		const Entity* entity = entities->getEntity(i);
		if ((!_has_location_db) || //if not using location database
				(!entity->getType().matchesGPE() && !entity->getType().matchesLOC()) || //or entity not a gpe or loc
				(leftover.find(i) != leftover.end())) { //or entity was not given a uri in previous phase

			name_mention_t best_name_mention = entity->getBestNameWithSourceMention(doc_theory);
			int n_added = add_bound_uris_for_entity_w_or_wo_docid(entity, best_name_mention.first, docid, eid_to_bound_uris_best, 
				bound_uri_to_eids_best, doc_theory);
			add_xdoc_bound_entities(entity, eid_to_bound_uris_all, bound_uri_to_eids_all, doc_theory,
				best_name_mention.first, n_added);
			IntToSetOfStringsMap::const_iterator iter = eid_to_alignable_strings.find(entity->getID());
			if(iter != eid_to_alignable_strings.end()){
				BOOST_FOREACH(std::wstring name, (*iter).second){
					n_added += add_bound_uris_for_entity_w_or_wo_docid(entity, name, docid, eid_to_bound_uris_all, bound_uri_to_eids_all, 
						doc_theory);
				}
			}
		}
	}
	
	for(int i = 0; i < entities->getNEntities(); i++){
		transfer_to_best_bound_uri(entities->getEntity(i), eid_to_bound_uris_best, bound_uri_to_eids_best, _eid_to_bound_uris, 
			bound_uri_to_eids_final, doc_theory);
	}
	for(int i = 0; i < entities->getNEntities(); i++){
		transfer_to_best_bound_uri(entities->getEntity(i), eid_to_bound_uris_all, bound_uri_to_eids_all, _eid_to_bound_uris, 
			bound_uri_to_eids_final, doc_theory);
	}
	for(int i = 0; i < entities->getNEntities(); i++){
		transfer_to_best_bound_uri(entities->getEntity(i), eid_to_bound_uris_best, bound_uri_to_eids_best, _eid_to_bound_uris_merged, 
			bound_uri_to_eids_final, doc_theory, true);
	}
	static const std::wstring WARN_MSG (L"\nWarning: ElfMultiDoc::generate_entity_and_bound_uri_maps(): "
		L"Entity links to multiple XDoc IDs; generate a local ID: \n\tXDOC: ");
	typedef std::pair<int, SetOfStrings> bfe_t;
	BOOST_FOREACH(bfe_t entry, multi_match_entities_ids){
		int i = entry.first;
		std::wstringstream id;
		id << "bbn:xdoc-m-" << docid << "-"<<entities->getEntity(i)->getID();
		SessionLogger::info("LEARNIT")<< WARN_MSG;
		BOOST_FOREACH(std::wstring s, entry.second){
			SessionLogger::info("LEARNIT")<<s<<", "<<std::endl;
		}		
		SessionLogger::info("LEARNIT")<<"--> "<<id.str()<<std::endl;
		_eid_to_bound_uris[i].clear();
		_eid_to_bound_uris[i].insert(id.str());
		_eid_to_xdoc_ids[i].clear();
		_eid_to_xdoc_ids[i].insert(id.str());
	}

	int n_orgs_matched = 0;
	int n_oth_matched = 0;
	for(int i = 0; i < entities->getNEntities(); i++){
		const Entity* entity = entities->getEntity(i);
		if(entity->getType().matchesPER())
			continue;
		if(_eid_to_bound_uris.find(entity->getID()) != _eid_to_bound_uris.end()){
			if(entity->getType().matchesORG())
				n_orgs_matched++;
			else
				n_oth_matched++;
		}
	}
	static bool warn_when_few_entities_produced = 
		ParamReader::getOptionalTrueFalseParamWithDefaultVal(
			"warn_when_few_entities_produced", /*defaultVal=*/ false);
	if(n_orgs_matched + n_oth_matched < 2 && warn_when_few_entities_produced){
		SessionLogger::info("LEARNIT")<<"\nWARNING: "
			<<doc_theory->getDocument()->getName()
			<<": #ORGS: "<<n_orgs_matched<<" #GPE: "<<n_oth_matched<<" TOTAL: "<<(n_orgs_matched + n_oth_matched )<<std::endl;
	}
	//unused_method(doc_theory, eid_to_alignable_strings);
	//return std::pair<IntToSetOfStringsMap, IntToSetOfStringsMap>(eid_to_xdoc_id_final, eid_to_bound_uri_final);
}

// called by generate_entity_and_id_maps()
void ElfMultiDoc::add_xdoc_bound_entities(const Entity *entity, IntToSetOfStringsMap & eid_to_bound_uri_all, 
										  StringToSetOfIntsMap & bound_uri_to_eid_all, 
										  const DocTheory* doc_theory, 
										  const std::wstring & best_name_mention_first, int & n_added) {
	if(_eid_to_xdoc_ids.find(entity->getID()) == _eid_to_xdoc_ids.end()){
		return;
	}
	std::vector<std::wstring> other_names;
	std::set<XDocIdType> ids = (*_eid_to_xdoc_ids.find(entity->getID())).second;
	XDocIdType xdoc_id = (*ids.begin());
	EntityTypeAndXdocIdPairToNamesMap::iterator cluster_i = ElfMultiDoc::_entity_type_and_xdoc_id_to_names.find(
		EntityTypeAndXdocIdPairToNamesMap::key_type(entity->getType().getName(), xdoc_id.as_string()));
    // typical value of cluster_i: (FAC, "bbn:xdoc-730") -> [1]("al anad")
    // typical value of best_name_mention_first: "Al - Anad"
	other_names.insert(other_names.end(), cluster_i->second.begin(), cluster_i->second.end());
	other_names.push_back(best_name_mention_first);
	std::wstring docid = doc_theory->getDocument()->getName().to_string();
	BOOST_FOREACH(std::wstring name, other_names){
		n_added += add_bound_uris_for_entity_w_or_wo_docid(entity, name, docid, eid_to_bound_uri_all, bound_uri_to_eid_all, 
			doc_theory);
	}

	if (ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_mr_xdoc_backoff_bound_uris", false)) {
		// if a mapping exists in the MR xdoc problem file, use it
        // "X2B" = XDoc ID to bound URI
		X2BMap::const_iterator probe = _xdoc_id_to_bound_uri.find(xdoc_id);
		if (probe != _xdoc_id_to_bound_uri.end()) {
			std::wstring bound_uri = probe->second;
			add_to_bound_uri_tables(entity, bound_uri, eid_to_bound_uri_all, bound_uri_to_eid_all);
		}
	}
}

/**
* This is for getting the different forms of cross document mentions for GPE URI binding
*/
std::set<std::wstring> ElfMultiDoc::get_xdoc_entity_names(const Entity *entity, const DocTheory* doc_theory) {
	std::set<std::wstring> result;	
	if(_eid_to_xdoc_ids.find(entity->getID()) != _eid_to_xdoc_ids.end()){
		std::vector<std::wstring> other_names;
		std::set<XDocIdType> ids = (*_eid_to_xdoc_ids.find(entity->getID())).second;
		XDocIdType xdoc_id = (*ids.begin());
		EntityTypeAndXdocIdPairToNamesMap::iterator cluster_i = ElfMultiDoc::_entity_type_and_xdoc_id_to_names.find(
		EntityTypeAndXdocIdPairToNamesMap::key_type(entity->getType().getName(), xdoc_id.as_string()));
		// typical value of cluster_i: (FAC, "bbn:xdoc-730")->[1]("al anad")
		// typical value of best_name_mention_first: "Al -Anad"
		BOOST_FOREACH(std::wstring name, cluster_i->second){
			result.insert(UnicodeUtil::normalizeTextString(name));
		}
	} 

	//always add best mention name
	name_mention_t best_name_mention = entity->getBestNameWithSourceMention(doc_theory);
	std::wstring name = UnicodeUtil::normalizeTextString(best_name_mention.first);

	//hack for combined korea in DB
	if (boost::ends_with(name, L" korea"))
		name = L"korea";

	//don't insert junk best name mentions
	if (name != L"")
		result.insert(name);

	return result;
}

/**
 * Given an ACE entity type and a name string, returns an XDoc (cluster) ID.
 * @param entity_type ACE entity type (e.g., "PER", "ORG")
 * @param name_string Name (which will be normalized internally to the method)
 * @return XDoc (cluster) ID
 * @example get_xdoc_id("GPE", "aden") -> "bbn:xdoc-517"
 **/
XDocIdType ElfMultiDoc::get_xdoc_id(const Symbol & entity_type, const std::wstring & name_string) {
    XDocIdType ret;
	// Make sure the string to cluster map is initialized
	if (!ElfMultiDoc::_initialized_entity_type_and_name_to_xdoc_id)
		throw std::runtime_error("ElfMultiDoc::get_xdoc_id(Symbol, std::wstring): "
		"static method called before table is initialized");

	// Normalize the name string
	std::wstring normalized_string = UnicodeUtil::normalizeTextString(name_string);

	// Find a matching XDoc ID for this type and name, if any
	EntityTypeAndNamePairToXDocIdMap::iterator cluster_i = ElfMultiDoc::_entity_type_and_name_to_xdoc_id.find(
		EntityTypeAndNamePairToXDocIdMap::key_type(entity_type, normalized_string));
	if (cluster_i != ElfMultiDoc::_entity_type_and_name_to_xdoc_id.end()) {
		//SessionLogger::info("LEARNIT") << "Mapped <" << entity_type << ", " << name_string << "> "
		// "to <" << cluster_i->second << ">." << std::endl;
		if ((cluster_i->second).is_valid()) {
			ret = cluster_i->second;
		}
	}
	return ret;
}

/**
 * Looks for an XDoc ID by name string
 * (which will be normalized), trying each ACE type.
 *
 * Only allow one ACE type to connect. This prevents conflating, for example:
 *   0	GPE	american, usa, america, u s, us, united states, 
 *   20	ORG	american, u s, us, united states, department of defense, us department of defense, state, state department, 
 *
 * @param name_string The unnormalized mention name.
 * @param matched_entity_type The entity type that matched. Pass-by-reference.
 * @return The cross-document cluster URI.
 *
 * @author nward@bbn.com
 * @date 2010.08.29
 **/
XDocIdType ElfMultiDoc::get_xdoc_id_and_entity_type(const std::wstring & name_string, Symbol& matched_entity_type) {
	XDocIdType best_xdoc_id;
	// Iterate over all the ACE entity types
	static Symbol entity_types[5] = {Symbol(L"GPE"), Symbol(L"ORG"), Symbol(L"PER"), Symbol(L"LOC"), Symbol(L"FAC")};
	for (int i = 0; i < 5; i++) {
		best_xdoc_id = ElfMultiDoc::get_xdoc_id(entity_types[i], name_string);
		if (best_xdoc_id.is_valid()) {
			matched_entity_type = entity_types[i];
			return best_xdoc_id;
		}
	}

	// No match
	matched_entity_type = Symbol(L"NONE");
	return best_xdoc_id;
}

/**
 * Gets a cross-document URI, then uses it to find all
 * the other names associated with that cluster, so we can
 * do "expanded" lookups into ElfMultiDoc::_docid_type_to_srm.
 *
 * @param entity_type The ACE type symbol to look for.
 * @param name_string The unnormalized mention name.
 * @return The cross-document cluster URI, if there is one. Otherwise, the input name (which can be quite long).
 * @example get_other_names("NONE", "Southern Yemeni troops fought...") -> [1]"Southern Yemeni troops fought..."
 *             
 *
 * @author nward@bbn.com
 * @date 2010.08.29
 **/
std::vector<std::wstring> ElfMultiDoc::get_other_names(const Symbol & entity_type, const std::wstring & name_string) {
	// Try to get a xdoc ID
	std::vector<std::wstring> other_names;
	XDocIdType xdoc_id = get_xdoc_id(entity_type, name_string);

	// If we didn't get one, just return the input name
	if (!xdoc_id.is_valid()) {
		other_names.push_back(name_string);
		return other_names;
	} else {
		// Look for other names in this cluster
		EntityTypeAndXdocIdPairToNamesMap::iterator cluster_i 
			= ElfMultiDoc::_entity_type_and_xdoc_id_to_names.find(EntityTypeAndXdocIdPairToNamesMap::key_type(entity_type, xdoc_id.as_string()));
		if (cluster_i != ElfMultiDoc::_entity_type_and_xdoc_id_to_names.end()) {
			other_names.insert(other_names.end(), cluster_i->second.begin(), cluster_i->second.end());
		} else {
			other_names.push_back(name_string);
		}
	}

	// Done
	return other_names;
}

/** 
 * Reads either (a) three-column lines (type, name, URI) such as the following: 
 *		nfl:NFLTeam	Arizona Cardinals	nfl:ArizonaCardinals
 *		nfl:NFLTeam	Cardinals	nfl:ArizonaCardinals
 * in which case the docid is assumed to be the special string L"ALL",
 * or (b) four-column lines, where the first column is an actual docid.
 * For each line, produces a mapping from a (docid, type) pair to a triplet of (name, URI) pairs,
 * used to map from original, uppercased, and lowercased versions of the name to a URI. (If, e.g.,
 * original and lowercased versions of the name are identical, only two mappings will be produced.)
 * Example of mappings produced from the lines above:
 * ("ALL", nfl:NFLTeam) -> { ("Arizona Cardinals", "nfl:ArizonaCardinals",
 *                           ("arizona cardinals", "nfl:ArizonaCardinals",
 *							  "ARIZONA CARDINALS", "nfl:ArizonaCardinals")}
 * ("ALL", nfl:NFLTeam) -> { ("Cardinals", "nfl:ArizonaCardinals",
 *                           ("cardinals", "nfl:ArizonaCardinals",
 *							  "CARDINALS", "nfl:ArizonaCardinals")}
 * 
 * @param path Path to an arg map (which may contain a bogus line or have substantive content).
 **/
void ElfMultiDoc::init_type_w_or_wo_docid_to_srm_map(const std::string & path) {
	if (path == "")	{
		std::ostringstream out;
		out << "Argument map was not specified but is mandatory";
		throw std::runtime_error(out.str());
	}
	else if (!boost::filesystem::exists(path)) {
		std::ostringstream out;
		out << "Argument map (" << path << ") doesn't exist";
		throw std::runtime_error(out.str());
	}

	SessionLogger::info("read_argm_0") << "Reading argument map " << path << "..." << std::endl;
	std::wstring line;
	std::wifstream arg_map_wifstream(path.c_str());
	int lines_inserted(0);
	while (arg_map_wifstream.is_open() && getline(arg_map_wifstream, line)) {
		// Read the tab-delimited table row (where initial hash mark comments out a line)
		boost::algorithm::trim(line);
		if (!line.empty() && line[0] != L'#') {
			// Split the line
			std::vector<std::wstring> line_tokens;
			boost::algorithm::split(line_tokens, line, boost::algorithm::is_from_range(L'\t', L'\t'));
			// Check whether this list entry specifies by docid or not
			if (line_tokens.size() == 4) {
				process_srm_map_line_w_docid(line_tokens);
				++lines_inserted;
			} else if (line_tokens.size() == 3) {
				process_srm_map_line_wo_docid(line_tokens);
				++lines_inserted;
			} else {
				SessionLogger::info("LEARNIT") << "Bad argument map line: '" << line << "'" << std::endl;
				continue;
			}
		}
	}

	if (lines_inserted == 0) {
		std::ostringstream out;
		out << "Argument map (" << path << ") contains no valid lines";
		throw std::runtime_error(out.str());
	}
}

void ElfMultiDoc::load_title_bound_uris(const std::string& path) {
	if (path == "") {
		SessionLogger::warn("title_bound_uris") << L"ElfMultiDoc::load_title_bound_uris"
			<<L" called with null string.";
	} else if (!boost::filesystem::exists(path)) {
		std::ostringstream out;
		out << "Specified bound title URIs file " << path << L" doesn't exist.";
		throw UnexpectedInputException("ElfMultiDoc::load_title_bound_uris", out.str().c_str());
	}

	SessionLogger::info("title_bound_uris") << "Reading title bound URIs file "
		<< path << "...";

	std::wstring line;
	std::wifstream inp(path.c_str());
	int lines_inserted = 0;
	while (inp.is_open() && getline(inp, line)) {
		boost::algorithm::trim(line);
		if (!line.empty() && line[0] !=L'#') {
			std::vector<std::wstring> line_tokens;
			boost::algorithm::split(line_tokens, line, boost::algorithm::is_from_range(L'\t', L'\t'));
			if (line_tokens.size() == 3) {
				process_title_bound_uri_line(line_tokens);
				++lines_inserted;
			} else {
				SessionLogger::info("title_bound_uris") 
					<< L"Bad title bound URIs line: '"  << line << L"'";
			}
		}
	}

	if (lines_inserted == 0) {
		std::ostringstream out;
		out << "Title bound URI file " << path << " contains no valid lines.";
		throw UnexpectedInputException("ElfMultiDoc::load_title_bound_uris", out.str().c_str());
	}

	SessionLogger::info("title_bound_uris")
		<< L"Loaded the following title bound uri map:";
	BOOST_FOREACH(const BoundURIMapping& mp, _title_bound_uris) {
		SessionLogger::info("title_bound_uris")
			<< L"\t" << mp.str << L"\t(" << mp.uriType.type << L", " << mp.uriType.uri << L")";
	}
}

void ElfMultiDoc::process_title_bound_uri_line(const std::vector<std::wstring>& tokens) {
	std::wstring type = tokens[0];
	std::wstring str = tokens[1];
	std::wstring boundID = tokens[2];

	boost::algorithm::trim(type);
	boost::algorithm::trim(str);
	boost::algorithm::trim(boundID);

	_title_bound_uris.push_back(BoundURIMapping(type, str, boundID));
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	_title_bound_uris.push_back(BoundURIMapping(type, str, boundID));

	int start_idx = 0;
	if (str.find(L"a ") == 0) {
		start_idx = 2;
	} else if (str.find(L"an ") == 0) {
		start_idx = 3;
	} else if (str.find(L"the ") == 0) {
		start_idx = 4;
	}

	if (start_idx > 0) {
		str = str.substr(start_idx);
		_title_bound_uris.push_back(BoundURIMapping(type, str, boundID));
	}

	// crude stemming
	if (str.length() > 3) {
		if (str[str.length()-1] == L's') {
			str = str.substr(0, str.length()-1);
			_title_bound_uris.push_back(BoundURIMapping(type, str, boundID));
			if (str[str.length()-1] == L'e') {
				str = str.substr(0, str.length()-1);
				_title_bound_uris.push_back(BoundURIMapping(type, str, boundID));
			}
		}
	}
}

ElfMultiDoc::BoundTypeURIs ElfMultiDoc::title_bound_uri(const std::wstring& curType,
		const std::wstring& origLookup)
{
	std::wstring lookup = origLookup;
	BoundTypeURIs ret = title_bound_uri_internal(curType, lookup);
	if (ret.empty()) {
		if (lookup.length() > 1) {
			if (lookup[lookup.length()-1] == L's') {
				lookup = lookup.substr(0, lookup.length() -1);
				ret = title_bound_uri_internal(curType, lookup);
				if (ret.empty()) {
					if (lookup.length() > 1 && lookup[lookup.length()-1] == L'e') {
						lookup = lookup.substr(0, lookup.length() -1);
						ret = title_bound_uri_internal(curType,lookup);
					}
				}
			}
		}
	}

	return ret;
}

ElfMultiDoc::BoundTypeURIs ElfMultiDoc::title_bound_uri_internal(const std::wstring& curType,
		const std::wstring& lookup) 
{
	BoundTypeURIs ret;

	// if we don't have any subtype information, take everything which see
	// which is a string match
	if (curType == L"kbp:Title") {
		BOOST_FOREACH(BoundURIMapping& mapping, _title_bound_uris) {
			if (mapping.str == lookup) {
				ret.push_back(mapping.uriType);
			}
		}
	} else {
		// otherwise, prefer things which match by string and type
		BOOST_FOREACH(BoundURIMapping& mapping, _title_bound_uris) {
			if (mapping.str == lookup && mapping.uriType.type == curType) {
				ret.push_back(mapping.uriType);
				break;
			}
		}

		// but failing that, we'll take a string match with no subtype
		if (ret.empty()) {
			BOOST_FOREACH(BoundURIMapping& mapping, _title_bound_uris) {
				if (mapping.str == lookup && mapping.uriType.type == L"kbp:Title") {
					ret.push_back(mapping.uriType);
					break;
				}
			}
		}
	}

	return ret;
}

void ElfMultiDoc::process_srm_map_line_w_docid(const std::vector<std::wstring> & line_tokens) {
	// Get the docid, type, name, and uri tokens
	DocIdAndTypeToSRMMap::key_type key = DocIdAndTypeToSRMMap::key_type(line_tokens[0], line_tokens[1]);
	std::wstring name = line_tokens[2];
	std::wstring uri = line_tokens[3];
	// Get the value string replacement map, or create an empty map
	std::pair<DocIdAndTypeToSRMMap::iterator, bool> type_insert = _docid_type_to_srm.insert(
		DocIdAndTypeToSRMMap::value_type(key, StringReplacementMap()));

	// Add this value replacement for this type, with the original name and lowercased/uppercased versions
	type_insert.first->second.insert(StringReplacementMap::value_type(name, uri));
	type_insert.first->second.insert(StringReplacementMap::value_type(boost::to_lower_copy(name), uri));
	type_insert.first->second.insert(StringReplacementMap::value_type(boost::to_upper_copy(name), uri));
}

void ElfMultiDoc::process_srm_map_line_wo_docid(const std::vector<std::wstring> & line_tokens) {
	// Get the type, name, and uri tokens (no docid)
	TypeToSRMMap::key_type key = TypeToSRMMap::key_type(line_tokens[0]);
	std::wstring name = line_tokens[1];
	std::wstring uri = line_tokens[2];
	// Get the value string replacement map, or create an empty map
	std::pair<TypeToSRMMap::iterator, bool> type_insert = _type_to_srm.insert(
		TypeToSRMMap::value_type(key, StringReplacementMap()));

	// Add this value replacement for this type, with the original name and lowercased/uppercased versions
	type_insert.first->second.insert(StringReplacementMap::value_type(name, uri));
	type_insert.first->second.insert(StringReplacementMap::value_type(boost::to_lower_copy(name), uri));
	type_insert.first->second.insert(StringReplacementMap::value_type(boost::to_upper_copy(name), uri));
}

void ElfMultiDoc::init_nfl_player_db(const std::string & path) {
	_nfl_player_db = boost::make_shared<PlayerDB>(path);
}

void ElfMultiDoc::init_location_db(const std::string & path) {
	if (path != "") {
		_has_location_db = true;
		_location_db = boost::make_shared<LocationDB>(path);
	} else {
		_has_location_db = false;
	}
}

PlayerResultMap ElfMultiDoc::get_teams_for_player(const std::wstring & player, int season) {
	return _nfl_player_db->get_teams_for_player(player, season);
}

std::set<XDocIdType> ElfMultiDoc::get_xdoc_ids_from_eid(int entity_id) {
	IntToSetOfXDocIdsMap::const_iterator iter = _eid_to_xdoc_ids.find(entity_id);
	if (iter == _eid_to_xdoc_ids.end()) {
		std::set<XDocIdType> empty_set;
		return empty_set;
	} else {
		return iter->second;
	}
}

SetOfStrings ElfMultiDoc::get_bound_uris_from_eid(int entity_id) {
	IntToSetOfStringsMap::const_iterator iter = _eid_to_bound_uris.find(entity_id);
	if (iter == _eid_to_bound_uris.end()) {
		SetOfStrings empty_set;
		return empty_set;
	} else {
		return iter->second;
	}
}


// Never used
//std::vector<std::wstring> ElfMultiDoc::get_strings_from_cluster(const EntityTypeAndXdocIdPairToNamesMap::key_type & cluster) {
//	EntityTypeAndXdocIdPairToNamesMap::const_iterator iter = _entity_type_and_xdoc_id_to_names.find(cluster);
//	if (iter == _entity_type_and_xdoc_id_to_names.end()) {
//		std::vector<std::wstring> empty_vec;
//		return empty_vec;
//	} else {
//		return iter->second;
//	}
//}

//called by get_mapped_arg_when_no_srm_entry_found()
/** 
 * "Greatest length" refers to the length (in number of words) of the mention head corresponding to an XDoc ID.
 **/
void ElfMultiDoc::collect_potential_xdoc_ids(const EntitySet * entity_set, const Entity * entity, const Symbol & entity_type, 
											 PotentialXdocIdToCountAndGreatestLengthMap & out_id_map) {
	for (int i = 0; i < entity->getNMentions(); i++) {
		entity->getMention(i);
		Mention* mention = entity_set->getMention(entity->getMention(i));
		if (mention->getMentionType() == Mention::NAME && mention->getHead() != NULL) {
			//use number of words for name length, (so that for people, we avoid one-word most-frequent names)
			//int name_length = mention->getHead()->toTextString().length();
			Symbol term_symbols[50];
			int name_length = mention->getHead()->getTerminalSymbols(term_symbols, 49);
			// Look up a xdoc ID for this name mention
			XDocIdType xdoc_id = ElfMultiDoc::get_xdoc_id(entity_type, mention->getHead()->toTextString());
			if (xdoc_id.is_valid()) {
				// Add an entry for this xdoc ID or increment the existing one
				std::pair<PotentialXdocIdToCountAndGreatestLengthMap::iterator, bool> potential_id_insert = out_id_map.insert(
					PotentialXdocIdToCountAndGreatestLengthMap::value_type(xdoc_id, std::pair<int, int>(1, name_length)));
				// The insert() call returns a pair<iterator,bool>. Its pair::first element is set to an iterator pointing 
				// to either the newly inserted element or to the element that already had its same value in the map. 
				// The pair::second element is set to true if a new element was inserted, false if an element with the same 
				// value existed.
				if (!potential_id_insert.second) {
					// We've seen this xdoc ID before, so increment the count and take the longer name length
					potential_id_insert.first->second.first++;
					if (potential_id_insert.first->second.second < name_length) {
						potential_id_insert.first->second.second = name_length;
					}
				}
			} else {
				SessionLogger::info("xdoc_id_fail") << "get_xdoc_id(" << entity_type << ", " << mention->getHead()->toTextString() 
					<< ") failed in collect_potential_xdoc_ids()." << std::endl;
			}
		}
	}
}

/** 
 * Finds the XDoc ID that is used most often and/or corresponds to the longest word sequence.
 **/
XDocIdType ElfMultiDoc::get_best_potential_xdoc_id(const Entity* entity, 
												   const PotentialXdocIdToCountAndGreatestLengthMap & potential_ids, 
												   const XDocIdType & best_xdoc_id_so_far) {
	// Determine which URI found from coreferent mentions we should use
	XDocIdType best_uri(best_xdoc_id_so_far);														 														 
	int max_count = 0;
	int max_count_size = 0;
	int longest_size = 0;
	int longest_count = 0;
	XDocIdType freq_uri, longest_uri;
	if (potential_ids.size() > 0) {
		// Find the potential ID mappings with the highest count and longest URI
		BOOST_FOREACH(PotentialXdocIdToCountAndGreatestLengthMap::value_type potential_id, potential_ids){
            // This code is not hit for the first batch of IC or NFL dry run.
			SessionLogger::info("potential_id_first") << "potential_id.first: <" << potential_id.first << ">" << std::endl;
			if (max_count <= potential_id.second.first) {
				max_count = potential_id.second.first;
				max_count_size = potential_id.second.second;
				freq_uri = potential_id.first;
			}
			if (longest_size <= potential_id.second.second) {
				longest_count = potential_id.second.first;
				longest_size = potential_id.second.second;
				longest_uri = potential_id.first;
			}
		}
		
		// Simple heuristic: if longest_count is within 2 of frequency of max_count, use longest_size; otherwise, use max_count.
		//                   For entities of type PER,  use the longest name (since shorter names are likely to be ambiguous)
		if (entity->getType().matchesPER()){
			//For people, avoid one word names
			if(max_count_size == longest_size){
				best_uri = freq_uri;
			} else {
				best_uri = longest_uri;
			}
		}
		else{
			if (longest_count >= max_count + 2) {
				best_uri = longest_uri;
			} else {
				best_uri = freq_uri; 
			}
		}
	}
	// Done; could still be invalid
	return best_uri;
}

/**
 * Create a new XDoc ID based on the original XDoc ID of the cluster, the split index
 * (a non-negative number, usually single-digit), and the maximum XDoc ID read in from
 * the file.
 * @param orig_xdoc_id Numeric representation of XDoc ID.
 * @param split_index A non-negative number, usually single-digit.
 * @return Numeric representation of new XDoc ID.
 **/
int ElfMultiDoc::new_xdoc_id_from_split_index(int orig_xdoc_id, int split_index) {
	int new_xdoc_id = ((_max_xdoc_id_in_file + 1) * (split_index + 1)) + orig_xdoc_id;
	// The advantage of the formula above (suggested by nward) is that if _max_xdoc_id_in_file is known, calculations 
	// can be performed on the output (new_xdoc_id) to determine the original XDoc ID and split index:
	//		orig_xdoc_id = new_xdoc_id % (_max_xdoc_id_in_file + 1)
	//		split_index = ((new_xdoc_id - orig_xdoc_id)/(_max_xdoc_id_in_file + 1)) - 1
	//SessionLogger::info("LEARNIT") << "new_xdoc_id: " << new_xdoc_id << " = ("
	//	<< _max_xdoc_id_in_file << " + 1) * (" << split_index << " + 1)) + " << orig_xdoc_id;
	return new_xdoc_id;
}

/**
 * Dumps all maps. Useful only when the eid (entity ID) maps are active (i.e., when a document is loaded).
 **/
void ElfMultiDoc::dump_all() {
	dump_non_eid_maps();
	dump_eid_maps();
}

/**
 * Dumps all eid (entity ID) maps. Useful only when a document is loaded.
 **/
void ElfMultiDoc::dump_eid_maps() {
	SessionLogger::info("LEARNIT") << "_eid_to_xdoc_ids:" << std::endl;
	BOOST_FOREACH(IntToSetOfXDocIdsMap::value_type val, _eid_to_xdoc_ids) {
		SessionLogger::info("LEARNIT") << val.first << " -> ";
		BOOST_FOREACH(XDocIdType xid, val.second) {
			SessionLogger::info("LEARNIT") << xid << ", ";
		}
		SessionLogger::info("LEARNIT") << std::endl;
	}

	SessionLogger::info("LEARNIT") << "_eid_to_bound_uris:" << std::endl;
	BOOST_FOREACH(IntToSetOfStringsMap::value_type val, _eid_to_bound_uris) {
		SessionLogger::info("LEARNIT") << val.first << " -> ";
		BOOST_FOREACH(std::wstring str, val.second) {
			SessionLogger::info("LEARNIT") << str << ", ";
		}
		SessionLogger::info("LEARNIT") << std::endl;
	}

	SessionLogger::info("LEARNIT") << "_eid_to_bound_uris_merged:" << std::endl;
	BOOST_FOREACH(IntToSetOfStringsMap::value_type val, _eid_to_bound_uris_merged) {
		SessionLogger::info("LEARNIT") << val.first << " -> ";
		BOOST_FOREACH(std::wstring str, val.second) {
			SessionLogger::info("LEARNIT") << str << ", ";
		}
		SessionLogger::info("LEARNIT") << std::endl;
	}
}

/** 
 * Dumps all non-entity-ID-specific maps (which are loaded at the beginning of a run over a corpus
 * and remain in memory until the run ends).
 **/
void ElfMultiDoc::dump_non_eid_maps() {
	SessionLogger::info("LEARNIT") << "_entity_type_and_name_to_xdoc_id:" << std::endl;
	BOOST_FOREACH(EntityTypeAndNamePairToXDocIdMap::value_type val, _entity_type_and_name_to_xdoc_id) {
		SessionLogger::info("LEARNIT") << val.first.first << ", " << val.first.second << " -> " << val.second << std::endl;
	}

	SessionLogger::info("LEARNIT") << "_entity_type_and_name_to_bound_uri:" << std::endl;
	BOOST_FOREACH(EntityTypeAndNamePairToBoundUriMap::value_type val, _entity_type_and_name_to_bound_uri) {
		SessionLogger::info("LEARNIT") << val.first.first << ", " << val.first.second << " -> " << val.second << std::endl;
	}

	SessionLogger::info("LEARNIT") << "_entity_type_and_xdoc_id_to_names:" << std::endl;
	BOOST_FOREACH(EntityTypeAndXdocIdPairToNamesMap::value_type val, _entity_type_and_xdoc_id_to_names) {
		SessionLogger::info("LEARNIT") << val.first.first << ", " << val.first.second << " -> ";
		BOOST_FOREACH(std::wstring name, val.second) {
			SessionLogger::info("LEARNIT") << name << ", ";
		}
		SessionLogger::info("LEARNIT") << std::endl;
	}

	SessionLogger::info("LEARNIT") << "_bound_uri_to_types:" << std::endl;
	BOOST_FOREACH(StringToSetOfStringsMap::value_type val, _bound_uri_to_types) {
		SessionLogger::info("LEARNIT") << val.first << " -> ";
		BOOST_FOREACH(std::wstring type, val.second) {
			SessionLogger::info("LEARNIT") << type << ", ";
		}
		SessionLogger::info("LEARNIT") << std::endl;
	}

	SessionLogger::info("LEARNIT") << "_xdoc_id_to_bound_uri:" << std::endl;
	BOOST_FOREACH(X2BMap::value_type val, _xdoc_id_to_bound_uri) {
		SessionLogger::info("LEARNIT") << val.first << " -> " << val.second << std::endl;
	}

	SessionLogger::info("LEARNIT") << "_docid_type_to_srm:" << std::endl;
	BOOST_FOREACH(DocIdAndTypeToSRMMap::value_type val, _docid_type_to_srm) {
		SessionLogger::info("LEARNIT") << val.first.first << ", " << val.first.second << " -> ";
		BOOST_FOREACH(StringReplacementMap::value_type val2, val.second) {
			SessionLogger::info("LEARNIT") << "(" << val2.first << ", " << val2.second << ") ";
		}
		SessionLogger::info("LEARNIT") << std::endl;
	}

	SessionLogger::info("LEARNIT") << "_type_to_srm:" << std::endl;
	BOOST_FOREACH(TypeToSRMMap::value_type val, _type_to_srm) {
		SessionLogger::info("LEARNIT") << val.first << " -> ";
		BOOST_FOREACH(StringReplacementMap::value_type val2, val.second) {
			SessionLogger::info("LEARNIT") << "(" << val2.first << ", " << val2.second << ") ";
		}
		SessionLogger::info("LEARNIT") << std::endl;
	}

	SessionLogger::info("LEARNIT") << "_name_docid_to_bound_uris:" << std::endl;
	BOOST_FOREACH(StringPairToSetOfStringsMap::value_type val, _name_docid_to_bound_uris) {
		SessionLogger::info("LEARNIT") << "(" << val.first.first << ", " << val.first.second << ") -> ";
		BOOST_FOREACH(std::wstring str, val.second) {
			SessionLogger::info("LEARNIT") << str << ", ";
		}
		SessionLogger::info("LEARNIT") << std::endl;
	}

	SessionLogger::info("LEARNIT") << "_name_to_bound_uris:" << std::endl;
	BOOST_FOREACH(StringToSetOfStringsMap::value_type val, _name_to_bound_uris) {
		SessionLogger::info("LEARNIT") << val.first << " -> ";
		BOOST_FOREACH(std::wstring str, val.second) {
			SessionLogger::info("LEARNIT") << str << ", ";
		}
		SessionLogger::info("LEARNIT") << std::endl;
	}
}

// This code was originally contained in generate_entity_and_id_maps(), but was commented out.
// Moved it here to make generate_entity_and_id_maps() easier to read.
//void ElfMultiDoc::unused_method(const DocTheory* doc_theory, const IntToSetOfStringsMap & eid_to_alignable_strings) {
//	const EntitySet* entities = doc_theory->getEntitySet();
//	for(int i = 0; i < entities->getNEntities(); i++){
//		const Entity* entity = entities->getEntity(i);
//		std::pair<std::wstring, const Mention*> best_name_mention = DistillUtilities::getBestNameWithSourceMention(doc_theory, entity);
//		if(best_name_mention.first == L"NO_NAME")
//			continue;
//		//if(_eid_to_bound_uris.find(entity->getID()) == _eid_to_bound_uris.end())
//		//	continue;
//		SessionLogger::info("LEARNIT")<<"Entity: "<<best_name_mention.first<<" "<<entity->getType().getName()<<", "<<entity->getID();
//		IntToSetOfStringsMap::const_iterator iter = eid_to_alignable_strings.find(entity->getID());
//		if(iter != eid_to_alignable_strings.end()){
//			SessionLogger::info("LEARNIT")<<"\n\t";
//			BOOST_FOREACH(std::wstring name, (*iter).second){
//				SessionLogger::info("LEARNIT")<<"("<<name<<"), ";
//			}
//		}
//		SessionLogger::info("LEARNIT")<<std::endl;
//		if(_eid_to_xdoc_ids.find(entity->getID()) != _eid_to_xdoc_ids.end()){
//			SetOfStrings ids = (*_eid_to_xdoc_ids.find(entity->getID())).second;
//			BOOST_FOREACH(std::wstring xdoc_id, ids){
//				std::vector<std::wstring> other_names;
//				EntityTypeAndXdocIdPairToNamesMap::iterator cluster_i = ElfMultiDoc::_entity_type_and_xdoc_id_to_names.find(
//					EntityTypeAndXdocIdPairToNamesMap::key_type(entity->getType().getName(), xdoc_id));
//				other_names.insert(other_names.end(), cluster_i->second.begin(), cluster_i->second.end());
//				SessionLogger::info("LEARNIT")<<"XDOCID: "<<xdoc_id<<"   ---> ";
//				BOOST_FOREACH(std::wstring n, other_names){
//					SessionLogger::info("LEARNIT")<<"("<<n<<"), ";
//				}
//				SessionLogger::info("LEARNIT")<<std::endl;
//			}
//		}
//		else{
//			SessionLogger::info("LEARNIT")<<"XDOCID: NONE"<<std::endl;
//		}
//		if(_eid_to_bound_uris.find(entity->getID()) != _eid_to_bound_uris.end()){
//			SetOfStrings ids = (*_eid_to_bound_uris.find(entity->getID())).second;
//			BOOST_FOREACH(std::wstring bound_uri, ids){
//				SessionLogger::info("LEARNIT")<<"BoundID: "<<bound_uri<<std::endl;
//			}
//		}
//		else{
//			SessionLogger::info("LEARNIT")<<"BOUNDID: NONE"<<std::endl;
//		}
//	}
//}

