// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/PreLinker.h"
#include "Generic/edt/xx_PreLinker.h"

boost::shared_ptr<PreLinker::PreLinkerInstance> &PreLinker::getInstance() {
    static boost::shared_ptr<PreLinkerInstance> instance(_new PreLinkerInstanceFor<GenericPreLinker>());
    return instance;
}
