// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/SymbolUtilities.h"
#include "Generic/common/xx_SymbolUtilities.h"

boost::shared_ptr<SymbolUtilities::SymbolUtilitiesInstance> &SymbolUtilities::getInstance() {
    static boost::shared_ptr<SymbolUtilitiesInstance> instance(_new SymbolUtilitiesInstanceFor<GenericSymbolUtilities>());
    return instance;
}


