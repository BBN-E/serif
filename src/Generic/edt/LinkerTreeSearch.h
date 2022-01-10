// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LINKER_TREESEARCH_H
#define LINKER_TREESEARCH_H

#include "Generic/edt/MentionLinker.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/common/DebugStream.h"

class LinkerTreeSearch {
public:
	//static const int MAX_BRANCH = 5;

	LinkerTreeSearch(int max_leaves = 0);
	~LinkerTreeSearch();

	int getNLeaves() const { return _nCurrentLeaves; }
	LexEntitySet *getLeaf(int i);

	void setMaxLeaves(int max_leaves);
	void resetSearch(LexEntitySet *newLeaves[], int nNewLeaves);
	void resetSearch(LexEntitySet *newRoot);
	void performSearch(GrowableArray <MentionUID> & mentions, const MentionSet *mentionSet, MentionLinker &linker);
	void processMention(MentionUID data, EntityType type, MentionLinker &linker);
	int updateWorkingSet(LexEntitySet * latestSolutions[], int nLatestSolutions, LexEntitySet * workingSet[], int nWorkingSet);
	int removeLeaves(LexEntitySet *results[], int max_results);	
	void clearLeaves();
	
private:
	//MentionLinker *_mentionLinker;
	int _max_leaves;
	LexEntitySet ** _currentLeaves;
	void initializeCurrentLeavesIfNeeded(int size);
	int _nCurrentLeaves;	

	static DebugStream &_debugOut;
};

#endif
