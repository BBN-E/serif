####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# TemporalDocToFV                                                  #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(TemporalDocToFV
  INSTALL LearnIt
  SOURCE_FILES
    TemporalDocToFV.cpp
  LINK_LIBRARIES
    ActiveLearning
    LearnIt
    Temporal
)
