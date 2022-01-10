// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/NameLinkFunctions.h"
#include "Generic/edt/xx_NameLinkFunctions.h"

boost::shared_ptr<NameLinkFunctions::NameLinkFunctionsInstance> &NameLinkFunctions::getInstance() {
    static boost::shared_ptr<NameLinkFunctionsInstance> instance(_new NameLinkFunctionsInstanceFor<GenericNameLinkFunctions>());
    return instance;
}
