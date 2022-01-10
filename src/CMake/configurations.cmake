###########################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                                #
# All Rights Reserved.                                                    #
#                                                                         #
# CMake/configurations.cmake                                              #
#                                                                         #
# Set up the configurations.  Currently, there are two configurations:    #
# Release & Debug.                                                        #
#                                                                         #
# On UNIX, the CMAKE_BUILD_TYPE is used to select the configuration, and  #
# must be set to one of the following values:                             #
#                                                                         #
#   * Release .............. Release configuration (optimized)            #
#   * Debug ................ Debug configuration                          #
#   * RelWithDebInfo ....... Uses the Release configuration, but adds     #
#                            the -g flag to add debug info.               #
#   * GProf ................ Uses the Release configuration, but adds     #
#                            the -pg flag to add gprof output.            #
#                                                                         #
###########################################################################

# There are only two configuration types now:
SET (CMAKE_CONFIGURATION_TYPES "Release;Debug;RelWithDebInfo" CACHE STRING 
     "List of available configuration types" FORCE)

# On UNIX, check to make sure that they specified a build type.
IF (UNIX)
    IF (NOT CMAKE_BUILD_TYPE)
        MESSAGE(FATAL_ERROR "You must specify a value for CMAKE_BUILD_TYPE")
    ENDIF (NOT CMAKE_BUILD_TYPE)

    IF (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        SET(CMAKE_BUILD_TYPE Release)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
        SET (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    ENDIF()

    IF (CMAKE_BUILD_TYPE STREQUAL "GProf")
        SET(CMAKE_BUILD_TYPE Release)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
        SET (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
    ENDIF()

    LIST(FIND CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} is_valid_build_type)
    IF (is_valid_build_type LESS 0)
        MESSAGE(FATAL_ERROR "\"${CMAKE_BUILD_TYPE}\" is not a valid build "
	    "type.  Valid build types are: ${CMAKE_CONFIGURATION_TYPES}")
    ENDIF()
ENDIF (UNIX)
