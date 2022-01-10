// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/descriptors/discmodel/featuretypes/RareHWListFT.h"

bool RareHWListFT::_initialized = false;
RareHWListFT::Table* RareHWListFT::_hwList = 0;
