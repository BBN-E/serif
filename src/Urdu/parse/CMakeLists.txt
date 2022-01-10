###############################################################
# Copyright (c) 2011 by Raytheon BBN Technologies Corp.       #
# All Rights Reserved.                                        #
#                                                             #
# Urdu/parse
###############################################################

ADD_SERIF_LIBRARY_SUBDIR(parse
  SOURCE_FILES
    #es_DescOracle.cpp
    #es_DescOracle.h
    #es_NameOracle.cpp
    #es_NameOracle.h

    # es_NodeInfo.cpp
    # es_NodeInfo.h
    # es_SignificantConstitOracle.cpp
    # es_SignificantConstitOracle.h

    ur_LanguageSpecificFunctions.cpp
    ur_LanguageSpecificFunctions.h
    ur_STags.cpp
    ur_STags.h
    ur_WordFeatures.cpp
    ur_WordFeatures.h
    ParserTrainer/ur_ParserTrainerLanguageSpecificFunctions.cpp
    ParserTrainer/ur_ParserTrainerLanguageSpecificFunctions.h
)
