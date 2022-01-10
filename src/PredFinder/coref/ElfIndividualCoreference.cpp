/**
 * Engine class for clustering coreferent individuals.
 *
 * @file ElfIndividualCoreference.cpp
 * @author nward@bbn.com
 * @date 2010.08.23
 **/

#include "Generic/common/leak_detection.h"
#include "ElfIndividualCoreference.h"
#include "boost/foreach.hpp"
#include "boost/make_shared.hpp"

/**
 * Adds a new member to the collection of clusters, either
 * as a new cluster with one member, or to an existing
 * cluster that is the best match (and still as a new cluster
 * if there's no best match).
 *
 * @param new_member The ElfIndividualClusterMember being added
 * @param incremental Optional, defaults to false; if true, tries
 * to add the member to an existing cluster, otherwise creates
 * a new cluster with just that member.
 *
 * @author nward@bbn.com
 * @date 2010.08.23
 **/
void ElfIndividualCoreference::add_cluster_member(ElfIndividual_ptr individual, ElfRelationMap relations) {
	// Just create a new cluster with one new member and add it to the collection of clusters
	ElfIndividualClusterMember_ptr new_member = boost::make_shared<ElfIndividualClusterMember>(_domain_prefix, individual, relations);
	_clusters.insert(boost::make_shared<ElfIndividualCluster>(_domain_prefix, new_member));
}

/**
 * Iterates through the cluster table, scoring all possible
 * cluster combinations and combining the best matches until
 * no additional cluster combinations meet the score threshold.
 * Calling this a second time with no cluster changes is a no-op.
 *
 * @author nward@bbn.com
 * @date 2010.08.23
 **/
void ElfIndividualCoreference::combine_clusters(void) {
	// Loop as long as we have combined at least one pair of clusters
	while (true) {
		// Score the current state of the cluster set
		ElfIndividualClusterPairScores cluster_similarity_table;
		int left_cluster_index = 0, right_cluster_index;
		for (ElfIndividualClusterSortedSet::iterator left_cluster = _clusters.begin(); left_cluster != _clusters.end(); left_cluster++, left_cluster_index++) {
			right_cluster_index = left_cluster_index;
			for (ElfIndividualClusterSortedSet::iterator right_cluster = left_cluster; right_cluster != _clusters.end(); right_cluster++, right_cluster_index++) {
				// Ignore the same cluster
				if (left_cluster != right_cluster) {
					// Store the score by cluster pair, left/outer first, for easy independent pair lookup
					double score = (*left_cluster)->calculate_similarity(*right_cluster);
					//SessionLogger::info("LEARNIT") << "cluster similarity " << left_cluster_index << "(" << hash_value(*left_cluster) << ")\t" << right_cluster_index << "(" << hash_value(*right_cluster) << ")\t" << score << std::endl;
					std::pair<ElfIndividualClusterPairScores::iterator, bool> cluster_pair_score_insert = cluster_similarity_table.insert(ElfIndividualClusterPairScores::value_type(*left_cluster, ElfIndividualClusterScores()));
					cluster_pair_score_insert.first->second.insert(ElfIndividualClusterScores::value_type(*right_cluster, score));
				}
			}
		}

		// Get all independent pairs of proposed best cluster combinations
		ElfIndividualClusterPairScores cluster_matches_table;
		int n_cluster_matches = 0;
		BOOST_FOREACH(ElfIndividualClusterPairScores::value_type cluster_similarities, cluster_similarity_table) {
			// Track the best possible match for this left/outer cluster
			ElfIndividualCluster_ptr best_match;
			double best_score = 0.0;
			BOOST_FOREACH(ElfIndividualClusterScores::value_type cluster_similarity, cluster_similarities.second) {
				// We always want the highest scoring match
				if (cluster_similarity.second > best_score) {
					best_match = cluster_similarity.first;
					best_score = cluster_similarity.second;
				}
			}

			// Make sure the best match beats the score threshold
			if (best_score > _cluster_similarity_threshold && best_match.get() != NULL) {
				// Make sure we haven't already assigned either of these clusters as another cluster's best match
				if (cluster_matches_table.find(cluster_similarities.first) == cluster_matches_table.end() && cluster_matches_table.find(best_match) == cluster_matches_table.end()) {
					// Store the best match for this left/outer cluster
					std::pair<ElfIndividualClusterPairScores::iterator, bool> cluster_match_insert = cluster_matches_table.insert(ElfIndividualClusterPairScores::value_type(cluster_similarities.first, ElfIndividualClusterScores()));
					cluster_match_insert.first->second.insert(ElfIndividualClusterScores::value_type(best_match, best_score));

					// Store a reference to the best match so we don't transitively overmatch
					cluster_match_insert = cluster_matches_table.insert(ElfIndividualClusterPairScores::value_type(best_match, ElfIndividualClusterScores()));
					cluster_match_insert.first->second.insert(ElfIndividualClusterScores::value_type(cluster_similarities.first, -1.0));

					// Track how many merges we're making
					n_cluster_matches++;
				}
			}
		}

		// Done?
		//SessionLogger::info("LEARNIT") << "Merging " << n_cluster_matches << "/" << _clusters.size() << " clusters" << std::endl;
		if (cluster_matches_table.size() == 0)
			break;

		// Combine clusters
		BOOST_FOREACH(ElfIndividualClusterPairScores::value_type cluster_match, cluster_matches_table) {
			// Make sure this is the actual cluster best match, not just the bidirectional blocker
			if (cluster_match.second.begin()->second != -1.0) {
				// Merge the two clusters, then delete the one we merged in
				ElfIndividualCluster_ptr merged_cluster = cluster_match.second.begin()->first;
				cluster_match.first->combine_cluster(merged_cluster);
				_clusters.erase(merged_cluster);
				//SessionLogger::info("LEARNIT") << "  Merged " << hash_value(merged_cluster) << " into " << hash_value(cluster_match.first) << std::endl;
			}
		}
	}

	// Additional domain-specific merging heuristics that don't have to do with direct cluster overlap
	if (_domain_prefix == L"nfl") {
		// Loop until we can't combine any more team clusters
		while (combine_nfl_clusters());
	}
}

/**
 * Walks through the cluster table once, trying to find NFL
 * domain-specific cluster matches that otherwise would fail
 * the threshold and combining the best matches until
 * no additional cluster combinations meet the score threshold.
 * Calling this a second time with no cluster changes is a no-op.
 *
 * @return True if any clusters were merged, false otherwise.
 *
 * @author nward@bbn.com
 * @date 2010.08.27
 **/
bool ElfIndividualCoreference::combine_nfl_clusters(void) {
	// Collect all clusters that contain a game or game subtype
	ElfIndividualClusterSortedSet game_clusters;
	BOOST_FOREACH(ElfIndividualCluster_ptr cluster, _clusters) {
		if (cluster->has_type(L"nfl:NFLGame") ||
			cluster->has_type(L"nfl:NFLRegularSeasonGame") ||
			cluster->has_type(L"nfl:NFLPlayoffGame") ||
			cluster->has_type(L"nfl:NFLExhibitionGame")) {
			game_clusters.insert(cluster);
		}
	}

	// Try to merge non-conflicting clusters even if their similarity score is low
	ElfIndividualClusterPairScores cluster_similarity_table;
	for (ElfIndividualClusterSortedSet::iterator left_cluster = _clusters.begin(); left_cluster != _clusters.end(); left_cluster++) {
		for (ElfIndividualClusterSortedSet::iterator right_cluster = left_cluster; right_cluster != _clusters.end(); right_cluster++) {
			// Ignore the same cluster
			if (left_cluster != right_cluster) {
				// Make sure both clusters are in our candidate map
				ElfIndividualClusterSortedSet::iterator left_game_cluster = game_clusters.find(*left_cluster);
				ElfIndividualClusterSortedSet::iterator right_game_cluster = game_clusters.find(*right_cluster);
				if (left_game_cluster != game_clusters.end() && right_game_cluster != game_clusters.end()) {
					// Get the maximum offsets for each cluster
					EDTOffset left_start, left_end, right_start, right_end;
					(*left_cluster)->get_spanning_offsets(left_start, left_end);
					(*right_cluster)->get_spanning_offsets(right_start, right_end);
					//SessionLogger::info("LEARNIT") << "left: " << (*left_cluster)->get_initial_uri() << " [" << left_start << "," << left_end << "] right: " << (*right_cluster)->get_initial_uri() << " [" << right_start << "," << right_end << "]" << std::endl;

					// Determine the text evidence union and intersection offsets
					EDTOffset union_start, union_end, intersection_start, intersection_end;
					if (left_start <= right_start) {
						union_start = left_start;
						intersection_start = right_start;
					} else {
						union_start = right_start;
						intersection_start = left_start;
					}
					if (left_end >= right_end) {
						union_end = left_end;
						intersection_end = right_end;
					} else {
						union_end = right_end;
						intersection_end = left_end;
					}
					int union_length = union_end.value() - union_start.value();
					int intersection_length = intersection_end.value() - intersection_start.value();

					// Check that one cluster at least partially overlaps another
					if (intersection_length > 0 && union_length > 0) {
						// Get the similarity score between these candidate clusters
						double similarity = (*left_cluster)->calculate_similarity(*right_cluster);

						// Calculate the percentage overlap between these candidate clusters
						double overlap = ((double)intersection_length)/union_length;

						// Blend the similarity and offset score, weighting by the threshold (which the similarity score must be lower than to get here)
						double score = _cluster_similarity_threshold*similarity + (1.0 - _cluster_similarity_threshold)*overlap;
						if (similarity == -1.0)
							// Conflicts always override
							score = -1.0;
						else if (similarity == 0.0 && !_sentence_level)
							// Pure offset alignment is only allowed in sentence_level
							score = -1.0;
						//SessionLogger::info("LEARNIT") << "s: " << similarity << " o: " << overlap << " = " << score << std::endl;

						// Add this cluster pair as a candidate, with blended similarity and offset score
						std::pair<ElfIndividualClusterPairScores::iterator, bool> team_cluster_pair_score_insert = cluster_similarity_table.insert(ElfIndividualClusterPairScores::value_type(*left_cluster, ElfIndividualClusterScores()));
						team_cluster_pair_score_insert.first->second.insert(ElfIndividualClusterScores::value_type(*right_cluster, score));
					}
				}
			}
		}
	}

	// Get all independent pairs of proposed best cluster combinations
	ElfIndividualClusterPairScores cluster_matches_table;
	int n_cluster_matches = 0;
	BOOST_FOREACH(ElfIndividualClusterPairScores::value_type cluster_similarities, cluster_similarity_table) {
		// Track the best possible match for this left/outer cluster
		ElfIndividualCluster_ptr best_match;
		double best_score = -1.0;
		BOOST_FOREACH(ElfIndividualClusterScores::value_type cluster_similarity, cluster_similarities.second) {
			// We always want the highest scoring match
			if (cluster_similarity.second > best_score) {
				best_match = cluster_similarity.first;
				best_score = cluster_similarity.second;
			}
		}

		// Make sure the best match is non-conflicting
		if (best_score >= 0.0 && best_match.get() != NULL) {
			// Make sure we haven't already assigned either of these clusters as another cluster's best match
			if (cluster_matches_table.find(cluster_similarities.first) == cluster_matches_table.end() && cluster_matches_table.find(best_match) == cluster_matches_table.end()) {
				// Store the best match for this left/outer cluster
				std::pair<ElfIndividualClusterPairScores::iterator, bool> cluster_match_insert = cluster_matches_table.insert(ElfIndividualClusterPairScores::value_type(cluster_similarities.first, ElfIndividualClusterScores()));
				cluster_match_insert.first->second.insert(ElfIndividualClusterScores::value_type(best_match, best_score));

				// Store a reference to the best match so we don't transitively overmatch
				cluster_match_insert = cluster_matches_table.insert(ElfIndividualClusterPairScores::value_type(best_match, ElfIndividualClusterScores()));
				cluster_match_insert.first->second.insert(ElfIndividualClusterScores::value_type(cluster_similarities.first, -1.0));

				// Track how many merges we're making
				n_cluster_matches++;
			}
		}
	}

	// Done?
	//SessionLogger::info("LEARNIT") << "NFL Merging " << n_cluster_matches << "/" << _clusters.size() << std::endl;
	if (cluster_matches_table.size() == 0)
		return false;

	// Combine clusters
	BOOST_FOREACH(ElfIndividualClusterPairScores::value_type cluster_match, cluster_matches_table) {
		// Make sure this is the actual cluster best match, not just the bidirectional blocker
		if (cluster_match.second.begin()->second != -1.0) {
			// Merge the two clusters, then delete the one we merged in
			ElfIndividualCluster_ptr merged_cluster = cluster_match.second.begin()->first;
			cluster_match.first->combine_cluster(merged_cluster);
			_clusters.erase(merged_cluster);
			//SessionLogger::info("LEARNIT") << "  Merged " << hash_value(merged_cluster) << " into " << hash_value(cluster_match.first) << std::endl;
		}
	}

	// Done, some clusters were merged
	return true;
}

/**
 * Collect the mapping of original input individual URIs to new
 * generated cluster individual URIs from each cluster.
 *
 * @return A map between URIs, intended for use in updating
 * the generated URIs of argument-bound ElfIndividuals that are
 * part of a cluster.
 *
 * @author nward@bbn.com
 * @date 2011.06.08
 **/
ElfIndividualUriMap ElfIndividualCoreference::get_individual_to_cluster_uri_map(std::wstring docid) const {
	// Combine the clustered individuals mapping for each cluster
	ElfIndividualUriMap individual_to_cluster_uri_map;
	BOOST_FOREACH(ElfIndividualCluster_ptr cluster, _clusters) {
		// Generate a URI for this cluster based on its member individuals
		std::wstring cluster_uri = cluster->get_generated_uri(docid);

		// Map from this cluster's member individual URIs to the cluster URI
		BOOST_FOREACH(ElfIndividual_ptr individual, cluster->get_member_individuals()) {
			// Only store a mapping if they're different
			std::wstring member_uri = individual->get_best_uri(docid);
			if (member_uri != cluster_uri)
				individual_to_cluster_uri_map.insert(ElfIndividualUriMap::value_type(member_uri, cluster_uri));
		}
	}
	return individual_to_cluster_uri_map;
}
