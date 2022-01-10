####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/values                                                   #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(values
  SOURCE_FILES
    DefaultValueRecognizer.cpp      # not included
    DefaultValueRecognizer.h        # not included
    DocValueProcessor.cpp
    DocValueProcessor.h
    PatternEventValueRecognizer.cpp
    PatternEventValueRecognizer.h
    DeprecatedEventValueRecognizer.cpp
    DeprecatedEventValueRecognizer.h
    NumberConverter.cpp
    NumberConverter.h
    PatternValueFinder.cpp
    PatternValueFinder.h
    TemporalNormalizer.cpp
    TemporalNormalizer.h
    ValuePromoter.cpp
    ValuePromoter.h
    ValueRecognizer.cpp
    ValueRecognizer.h
    ValueRuleRepository.h
    ValueRuleRepository.cpp
    xx_DeprecatedEventValueRecognizer.h
    xx_NumberConverter.h
    xx_PatternEventValueRecognizer.h
    xx_TemporalNormalizer.h
    xx_ValueRecognizer.h
)
