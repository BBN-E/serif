// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_MENTION_GROUP_CONFIGURATION_H
#define XX_MENTION_GROUP_CONFIGURATION_H

#include "Generic/edt/MentionGroups/MentionGroupConfiguration.h"

class GenericMentionGroupConfiguration: public MentionGroupConfiguration {
public:
	GenericMentionGroupConfiguration();
	
	MentionGroupMerger* buildMergers();
	MentionGroupMerger* buildMergers(MentionGroupConstraint_ptr defaultConstraints,
	                                 MentionGroupConstraint_ptr hpConstraints);
	
	MentionGroupConstraint_ptr buildConstraints(MentionGroupConstraint::Mode mode);
	std::vector<AttributeValuePairExtractor<Mention>::ptr_type> buildMentionExtractors();
	std::vector<AttributeValuePairExtractor<MentionPair>::ptr_type> buildMentionPairExtractors();
};

class GenericMentionGroupConfigurationFactory: public MentionGroupConfiguration::Factory {
	virtual MentionGroupConfiguration *build() { return new GenericMentionGroupConfiguration(); }
};

#endif
