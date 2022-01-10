// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/PNPChunking/NPChunkFinder.h"
#include "Generic/PNPChunking/xx_NPChunkFinder.h"




boost::shared_ptr<NPChunkFinder::Factory> &NPChunkFinder::_factory() {
	static boost::shared_ptr<NPChunkFinder::Factory> factory(new GenericNPChunkFinderFactory());
	return factory;
}

