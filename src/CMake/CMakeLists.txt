####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/CMakeList.txt file                                         #
#                                                                  #
####################################################################

#############################
#  add CMake/*.cmake files  #
#############################
SET (CMAKE_FILES configurations.cmake definitions.cmake 
		 macro.cmake FindBoost.cmake 
		 standalones-includes.cmake
		 dummy.cpp create_shortcut.pl)

SOURCE_GROUP("CMake" FILES ${CMAKE_FILES})

##########################################################
#  add CMake/Solutions/${Solution}/CMakeLists.txt files  #
##########################################################
SET (CMAKE_SOLUTION_FILES
		 Solutions/CMakeLists.txt
		 Solutions/Serif/CMakeLists.txt

		 Solutions/EnglishStandalones/CMakeLists.txt
		 Solutions/ArabicStandalones/CMakeLists.txt
		 Solutions/ChineseStandalones/CMakeLists.txt
#		 Solutions/KoreanStandalones/CMakeLists.txt
#		 Solutions/SpanishStandalones/CMakeLists.txt

		 Solutions/EnglishSerifSocketServer/CMakeLists.txt
		 Solutions/ArabicSerifSocketServer/CMakeLists.txt
		 Solutions/ChineseSerifSocketServer/CMakeLists.txt

		 Solutions/PIdFQuickLearnServer/CMakeLists.txt

		 Solutions/Cluster/CMakeLists.txt

		 Solutions/EnglishParserTrainer/CMakeLists.txt
		 Solutions/ArabicParserTrainer/CMakeLists.txt
		 Solutions/ChineseParserTrainer/CMakeLists.txt

		 Solutions/ATEASerif/CMakeLists.txt

       Solutions/Brandy/CMakeLists.txt
)


# generate the local project directory structure
GENERATE_PROJECT ( Configurations "${CMAKE_SOLUTION_FILES}" )
SOURCE_GROUP("CMake\\" FILES CMakeLists.txt)

# Add the main CMakeLists.txt file to this project
SET (MAIN_CMAKELISTS_FILE ${PROJECT_SOURCE_DIR}/CMakeLists.txt)

LIST (APPEND CMAKE_FILES ${CMAKE_SOLUTION_FILES})

# make sure they don't need to compile
FOREACH (FILE ${CMAKE_FILES})
   SET_SOURCE_FILES_PROPERTIES(${FILE} PROPERTIES HEADER_FILE_ONLY true)
ENDFOREACH (FILE ${CMAKE_FILES})


############################################
#  Add all the dynamically included files  #
############################################
SET (INCLUDE_DYNAMIC_INCLUDES_FILES   OFF  CACHE BOOL "Add dynamic_includes files to the solution")
MARK_AS_ADVANCED (INCLUDE_DYNAMIC_INCLUDES_FILES)
IF (INCLUDE_DYNAMIC_INCLUDES_FILES)
   FOREACH (FILE ${DYNAMIC_INCLUDES_FILES})
      SET_SOURCE_FILES_PROPERTIES(${FILE} PROPERTIES HEADER_FILE_ONLY true)
   ENDFOREACH (FILE ${DYNAMIC_INCLUDES_FILES})

   LIST(LENGTH DYNAMIC_INCLUDES_FILES N_DYNAMIC_INCLUDES_FILES)
   # to account for the initial ';' in the string
   MATH(EXPR N_DYNAMIC_INCLUDES_FILES ${N_DYNAMIC_INCLUDES_FILES}-1)
   FOREACH (INDX RANGE 0 ${N_DYNAMIC_INCLUDES_FILES})
      LIST(GET DYNAMIC_INCLUDES_FILES ${INDX} FILE) 
      LIST(GET DYNAMIC_INCLUDES_DIRS ${INDX} DIR)
      SOURCE_GROUP("dynamic_includes\\${DIR}" FILES ${FILE})
   ENDFOREACH (INDX)
ELSE (INCLUDE_DYNAMIC_INCLUDES_FILES)
   # don't add the files (since they appear as new files)
   INITIALIZE_INTERNAL_LIST (DYNAMIC_INCLUDES_FILES)
   INITIALIZE_INTERNAL_LIST (DYNAMIC_INCLUDES_DIRS)
ENDIF (INCLUDE_DYNAMIC_INCLUDES_FILES)


### create the project ###
ADD_CUSTOM_TARGET(Configurations 
    DEPENDS
        ${DYNAMIC_INCLUDES_FILES}
	${CMAKE_FILES} 
	${MAIN_CMAKELISTS_FILE})


############################################
#  Creates all the solutions (on Windows)  #
############################################
IF (WIN)
   SET(CREATE_SOLUTIONS ON CACHE BOOL "Create Solutions files")
   MARK_AS_ADVANCED (CREATE_SOLUTIONS)
   IF (CREATE_SOLUTIONS)
      ADD_SUBDIRECTORY(Solutions)
   ENDIF (CREATE_SOLUTIONS)
ENDIF (WIN)
