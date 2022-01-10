// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/parse/xx_NodeInfo.h"

boost::shared_ptr<NodeInfo::NodeInfoInstance> &NodeInfo::getInstance() {
	static boost::shared_ptr<NodeInfoInstance> instance(_new NodeInfoInstanceFor<GenericNodeInfo>());
	return instance;
}

