// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/ParserTrainer/ParserTrainerLanguageSpecificFunctions.h"
#include "Generic/parse/ParserTrainer/xx_ParserTrainerLanguageSpecificFunctions.h"

boost::shared_ptr<ParserTrainerLanguageSpecificFunctions::ParserTrainerLanguageSpecificFunctionsInstance> &ParserTrainerLanguageSpecificFunctions::getInstance() {
    static boost::shared_ptr<ParserTrainerLanguageSpecificFunctionsInstance> instance(_new ParserTrainerLanguageSpecificFunctionsInstanceFor<GenericParserTrainerLanguageSpecificFunctions>());
    return instance;
}
