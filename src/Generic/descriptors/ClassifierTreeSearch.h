// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CLASSIFIER_TREESEARCH_H
#define CLASSIFIER_TREESEARCH_H

#include "Generic/descriptors/MentionClassifier.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/common/DebugStream.h"

class ClassifierTreeSearch {
public:
	static const int MAX_BRANCH = 10;

	// if it's not branching, we'll make all changes in place
	// and not fork and not delete. Otherwise we will
	ClassifierTreeSearch(bool isBranching);
	~ClassifierTreeSearch();

	/**
	 * prepare the leaf set for a new sentence
	 * @param newLeaves the MentionSets of the last sentence
	 * @param max_leaves the Branching Factor of this stage
	 */
	void resetSearch(MentionSet *newLeaves[], int max_leaves);

	/**
	 * prepare the leaf set the first time out
	 * @param newRoot The MentionSet from which all others will derive
	 * @param max_leaves The Branching Factor of this stage
	 */
	void resetSearch(MentionSet *newRoot, int max_leaves);

	/**
	 * the called action function:
	 * determines alternative classifications for the mention and forks the
	 * MentionSet as need be
	 * @param data The node with the mention to be classified
	 * @param classifier The object that will do the classification
	 * 
	 */
	void performSearch(const SynNode *data, MentionClassifier &classifier);

	/**
	 * add winning new solutions in a sorted way
	 */
	int updateWorkingSet(MentionSet * latestSolutions[], int nLatestSolutions, MentionSet * workingSet[], int nWorkingSet);
	int transferLeaves(MentionSet *results[]);	
	
private:
	bool _is_branching;
	int _max_leaves;
	MentionSet ** _currentLeaves;
	void initializeCurrentLeavesIfNeeded(int size);
	/** 
	 * where the actual work is done. 
	 * get alternate classifications.
	 */
	void processMention(const SynNode * data, MentionClassifier &classifier);

	int _nCurrentLeaves;	

};

#endif
