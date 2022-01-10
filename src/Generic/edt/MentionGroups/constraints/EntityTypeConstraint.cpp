// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/constraints/EntityTypeConstraint.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Mention.h"

bool EntityTypeConstraint::violatesMergeConstraint(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	if (m1->isOfRecognizedType() && m2->isOfRecognizedType() && m1->getEntityType() != m2->getEntityType() && m1->getEntityType().getName() != Symbol(L"POG") && m2->getEntityType().getName() != Symbol(L"POG")) {
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) {
			SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by EntityType constraint";
		}	
		return true;
	}
	return false;
}
