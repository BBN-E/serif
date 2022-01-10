####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/edt/MentionGroups/constraints                            #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(constraints
  SOURCE_FILES
    ActorMatchConstraint.cpp
    ActorMatchConstraint.h
    CompositeMentionGroupConstraint.cpp
    CompositeMentionGroupConstraint.h
    DummyMentionGroupConstraint.h
    EntityTypeConstraint.cpp
    EntityTypeConstraint.h
    GenderClashConstraint.cpp
    GenderClashConstraint.h
    GlobalGPEAffiliationConstraint.cpp
    GlobalGPEAffiliationConstraint.h
    HeadWordClashConstraint.cpp
    HeadWordClashConstraint.h
    LocalGPEAffiliationConstraint.cpp
    LocalGPEAffiliationConstraint.h
    LocationNameOverlapConstraint.cpp
    LocationNameOverlapConstraint.h
    NumberClashConstraint.cpp
    NumberClashConstraint.h
    OperatorClashConstraint.cpp
    OperatorClashConstraint.h
    PairFeatureExistenceConstraint.cpp
    PairFeatureExistenceConstraint.h
    PairwiseMentionGroupConstraint.cpp
    PairwiseMentionGroupConstraint.h
    PartitiveConstraint.cpp
    PartitiveConstraint.h
)
