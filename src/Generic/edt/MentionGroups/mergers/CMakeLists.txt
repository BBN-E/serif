####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/edt/MentionGroups/mergers                                #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(mergers
  SOURCE_FILES
    AcronymMerger.cpp
    AcronymMerger.h
    AppositiveMerger.cpp
    AppositiveMerger.h
    CombinedP1RankingMerger.cpp
    CombinedP1RankingMerger.h
    CompositeMentionGroupMerger.cpp
    CompositeMentionGroupMerger.h
    CopulaMerger.cpp
    CopulaMerger.h
    DummyMentionGroupMerger.h
    FeatureExactMatchMerger.cpp
    FeatureExactMatchMerger.h
    FeatureUniqueMatchMerger.cpp
    FeatureUniqueMatchMerger.h
    FirstPersonPronounToSpeakerMerger.cpp
    FirstPersonPronounToSpeakerMerger.h
    MentionPointerMerger.cpp
    MentionPointerMerger.h
    P1RankingMerger.cpp
    P1RankingMerger.h
    PairwiseMentionGroupMerger.cpp
    PairwiseMentionGroupMerger.h
    SameDefiniteDescriptionMerger.cpp
    SameDefiniteDescriptionMerger.h
    SecondPersonPronounMerger.cpp
    SecondPersonPronounMerger.h
    SecondPersonPronounToSpeakerMerger.cpp
    SecondPersonPronounToSpeakerMerger.h
    TitleMerger.cpp
    TitleMerger.h
)
