// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/parse/xx_LanguageSpecificFunctions.h"

boost::shared_ptr<LanguageSpecificFunctions::LanguageSpecificFunctionsInstance> &LanguageSpecificFunctions::getInstance() {
    static boost::shared_ptr<LanguageSpecificFunctionsInstance> instance(_new LanguageSpecificFunctionsInstanceFor<GenericLanguageSpecificFunctions>());
    return instance;
}
