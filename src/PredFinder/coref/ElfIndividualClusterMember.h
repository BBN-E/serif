/**
 * An ElfIndividual and the associated evidence we need
 * to perform coreference clustering.
 *
 * @file ElfIndividualClusterMember.h
 * @author nward@bbn.com
 * @date 2010.08.20
 **/

#pragma once

#include "PredFinder/elf/ElfIndividual.h"
#include "PredFinder/common/ContainerTypes.h"
#include <vector>
class ElfRelationArg;
BSP_DECLARE(ElfRelationArg);
// Forward declaration to use shared pointer for class in class
class ElfIndividualClusterMember;

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.08.20
 **/
BSP_DECLARE(ElfIndividualClusterMember);

/**
 * Convenience type representing a relation name and arg role name.
 *
 * @author nward@bbn.com
 * @date 2010.08.25
 **/
typedef std::pair<std::wstring, std::wstring> ElfRelationRole;

/**
 * Stores a list of ElfRelationArgs by relation name and role.
 *
 * @author nward@bbn.com
 * @date 2010.08.25
 **/
typedef std::map<ElfRelationRole, std::vector<ElfRelationArg_ptr> > ElfRelationRoleMap;

/**
 * Represents a collection of likely coreferent
 * ElfIndividuals; when necessary, can generate
 * a merged ElfIndividual representing the best
 * combination of its ElfIndividualClusterMembers.
 *
 * @author nward@bbn.com
 * @date 2010.08.20
 **/
class ElfIndividualClusterMember {
public:
	ElfIndividualClusterMember(const std::wstring & domain_prefix, const ElfIndividual_ptr individual, const ElfRelationMap & relations);

	/**
	 * Inlined accessor to the cluster member's underlying individual.
	 *
	 * @return The value of _individual.
	 *
	 * @author nward@bbn.com
	 * @date 2010.08.23
	 **/
	ElfIndividual_ptr get_individual(void) const { return _individual; }

	/**
	 * Inlined accessor to the cluster member's cached unique relation args.
	 *
	 * @return The value of _unique_relation_args.
	 *
	 * @author nward@bbn.com
	 * @date 2010.08.26
	 **/
	ElfRelationRoleMap get_unique_relation_args(void) const { return _unique_relation_args; }

	double calculate_similarity(const ElfIndividualClusterMember_ptr other) const;

private:
	static std::vector<std::set<ElfRelationRole> > overlapping_relation_roles;
	static void init_nfl_overlapping_relation_roles(void);
	static bool do_relation_roles_overlap(const ElfRelationRole & left_relation_role, const ElfRelationRole & right_relation_role);

	static std::map<ElfRelationRole, std::set<ElfRelationRole> > must_equal_relation_roles;
	static void init_nfl_must_equal_relation_roles(void);
	static bool should_relation_roles_be_equal(const ElfRelationRole & left_relation_role, const ElfRelationRole & right_relation_role);

	static std::map<ElfRelationRole, std::set<ElfRelationRole> > must_not_equal_relation_roles;
	static void init_nfl_must_not_equal_relation_roles(void);
	static bool should_relation_roles_be_not_equal(const ElfRelationRole & left_relation_role, const ElfRelationRole & right_relation_role);

	static std::map<ElfRelationRole, std::set<ElfRelationRole> > must_equal_if_equal_relation_roles;
	static void init_nfl_must_equal_if_equal_relation_roles(void);
	static std::set<ElfRelationRole> get_consequents_if_relations_equal(const ElfRelation_ptr left_relation, const ElfRelation_ptr right_relation);

	bool is_nfl() {return (_domain_prefix == L"nfl");}

	/**
	 * The prefix for the ontology domain, used to
	 * guide clustering.
	 **/
	std::wstring _domain_prefix;

	/**
	 * The individual that might be coreferent with
	 * other individuals.
	 **/
	ElfIndividual_ptr _individual;

	/**
	 * Relations associated with this individual,
	 * one kind of evidence used for clustering.
	 **/
	ElfRelationMap _relations;

	/**
	 * Convenience dictionary of the unique arguments
	 * used in _relations, organized by relation name
	 * and arg role.
	 **/
	ElfRelationRoleMap _unique_relation_args;

	/**
	 * Cached convenience count of the leaves of
	 * _unique_relation_args. Should not be modified
	 * independently of the map.
	 **/
	int _n_unique_relation_args;
};
