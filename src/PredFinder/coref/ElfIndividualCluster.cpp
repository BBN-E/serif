/**
 * A cluster of coreferent individuals.
 *
 * @file ElfIndividualCluster.cpp
 * @author nward@bbn.com
 * @date 2010.08.23
 **/

#include "Generic/common/leak_detection.h"
#include "PredFinder/elf/ElfIndividualFactory.h"
#include "PredFinder/elf/ElfRelationArg.h"
#include "ElfIndividualCluster.h"
#include "boost/foreach.hpp"

/**
 * Calculate a similarity score between two clusters by calculating
 * the average cluster member similarity for the best matching cluster
 * members. Empty clusters have similarity 0. If any member reports
 * a strong mismatch (-1.0), the clusters mismatch
 *
 * @param other The cluster to score against
 * @return An average similarity score between cluster member sets [0.0,1.0]
 *
 * @author nward@bbn.com
 * @date 2010.08.23
 **/
double ElfIndividualCluster::calculate_similarity(ElfIndividualCluster_ptr other) {
	// Make sure that combining these clusters wouldn't violate any constraints
	if (would_exceed_max_distinct_role_values(other))
		return -1.0;

	// Loop over the members of the other cluster
	double sum_score = 0.0;
	int n_members = 0;
	BOOST_FOREACH(ElfIndividualClusterMember_ptr other_member, other->_members) {
		// Loop over the members of this cluster, finding the best match
		double best_score = 0.0;
		BOOST_FOREACH(ElfIndividualClusterMember_ptr member, _members) {
			double score = member->calculate_similarity(other_member);
			if (score == -1.0)
				return -1.0;
			//SessionLogger::info("LEARNIT") << other_member->get_individual()->get_id() << "\t" << member->get_individual()->get_id() << "\t" << score << std::endl;
			if (score > best_score)
				best_score = score;
		}
		sum_score += best_score;
		n_members++;
	}

	// Calculate the average similarity over the other cluster's members
	if (n_members > 0)
		return sum_score/n_members;
	else
		return 0.0;
}

/**
 * Copies all of the members of another cluster into
 * this cluster. Since we're using shared pointers,
 * it's assumed that the other cluster will stop being
 * used at some point.
 *
 * @param other The cluster to copy members from.
 *
 * @author nward@bbn.com
 * @date 2010.08.23
 **/
void ElfIndividualCluster::combine_cluster(ElfIndividualCluster_ptr other) {
	// Just copy in all of the other cluster's members
	BOOST_FOREACH(ElfIndividualClusterMember_ptr other_member, other->_members) {
		_members.push_back(other_member);
	}
}

std::wstring ElfIndividualCluster::get_generated_uri(const std::wstring & docid) const {
	// If there's only one member, just return its best URI
	if (_members.size() == 1)
		return (*(_members.begin()))->get_individual()->get_best_uri(docid);

	// Collect all of the type evidence and hash the IDs for this individual
	boost::hash<std::wstring> id_hasher;
	size_t id_hash = 0;
	std::multiset<std::wstring> type_strings;
	BOOST_FOREACH(ElfIndividualClusterMember_ptr member, _members) {
		// Get the hash for this member's ID
		id_hash ^= id_hasher(member->get_individual()->get_best_uri(docid));

		// Accumulate the type URIs for each member's individual
		std::multiset<std::wstring> member_type_strings = member->get_individual()->get_type_strings();
		type_strings.insert(member_type_strings.begin(), member_type_strings.end());
	}

	// Determine the most common type of this cluster individual
	std::wstring most_common_type = L"";
	int most_common_type_count = 0;
	BOOST_FOREACH(std::wstring type_string, type_strings) {
		int type_count = type_strings.count(type_string);
		if (type_count > most_common_type_count) {
			most_common_type = type_string;
			most_common_type_count = type_count;
		}
	}

	// Strip any namespace prefix off of the type URI
	most_common_type = most_common_type.substr(most_common_type.find(':') + 1);

	// Generate an ID based on the document ID, member individual hash, and most common clustered individual type
	std::wstringstream generated_uri;
	generated_uri << "bbn:cluster-";
	if (docid != L"")
		generated_uri << docid << "-";
	if (most_common_type != L"")
		generated_uri << most_common_type << "-";
	generated_uri << id_hash;

	// Generate the individual
	return generated_uri.str();
}

/**
 * Accessor to collect all of the underlying individuals, one
 * from each cluster member.
 *
 * @return The individuals contained in this cluster's members.
 *
 * @author nward@bbn.com
 * @date 2010.08.23
 **/
std::vector<ElfIndividual_ptr> ElfIndividualCluster::get_member_individuals(void) const {
	// Loop over all the members, collecting each individual
	std::vector<ElfIndividual_ptr> member_individuals;
	BOOST_FOREACH(ElfIndividualClusterMember_ptr member, _members) {
		member_individuals.push_back(member->get_individual());
	}
	return member_individuals;
}

/**
 * For each member of this cluster, collects all of its
 * already uniquified relation arguments and returns them
 * in an ElfRelationArgMap (keyed by argument role).
 *
 * @return The map of argument roles to lists of unique arguments.
 *
 * @author nward@bbn.com
 * @date 2010.08.23
 **/
ElfRelationArgMap ElfIndividualCluster::get_unique_relation_args(void) const {
	// Collect args from cluster members by role
	ElfRelationArgMap unique_relation_args;
	BOOST_FOREACH(ElfIndividualClusterMember_ptr member, _members) {
		ElfRelationRoleMap member_unique_relation_args_by_role = member->get_unique_relation_args();
		BOOST_FOREACH(ElfRelationRoleMap::value_type member_unique_relation_args, member_unique_relation_args_by_role) {
			BOOST_FOREACH(ElfRelationArg_ptr member_unique_relation_arg, member_unique_relation_args.second) {
				// Make sure we haven't seen this arg already
				ElfRelationArgMap::iterator seen_relation_arg_i = unique_relation_args.find(member_unique_relation_args.first.second);
				if (seen_relation_arg_i != unique_relation_args.end()) {
					bool seen = false;
					BOOST_FOREACH(ElfRelationArg_ptr unique_relation_arg, seen_relation_arg_i->second) {
						if (unique_relation_arg->offsetless_equals(member_unique_relation_arg)) {
							seen = true;
							break;
						}
					}
					if (seen) {
						continue;
					}
				}

				// Add this unique arg by role
				std::pair<ElfRelationArgMap::iterator, bool> unique_relation_arg_insert = unique_relation_args.insert(ElfRelationArgMap::value_type(member_unique_relation_args.first.second, std::vector<ElfRelationArg_ptr>()));
				unique_relation_arg_insert.first->second.push_back(member_unique_relation_arg);
			}
		}
	}

	// Done
	return unique_relation_args;
}

/**
 * Checks each of this cluster's members individuals to
 * see if they have the specified type.
 *
 * @param type_string The type URI to search for.
 * @return True if the type is found on at least one individual.
 *
 * @author nward@bbn.com
 * @date 2011.06.08
 **/
bool ElfIndividualCluster::has_type(const std::wstring & type_string) const {
	// Check each cluster member
	BOOST_FOREACH(ElfIndividualClusterMember_ptr member, _members) {
		if (member->get_individual()->has_type(type_string))
			return true;
	}

	// Not found
	return false;
}

/**
 * Finds the lowest start offset and highest
 * end offset of any relation arg contained
 * in one of this cluster's members.
 *
 * @param start The lowest ElfRelationArg _start. Pass-by-reference.
 * @param end The highest ElfRelationArg _end. Pass-by-reference.
 *
 * @author nward@bbn.com
 * @date 2010.08.27
 **/
void ElfIndividualCluster::get_spanning_offsets(EDTOffset& start, EDTOffset& end) const {
	// Loop over all of the members' relations' args and individuals
	start = EDTOffset(INT_MAX);
	end = EDTOffset(0);
	BOOST_FOREACH(ElfIndividualClusterMember_ptr member, _members) {
		// Loop over all of the cached unique relation args
		BOOST_FOREACH(ElfRelationRoleMap::value_type args, member->get_unique_relation_args()) {
			BOOST_FOREACH(ElfRelationArg_ptr arg, args.second) {
				EDTOffset arg_start = arg->get_start();
				if (arg_start.is_defined() && arg_start < start)
					start = arg_start;
				EDTOffset arg_end = arg->get_end();
				if (arg_end.is_defined() && arg_end > end)
					end = arg_end;
			}
		}

		// Grab offsets from the individual's type assertions
		ElfIndividual_ptr individual = member->get_individual();
		if (individual.get() != NULL) {
			EDTOffset individual_start, individual_end;
			individual->get_spanning_offsets(individual_start, individual_end);
			if (individual_start.is_defined() && individual_start < start)
				start = individual_start;
			if (individual_end.is_defined() && individual_end > end)
				end = individual_end;
		}
	}
}

/**
 * Stores maximum distinct values per cluster by role.
 * Initialized once when first queried.
 *
 * @author nward@bbn.com
 * @date 2010.09.23
 **/
StringCountMap ElfIndividualCluster::max_distinct_role_values;

/**
 * Loads the static max_distinct_role_values table
 * using hard-coded logic based on world knowledge.
 *
 * @author nward@bbn.com
 * @date 2010.09.23
 **/
void ElfIndividualCluster::init_max_distinct_role_values(void) {
	// No more than two teams per game
	//   Some LearnIt patterns use more specific roles for their team-containing args
	std::set<std::wstring> team_role_set;
	team_role_set.insert(L"eru:NFLTeam");
	team_role_set.insert(L"eru:gameWinner");
	team_role_set.insert(L"eru:gameLoser");
	team_role_set.insert(L"eru:teamScoring");
	team_role_set.insert(L"eru:teamInGame1");
	team_role_set.insert(L"eru:teamInGame2");
	max_distinct_role_values.insert(StringCountMap::value_type(team_role_set, 2));
}

/**
 * Checks the contents of max_distinct_role_values to
 * see if the specified cluster, when combined with this one, would
 * exceed any of the maxima.
 *
 * @author nward@bbn.com
 * @date 2010.09.23
 **/
bool ElfIndividualCluster::would_exceed_max_distinct_role_values(ElfIndividualCluster_ptr other) const {
	// Make sure the lmits are initialized
	if (max_distinct_role_values.size() == 0)
		init_max_distinct_role_values();

	// Get the unique arguments from this and the other cluster
	ElfRelationArgMap unique_relation_args = get_unique_relation_args();
	ElfRelationArgMap other_unique_relation_args = other->get_unique_relation_args();

	// Loop through the distinctness constraints
	BOOST_FOREACH(StringCountMap::value_type role_maximum, max_distinct_role_values) {
		// Collect all of the distinct values
		std::set<std::wstring> distinct_values;
		BOOST_FOREACH(std::wstring role, role_maximum.first) {
			// Get the unique args that have this role
			ElfRelationArgMap::iterator unique_relation_args_i = unique_relation_args.find(role);
			ElfRelationArgMap::iterator other_unique_relation_args_i = other_unique_relation_args.find(role);

			// Check if we have some matching args for this role in each cluster
			if (unique_relation_args_i != unique_relation_args.end()) {
				// Accumulate the set of distinct values for this role across this cluster
				BOOST_FOREACH(ElfRelationArg_ptr arg, unique_relation_args_i->second) {
					if (arg->get_individual().get() != NULL) {
						if (arg->get_individual()->has_value())
							distinct_values.insert(arg->get_individual()->get_value());
						else
							distinct_values.insert(arg->get_individual()->get_best_uri());
                    }
				}
			}
			if (other_unique_relation_args_i != other_unique_relation_args.end()) {
				// Accumulate the set of distinct values for this role across the other cluster
				BOOST_FOREACH(ElfRelationArg_ptr other_arg, other_unique_relation_args_i->second) {
					if (other_arg->get_individual().get() != NULL) {
						if (other_arg->get_individual()->has_value())
							distinct_values.insert(other_arg->get_individual()->get_value());
						else
							distinct_values.insert(other_arg->get_individual()->get_best_uri());
                    }
				}
			}
		}

		// Check if the count exceeds the limit
		/*
		SessionLogger::info("LEARNIT") << UnicodeUtil::toUTF8StdString(role_maximum.first) << ": " << distinct_values.size() << " >? " << role_maximum.second << " in [" << unique_relation_args_i->second.size() << "," << other_unique_relation_args_i->second.size() << "]" << std::endl;
		BOOST_FOREACH(std::wstring distinct_value, distinct_values) {
			SessionLogger::info("LEARNIT") << "  " << distinct_value << std::endl;
		}
		*/
		if (distinct_values.size() > role_maximum.second)
			return true;
	}

	// No limit hit, must be okay
	return false;
}

/**
 * Hash implementation for ElfIndividualCluster shared_ptrs
 * that will be used by boost::hash to generate unique
 * IDs but also by various keyed containers.
 *
 * @param cluster The pointer referencing the cluster
 * to be hashed.
 * @return An integer hash of cluster's _initial_id.
 **/
size_t hash_value(ElfIndividualCluster_ptr const& cluster) {
	// Use the initial ID of the cluster (from its first individual) for hashing.
	boost::hash<std::wstring> id_hasher;
	return id_hasher(cluster->get_initial_uri());
}
