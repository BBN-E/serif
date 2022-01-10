####################################################################
####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Main CMakeList.txt file                                          #
#                                                                  #
####################################################################
####################################################################

# Require CMake 2.8+
cmake_minimum_required(VERSION 2.8)

# Set up configurations (such as Release and Debug).  This must be
# done before we create SERIF_PROJECT.
INCLUDE ( CMake/configurations.cmake )

# Name the CMake project.  On Windows, CMake will automatically create
# a solution with this name (SERIF_PROJECT.sln), containing *all*
# projects.  Note that we also create individual solution files (such
# as Serif.sln and ParserTrainer.sln) that contain subsets of projects.
# See ./CMake/Solutions/CMakeLists.txt for more info.
PROJECT (SERIF_PROJECT)

# Verbose makes so we see what is going on.
SET (CMAKE_VERBOSE_MAKEFILE true)

# Search path for CMake Modules and includes (e.g. FindBoost)

SET (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake ${CMAKE_MODULE_PATH})

# Include some MACRO definitions.
INCLUDE ( CMake/macro.cmake )

# Set up compiler flags and linker flags.
INCLUDE ( CMake/definitions.cmake )

# define the path to be used by "make install"
SET (CMAKE_INSTALL_PREFIX "${CMAKE_HOME_DIRECTORY}/../bin")
IF (UNIX)
    SET (CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}$ENV{ARCH_SUFFIX}")
ENDIF (UNIX)

######################################################################
# Feature Modules
#---------------------------------------------------------------------
# Create two "global" variables to track the names of all feature
# module projects, and the set of feature modules that should be
# statically linked into executables.
######################################################################

INITIALIZE_INTERNAL_LIST(ALL_FEATURE_MODULES)
INITIALIZE_INTERNAL_LIST(STATICALLY_LINKED_FEATURE_MODULES)

######################################################################
# Dynamically generated files
#---------------------------------------------------------------------
# Create a directory for dynamically generated source files, and
# a set of "global" variables to track their paths (so we can 
# include them in the 'Configurations' project).
######################################################################

SET (DYNAMIC_INCLUDES_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}/source)
SET_INTERNAL_VAR(DYNAMIC_INCLUDES_DIR 
                 ${DYNAMIC_INCLUDES_BASE_DIR}/dynamic_includes)
SET_INTERNAL_VAR(DYNAMIC_STATIC_FEATURE_MODULES_DIR 
                 ${DYNAMIC_INCLUDES_BASE_DIR}/static_feature_modules)
FILE (MAKE_DIRECTORY ${DYNAMIC_STATIC_FEATURE_MODULES_DIR})
INITIALIZE_INTERNAL_LIST (DYNAMIC_INCLUDES_FILES)
INITIALIZE_INTERNAL_LIST (DYNAMIC_INCLUDES_DIRS)

# Create the directories.
FILE (MAKE_DIRECTORY ${DYNAMIC_INCLUDES_BASE_DIR})
FILE (MAKE_DIRECTORY ${DYNAMIC_INCLUDES_DIR})

######################################################################
# Commercial Off-The-Shelf ("COTS") Location
#---------------------------------------------------------------------
# On UNIX, the user may specify a directory where external 
# dependencies (such as xerces) should be found.  This is known
# as the "commercial off-the-shelf directory", or "COTS".
######################################################################
IF (UNIX)
    SET (COTS_DIR ${COTS_DIR} CACHE String "COTS (commercial off-the-shelf) directory")
ENDIF(UNIX)

######################################################################
# Boost Libraries
#---------------------------------------------------------------------
# Find and configure the "Boost" C++ libraries.
######################################################################

# We prefer for the user to define the environment variable $BOOST_ROOT,
# but in case they don't, specify a list of additional search locations 
# where we can look for boost.
SET(Boost_SEARCH_PATH
  "C:/boost_1_59_0" "C:/boost_1_56_0"
  "C:/boost_1_44_0" "C:/boost_1_40_0"
  "D:/boost_1_40_0" "D:/boost_1_40_0" 
  "C:/boost/" "D:/boost/" "E:/boost/"
  "${COTS_DIR}/opt/boost_1_40_0-gcc-4.1.2$ENV{ARCH_SUFFIX}"
  "${COTS_DIR}/opt/boost_1_55_0-gcc-4.4.6$ENV{ARCH_SUFFIX}"
  )

# Link statically with Boost -- this makes us more portable.
SET(Boost_USE_STATIC_LIBS ON)

# Provide a list of additional versions we support.
SET(Boost_ADDITIONAL_VERSIONS "1.59.0" "1.56.0" "1.55.0" "1.44.0" "1.40.0")

# A list of the Boost libraries that may be used by SERIF.
# If you add a library to this list, then you should also add
# it to "Generic/common/BoostLibrariesFound.h.in".
SET(Boost_COMPONENTS regex date_time filesystem iostreams wserialization
                     serialization system thread program_options unit_test_framework chrono)

#SET(Boost_DEBUG true) # <- can be useful if FIND_PACKAGE fails

# Find boost.  Require that we find at least version 1.40.
#MESSAGE(STATUS "Searching for boost...")
FIND_PACKAGE(Boost 1.40.0 REQUIRED COMPONENTS ${Boost_COMPONENTS})
#MESSAGE(STATUS "  Boost includes: ${Boost_INCLUDE_DIR}")
#MESSAGE(STATUS "  Boost libraries: ${Boost_LIBRARY_DIRS}")


# Get the boost shared library files.  This is a bit of a hack, but
# it works.  The alternative would be to have two entirely separate
# build configurations, one using boost static libraries and one
# using boost shared libraries (since FindBoost.cmake makes us choose
# one copy or the other).
SET(Boost_shared_LIBRARIES)
IF (UNIX)
  FOREACH(static_lib ${Boost_LIBRARIES})
    STRING(REGEX REPLACE ".a$" ".so" shared_lib ${static_lib})
    SET(Boost_shared_LIBRARIES ${Boost_shared_LIBRARIES} ${shared_lib})
  ENDFOREACH(static_lib ${Boost_LIBRARIES})
ENDIF (UNIX)



############
#  XercesC #
############
# In order to use this custom FindXercesC CMake Module, you need the environment
# variable XERCESROOT set to your Xerces-C path. It will try to fall back to
# reasonable defaults.
FIND_PACKAGE(XercesC REQUIRED)

###########
# OpenSSL #
###########
IF(USE_POSTGRES)
  FIND_PACKAGE(OpenSSL REQUIRED)
ELSE()
  FIND_PACKAGE(OpenSSL)
ENDIF(USE_POSTGRES)

###########
# LBFGS-B #
###########
add_library(LBFGS_LIBRARY STATIC IMPORTED)
SET_PROPERTY(TARGET LBFGS_LIBRARY PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../External/LBFGS-B/liblbfgsb_standalone.a)

###########################
# Google Protocol Buffers #
###########################
add_library(GOOGPB_LIBRARY STATIC IMPORTED)
IF (UNIX)
	SET_PROPERTY(TARGET GOOGPB_LIBRARY PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../External/google/src/.libs/libprotobuf.a)
ENDIF(UNIX)
if (WIN32)
	SET_PROPERTY(TARGET GOOGPB_LIBRARY PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../External/google/vsprojects/Release/libprotobuf.lib)
ENDIF(WIN32)

SET(LearnIt_Active_LIBRARY ${LearnIt_INCLUDE_DIR}/English_Release/ActiveLearning.lib)
SET(LearnIt_LIBRARIES ${LearnIt_LIBRARY} ${LearnIt_Temporal_LIBRARY} ${LearnIt_Active_LIBRARY})
#########################
#  Include Directories  #
#########################
INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Generic
  ${CMAKE_CURRENT_SOURCE_DIR}/../../External/google/src
  ${Boost_INCLUDE_DIR}
  ${XercesC_INCLUDE_DIR}
  ${DYNAMIC_INCLUDES_BASE_DIR})

#############
#  Generic  #
#############
SET(GENERIC_LIBRARIES Generic)
ADD_SUBDIRECTORY (Generic)

##############################
#  Language Libraries        #
##############################

# All possible language that are supported should be defined here.
SET (ALL_POSSIBLE_LANGUAGES English Arabic Chinese Spanish Korean Thai Urdu)

# Include each languages library.  Note that if a library's CMakeFile
# uses the EXCLUDED_BY_DEFAULT flag, then that library will only be
# included in the cmake output if the user manually specifies that they 
# want it (using the cmake variable FEATUREMODULE_<language>_INCLUDED).
FOREACH (LANG ${ALL_POSSIBLE_LANGUAGES})
  ADD_SUBDIRECTORY (${LANG})
ENDFOREACH ()

###########
# LearnIt #
###########
ADD_SUBDIRECTORY(ActiveLearning)
ADD_SUBDIRECTORY(LearnIt)
ADD_SUBDIRECTORY(Temporal)

##########################
#  MR Projects           #
##########################
ADD_SUBDIRECTORY(PredFinder)

##############################
#      Feature Modules       #
##############################
ADD_SUBDIRECTORY (BasicCipherStream)
ADD_SUBDIRECTORY (EnglishTest)
ADD_SUBDIRECTORY (KbaStreamCorpus)
ADD_SUBDIRECTORY (License)
ADD_SUBDIRECTORY (MySQL)
ADD_SUBDIRECTORY (OpenSSLCipherStream)
ADD_SUBDIRECTORY (Oracle)
ADD_SUBDIRECTORY (PerformanceTools)
ADD_SUBDIRECTORY (Postgres)
ADD_SUBDIRECTORY (SerifHTTPServer)
#ADD_SUBDIRECTORY (SevenZip)

#######################
#  Serif Executables  #
#######################
ADD_SUBDIRECTORY (SerifMain)
ADD_SUBDIRECTORY (SerifTest)

##########################
#  Standalones Projects  #
##########################
INCLUDE(CMake/standalones-includes.cmake)

##########################
#    Brandy Projects     #
##########################
INCLUDE(CMake/brandy.cmake)

##########################
#    ProfileGenerator    #
##########################
INCLUDE(CMake/profile-generator.cmake)

##########################
#  ATEA Projects         #
##########################
if (WIN)
    ADD_SUBDIRECTORY(ATEASerif_Generic)
    ADD_SUBDIRECTORY(ATEASerif_English)
    ADD_SUBDIRECTORY(ATEASerif_Batch_English)
ENDIF(WIN)

######################################################################
# Statically Linked Feature Modules
#---------------------------------------------------------------------
# This defines a single function, registerStaticFeatureModules(),
# which is used by FeatureModule.cpp to record the appropriate setup
# function names for any statically linked feature modules.
######################################################################

ADD_SUBDIRECTORY(StaticFeatureModules)

########################################################################
# Create a project for CMake files and for dynamically included files. #
# Create the solution files.                                           #
########################################################################
IF (WIN)
   ADD_SUBDIRECTORY(CMake)
ENDIF (WIN)

# Enable CMake based testing
ENABLE_TESTING()

# adds package & package_source targets to the Makefile to create binary and source archives
#INCLUDE(CPack)


# # Report which feature modules are included:
# MESSAGE(STATUS "The following SERIF feature modules will be included:")
# FOREACH(lib ${ALL_FEATURE_MODULES})
#     LIST(FIND STATICALLY_LINKED_FEATURE_MODULES "${lib}" is_static)
#     IF (is_static GREATER -1)
#         MESSAGE(STATUS "  * ${lib}")
#     ELSE()
#         MESSAGE(STATUS "  * ${lib}_MODULE (dll)")
#     ENDIF()
# ENDFOREACH()

######################################################################
# CMake Variables
#---------------------------------------------------------------------
# Hide some of the unused default CMake options
######################################################################

MARK_AS_ADVANCED (LIBRARY_OUTPUT_PATH 
                  EXECUTABLE_OUTPUT_PATH 
                  CMAKE_INSTALL_PREFIX)

######################################################################
# CMake Export
#---------------------------------------------------------------------
# Create a cmake export file so that other projects can import the
# SERIF libraries from other cmake projects.
######################################################################

# The list of targets we should export:
SET(EXPORT_TARGETS Generic ActiveLearning LearnIt Temporal ${ALL_FEATURE_MODULES})

# The export file's filename:
SET(EXPORT_FILE "${CMAKE_CURRENT_BINARY_DIR}/export.cmake")

GET_DIRECTORY_PROPERTY(SERIF_INCLUDE_DIRS INCLUDE_DIRECTORIES)
STRING(REPLACE "\\" "\\\\" SERIF_INCLUDE_DIRS_ESC "${SERIF_INCLUDE_DIRS}")
STRING(REPLACE "\\" "\\\\" BOOST_LIBRARY_DIRS_ESC "${Boost_LIBRARY_DIRS}")
CONFIGURE_FILE(
	${CMAKE_CURRENT_SOURCE_DIR}/export.cmake.in 
	${EXPORT_FILE} @ONLY)
FOREACH(target ${EXPORT_TARGETS})
    IF (NOT target STREQUAL "Unspecified")
        FILE(APPEND "${EXPORT_FILE}" "\n#======= ${target}=======\n")
        IF (NOT SERIF_IS_IMPORTED_FROM_ANOTHER_PROJECT)
            EXPORT(TARGETS ${target} APPEND FILE export.cmake)
        ENDIF (NOT SERIF_IS_IMPORTED_FROM_ANOTHER_PROJECT)
        GET_TARGET_PROPERTY(SOURCE_FILES ${target} SOURCES)
        FILE(APPEND "${EXPORT_FILE}" 
            "\nSET_TARGET_PROPERTIES(${target} PROPERTIES "
            "SOURCES \"${SOURCE_FILES}\")\n")
    ENDIF (NOT target STREQUAL "Unspecified")
ENDFOREACH(target)
