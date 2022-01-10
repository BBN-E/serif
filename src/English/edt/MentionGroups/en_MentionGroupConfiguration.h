// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_MENTION_GROUP_CONFIGURATION_H
#define EN_MENTION_GROUP_CONFIGURATION_H

#include "Generic/edt/MentionGroups/MentionGroupConfiguration.h"
#include "Generic/edt/MentionGroups/xx_MentionGroupConfiguration.h"

class EnglishMentionGroupConfiguration: public MentionGroupConfiguration {
public:
	EnglishMentionGroupConfiguration();
	
	MentionGroupMerger* buildMergers();
	MentionGroupConstraint_ptr buildConstraints(MentionGroupConstraint::Mode mode);
	std::vector<AttributeValuePairExtractor<Mention>::ptr_type> buildMentionExtractors();
	std::vector<AttributeValuePairExtractor<MentionPair>::ptr_type> buildMentionPairExtractors();
private:
	GenericMentionGroupConfiguration _defaultConfiguration;
};

class EnglishMentionGroupConfigurationFactory: public MentionGroupConfiguration::Factory {
	virtual MentionGroupConfiguration *build() { return new EnglishMentionGroupConfiguration(); }
};

#endif
