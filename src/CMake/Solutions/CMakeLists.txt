####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/Solutions/CMakeLists.txt                                   #
#                                                                  #
# Responsible for creating the solution files.                     #
####################################################################

IF(WIN)
   SET(SOLUTIONS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

   # Solutions that include all languages
   ADD_SUBDIRECTORY(Serif)

   # Solution for building dynamically linked feature module DLLs
   ADD_SUBDIRECTORY(FeatureModules)

   IF (CREATE_STANDALONE_PROJECTS)
     ADD_SUBDIRECTORY(ATEASerif)
     ADD_SUBDIRECTORY(PIdFQuickLearnServer)
     ADD_SUBDIRECTORY(Cluster)
     ADD_SUBDIRECTORY(Standalones)
     ADD_SUBDIRECTORY(ParserTrainer)
     ADD_SUBDIRECTORY(SerifSocketServer)
   ENDIF (CREATE_STANDALONE_PROJECTS)

   IF (CREATE_BRANDY_PROJECTS)
     ADD_SUBDIRECTORY(Brandy)
   ENDIF (CREATE_BRANDY_PROJECTS)

   IF (CREATE_PROFILE_GENERATOR_SOLUTION)
     ADD_SUBDIRECTORY(ProfileGenerator)
   ENDIF (CREATE_PROFILE_GENERATOR_SOLUTION)

ENDIF(WIN)
