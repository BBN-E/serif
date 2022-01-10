/**
 * Factory class for ElfRelation.
 *
 * @file ElfRelation.h
 * @author afrankel@bbn.com
 * @date 2010.10.11
 **/

#pragma once

#include "ElfRelation.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/common/bsp_declare.h"

#include <set>

BSP_DECLARE(PatternSet);
BSP_DECLARE(ElfRelationArg);

/**
 * Class that provides methods for creating fake relations from LearnIt and manual 
 * distillation patterns without reading them from documents. These are used for
 * pattern validation against macro sets.
 *
 * @author afrankel@bbn.com
 * @date 2010.10.11
 **/
class ElfRelationFactory {
public:
    // RELATIONS FROM MANUAL PATTERNS
	static void fake_relations_from_man_pattern_set(const std::vector<PatternSet_ptr> & pattern_set_set, 
		ElfRelationMultisetSortedByName & output_relations, std::set<std::wstring> & tbd_pattern_set_names,	
		std::set<std::wstring> & tbd_predicates, bool dump = false);
	static void fake_args_from_man_pattern(const Symbol & pattern_set_name,
		const IDToPatternReturnVecSeqMap::value_type & id_vec_seq_pair,
		std::vector< std::vector< ElfRelationArg_ptr > > & args);
	static void add_arg_from_pattern_return(const PatternReturn_ptr pr_ptr, 
		std::set<std::wstring> & roles_already_found, std::vector< ElfRelationArg_ptr > & arg_vec);
	static bool find_tbd_patterns(const IDToPatternReturnVecSeqMap & ipr_vec_seq_map, 
	    std::set<std::wstring> & tbd_predicates, bool dump);
        
    // RELATIONS FROM LEARNIT PATTERNS
    static void fake_relations_from_li_pattern_set(const std::vector<LearnItPattern_ptr> & pattern_set,
        ElfRelationMultisetSortedByName & output_relations);
	static void fake_args_from_li_pattern(const LearnItPattern_ptr & li_pattern,
		std::vector< ElfRelationArg_ptr > & arg_vec);
	static void add_arg_from_slot_constraints(const SlotConstraints_ptr sc_ptr, 
		std::set<std::wstring> & roles_already_found, std::vector< ElfRelationArg_ptr > & arg_vec);

    // USED FOR RELATIONS FROM BOTH MANUAL AND LEARNIT PATTERNS
	static void add_relation_from_arg_vec(const std::vector<ElfRelationArg_ptr> & arg_vec, 
		const std::wstring & pattern_set_name, const std::wstring & top_level_id,
		ElfRelationMultisetSortedByName & output_relations);
    static void update_fake_value_from_type(const std::wstring & type, std::wstring & value);
        
    // MISCELLANEOUS
	static std::set<ElfRelation_ptr> from_elf_individual(const std::wstring& domain_prefix, const ElfIndividual_ptr individual);
	static ElfRelation_ptr from_elf_string(const std::wstring& domain_prefix, const ElfIndividual_ptr individual, const ElfString_ptr string);
private:
};
