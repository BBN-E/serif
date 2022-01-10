####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/edt                                                      #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(edt
  SOURCE_FILES
    AbbrevMaker.cpp
    AbbrevMaker.h
    AbbrevTable.cpp
    AbbrevTable.h
    AcronymMaker.cpp
    AcronymMaker.h
#    ApposLinker.cpp
#    ApposLinker.h
    CorefUtilities.h
    CorefUtilities.cpp
    CountsTable.cpp
    CountsTable.h
    DescLinkEvent.h
    DescLinkFeatureFunctions.cpp
    DescLinkFeatureFunctions.h
    DescLinkFunctions.cpp
    DescLinkFunctions.h
    DescLinker.cpp
    DescLinker.h
    DummyReferenceResolver.h
    EntityGuess.h
#    FullEntitySetCache.cpp
#    FullEntitySetCache.h
    Guesser.h
    Guesser.cpp
    HobbsDistance.cpp
    HobbsDistance.h
    LexDataCache.cpp
    LexDataCache.h
    LexEntity.cpp
    LexEntity.h
    LexEntitySet.cpp
    LexEntitySet.h
    LinkGuess.h
    LinkerTreeSearch.cpp
    LinkerTreeSearch.h
    MentionLinker.h
    NameLinkFunctions.cpp
    NameLinkFunctions.h
#    NameLinker.cpp
    NameLinker.h
#    NameToDescriptorMap.cpp
#    NameToDescriptorMap.h
    OutsideDescLinker.cpp
    OutsideDescLinker.h
    PreLinker.cpp
    PreLinker.h
    PronounLinker.cpp
    PronounLinker.h
    PronounLinkerUtils.h
    PronounLinkerUtils.cpp
    ReferenceResolver.cpp
    ReferenceResolver.h
    RuleDescLinker.cpp
    RuleDescLinker.h
    RuleNameLinker.cpp
    RuleNameLinker.h
    SimpleQueue.h
    SimpleRuleNameLinker.cpp
    SimpleRuleNameLinker.h
    StatDescLinker.cpp
    StatDescLinker.h
    StatNameLinker.cpp
    StatNameLinker.h
#    TreeSearch.cpp
    xx_AcronymMaker.h
    xx_AcronymMaker.cpp
    xx_DescLinkFeatureFunctions.cpp
    xx_DescLinkFeatureFunctions.h
    xx_DescLinkFunctions.h
#    xx_DescLinker.h
    xx_Guesser.h
    xx_NameLinkFunctions.h
    xx_PreLinker.cpp
    xx_PreLinker.h
    xx_PronounLinker.h
    xx_PronounLinkerUtils.h
    xx_RuleDescLinker.h
    xx_RuleNameLinker.h
  SUBDIRS
    DescriptorLinkerTrainer
    MentionGroups
    NameLinkerTrainer
    PronounLinkerTrainer
    discmodel
)
