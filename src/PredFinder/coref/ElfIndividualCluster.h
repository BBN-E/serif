/**
 * A cluster of coreferent individuals.
 *
 * @file ElfIndividualCluster.h
 * @author nward@bbn.com
 * @date 2010.08.20
 **/

#pragma once

#include "ElfIndividualClusterMember.h"
#include "boost/enable_shared_from_this.hpp"

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.08.20
 **/
BSP_DECLARE(ElfIndividualCluster);

/**
 * Stores a count (probably used for max/min on container
 * size) by equivalent string set.
 *
 * @author nward@bbn.com
 * @date 2010.09.23
 **/
typedef std::map<std::set<std::wstring>, size_t> StringCountMap;

/**
 * Represents a collection of likely coreferent
 * ElfIndividuals; when necessary, can generate
 * a merged ElfIndividual representing the best
 * combination of its ElfIndividualClusterMembers.
 *
 * @author nward@bbn.com
 * @date 2010.08.20
 **/
class ElfIndividualCluster : public boost::enable_shared_from_this<ElfIndividualCluster> {
public:
	ElfIndividualCluster(std::wstring domain_prefix, ElfIndividualClusterMember_ptr initial_member) : _domain_prefix(domain_prefix) { _members.push_back(initial_member); _initial_uri = initial_member->get_individual()->get_best_uri(); };

	/**
	 * Inlined accessor to the cluster's initial ID.
	 *
	 * @return The value of _initial_id.
	 *
	 * @author nward@bbn.com
	 * @date 2010.12.10
	 **/
	std::wstring get_initial_uri(void) const { return _initial_uri; }

	/**
	 * Inlined accessor to the cluster's members.
	 *
	 * @return The value of _members.
	 *
	 * @author nward@bbn.com
	 * @date 2010.12.09
	 **/
	std::vector<ElfIndividualClusterMember_ptr> get_members(void) const { return _members; }

	double calculate_similarity(ElfIndividualCluster_ptr other);

	void combine_cluster(ElfIndividualCluster_ptr other);

	std::wstring get_generated_uri(const std::wstring & docid = L"") const;
	std::vector<ElfIndividual_ptr> get_member_individuals(void) const;
	ElfRelationArgMap get_unique_relation_args(void) const;

	bool has_type(const std::wstring & type_string) const;

	void get_spanning_offsets(EDTOffset& start, EDTOffset& end) const;

	bool operator<(const ElfIndividualCluster& other) const { return _initial_uri < other._initial_uri; }
	bool operator==(const ElfIndividualCluster& other) const { return _initial_uri == other._initial_uri; }

private:
	static StringCountMap max_distinct_role_values;
	static void init_max_distinct_role_values(void);
	bool would_exceed_max_distinct_role_values(ElfIndividualCluster_ptr other) const;

	/**
	 * The prefix for the ontology domain, used to
	 * guide clustering.
	 **/
	std::wstring _domain_prefix;

	/**
	 * The best URI of the first individual in the cluster,
	 * used internally for sorting/hashing. This works because
	 * the set of individual URIs is by definition unique within
	 * a document.
	 **/
	std::wstring _initial_uri;

	/**
	 * The current members of this cluster. Trying to add
	 * a new member may fail if it doesn't sufficiently align
	 * with the current members of a non-empty cluster.
	 **/
	std::vector<ElfIndividualClusterMember_ptr> _members;
};

size_t hash_value(ElfIndividualCluster_ptr const& cluster);

/**
 * std::set sorting for ElfIndividualClusters
 *
 * @author nward@bbn.com
 * @date 2010.12.09
 **/
class ElfIndividualClusterPtrSortCriterion {
public:
	bool operator() (const ElfIndividualCluster_ptr& p1, const ElfIndividualCluster_ptr& p2) const {
		return (*p1 < *p2);
	}
};

/**
 * Stores a set of clusters, sorted, during coref processing.
 *
 * @author nward@bbn.com
 * @date 2010.12.09
 **/
typedef std::set<ElfIndividualCluster_ptr, ElfIndividualClusterPtrSortCriterion> ElfIndividualClusterSortedSet;
