####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/Solutions/PIdFQuickLearnServer/CMakeLists.txt              #
#                                                                  #
# This file produces the PIdFQuickLearnServer solution.            #
#                                                                  #
####################################################################

# Only add this solution if we build PIdFQuickLearnServer.
LIST(FIND STANDALONE_PROJECTS PIdFQLSocketServer PIdFQLSocketServer_index)
IF(NOT PIdFQLSocketServer_index EQUAL -1)

  ADD_SERIF_SOLUTION(PIdFQuickLearnServer
    PROJECTS PIdFQLSocketServer)

ENDIF(NOT PIdFQLSocketServer_index EQUAL -1)
