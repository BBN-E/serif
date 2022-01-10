####################################################################
# Copyright (c) 2016 by Raytheon BBN Technologies                  #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/profile-generator.cmake                                    #
#                                                                  #
# Adding executables for ProfileGenerator solution                 #
####################################################################

SET(CREATE_PROFILE_GENERATOR_SOLUTION OFF CACHE BOOL
    "Build the ProfileGenerator solution")
MARK_AS_ADVANCED (CREATE_PROFILE_GENERATOR_SOLUTION)

IF (CREATE_PROFILE_GENERATOR_SOLUTION)

    IF (NOT USE_POSTGRES)
        MESSAGE( WARNING "When building ProfileGenerator, you
        probably want to enable USE_POSTGRES" )
    ENDIF ()

    ADD_SUBDIRECTORY(ProfileGenerator)

ENDIF (CREATE_PROFILE_GENERATOR_SOLUTION)
