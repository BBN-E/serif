// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/edt/MentionGroups/en_DTRAMentionGroupConfiguration.h"
#include "Generic/common/Symbol.h"

// Mergers
#include "Generic/edt/MentionGroups/mergers/CompositeMentionGroupMerger.h"
#include "Generic/edt/MentionGroups/mergers/FeatureExactMatchMerger.h"
#include "English/edt/MentionGroups/mergers/en_ParentheticalMerger.h"

// Constraints
#include "Generic/edt/MentionGroups/constraints/CompositeMentionGroupConstraint.h"

// Mention extractors
#include "English/edt/MentionGroups/extractors/en_ParentheticalExtractor.h"

// MentionPair extractors
#include "English/edt/MentionGroups/extractors/en_ParentheticalPairExtractor.h"

EnglishDTRAMentionGroupConfiguration::EnglishDTRAMentionGroupConfiguration() {}

MentionGroupMerger* EnglishDTRAMentionGroupConfiguration::buildMergers() {
	boost::shared_ptr<MentionGroupConstraint> defaultConstraints = buildConstraints(MentionGroupConstraint::DEFAULT);

	CompositeMentionGroupMerger *result = _new CompositeMentionGroupMerger();

	// Parenthetical merging
	result->add(MentionGroupMerger_ptr(_new FeatureExactMatchMerger(Symbol(L"Mention-parenthetical"), Symbol(L"mention-text"), defaultConstraints)));
	result->add(MentionGroupMerger_ptr(_new EnglishParentheticalMerger(boost::make_shared<CompositeMentionGroupConstraint>()))); // No constraints on this special merge because of placeholder EntityType POG

	// Default English mergers
	result->add(MentionGroupMerger_ptr(_defaultConfiguration.buildMergers()));
	
	return result;
}

MentionGroupConstraint_ptr EnglishDTRAMentionGroupConfiguration::buildConstraints(MentionGroupConstraint::Mode mode) {
	CompositeMentionGroupConstraint *result = _new CompositeMentionGroupConstraint();
	result->add(_defaultConfiguration.buildConstraints(mode));
	if (mode == MentionGroupConstraint::DEFAULT) {
	}
	return MentionGroupConstraint_ptr(result);
}

std::vector<AttributeValuePairExtractor<Mention>::ptr_type> EnglishDTRAMentionGroupConfiguration::buildMentionExtractors() {
	std::vector<AttributeValuePairExtractor<Mention>::ptr_type> result = _defaultConfiguration.buildMentionExtractors();
	result.push_back(AttributeValuePairExtractor<Mention>::ptr_type(_new EnglishParentheticalExtractor()));
	return result;	
}

std::vector<AttributeValuePairExtractor<MentionPair>::ptr_type> EnglishDTRAMentionGroupConfiguration::buildMentionPairExtractors() {
	std::vector<AttributeValuePairExtractor<MentionPair>::ptr_type> result = _defaultConfiguration.buildMentionPairExtractors();
	result.push_back(AttributeValuePairExtractor<MentionPair>::ptr_type(_new EnglishParentheticalPairExtractor()));
	return result;	
}
