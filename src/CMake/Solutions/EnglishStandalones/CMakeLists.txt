####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/Solutions/EnglishStandalones/CMakeLists.txt                #
#                                                                  #
# This file produces the EnglishStandalones solution.              #
#                                                                  #
####################################################################

ADD_SERIF_SOLUTION(EnglishStandalones
  LANGUAGES English
  PROJECTS ${STANDALONE_PROJECTS}
  DEPENDENCIES
    StandaloneParser:English
)
  
