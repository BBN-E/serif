###############################################################
# Copyright (c) 2011 by Raytheon BBN Technologies Corp.       #
# All Rights Reserved.                                        #
#                                                             #
# Spanish/parse
###############################################################

ADD_SERIF_LIBRARY_SUBDIR(parse
  SOURCE_FILES
    #es_DescOracle.cpp
    #es_DescOracle.h
    #es_NameOracle.cpp
    #es_NameOracle.h

    es_NodeInfo.cpp
    es_NodeInfo.h
    es_SignificantConstitOracle.cpp
    es_SignificantConstitOracle.h

    es_LanguageSpecificFunctions.cpp
    es_LanguageSpecificFunctions.h
    es_STags.cpp
    es_STags.h
    es_WordFeatures.cpp
    es_WordFeatures.h
    ParserTrainer/es_ParserTrainerLanguageSpecificFunctions.cpp
    ParserTrainer/es_ParserTrainerLanguageSpecificFunctions.h
)
