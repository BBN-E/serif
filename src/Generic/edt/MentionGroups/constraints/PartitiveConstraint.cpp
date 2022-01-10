// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/constraints/PartitiveConstraint.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Mention.h"

bool PartitiveConstraint::violatesMergeConstraint(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	if (m1->getMentionType() == Mention::PART || m2->getMentionType() == Mention::PART) {
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) {
			SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by partitive constraint";
		}	
		return true;
	}
	return false;
}
