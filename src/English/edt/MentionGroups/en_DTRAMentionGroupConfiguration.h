// Copyright (c) 2014 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_DTRA_MENTION_GROUP_CONFIGURATION_H
#define EN_DTRA_MENTION_GROUP_CONFIGURATION_H

#include "Generic/edt/MentionGroups/MentionGroupConfiguration.h"
#include "English/edt/MentionGroups/en_MentionGroupConfiguration.h"

class EnglishDTRAMentionGroupConfiguration: public MentionGroupConfiguration {
public:
	EnglishDTRAMentionGroupConfiguration();
	
	MentionGroupMerger* buildMergers();
	MentionGroupConstraint_ptr buildConstraints(MentionGroupConstraint::Mode mode);
	std::vector<AttributeValuePairExtractor<Mention>::ptr_type> buildMentionExtractors();
	std::vector<AttributeValuePairExtractor<MentionPair>::ptr_type> buildMentionPairExtractors();
private:
	EnglishMentionGroupConfiguration _defaultConfiguration;
};

class EnglishDTRAMentionGroupConfigurationFactory: public MentionGroupConfiguration::Factory {
	virtual MentionGroupConfiguration *build() { return new EnglishDTRAMentionGroupConfiguration(); }
};

#endif
