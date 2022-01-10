####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# LearnIt                                                          #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY(LearnIt
  SUBDIRS
    db
    pb
    features
    lbfgs
    util
  SOURCE_FILES
    BestName.h
    BestName.cpp
    Instance.h
    Instance.cpp
    InstanceLoader.h
    InstanceLoader.cpp
    LearnIt2.h
    LearnIt2Matcher.h
    LearnIt2Matcher.cpp
    LearnItAnalyzer.h
    LearnItAnalyzer.cpp
    LearnItPattern.h
    LearnItPattern.cpp
    MainUtilities.h
    MainUtilities.cpp
    MatchInfo.h
    MatchInfo.cpp
    MentionToEntityMap.h
    MentionToEntityMap.cpp
    ObjectWithSlots.h
    ObjectWithSlots.cpp
    ProgramOptionsUtils.h
    ProgramOptionsUtils.cpp
    Seed.h
    Seed.cpp
    SlotConstraints.h
    SlotConstraints.cpp
    SlotFiller.h
    SlotFiller.cpp
    SlotFillerTypes.h
    SlotFillerTypes.cpp
    SlotPairConstraints.h
    Target.h
    Target.cpp
  LINK_LIBRARIES
    Generic
    ActiveLearning
    ${Boost_LIBRARIES}
)
