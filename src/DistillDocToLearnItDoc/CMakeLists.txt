####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# DistillDocToLearnItDoc                                           #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(DistillDocToLearnItDoc
  INSTALL LearnIt
  SOURCE_FILES
    DistillDocToLearnItDoc.cpp
    LearnItDocConverter.cpp
    LearnItDocConverter.h
    PatternIRDocConverter.cpp
    PatternIRDocConverter.h
    PropIRDocConverter.cpp
    PropIRDocConverter.h
    SeedIRDocConverter.cpp
    SeedIRDocConverter.h
  LINK_LIBRARIES
    ActiveLearning
    LearnIt
)
