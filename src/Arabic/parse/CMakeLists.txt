####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Arabic/parse 
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(parse
  SOURCE_FILES
    ar_BWChartDecoder.cpp
    ar_BWChartDecoder.h
#  ar_ChartDecoder.cpp
#  ar_ChartDecoder.h
    ar_ChartEntry.h
    ar_DescOracle.cpp
    ar_DescOracle.h
    ar_LanguageSpecificFunctions.cpp
    ar_LanguageSpecificFunctions.h
    ar_NameOracle.cpp
    ar_NameOracle.h
    ar_NodeInfo.cpp
    ar_NodeInfo.h
    ar_Parser.cpp
    ar_Parser.h
    ar_STags.cpp
    ar_STags.h
#  ar_Segment.cpp
#  ar_Segment.h
    ar_SignificantConstitOracle.cpp
    ar_SignificantConstitOracle.h
#  ar_SplitTokenSequence.cpp
#  ar_SplitTokenSequence.h
    ar_WordFeatures.cpp
    ar_WordFeatures.h
#  ar_WordSegment.cpp
#  ar_WordSegment.h
    ParserTrainer/ar_ParserTrainerLanguageSpecificFunctions.cpp
    ParserTrainer/ar_ParserTrainerLanguageSpecificFunctions.h
)
