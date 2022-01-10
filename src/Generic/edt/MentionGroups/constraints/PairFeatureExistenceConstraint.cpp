// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/constraints/PairFeatureExistenceConstraint.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/SessionLogger.h"

bool PairFeatureExistenceConstraint::violatesMergeConstraint(const Mention *m1, 
															 const Mention *m2, 
															 LinkInfoCache& cache) const 
{
	std::vector<AttributeValuePair_ptr> features = cache.getMentionPairFeaturesByName(m1, m2, _extractorName, _featureName);
	if (!features.empty()) {
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_violatesConstraint")) {
			SessionLogger::dbg("MentionGroups_violatesConstraint") << "Merge blocked by " << _extractorName.to_string() 
				<< L":" << _featureName.to_string() << " constraint";
		}	
		return true;
	}
	return false;
}
