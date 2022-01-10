####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# CombineTemporalTraining                                          #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(CombineTemporalTraining
  INSTALL LearnIt
  SOURCE_FILES
    CombineTemporalTraining.cpp
  LINK_LIBRARIES
    ActiveLearning
    LearnIt
    Temporal
)
