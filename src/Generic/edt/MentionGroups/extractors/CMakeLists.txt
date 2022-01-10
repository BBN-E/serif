####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/edt/MentionGroups/extractors                             #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(extractors
  SOURCE_FILES
    AcronymExtractor.cpp
    AcronymExtractor.h
    ConfidentActorMatchExtractor.cpp
    ConfidentActorMatchExtractor.h
    EditDistanceExtractor.cpp
    EditDistanceExtractor.h
    GenderFeatureExtractor.cpp
    GenderFeatureExtractor.h
    GPEAffiliationExtractor.cpp
    GPEAffiliationExtractor.h
    HeadWordExtractor.cpp
    HeadWordExtractor.h
    NormalizedNameFeatureExtractor.cpp
    NormalizedNameFeatureExtractor.h
    NumberFeatureExtractor.cpp
    NumberFeatureExtractor.h
    PropositionExtractor.cpp
    PropositionExtractor.h
    SharedPropositionExtractor.cpp
    SharedPropositionExtractor.h

    # These are included in the Generic module, rather than the
    # English module, because non-English documents often use English
    # naming conventions when mentioning names of English-speaking
    # people.
    EnglishPersonNameVariationsExtractor.cpp
    EnglishPersonNameVariationsExtractor.h
)
