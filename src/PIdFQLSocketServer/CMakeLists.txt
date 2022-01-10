####################################################################
# Copyright (c) 2005 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# PIdFQuickLearn                                                   #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(PIdFQLSocketServer
  SOURCE_FILES
    PIdFQLSocketServer.cpp
  WIN_LINK_LIBRARIES
    ws2_32
)
