// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef MERGE_HISTORY_H
#define MERGE_HISTORY_H

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <string>

class Mention;
class MentionGroupMerger;

class MergeHistory;
typedef boost::shared_ptr<MergeHistory> MergeHistory_ptr;

/**
  *  Base class for representing a history of merge operations.
  */
class MergeHistory : boost::noncopyable {
public:
	/** Create a new history with a single leaf mention. */
	static MergeHistory_ptr create(const Mention *mention);

	/** Return the new MergeHistory created by merging left and right nodes. */
	static MergeHistory_ptr create(MergeHistory_ptr left, MergeHistory_ptr right, MentionGroupMerger* merger, double score);

	// here’s where we’ll want to define an interface for accessing the merge history	

	virtual std::wstring toString(const std::wstring prefix=std::wstring(L" "),
								  wchar_t left=L' ', wchar_t right=L' ') const = 0;

protected:
	MergeHistory() {}
};

/**
  *  Represents a node in a tree of binary merge operations.
  */
class MergeHistoryNode : public MergeHistory {
public:
	MergeHistoryNode(MergeHistory_ptr left, MergeHistory_ptr right, MentionGroupMerger* merger, double score) :
	  _leftChild(left), _rightChild(right), _merger(merger), _merge_score(score) {};

	  std::wstring toString(const std::wstring prefix,
							wchar_t left, wchar_t right) const;

private:
	//merge decision info
	double _merge_score;
	MentionGroupMerger* _merger;

	// tree structure
	MergeHistory_ptr _leftChild;
	MergeHistory_ptr _rightChild;
};

/**
  *  Represents a leaf (i.e. an atomic Mention) in a tree of merge operations.
  */
class MergeHistoryLeaf: public MergeHistory {
public:
	MergeHistoryLeaf(const Mention *mention) : _mention(mention) {};

	std::wstring toString(const std::wstring prefix, 
						  wchar_t left, wchar_t right) const;
private:
	const Mention *_mention;
};

#endif
