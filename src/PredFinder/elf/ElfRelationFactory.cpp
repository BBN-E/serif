/**
 * Factory class for ElfRelation and its subclasses.
 *
 * @file ElfRelationFactory.cpp
 * @author afrankel@bbn.com
 * @date 2010.10.11
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "ElfRelationFactory.h"
#include "ElfDescriptor.h"
#include "Generic/patterns/TextPattern.h"
#include "Generic/patterns/PatternSet.h"
#include "LearnIt/LearnItPattern.h"
#include "LearnIt/SlotConstraints.h"
#include "PredFinder/inference/EITbdAdapter.h"
#include "LearnIt/Target.h"
#include "boost/make_shared.hpp"
#include "boost/algorithm/string/trim.hpp"
#include "boost/algorithm/string/split.hpp"
#pragma warning(push, 0)
#include "boost/regex.hpp"
#pragma warning(pop)
#include "boost/lexical_cast.hpp"
#include "boost/foreach.hpp"

using namespace std; 

// RELATIONS FROM MANUAL PATTERNS
/**
 * Static factory method that builds an ElfRelation_ptr multiset (with the relation name as the key)
 * from a PatternSet found using manual Distillation patterns. 
 * Args filled with fake but plausible values. Used only for pattern validation.
 *
 * @param pattern_set_set The set of PatternSet items containing
 * the pattern(s); may be from multiple pattern files (eru:attendedSchool, eru:employs, etc.).
 * @param output_relations A pointer to the multiset of ElfRelation pointers that will be
 * filled by this method.
 * @param tbd_predicates A set of the "output" tbd predicates (e.g., "eru:bombingAttackEvent") 
 * that can be created from PatternReturns found in the patterns (e.g., "bombing").
 * @param dump Whether to dump information (which is voluminous).
 *
 * @author afrankel@bbn.com
 * @date 2010.10.11
 **/
void ElfRelationFactory::fake_relations_from_man_pattern_set(
										  const std::vector<PatternSet_ptr> & pattern_set_set, 
										  ElfRelationMultisetSortedByName & output_relations,
										  std::set<std::wstring> & tbd_pattern_set_names,
										  std::set<std::wstring> & tbd_predicates,
										  bool dump /* = false */) {
	// Hierarchy:
	
	// Predicate pattern set (e.g., the set {eru:attendedSchool, eru:employs, ...}) (passed in as a param to this method)
	//		Predicate pattern (e.g., eru:attendedSchool)
	//				Top-level pattern, w/unique ID (e.g., graduated_from_vprop or attended_vprop)

	// An IDToPatternReturnVecMap maps an id to a vector of PatternReturn pointers. 
	// Each PatternReturn pointer corresponds to the information that will be stored in an arg.
    // In the pattern file, the info that goes into a PatternReturn is represented as an sexp
	// headed by the keyword "return". Example:
	//		(return (role eru:educationalInstitution) (type ic:EducationalInstitution))

	// Retrieve each predicate pattern in the pattern set. 
	bool verbose = ParamReader::getOptionalTrueFalseParamWithDefaultVal(
		"verbose_pattern_verification", /* defaultVal=*/ false);
	BOOST_FOREACH(PatternSet_ptr ps, pattern_set_set) {
		IDToPatternReturnVecSeqMap ipr_vec_seq_map;
		bool tbd_found(false);
		if (ps) {
			ps->getPatternReturns(ipr_vec_seq_map, dump);
			tbd_found = find_tbd_patterns(ipr_vec_seq_map, tbd_predicates, dump);
		}

		if (verbose) {
			SessionLogger::info("patt_set_name_0") << ps->getPatternSetName() << endl;
		}
		// Retrieve each top-level pattern in the predicate pattern.
		// For each top-level pattern, instantiate a relation and 
		// push it onto the end of output_relations (unless output_relations already contains it).

		if (tbd_found) {
			tbd_pattern_set_names.insert(ps->getPatternSetName().to_string());
		} else {
			BOOST_FOREACH(IDToPatternReturnVecSeqMap::value_type id_vec_seq_pair, ipr_vec_seq_map) {
				if (verbose) {
					SessionLogger::info("top_level_id_0") << "  " << id_vec_seq_pair.first << endl;
				}
				std::vector< std::vector< ElfRelationArg_ptr > > arg_vec_vec;
				fake_args_from_man_pattern(ps->getPatternSetName(), id_vec_seq_pair, arg_vec_vec);
				BOOST_FOREACH(std::vector< ElfRelationArg_ptr > arg_vec, arg_vec_vec) {
					add_relation_from_arg_vec(arg_vec, ps->getPatternSetName().to_string(), 
						id_vec_seq_pair.first.to_string(), output_relations);
				}
			}
		}
	}
	if (dump) {
		BOOST_FOREACH(ElfRelation_ptr ptr, output_relations) {
			std::ostringstream ostr;
			ptr->dump(ostr, /*indent=*/ 0);
			SessionLogger::info("fake_rel_dump_0") << ostr.str();
		}
		if (!tbd_pattern_set_names.empty()) {
			SessionLogger::info("fake_rel_tbd_1") << "'tbd' pattern set names: <" 
				<< boost::algorithm::join(tbd_pattern_set_names, ",") << ">\n";
		}
		if (!tbd_predicates.empty()) {
			SessionLogger::info("fake_rel_tbd_2") << "'tbd' predicates: <" 
				<< boost::algorithm::join(tbd_predicates, ",") << ">\n";
		}
	}
	return;
}

/**
 * Static factory method that builds a vector of ElfArg_ptr vectors
 * from an IDToPatternReturnVecMap::value_type 
 * (i.e., a mapping of a top-level pattern ID to a vector of PatternReturn ptrs)
 * found by using manual Distillation patterns. Args filled with fake but 
 * plausible values. Used only for pattern validation.
 *
 * @param pattern_set_name The ID of the predicate pattern (e.g., "eru:attendedSchool").
 * @param id_vec_seq_pair A mapping from the top-level pattern ID (e.g., "attended_vprop")
 * to a vector of PatternReturn pointers, each pointer containing the info to construct an arg.
 * @param args A pointer to the vector of vectors of ElfRelationArg pointers that will be
 * filled by this method.
 *
 * @author afrankel@bbn.com
 * @date 2011.03.17
 **/
void ElfRelationFactory::fake_args_from_man_pattern(
	const Symbol & pattern_set_name,
	const IDToPatternReturnVecSeqMap::value_type & id_vec_seq_pair,
	std::vector< std::vector< ElfRelationArg_ptr > > & args) {
	
	BOOST_FOREACH(PatternReturnVec vec, id_vec_seq_pair.second) {
		std::set<std::wstring> roles_already_found;
		std::vector< ElfRelationArg_ptr > arg_vec;
		BOOST_FOREACH(PatternReturn_ptr pr_ptr, vec) {
			add_arg_from_pattern_return(pr_ptr, roles_already_found, arg_vec);
		}
		if (!arg_vec.empty()) {
			args.push_back(arg_vec);
		}
	}
}

/**
 * Given a pattern return, extract the role. If the role is nonempty and is not
 * already contained in roles_already_found, also extract the type and value 
 * (for some types, filling in the value with a fake but plausible entry). Build
 * it into an arg, and push it back onto the end of arg_vec.
 *
 * @param pr_ptr The PatternReturn from which information is to be extracted.
 * @param roles_already_found A set of the roles already found.
 * @param arg_vec A vector of args, passed by reference, to which this method will append
 * a single arg if it has a different role from the ones already contained in the vector.
 *
 * @author afrankel@bbn.com
 **/
 void ElfRelationFactory::add_arg_from_pattern_return(const PatternReturn_ptr pr_ptr, 
													 std::set<std::wstring> & roles_already_found, 
													 std::vector< ElfRelationArg_ptr > & arg_vec) {
	// TODO: We may want to process <ElfIndividual>s here, too. For now, simply
	// reject every case where no role is found.
	std::wstring role = pr_ptr->getValue(L"role");
	if (role.empty()) {
		return;
	}
	if (roles_already_found.find(role) == roles_already_found.end()) {
		roles_already_found.insert(role);
		std::wstring value = pr_ptr->getValue(L"value");
		std::wstring type = pr_ptr->getValue(L"type");
        update_fake_value_from_type(type, value);
		ElfRelationArg_ptr arg = boost::make_shared<ElfRelationArg>(role, type, value);
		arg_vec.push_back(arg);
	} 
	// These messages were often bogus. For instance, sprop patterns
	// often generated these warnings, even when the patterns were legit.
	// If it turns out that we need the warnings in the future and have
	// some means for filtering out the legitimate instances, we could
	// reinstate this branch.
	//else {
		//SessionLogger::info("LEARNIT") << pattern_set_name << ": "
		//	  << "role <" << role << "> for "
		//	  << "<" << id_vec_seq_pair.first.to_string() << "> already found. "
		//	  << "This instance will be ignored."
		//	  << endl;
		;
	//}
}

// RELATIONS FROM LEARNIT PATTERNS
/**
 * Static factory method that builds an ElfRelation_ptr multiset (with the relation name as the key)
 * from a set of Pattern pointers found using LearnIt patterns. 
 * Some args are filled with fake but plausible values; most are left blank. Used only for pattern validation.
 *
 * @param pattern_set The set of Pattern items containing
 * the pattern(s); may be from multiple LearnIt databases (GPEHasLeader.db, hasChild.db, etc.).
 * @param output_relations A pointer to the multiset of ElfRelation pointers that will be
 * filled by this method.
 *
 * @author afrankel@bbn.com
 * @date 2011.07.07
 **/
void ElfRelationFactory::fake_relations_from_li_pattern_set(const std::vector<LearnItPattern_ptr> & pattern_set,
    ElfRelationMultisetSortedByName & output_relations) 
{
	bool verbose = ParamReader::getOptionalTrueFalseParamWithDefaultVal(
		"verbose_pattern_verification", /* defaultVal=*/ false);
	std::set<std::wstring> rel_names;
	BOOST_FOREACH(LearnItPattern_ptr pattern, pattern_set) {
        if (verbose) {
            SessionLogger::info("patt_name_1") << pattern->getTarget()->getName() << endl;
        }
        std::wstring rel_name = pattern->getTarget()->getELFOntologyType();
        std::vector< ElfRelationArg_ptr > arg_vec;

        if (rel_names.find(rel_name) == rel_names.end()) {
            fake_args_from_li_pattern(pattern, arg_vec);
            add_relation_from_arg_vec(arg_vec, pattern->getTarget()->getELFOntologyType(), 
				pattern->getTarget()->getName(), output_relations);
		} else {
			SessionLogger::info("patt_name_2") << "Relation <" << rel_name << "> already encountered.\n";
		}
    }
}

/**
 * Static factory method that builds a vector of ElfArg_ptrs
 * from a Pattern_ptr found by using LearnIt databases. Some args are filled with fake but 
 * plausible values while most are left blank. Used only for pattern validation.
 *
 * @param li_pattern A pointer to the LearnIt pattern.
 * @param arg_vec A vector of ElfRelationArg pointers that will be
 * filled by this method.
 *
 * @author afrankel@bbn.com
 * @date 2011.07.07
 **/
void ElfRelationFactory::fake_args_from_li_pattern(const LearnItPattern_ptr & li_pattern,
    std::vector< ElfRelationArg_ptr > & arg_vec) 
{
	Target_ptr tgt = li_pattern->getTarget();
    int num_slots(tgt->getNumSlots());
    std::set<std::wstring> roles_already_found;
    for (int slot_num = 0; slot_num < num_slots; ++slot_num) {
        SlotConstraints_ptr sc = tgt->getSlotConstraints(slot_num);
        add_arg_from_slot_constraints(sc, roles_already_found, arg_vec);
    }
}

/**
 * Given a SlotConstraints_ptr, extract the role. If the role is nonempty and is not
 * already contained in roles_already_found, also extract the type and value 
 * (for some types, filling in the value with a fake but plausible entry). Build
 * it into an arg, and push it back onto the end of arg_vec.
 *
 * @param sc_ptr The SlotConstraints_ptr from which information is to be extracted.
 * @param roles_already_found A set of the roles already found.
 * @param arg_vec A vector of args, passed by reference, to which this method will append
 * a single arg if it has a different role from the ones already contained in the vector.
 *
 * @author afrankel@bbn.com
 * @date 2011.07.07
 **/
void ElfRelationFactory::add_arg_from_slot_constraints(const SlotConstraints_ptr sc_ptr, 
    std::set<std::wstring> & roles_already_found, std::vector< ElfRelationArg_ptr > & arg_vec) 
{
	std::wstring role = sc_ptr->getELFRole();
	if (role.empty()) {
		return;
	}
	if (roles_already_found.find(role) == roles_already_found.end()) {
		roles_already_found.insert(role);
        std::wstring type = sc_ptr->getELFOntologyType();
        std::wstring value;
        update_fake_value_from_type(type, value);
		ElfRelationArg_ptr arg = boost::make_shared<ElfRelationArg>(role, type, value);
		arg_vec.push_back(arg);
    }    
}    

// USED FOR RELATIONS FROM BOTH MANUAL AND LEARNIT PATTERNS

/**
 * Given an arg_vec, produce a relation, and if output_relations does not already contain it, add it.
 *
 * @param arg_vec A vector of the args from which the relation will be constructed.
 * @param pattern_set_name Name of the pattern set (for filling in the src attribute).
 * @param top_level_id Top-level pattern ID (for filling in the src attribute); blank for LearnIt.
 * @param output_relations Passed by reference; may contain zero or more relations when passed in;
 * when returned, will contain one additional relation (if it does not match any in the original multiset).
 *
 * @author afrankel@bbn.com
 * @date 2011.07.06
 **/
void ElfRelationFactory::add_relation_from_arg_vec(const std::vector<ElfRelationArg_ptr> & arg_vec, 
												   const std::wstring & pattern_set_name,
												   const std::wstring & top_level_id,
												   ElfRelationMultisetSortedByName & output_relations) {
	typedef ElfRelationMultisetSortedByName::const_iterator RMSConstIter;
	typedef pair<RMSConstIter, RMSConstIter> RMSConstIterPair;
	static ElfRelation_name_less_than comp_obj;
	if (arg_vec.size() <= 1) { // if arg_vec.size == 1, we have an individual, not a relation
		return;
	}
	ElfRelation_ptr relation = boost::make_shared<ElfRelation> (pattern_set_name, arg_vec, L"", 
		EDTOffset(), EDTOffset(), Pattern::UNSPECIFIED_SCORE, Pattern::UNSPECIFIED_SCORE_GROUP);
	bool found_match(false);
	// If the relation is not already contained in *output_relations, copy it over.
	// If equal_range() produces a nonempty range, we found a match in terms of relation name, 
	// but we still perform offsetless_equals to look for a full match. This is quicker
	// than using offsetless_equals to do the sorting and matching in the first place.
	RMSConstIterPair p = equal_range(output_relations.begin(),	output_relations.end(), 
		relation, comp_obj);
	for (RMSConstIter iter = p.first; iter != p.second; ++iter) {
		if ((*iter)->offsetless_equals(relation)) {
			found_match = true;
			break;
		}
	}
	if (!found_match) {
		std::wstring src(L"bbn:checker-");
		src += pattern_set_name;
        src += L"-";
        src += top_level_id;
		relation->set_source(src);
		output_relations.insert(relation);
	}
}

/**
 * Assign some plausible values (which won't affect our validation routine).
 * @param type Input ELF ontology type.
 * @param value Output value (changed only for certain types).
 **/
void ElfRelationFactory::update_fake_value_from_type(const std::wstring & type, std::wstring & value) {
    if (type == L"xsd:int")
        value = L"0";
    else if (type == L"xsd:date")
        value = L"1969-12-31";
    else if (type == L"nfl:NFLTeam")
        value = L"nfl:MinnesotaVikings";
}
     

/**
 * Converts the specified ElfIndividual to its equivalent relations,
 * following the convention that <name> becomes ic:hasName, <desc>
 * becomes rdfs:label, and <type> becomes rdf:type.
 *
 * Intended for use during R-ELF macro conversion.
 *
 * @param individual The individual being converted.
 * @return The set of relations representing the individual's
 * various string contents.
 *
 * @author nward@bbn.com
 * @date 2010.10.26
 **/
std::set<ElfRelation_ptr> ElfRelationFactory::from_elf_individual(const std::wstring& domain_prefix, const ElfIndividual_ptr individual) {
	// The relations generated from this individual
	std::set<ElfRelation_ptr> relations;

	// The name/desc as a hasName/label, if any
	if (individual->get_name_or_desc().get() != NULL && individual->get_name_or_desc()->get_value() != L"") {
		relations.insert(from_elf_string(domain_prefix, individual, individual->get_name_or_desc()));
	}

	// Each type
	relations.insert(from_elf_string(domain_prefix, individual, individual->get_type()));

	// Done
	return relations;
}

/**
 * Converts the specified ElfString to its equivalent relations,
 * following the convention that <name> becomes ic:hasName, <desc>
 * becomes rdfs:label, and <type> becomes rdf:type.
 *
 * Intended for use during R-ELF macro conversion.
 *
 * @param individual The string's containing individual.
 * @param string The string being converted, with provenance
 * @return The relation asserting the specified string.
 *
 * @author nward@bbn.com
 * @date 2010.10.26
 **/
ElfRelation_ptr ElfRelationFactory::from_elf_string(const std::wstring& domain_prefix, const ElfIndividual_ptr individual, 
													const ElfString_ptr string) {
	// Determine a predicate name and types based on the string subtype
	std::wstring predicate = L"";
	std::wstring object_type = L"";
	switch (string->get_type()) {
	case ElfString::NAME:
		if (domain_prefix == L"ic")
			predicate = L"ic:hasName";
		else
			predicate = L"rdfs:label";
		object_type = L"xsd:string";
		break;
	case ElfString::DESC:
		predicate = L"rdfs:label";
		object_type = L"xsd:string";
		break;
	case ElfString::TYPE:
		predicate = L"rdf:type";
		break;
	}

	// Create the subject argument from the individual and associate the provenance
	std::vector<ElfRelationArg_ptr> args;
	args.push_back(boost::make_shared<ElfRelationArg>(L"rdf:subject", individual));

	// Create the object argument from the string
	args.push_back(boost::make_shared<ElfRelationArg>(L"rdf:object", object_type, string->get_value()));

	// Create the relation from the arguments
	//   No relation provenance, we associate that with the object arg
	ElfRelation_ptr relation = boost::make_shared<ElfRelation>(predicate, args, L"", EDTOffset(), EDTOffset(), Pattern::UNSPECIFIED_SCORE, Pattern::UNSPECIFIED_SCORE_GROUP);

	// Done
	return relation;
}

/**
 * Searches for "tbd" pattern returns (e.g., "attack", "bombing") that are not equal to "agent" or "patient". 
 * If any are found, fills tbd_predicates with the with-agent and without-agent equivalents for the input tbd strings.
 *
 * @param ipr_vec_seq_map The structure to be searched for "tbd" pattern returns. NOTE: This structure should
 * contain pattern returns from a single PatternSet, not multiple PatternSets! This is based on the assumption
 * that a single PatternSet is likely to contain either all "tbd" patterns (as does "eru:attacks") or none
 * (like "eru:attendedSchool").
 * @param tbd_predicates Will be filled with the with-agent and without-agent equivalents for the input tbd strings.
 * @param bool Indicates whether information should be logged.
 *
 * @return True if any "tbd" pattern returns were found (in which case ipr_vec_seq_map should not be used to 
 * construct fake relations), false otherwise.
 **/
 bool ElfRelationFactory::find_tbd_patterns(const IDToPatternReturnVecSeqMap & ipr_vec_seq_map, 
	 std::set<std::wstring> & tbd_predicates, bool dump) 
{
	bool tbd_found(false);													
	BOOST_FOREACH(IDToPatternReturnVecSeqMap::value_type id_vec_seq_pair, ipr_vec_seq_map) {
		std::wstring id = id_vec_seq_pair.first.to_string();
		BOOST_FOREACH(PatternReturnVec vec, id_vec_seq_pair.second) {
			BOOST_FOREACH(PatternReturn_ptr pr, vec) {
				std::wstring tbd_value = pr->getValue(L"tbd");
				if (!tbd_value.empty() && tbd_value != L"agent" && tbd_value != L"patient" && tbd_value != L"bad_date") {
					std::wstring tbd_output_w_name = EITbdAdapter::getEventName(tbd_value, /*has_agent=*/true);
					if (!tbd_output_w_name.empty()) {
						tbd_predicates.insert(tbd_output_w_name);
						if (dump) {
							SessionLogger::info("tbd_output_w_name_0") << "Inserted " << tbd_output_w_name;
						}
					} else if (EITbdAdapter::getEventRole(tbd_value).empty()){ //if it's a valid event role, we ignore it
						std::ostringstream ostr;
						ostr << "Pattern <" << id << "> has a 'tbd' value <" << tbd_value << ">, "
							<< "which is not recognized by EITbdAdapter::getEventName(..., true).";
						throw UnexpectedInputException("ElfRelationFactory::find_tbd_patterns()", ostr.str().c_str());
					}
					std::wstring tbd_output_wo_name = EITbdAdapter::getEventName(tbd_value, /*has_agent=*/false);
					if (!tbd_output_wo_name.empty()) {
						tbd_predicates.insert(tbd_output_wo_name);
						if (dump) {
							SessionLogger::info("tbd_output_wo_name_0") << "Inserted " << tbd_output_wo_name;
						}
					} else if (EITbdAdapter::getEventRole(tbd_value).empty()){ //if it's a valid event role, we ignore it
						std::ostringstream ostr;
						ostr << "Pattern <" << id << "> has a 'tbd' value <" << tbd_value << ">, "
							<< "which is not recognized by EITbdAdapter::getEventName(..., false).";
						throw UnexpectedInputException("ElfRelationFactory::find_tbd_patterns()", ostr.str().c_str());
					}
					tbd_found = true;
				}
			}
		}
	}
	return tbd_found;
}
