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
    es_DescLinkFeatureFunctions.cpp
    es_DescLinkFeatureFunctions.h
    es_DescLinkFunctions.cpp
    es_DescLinkFunctions.h
    es_Guesser.cpp
    es_Guesser.h
    es_NameLinkFunctions.cpp
    es_NameLinkFunctions.h
    es_PMPronounLinker.cpp
    es_PMPronounLinker.h
    es_PronounLinker.cpp
    es_PronounLinker.h
    es_PronounLinkerUtils.cpp
    es_PronounLinkerUtils.h
    es_RuleDescLinker.cpp
    es_RuleDescLinker.h
    es_RuleNameLinker.cpp
    es_RuleNameLinker.h
    discmodel/es_DTCorefFeatureTypes.cpp
    discmodel/es_DTCorefFeatureTypes.h
    discmodel/featuretypes/PLTypeNumberGenderFT.h
    discmodel/featuretypes/PLTypeNumberGenderHWFT.h
    discmodel/featuretypes/PLTypeParentWordFT.h
    discmodel/featuretypes/PLTypeParentWordHWFT.h
    discmodel/featuretypes/PLTypeParentWordWCFT.h
  SUBDIRS
    MentionGroups
)
