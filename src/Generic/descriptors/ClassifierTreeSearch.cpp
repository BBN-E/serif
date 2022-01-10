// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/descriptors/ClassifierTreeSearch.h"
#include "Generic/descriptors/MentionClassifier.h"
#include "Generic/theories/SynNode.h"

ClassifierTreeSearch::ClassifierTreeSearch(bool isBranching) : _is_branching(isBranching) {
	_nCurrentLeaves = 0;
	_currentLeaves = 0;
	// initialization of _currentLeaves happens in a method called at reset
}

ClassifierTreeSearch::~ClassifierTreeSearch() {
	int i;
	if (_currentLeaves != 0) {
		if (_is_branching) {
			for (i=0; i<_nCurrentLeaves; i++) 
				delete _currentLeaves[i]; 
		}
		delete [] _currentLeaves;
	}
}


// cause we don't know the size at creation
void ClassifierTreeSearch::initializeCurrentLeavesIfNeeded(int size) {
	if (_currentLeaves != 0)
		return;
	_currentLeaves = _new MentionSet*[size];
	int i;
	for (i = 0; i < size; i++)
		_currentLeaves[i] = 0;
	_max_leaves = size;
}


// put the newLeaves set in place, as the ancestors from which
// the classifier will fork and update
// the leaves that were here before are not referenced by anyone now
void ClassifierTreeSearch::resetSearch(MentionSet *newLeaves[], int max_leaves) {
	initializeCurrentLeavesIfNeeded(max_leaves);
	int i;
	_nCurrentLeaves = 0;
	for (i=0; i<max_leaves; i++) {
		if (_is_branching) {
			if (_currentLeaves[i] != NULL)
				delete _currentLeaves[i];
		}
		_currentLeaves[i] = newLeaves[i];
		if (newLeaves[i]!= NULL)
			_nCurrentLeaves++;
	}
}

// assuming we start with a single mention set, use that as the 
// foothold to start the process
void ClassifierTreeSearch::resetSearch(MentionSet *newRoot, int max_leaves) {
	initializeCurrentLeavesIfNeeded(max_leaves);
	if (_is_branching) {
		int i;
		for (i=0; i<max_leaves; i++) {
			if (_currentLeaves[i] != NULL) {
				delete _currentLeaves[i];
				_currentLeaves[i] = NULL;
			}
		}
	}
	_currentLeaves[0] = newRoot;
	_nCurrentLeaves = 1;
}

void ClassifierTreeSearch::performSearch(const SynNode *data, MentionClassifier &classifier) {
	processMention(data, classifier);
}

void ClassifierTreeSearch::processMention(const SynNode *data, MentionClassifier &classifier) {
	int i, nLatestSolutions = 0, nWorkingSet = 0;
	// workingSet is the same dimensionality as _currentLeaves
	// and will in fact replace it
	MentionSet ** workingSet = _new MentionSet*[_max_leaves];
	MentionSet * latestSolutions[MAX_BRANCH];
	for (i=0; i<_nCurrentLeaves; i++) {
		Mention* currMention = _currentLeaves[i]->getMentionByNode(data);
		nLatestSolutions = classifier.classifyMention(_currentLeaves[i], currMention, latestSolutions, MAX_BRANCH, _is_branching);

		// classifiers should at least return the solution they came in with
		if (nLatestSolutions < 1)
			throw InternalInconsistencyException("ClassifierTreeSearch::processMention()",
				"no solutions returned from classifier");
			
		if (_is_branching) 
			nWorkingSet = updateWorkingSet(latestSolutions, nLatestSolutions, workingSet, nWorkingSet);
		else {
			nWorkingSet = 1;
			workingSet[0] = latestSolutions[0];
		}

	}

	// workingSet is directly transferred to replace currentLeaves
	if (_is_branching) {
		for (i=0; i<nWorkingSet; i++) {
			if (i < _nCurrentLeaves)
				delete _currentLeaves[i];
			_currentLeaves[i] = workingSet[i];
			workingSet[i] = 0;
		}
	}
	else {
		_currentLeaves[0] = workingSet[0];
		workingSet[0] = 0;
		// Tal: shouldn't there be a delete _currentLeaves[0] here?
	}
	_nCurrentLeaves = nWorkingSet;
	delete [] workingSet;
}

// for branching only
inline
int ClassifierTreeSearch::updateWorkingSet(MentionSet * latestSolutions[], int nLatestSolutions, MentionSet * workingSet[], int nWorkingSet) {
	int i, j, k;

	for (i = 0; i < nLatestSolutions; i++) {
		if(nWorkingSet==0) {			
			workingSet[nWorkingSet++] = latestSolutions[i];
			latestSolutions[i] = NULL;
			continue;
		}
		// find where the solution should fit into the working set,
		// and shift the other solutions down (or out)
		bool insertedSolution = false;
		for (j = 0; j < nWorkingSet; j++) {
			if(latestSolutions[i]->getDescScore() > workingSet[j]->getDescScore()) {
				//insert this solution into the workingSet

				// kill the last member if we're at overflow
				if (nWorkingSet == _max_leaves)
					delete workingSet[_max_leaves-1];
				else if (nWorkingSet < _max_leaves) 
					nWorkingSet++;
				for (k = nWorkingSet-1; k>j; k--) 
					workingSet[k] = workingSet[k-1];
				workingSet[j] = latestSolutions[i];
				latestSolutions[i] = NULL;
				insertedSolution = true;
				break;
			}
		}
		// solution doesn't make the cut. if there's room, insert it at the end
		// otherwise, ditch it.
		if (!insertedSolution) {
			if (nWorkingSet < _max_leaves)
				workingSet[nWorkingSet++] = latestSolutions[i];
			else
				delete latestSolutions[i];
			latestSolutions[i] = NULL;
		}
	}
	return nWorkingSet;	
}

int ClassifierTreeSearch::transferLeaves(MentionSet *results[]) {
	int nResults = 0, i;
	for (i=0; i<_nCurrentLeaves; i++) {
		results[i] = _currentLeaves[i];
		_currentLeaves[i] = NULL;
		nResults++;
	}
	// zero-out the remaining members of results, just to make sure there's no leftovers
	for (; i < _max_leaves; i++)
		results[i] = NULL;
	return nResults;	
}
