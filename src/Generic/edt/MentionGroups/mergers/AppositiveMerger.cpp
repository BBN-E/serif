// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/mergers/AppositiveMerger.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"

AppositiveMerger::AppositiveMerger(MentionGroupConstraint_ptr constraints) : PairwiseMentionGroupMerger(Symbol(L"Appositive"), constraints) {}

bool AppositiveMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	// at least one of the mentions must have a recognized type
	if (!m1->isOfRecognizedType() && !m2->isOfRecognizedType())
		return false;

	const Mention *m1Parent = m1->getParent();
	const Mention *m2Parent = m2->getParent();

	if (m2->getMentionType() == Mention::NAME) {
		while (m1Parent != NULL && m1Parent->getMentionType() == Mention::LIST)
			m1Parent = m1Parent->getParent();
	}
	if (m1->getMentionType() == Mention::NAME) {
		while (m2Parent != NULL && m2Parent->getMentionType() == Mention::LIST)
			m2Parent = m2Parent->getParent();
	}

	if (m1Parent != NULL && m1Parent == m2Parent && m1Parent->getMentionType() == Mention::APPO) {
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
			SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " 
				<< m1->getUID() << " (" << m1->toCasedTextString() << ") and " 
				<< m2->getUID() << " (" << m2->toCasedTextString() << ")";
			SessionLogger::dbg("MentionGroups_shouldMerge") << "APPOSITIVE FOUND (CHILDREN)";	
		}
		return true;
	}

	if ((m1->getMentionType() == Mention::APPO && m2Parent == m1) ||
		(m2->getMentionType() == Mention::APPO && m1Parent == m2))
	{
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
			SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " 
				<< m1->getUID() << " (" << m1->toCasedTextString() << ") and " 
				<< m2->getUID() << " (" << m2->toCasedTextString() << ")";
			SessionLogger::dbg("MentionGroups_shouldMerge") << "APPOSITIVE FOUND (PARENT)";	
		}
		return true;
	}

	return false;
}
