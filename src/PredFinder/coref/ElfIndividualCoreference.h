/**
 * Engine class for clustering coreferent individuals.
 *
 * @file ElfIndividualCoreference.h
 * @author nward@bbn.com
 * @date 2010.08.20
 **/

#pragma once

#include "ElfIndividualCluster.h"
#include "ElfIndividualClusterMember.h"
#include <vector>

/**
 * Takes a set of ElfIndividuals and the ElfRelations that
 * probably involve them (directly or indirectly), generates
 * a set of ElfIndividualClusters, combines those clusters
 * using various features, and provides as final output
 * a map from original individual IDs to clustered individual
 * IDs, suitable for use in modifying an ElfDocument's contents.
 *
 * @author nward@bbn.com
 * @date 2010.08.20
 **/
class ElfIndividualCoreference {
public:
	ElfIndividualCoreference(std::wstring domain_prefix, bool sentence_level = false) : _domain_prefix(domain_prefix), _sentence_level(sentence_level), _cluster_similarity_threshold(0.3) {};

	void add_cluster_member(ElfIndividual_ptr individual, ElfRelationMap relations);

	void combine_clusters(void);

	ElfIndividualUriMap get_individual_to_cluster_uri_map(std::wstring docid) const;

private:
	bool combine_nfl_clusters(void);

	/**
	 * The current collection of individual clusters.
	 **/
	ElfIndividualClusterSortedSet _clusters;

	/**
	 * The prefix for the ontology domain, used to
	 * guide clustering.
	 **/
	std::wstring _domain_prefix;

	/**
	 * Whether this coref instance is clustering at the sentence
	 * or document level.
	 **/
	bool _sentence_level;

	/**
	 * A hard threshold for minimum cluster similarity score
	 * necessary to combine two clusters. Determined empirically.
	 **/
	double _cluster_similarity_threshold;
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.08.20
 **/
typedef boost::shared_ptr<ElfIndividualCoreference> ElfIndividualCoreference_ptr;


/**
 * Container for storing scores for a particular cluster,
 * presumably relative to another cluster. Intended for
 * use in ElfIndividualClusterPairScores.
 *
 * @author nward@bbn.com
 * @date 2010.08.23
 **/
typedef std::map<ElfIndividualCluster_ptr, double, ElfIndividualClusterPtrSortCriterion> ElfIndividualClusterScores;

/**
 * Container for storing pair-wise cluster scores.
 *
 * @author nward@bbn.com
 * @date 2010.08.23
 **/
typedef std::map<ElfIndividualCluster_ptr, ElfIndividualClusterScores, ElfIndividualClusterPtrSortCriterion> ElfIndividualClusterPairScores;
