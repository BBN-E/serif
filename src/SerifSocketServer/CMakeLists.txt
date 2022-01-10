####################################################################
# Copyright (c) 2005 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# SerifSocketServer                                                #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(SerifSocketServer
  SOURCE_FILES
    socket-server.cpp
  WIN_LINK_LIBRARIES
    ws2_32.lib)
