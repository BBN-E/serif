###############################################################
# Copyright (c) 2011 by Raytheon BBN Technologies Corp.       #
# All Rights Reserved.                                        #
#                                                             #
# English/edt
###############################################################

ADD_SERIF_LIBRARY_SUBDIR(edt
  SOURCE_FILES
    en_DescLinkFeatureFunctions.cpp
    en_DescLinkFeatureFunctions.h
    en_DescLinkFunctions.cpp
    en_DescLinkFunctions.h
    en_DescLinker.cpp
    en_DescLinker.h
    en_Guesser.cpp
    en_Guesser.h
    en_NameLinkFunctions.cpp
    en_NameLinkFunctions.h
    en_PMPronounLinker.cpp
    en_PMPronounLinker.h
    en_PreLinker.cpp
    en_PreLinker.h
    en_PronounLinker.cpp
    en_PronounLinker.h
    en_PronounLinkerUtils.cpp
    en_PronounLinkerUtils.h
    en_RuleDescLinker.cpp
    en_RuleDescLinker.h
    en_RuleNameLinker.cpp
    en_RuleNameLinker.h
  SUBDIRS
#    PronounLinkerTrainer # this should not be included
    discmodel
    MentionGroups
)
