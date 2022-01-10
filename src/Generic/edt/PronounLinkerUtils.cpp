// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/PronounLinkerUtils.h"
#include "Generic/edt/xx_PronounLinkerUtils.h"

boost::shared_ptr<PronounLinkerUtils::PronounLinkerUtilsInstance> &PronounLinkerUtils::getInstance() {
    static boost::shared_ptr<PronounLinkerUtilsInstance> instance(_new PronounLinkerUtilsInstanceFor<GenericPronounLinkerUtils>());
    return instance;
}
