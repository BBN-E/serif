// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/xx_MentionGroupConfiguration.h"

boost::shared_ptr<MentionGroupConfiguration::Factory> &MentionGroupConfiguration::_factory() {
	static boost::shared_ptr<MentionGroupConfiguration::Factory> factory(new GenericMentionGroupConfigurationFactory());
	return factory;
}
