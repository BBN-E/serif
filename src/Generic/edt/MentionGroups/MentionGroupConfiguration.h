// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTION_GROUP_CONFIGURATION_H
#define MENTION_GROUP_CONFIGURATION_H

#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/edt/MentionGroups/MentionGroupConstraint.h"

#include <boost/shared_ptr.hpp>
#include <vector>

class Mention;
class MentionGroupMerger;

typedef std::pair<const Mention*, const Mention*> MentionPair;

class MentionGroupConfiguration {
public:
	/** Create and return a new MentionGroupConfiguration. */
	static MentionGroupConfiguration *build() { return _factory()->build(); }
	/** Hook for registering new MentionGroupConfiguration factories */
	struct Factory { virtual MentionGroupConfiguration *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~MentionGroupConfiguration() {}

	virtual MentionGroupMerger* buildMergers() = 0;
	virtual MentionGroupConstraint_ptr buildConstraints(MentionGroupConstraint::Mode mode) = 0;
	virtual std::vector<AttributeValuePairExtractor<Mention>::ptr_type> buildMentionExtractors() = 0;
	virtual std::vector<AttributeValuePairExtractor<MentionPair>::ptr_type> buildMentionPairExtractors() = 0;

protected:
	MentionGroupConfiguration() {}

private:
	static boost::shared_ptr<Factory> &_factory();
};

#endif

