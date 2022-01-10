// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/mergers/FeatureExactMatchMerger.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Mention.h"

#include <boost/foreach.hpp>

namespace {
	// Helper function to build the feature extractors' name.
	Symbol makeName(Symbol extractorName, Symbol featureName) {
		std::wostringstream s;
		s << L"FeatureExactMatch(" << extractorName
		  << L", " << featureName << ")";
		return Symbol(s.str().c_str());
	}
}

FeatureExactMatchMerger::FeatureExactMatchMerger(Symbol extractorName, Symbol featureName, MentionGroupConstraint_ptr constraints) : 
	PairwiseMentionGroupMerger(makeName(extractorName, featureName), constraints),
	_extractorName(extractorName), _featureName(featureName) {};

bool FeatureExactMatchMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const { 
	std::vector<AttributeValuePair_ptr> m1Features = cache.getMentionFeaturesByName(m1, _extractorName, _featureName);
	std::vector<AttributeValuePair_ptr> m2Features = cache.getMentionFeaturesByName(m2, _extractorName, _featureName);
	for (std::vector<AttributeValuePair_ptr>::iterator it1 = m1Features.begin(); it1 != m1Features.end(); ++it1) {
		for (std::vector<AttributeValuePair_ptr>::iterator it2 = m2Features.begin(); it2 != m2Features.end(); ++it2) {
			if ((*it1)->equals(*it2)) {
				if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
					SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " << m1->getUID() << " and " << m2->getUID();
					SessionLogger::dbg("MentionGroups_shouldMerge") << "\tfeature1: " << (*it1)->toString() 
						<< " feature2: " << (*it2)->toString();
					SessionLogger::dbg("MentionGroups_shouldMerge") << "EQUAL";			
				}		
				return true;
			}
		}
	}
	return false;
}
