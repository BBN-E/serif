####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# InstanceFinder                                                   #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(InstanceFinder
  INSTALL LearnIt
  SOURCE_FILES
    InstanceFinder.cpp
  LINK_LIBRARIES
    ActiveLearning
    LearnIt
    GOOGPB_LIBRARY
)
