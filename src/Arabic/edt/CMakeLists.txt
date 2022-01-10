####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Arabic/edt
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(edt
  SOURCE_FILES
    ar_BaseFormMaker.cpp
    ar_BaseFormMaker.h
    ar_DescLinkFeatureFunctions.cpp
    ar_DescLinkFeatureFunctions.h
    ar_DescLinker.cpp   
    ar_DescLinker.h 
    ar_NameLinkFunctions.cpp
    ar_NameLinkFunctions.h
    ar_PreLinker.cpp
    ar_PreLinker.h
    ar_PronounLinker.cpp
    ar_PronounLinker.h
    ar_RuleDescLinker.cpp
    ar_RuleDescLinker.h
    discmodel/ar_DTCorefFeatureTypes.cpp
    discmodel/ar_DTCorefFeatureTypes.h
    discmodel/featuretypes/AdjNationality.h
)
