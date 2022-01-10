####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Arabic/BuckWalter
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(BuckWalter
  SOURCE_FILES
    ar_BuckWalterFunctions.cpp
    ar_BuckWalterFunctions.h
    ar_BuckWalterizer.cpp
    ar_BuckWalterizer.h
    ar_BuckWalterLimits.h
    ar_BWDictionaryReader.cpp
    ar_BWDictionaryReader.h
    ar_BWNormalizer.cpp
    ar_BWNormalizer.h
    ar_BWRuleDictionary.cpp
    ar_BWRuleDictionary.h
#  ar_BWRuleDictionaryReader.cpp    # not included
#  ar_BWRuleDictionaryReader.h      # not included
    ar_FeatureValueStructure.cpp
    ar_FeatureValueStructure.h
    ar_LexiconFactory.h
    ar_MorphologicalAnalyzer.cpp
    ar_MorphologicalAnalyzer.h
    ar_ParseSeeder.cpp
    ar_ParseSeeder.h
    ar_Retokenizer.cpp
    ar_Retokenizer.h
)
