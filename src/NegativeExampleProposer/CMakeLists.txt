####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# NegativeExampleProposer                                          #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(NegativeExampleProposer
  INSTALL LearnIt
  SOURCE_FILES
    NegativeExampleProposer.cpp
  LINK_LIBRARIES
    ActiveLearning
    LearnIt
)
