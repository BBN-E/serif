####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# L2Analyzer                                                       #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(L2Analyzer
  INSTALL LearnIt
  SOURCE_FILES
    L2Analyzer.cpp
  LINK_LIBRARIES
    ActiveLearning
    LearnIt
)
