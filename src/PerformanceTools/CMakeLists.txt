###############################################################
# Copyright 2016 by Raytheon BBN Technologies Corp            #
# All Rights Reserved                                         #
#                                                             #
# PerformanceTools                                            #
# Wrapper for gperftools.                                     #
###############################################################

IF(FEATUREMODULE_PerformanceTools_INCLUDED)
  # Locate gperftools based on environment and append its found
  # include directory
  SET(Gperftools_ROOT_DIR $ENV{GPERFTOOLS_ROOT})
  FIND_PACKAGE(Gperftools REQUIRED)
  INCLUDE_DIRECTORIES(${GPERFTOOLS_INCLUDE_DIR})
ENDIF()

ADD_SERIF_FEATURE_MODULE(PerformanceTools
  EXCLUDED_BY_DEFAULT
  SOURCE_FILES
    PerformanceToolsModule.cpp
    PerformanceToolsModule.h
  LINK_LIBRARIES
    ${GPERFTOOLS_LIBRARIES}
)
