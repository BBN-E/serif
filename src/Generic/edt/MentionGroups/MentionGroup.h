// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTION_GROUP_H
#define MENTION_GROUP_H

#include "Generic/common/BoostUtil.h"
#include "Generic/theories/Mention.h"

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <set>
#include <list>

// Forward declarations.
class MentionGroup;
class MentionGroupMerger;
class MergeHistory;

typedef boost::shared_ptr<MentionGroup> MentionGroup_ptr;
typedef boost::shared_ptr<MergeHistory> MergeHistory_ptr;
typedef std::vector<MentionGroup_ptr> MentionGroupList;

/**
  *  Represents a group of Mentions deemed to be co-referent, as well
  *  as a history of merge operations taken to arrive at this state.
  */
class MentionGroup : boost::noncopyable {
private: 
	// Use boost::make_shared<MentionGroup> to construct MentionGroups.
	MentionGroup(Mention* mention);
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(MentionGroup, Mention*);

	struct ClassComp {
		bool operator()(const Mention* lhs, const Mention *rhs) const {
			return (lhs->getUID() < rhs->getUID());
		}
	};

	typedef std::set<const Mention*, ClassComp> MentionSetType;

public:

	// iterator typedef
	typedef MentionSetType::const_iterator const_iterator;
	typedef MentionSetType::const_reverse_iterator const_reverse_iterator;

	/** Merge this group with other and update history. */
	void merge(MentionGroup &other, MentionGroupMerger *merger, double score=0);

	/** Return an iterator to the first Mention in the group. */
	const_iterator begin() const { return _mentions.begin(); }

	/** Return an iterator to the end of the group of Mentions. */
	const_iterator end() const { return _mentions.end(); }

	/** Return a reverse iterator to the last Mention in the group. */
	const_reverse_iterator rbegin() const { return _mentions.rbegin(); }

	/** Return true if mention is a member of this MentionGroup */
	bool contains(const Mention *mention) const;

	/** Return earliest sentence represented by this MentionGroup */
	int getFirstSentenceNumber() const;

	/** Return latest sentence represented by this MentionGroup */
	int getLastSentenceNumber() const;

	/** Return the dominant EntityType of the Mentions. */
	EntityType getEntityType() const;

	std::wstring toString() const;
	std::wstring getMergeHistoryString() const;

	size_t size() const { return _mentions.size(); }

private:

	// The set of co-referent Mentions.
	MentionSetType _mentions;
	
	// The history of merge operations.
	MergeHistory_ptr _history;
};

#endif
