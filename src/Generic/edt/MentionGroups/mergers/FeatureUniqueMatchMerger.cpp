// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/mergers/FeatureUniqueMatchMerger.h"
#include "Generic/edt/MentionGroups/mergers/FeatureExactMatchMerger.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/edt/MentionGroups/MentionGroupUtils.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Mention.h"

#include <boost/shared_ptr.hpp>

namespace {
	// Helper function to build the feature extractors' name.
	Symbol makeName(Symbol extractorName, Symbol featureName) {
		std::wostringstream s;
		s << L"FeatureUniqueMatch(" << extractorName
		  << L", " << featureName << ")";
		return Symbol(s.str().c_str());
	}
}

FeatureUniqueMatchMerger::FeatureUniqueMatchMerger(Symbol extractorName, Symbol featureName, MentionGroupConstraint_ptr constraints) :
	MentionGroupMerger(makeName(extractorName, featureName), constraints),
	_extractorName(extractorName), _featureName(featureName) {};

bool FeatureUniqueMatchMerger::shouldMerge(const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const {
	for (MentionGroup::const_iterator it1 = g1.begin(); it1 != g1.end(); ++it1) {
		for (MentionGroup::const_iterator it2 = g2.begin(); it2 != g2.end(); ++it2) {
			Symbol value;
			if (shouldMerge(*it1, *it2, cache, value)){
				if (checkForUniqueness(value, g1, g2, cache)) {
					if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
						SessionLogger::dbg("MentionGroups_shouldMerge") << "    Unique match found";			
					}					
					return true;
				} else {
					SessionLogger::dbg("MentionGroups_shouldMerge") << "    Match is not unique";
				}
			}
		}
	}
	return false;
}

bool FeatureUniqueMatchMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache, Symbol &value) const {
	std::vector<AttributeValuePair_ptr> m1Features = cache.getMentionFeaturesByName(m1, _extractorName, _featureName);
	std::vector<AttributeValuePair_ptr> m2Features = cache.getMentionFeaturesByName(m2, _extractorName, _featureName);
	for (std::vector<AttributeValuePair_ptr>::iterator it1 = m1Features.begin(); it1 != m1Features.end(); ++it1) {
		for (std::vector<AttributeValuePair_ptr>::iterator it2 = m2Features.begin(); it2 != m2Features.end(); ++it2) {
			if ((*it1)->equals(*it2)) {
				boost::shared_ptr< AttributeValuePair<Symbol> > f = boost::dynamic_pointer_cast< AttributeValuePair<Symbol> >(*it1);
				if (f != 0) {
					if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
						SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " << m1->getUID() << " and " << m2->getUID();
						SessionLogger::dbg("MentionGroups_shouldMerge") << "\tfeature1: " << (*it1)->toString() 
							<< " feature2: " << (*it2)->toString();
					}
					value = f->getValue();
					return true;
				}
			}
		}
	}
	return false;
}

bool FeatureUniqueMatchMerger::checkForUniqueness(Symbol value, const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const {
	return MentionGroupUtils::featureIsUniqueInDoc(_extractorName, _featureName, value, g1, g2, cache);
}
