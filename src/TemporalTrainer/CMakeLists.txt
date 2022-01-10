####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# TemporalTrainer                                                  #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(TemporalTrainer
  INSTALL LearnIt
  SOURCE_FILES
    TemporalTrainer.h
    TemporalTrainer.cpp
    TemporalTrainer.cpp
    TemporalTrainerDriver.cpp
    TemporalTrainerUI.h
    TemporalTrainerUI.cpp
  LINK_LIBRARIES
    ActiveLearning
    LearnIt
    Temporal
  WIN_LINK_LIBRARIES
    ws2_32
)
