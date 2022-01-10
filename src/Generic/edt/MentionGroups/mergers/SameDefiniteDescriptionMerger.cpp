// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/mergers/SameDefiniteDescriptionMerger.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

#include <boost/foreach.hpp>

namespace {
	// Helper function to build the feature extractors' name.
	Symbol makeName(Symbol extractorName, Symbol featureName) {
		std::wostringstream s;
		s << L"SameDefiniteDescription(" << extractorName
		  << L", " << featureName << ")";
		return Symbol(s.str().c_str());
	}
}

SameDefiniteDescriptionMerger::SameDefiniteDescriptionMerger(MentionGroupConstraint_ptr constraints) : 
	PairwiseMentionGroupMerger(Symbol(L"same-definite-description"), constraints) {}

bool SameDefiniteDescriptionMerger::shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const { 
	if (WordConstants::isDefiniteArticle(m1->node->getFirstTerminal()->getTag())) {
		// xx should this be more liberal -- e.g., if they just differ
		// by a comma or something like that, then still allow it?
		if (m1->node->toTextString() ==m2->node->toTextString()) {
			if (SessionLogger::dbg_or_msg_enabled("MentionGroups_shouldMerge"))
				SessionLogger::dbg("MentionGroups_shouldMerge") 
					<< getName() << ": merge " << m1->getUID() << " and " << m2->getUID();
			return true;
		}
	}
	return false;
}
