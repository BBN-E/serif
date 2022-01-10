// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ES_MENTION_GROUP_CONFIGURATION_H
#define ES_MENTION_GROUP_CONFIGURATION_H

#include "Generic/edt/MentionGroups/MentionGroupConfiguration.h"
#include "Generic/edt/MentionGroups/xx_MentionGroupConfiguration.h"

class SpanishMentionGroupConfiguration: public MentionGroupConfiguration {
public:
	SpanishMentionGroupConfiguration();
	
	MentionGroupMerger* buildMergers();
	MentionGroupConstraint_ptr buildConstraints(MentionGroupConstraint::Mode mode);
	std::vector<AttributeValuePairExtractor<Mention>::ptr_type> buildMentionExtractors();
	std::vector<AttributeValuePairExtractor<MentionPair>::ptr_type> buildMentionPairExtractors();
private:
	GenericMentionGroupConfiguration _defaultConfiguration;
};

class SpanishMentionGroupConfigurationFactory: public MentionGroupConfiguration::Factory {
	virtual MentionGroupConfiguration *build() { return new SpanishMentionGroupConfiguration(); }
};

#endif
