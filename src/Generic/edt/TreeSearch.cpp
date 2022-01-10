// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"
#include "common/InternalInconsistencyException.h"
#include "edt/TreeSearch.h"

DebugStream &TreeSearch::_debugOut = DebugStream::referenceResolverStream;

TreeSearch::TreeSearch() {
	//_mentionLinker = NULL;
	_nCurrentLeaves = 0;
	_currentLeaves = 0;
	// initialization of _currentLeaves happens in a method called at reset
}

TreeSearch::~TreeSearch() {
	int i;
	if (_currentLeaves != 0) {
		for (i=0; i<_nCurrentLeaves; i++) 
			delete _currentLeaves[i]; 
		delete [] _currentLeaves;
	}
}

/*
void TreeSearch::loadMentionLinker(MentionLinker *newLinker) {
	_mentionLinker = newLinker;
}
*/

// cause we don't know the size at creation
void TreeSearch::initializeCurrentLeavesIfNeeded(int size) {
	if (_currentLeaves != 0)
		return;
	_currentLeaves = _new EntitySet*[size];
	int i;
	for (i = 0; i < size; i++)
		_currentLeaves[i] = 0;
	_max_leaves = size;
}



void TreeSearch::resetSearch(EntitySet *newLeaves[], int max_leaves) {
	initializeCurrentLeavesIfNeeded(max_leaves);
	int i;
	_nCurrentLeaves = 0;
	for (i=0; i<max_leaves; i++) {
		if (_currentLeaves[i] != NULL)
			delete _currentLeaves[i];
		_currentLeaves[i] = newLeaves[i];
		if (newLeaves[i]!= NULL)
			_nCurrentLeaves++;
	}
}

void TreeSearch::resetSearch(EntitySet *newRoot, int max_leaves) {
	initializeCurrentLeavesIfNeeded(max_leaves);
	int i;
	for (i=0; i<max_leaves; i++) {
		if (_currentLeaves[i] != NULL) {
			delete _currentLeaves[i];
			_currentLeaves[i] = NULL;
		}
	}
	_currentLeaves[0] = newRoot;
	_nCurrentLeaves = 1;
}

void TreeSearch::performSearch(GrowableArray <Mention *> & mentions, MentionLinker &linker) {
	int i;
	for (i=0; i<mentions.length(); i++) {
		processMention(mentions[i], linker);
		//_debugOut << "\nDone processing mention #" << i << "\n\n";
	}
}

void inline TreeSearch::processMention(Mention * data, MentionLinker &linker) {
	int i, nLatestSolutions = 0, nWorkingSet = 0;
	// workingSet is the same dimensionality as _currentLeaves
	// and will in fact replace it
	EntitySet ** workingSet = _new EntitySet*[_max_leaves];
	EntitySet * latestSolutions[MAX_BRANCH];
	for (i=0; i<_nCurrentLeaves; i++) {
		nLatestSolutions = linker.linkMention(_currentLeaves[i], data, latestSolutions, MAX_BRANCH);
		nWorkingSet = updateWorkingSet (latestSolutions, nLatestSolutions, workingSet, nWorkingSet);
	}

	// we can only increase in solution size
	if (nWorkingSet < _nCurrentLeaves)
		throw InternalInconsistencyException("TreeSearch::processMention()",
											 "linking solution size not monotonic");
	for (i=0; i<nWorkingSet; i++) {
		if (i < _nCurrentLeaves)
			delete _currentLeaves[i];
		_currentLeaves[i] = workingSet[i];
	}
	_nCurrentLeaves = nWorkingSet;
	delete [] workingSet;
}

int inline TreeSearch::updateWorkingSet(EntitySet * latestSolutions[], int nLatestSolutions, EntitySet * workingSet[], int nWorkingSet) {
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
			if(latestSolutions[i]->getScore() > workingSet[j]->getScore()) {
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

// note: this was removeLeaves, but there should be no deleting now, since
// the currentLeaves and results arrays are the same size
int TreeSearch::transferLeaves(EntitySet *results[]) {
	int nResults = 0, i;
	for (i=0; i<_nCurrentLeaves; i++) {
		results[i] = _currentLeaves[i];
		_currentLeaves[i] = NULL;
		nResults++;
	}
	// zero-out the remaining members of results, just to make sure there's no leftovers
	for (; i < _max_leaves; i++)
		results[i] = 0;
	return nResults;	
}
