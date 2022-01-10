// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/mergers/MentionPointerMerger.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

#include <boost/foreach.hpp>

namespace {
	// Helper function to build the feature extractors' name.
	Symbol makeName(Symbol extractorName, Symbol featureName) {
		std::wostringstream s;
		s << L"MentionPointer(" << extractorName
		  << L", " << featureName << ")";
		return Symbol(s.str().c_str());
	}
}

MentionPointerMerger::MentionPointerMerger(Symbol extractorName, Symbol featureName, MentionGroupConstraint_ptr constraints) : 
	PairwiseMentionGroupMerger(makeName(extractorName, featureName), constraints),
    _extractorName(extractorName), _featureName(featureName)
{}

bool MentionPointerMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	// We only get called once per pair of mentions -- try both directions.
	return (shouldMergeOneDirection(m1, m2, cache) || shouldMergeOneDirection(m2, m1, cache));
}

bool MentionPointerMerger::shouldMergeOneDirection(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {
	std::vector<AttributeValuePair_ptr> features = cache.getMentionFeaturesByName(
			m1, _extractorName, _featureName);

	BOOST_FOREACH(AttributeValuePair_ptr feature, features) {
		boost::shared_ptr<AttributeValuePair<const Mention*> > f = boost::dynamic_pointer_cast<AttributeValuePair<const Mention*> >(feature);
		if (f==0)
			throw InternalInconsistencyException("MentionPointerMerger::shouldMerge",
												 "Expected a Mention*-valued feature");
		const Mention* target = f->getValue();
		if (m2 == target) {
			if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge")) {
				SessionLogger::dbg("MentionGroups_shouldMerge") 
					<< getName() << ": merge " << m1->getUID() << " and " << m2->getUID();
			}
			return true;
		}
	}
	return false;
}
