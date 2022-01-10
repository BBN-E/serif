####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Chinese/edt
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(edt
  SOURCE_FILES
    AliasTable.cpp
    AliasTable.h
    ch_DescLinkFeatureFunctions.cpp
    ch_DescLinkFeatureFunctions.h
    ch_DescLinkFunctions.cpp
    ch_DescLinkFunctions.h
    ch_Guesser.cpp
    ch_Guesser.h
    ch_NameLinkFunctions.cpp
    ch_NameLinkFunctions.h
    ch_PMPronounLinker.cpp
    ch_PMPronounLinker.h
    ch_PronounLinker.cpp
    ch_PronounLinker.h
    ch_PronounLinkerUtils.cpp
    ch_PronounLinkerUtils.h
    ch_RuleDescLinker.cpp
    ch_RuleDescLinker.h
#  PronounLinkerTrainer/ch_PronounLinkerTrainer.cpp # not included
#  PronounLinkerTrainer/ch_PronounLinkerTrainer.h   # not included
    discmodel/ch_DTCorefFeatureTypes.cpp
    discmodel/ch_DTCorefFeatureTypes.h
    discmodel/featuretypes/PLTypeNumberGenderFT.h
    discmodel/featuretypes/PLTypeNumberGenderHWFT.h
    discmodel/featuretypes/PLTypeParentWordFT.h
    discmodel/featuretypes/PLTypeParentWordHWFT.h
    discmodel/featuretypes/PLTypeParentWordWCFT.h
)
