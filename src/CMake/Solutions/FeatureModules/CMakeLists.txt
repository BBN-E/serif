####################################################################
# Copyright (c) 2011 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/Solutions/FeatureModules/CMakeLists.txt                    #
#                                                                  #
# Responsible for creating FeatureModules.sln                      #
# Only for Windows                                                 #
####################################################################

SET(FEATURE_MODULE_LIBS)
FOREACH(m ${ALL_FEATURE_MODULES})
    LIST(APPEND FEATURE_MODULE_LIBS "${m}_MODULE")
ENDFOREACH()

ADD_SERIF_SOLUTION(FeatureModules
  NO_STATIC_FEATURE_MODULES
  #NO_GENERIC
  PROJECTS 
    ${FEATURE_MODULE_LIBS})
