####################################################################
# Copyright (c) 2011 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/Solutions/ATEASerif/CMakeLists.txt                         #
#                                                                  #
# This file produces the ATEASerif solution.                       #
#                                                                  #
####################################################################

ADD_SERIF_SOLUTION(ATEASerif
  PROJECTS 
    ATEASerif_Generic 
    ATEASerif_English
    ATEASerif_Batch_English
  DEPENDENCIES 
    ATEASerif_English:ATEASerif_Generic
    ATEASerif_Batch_English:ATEASerif_Generic)
