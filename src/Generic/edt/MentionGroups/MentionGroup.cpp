// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/edt/MentionGroups/MergeHistory.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/SynNode.h"

MentionGroup::MentionGroup(Mention* mention) : _history(MergeHistory::create(mention)) { 
	_mentions.insert(mention);
}

void MentionGroup::merge(MentionGroup &other, MentionGroupMerger *merger, double score) {
	_mentions.insert(other._mentions.begin(), other._mentions.end());
	_history = MergeHistory::create(_history, other._history, merger, score);
}

bool MentionGroup::contains(const Mention *mention) const {
	return (_mentions.find(mention) != _mentions.end());
}

int MentionGroup::getFirstSentenceNumber() const {
	if (size() == 0)
		return -1;
	MentionGroup::const_iterator it = this->begin();
	return (*it)->getSentenceNumber();
}

int MentionGroup::getLastSentenceNumber() const {
	if (size() == 0)
		return -1;
	MentionGroup::const_reverse_iterator it = this->rbegin();
	return (*it)->getSentenceNumber();
}

EntityType MentionGroup::getEntityType() const {
	EntityType type = EntityType::getUndetType();
	for (MentionGroup::const_iterator it = this->begin(); it != this->end(); ++it) {
		EntityType t = (*it)->getEntityType();
		if (!type.isRecognized()) {
			if (t.isRecognized()) 
				type = t;
		}
		else {
			if (t.isRecognized() && t != type) {
				SessionLogger::warn("mention_groups") << "MentionGroup with more than one valid entity type.";
			}
		}
	}
	return type;
}

std::wstring MentionGroup::toString() const {
	std::wstringstream result;
	result << "[";
	for (const_iterator it = begin(); it != end(); ++it) {
		if (it != begin())
			result << ", ";
		result << (*it)->getUID();
		//result << (*it)->getNode()->toTextString();
	}
	result << "]";
	return result.str();
}

std::wstring MentionGroup::getMergeHistoryString() const {
	return _history->toString();
}
