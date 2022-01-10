
#ifndef DISTRIBUTIONAL_CLUSTER_CLASS_H
#define DISTRIBUTIONAL_CLUSTER_CLASS_H

#include <set>
class Symbol;

/*
    For each verb, we had produced two sets of clusters:
    - clusters of nouns found to be in <sub> role to verb. Each cluster is represented as a running integer.
    - clusters of nouns found to be in <obj> role to verb. Each cluster is represented as a running integer.

    For instance, if a particular verb v has 100 sub clusters, and 150 obj clusters, 
    then its sub clusters ids are {0,1,2,...,99} and obj clusters ids are {0,1,2,...,149}

    // =================
    Thus, each cluster id by itself is not unique.
    But, the tuple or pair (anchor, cluster-id) and combined with the knowledge of whether this is a <sub> cluster or a <obj> cluster; uniquely defines a cluster
    Then, you can say things like: sub cluster-3 of the verb v

    In this class, we want to produce two kinds of features based on the current candidate argument: 
    - anchor specific cluster id
    - all plausible verb cluster ids

    Anchor specific cluster id:
    The <sub> and <obj> cluster ids with the current anchor, e.g.:
    - sub : (_anchor, cluster 3)  // this means candidate argument belongs to sub-cluster-3 of current _anchor
    - obj : (_anchor, cluster 1)  // this means candidate argument belongs to obj-cluster-1 of current _anchor

    All plausible verb cluster ids:
    For the current candidate argument, find me:
    - all verbs that this arg is <sub> of. Then for each of these verbs, which sub cluster is arg in?
    - all verbs that this arg is <obj> of. Then for each of these verbs, which obj cluster is arg in?
    So, you will get two lists, the sub list and the obj list. For instance:
        sub:(verb_i, cluster 2)    obj:(verb_i, cluster 2)
        sub:(verb_j, cluster 1)    obj:(verb_j, cluster 53)
        sub:(verb_k, cluster 45)                             // candidate arg hasn't been found to be an obj of verb_k
                                   obj:(verb_l, cluster 6)   // candidate arg hasn't been found to be a sub of verb_l
    // ===============

    24/10/2013. Yee Seng Chan.
    The above was the old way of doing things. But good to keep the description around as more information on how things could have been done. 
    To minimize memory footprint, we have now made all cluster unique by assiging a running integer number to each cluster as its id.
    The integer runs over the set of <sub> and <obj> clusters, so the integer is globally unique.
    For each (noun) _arg, we now only store 2 sets of integers: 
      - an integer set representing its <sub> cluster-ids 
      - an integer set representing its <obj> cluster-ids
    A cavet to this is that given a pair of (verb, noun), we could no longer return you the cluster-id for it. However, in prior experiments,
    this feature was found to be too sparse anyway to be useful. 

*/
class DistributionalClusterClass {
private:
	Symbol _anchor, _arg;	// (anchor, arg) for this particular event-aa mention

	std::set<int> _anchorAllSubCids;	// for the _arg, the associated set of <sub> cluster ids
	std::set<int> _anchorAllObjCids;	// for the _arg, the associated set of <obj> cluster ids

	void assignAnchorAllClusterIds(const Symbol& anchor, const Symbol& arg);

public:
	DistributionalClusterClass(const Symbol& anchor, const Symbol& arg);

	// ==== start of accessors ====
	std::set<int> anchorAllSubCids() const { return _anchorAllSubCids; }
	std::set<int> anchorAllObjCids() const { return _anchorAllObjCids; }
	// ==== end of accessors ====
};

#endif

