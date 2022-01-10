// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/edt/LinkerTreeSearch.h"

DebugStream &LinkerTreeSearch::_debugOut = DebugStream::referenceResolverStream;

LinkerTreeSearch::LinkerTreeSearch(int max_leaves) {
	_nCurrentLeaves = 0;
	_max_leaves = max_leaves;
	_currentLeaves = _new LexEntitySet *[_max_leaves];
	for (int i = 0; i < _max_leaves; i++)
		_currentLeaves[i] = 0;
}

LinkerTreeSearch::~LinkerTreeSearch() {
	clearLeaves();
	delete[] _currentLeaves;
}

LexEntitySet *LinkerTreeSearch::getLeaf(int i) {
	if ((unsigned) i >= (unsigned) _nCurrentLeaves) {
		throw InternalInconsistencyException::arrayIndexException(
			"LinkerTreeSearch::getLeaf()", _nCurrentLeaves, i);
	}
	else {
		return _currentLeaves[i];
	}
}

/*
// cause we don't know the size at creation
void LinkerTreeSearch::initializeCurrentLeavesIfNeeded(int size) {
	if (_currentLeaves != 0)
		return;
	_currentLeaves = _new EntitySet*[size];
	int i;
	for (i = 0; i < size; i++)
		_currentLeaves[i] = 0;
	_max_leaves = size;
}

*/

//clears the contents of the _currentLeaves array, but leaves the actual array there
void LinkerTreeSearch::clearLeaves() {
	int i;
	if (_currentLeaves != 0) {
		for (i=0; i<_nCurrentLeaves; i++) {
			delete _currentLeaves[i]; 
			_currentLeaves[i] = 0;
		}
	}
	_nCurrentLeaves = 0;
}

//reset the capacity of the _currentLeaves array
void LinkerTreeSearch::setMaxLeaves(int max_leaves) {
	if (max_leaves != _max_leaves) {
		clearLeaves();
		delete [] _currentLeaves;
		_currentLeaves = _new LexEntitySet*[max_leaves];
		_max_leaves = max_leaves;
	}
}
	
//clear old leaves, insert new leaves
void LinkerTreeSearch::resetSearch(LexEntitySet *newLeaves[], int nNewLeaves) {
	int i;
	clearLeaves();
	if(nNewLeaves > _max_leaves)
		setMaxLeaves(nNewLeaves);
	for (i=0; i<nNewLeaves; i++)
		_currentLeaves[i] = newLeaves[i];
	_nCurrentLeaves = nNewLeaves;
}

//clear old leaves, insert a single new leaf
void LinkerTreeSearch::resetSearch(LexEntitySet *newRoot) {
	resetSearch(&newRoot, 1);
}

//execute search
void LinkerTreeSearch::performSearch(GrowableArray <MentionUID> & mentions, 
									 const MentionSet *mentionSet, 
									 MentionLinker &linker) 
{
	for (int i=0; i<mentions.length(); i++) {
		Mention *mention = mentionSet->getMention(mentions[i]);
		processMention(mentions[i], mention->getEntityType(), linker);
		if (mention->hasIntendedType())
			processMention(mentions[i], mention->getIntendedType(), linker);
		//_debugOut << "\nDone processing mention #" << i << "\n\n";
	}
}

//advance the search process by one step
inline 
void LinkerTreeSearch::processMention(MentionUID data, EntityType type, MentionLinker &linker) {
	int i, nLatestSolutions = 0, nWorkingSet = 0;
	// workingSet is the same dimensionality as _currentLeaves
	// and will in fact replace it

	LexEntitySet ** workingSet = _new LexEntitySet*[_max_leaves];
	LexEntitySet ** latestSolutions = _new LexEntitySet*[_max_leaves];
	for (i=0; i<_nCurrentLeaves; i++) {
		nLatestSolutions = linker.linkMention(_currentLeaves[i], data, type, latestSolutions, _max_leaves);
		nWorkingSet = updateWorkingSet (latestSolutions, nLatestSolutions, workingSet, nWorkingSet);
	}
	// we can only increase in solution size
	if (nWorkingSet < _nCurrentLeaves)
		throw InternalInconsistencyException("LinkerTreeSearch::processMention()",
											 "linking solution size not monotonic");
	for (i=0; i<nWorkingSet; i++) {
		if (i < _nCurrentLeaves){
			delete _currentLeaves[i];
		}
		_currentLeaves[i] = workingSet[i];
	}
	_nCurrentLeaves = nWorkingSet;
	delete [] workingSet;
	delete [] latestSolutions;
}

//
inline
int LinkerTreeSearch::updateWorkingSet(LexEntitySet * latestSolutions[], int nLatestSolutions, LexEntitySet * workingSet[], int nWorkingSet) {
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
			else{
				delete latestSolutions[i];
			}
			latestSolutions[i] = NULL;
		}
	}
	return nWorkingSet;	
}

//clear leaves from the searcher, load as many into results[] as will fit (delete the extras)
int LinkerTreeSearch::removeLeaves(LexEntitySet *results[], int max_results) {
	int nResults = 0, i;
	//delete extras
	while(_nCurrentLeaves > max_results) {
		delete _currentLeaves[--_nCurrentLeaves];
		_currentLeaves[_nCurrentLeaves] = NULL;
	}
	for (i=0; i<_nCurrentLeaves; i++) {
		results[i] = _currentLeaves[i];
		_currentLeaves[i] = NULL;
		nResults++;
	}
	_nCurrentLeaves = 0;
	return nResults;	
}
