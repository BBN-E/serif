####################################################################
# Copyright (c) 2011 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/Solutions/Serif/CMakeLists.txt                             #
#                                                                  #
# Responsible for creating Serif.sln                               #
# Only for Windows                                                 #
####################################################################

SET(SERIF_PROJECTS Serif SerifTest ActiveLearning LearnIt Temporal)

ADD_SERIF_SOLUTION(Serif
  PROJECTS ${SERIF_PROJECTS})
