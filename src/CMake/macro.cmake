####################################################################
# Copyright (c) 2011 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/macro.cmake                                                #
#                                                                  #
# Common MACRO definitions                                         #
#                                                                  #
# If you add or edit any macros here, please update the            #
# descriptions at:                                                 #
#                                                                  #
#   <http://wiki.d4m.bbn.com/wiki/Serif_CMake_Information>         #
#                                                                  #
####################################################################

###########################################################################
# WIN Variable
#--------------------------------------------------------------------------
# set WIN to True if we're on a Windows machine.
###########################################################################
SET(WIN ${WIN32} ${WIN64})
IF ((CMAKE_CL_64) OR (CMAKE_GENERATOR MATCHES "Win64"))
  SET(WIN64_TARGET ${WIN})
ENDIF ((CMAKE_CL_64) OR (CMAKE_GENERATOR MATCHES "Win64"))

###########################################################################
# OSX Variable
#--------------------------------------------------------------------------
# set OSX to True if we're on an OS X or iOS machine.
###########################################################################
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET(OSX TRUE)
ENDIF()

###########################################################################
# PARSE_ARGUMENTS(<prefix> <arg_names> <option_names>)
#--------------------------------------------------------------------------
# This macro is used by other macros to parse argument lists.  For
# further details, see:
#   <http://www.cmake.org/Wiki/CMakeMacroParseArguments#Usage>
#
# One extra output variable was added: <prefix>_OPTIONS, which is
# set to a list of all options that were specified.
###########################################################################
MACRO(PARSE_ARGUMENTS prefix arg_names option_names)
  SET(DEFAULT_ARGS)
  FOREACH(arg_name ${arg_names})
    SET(${prefix}_${arg_name})
  ENDFOREACH(arg_name)
  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH(option)
  SET(${prefix}_OPTIONS) # [XX] Added 9/19/11

  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})
    SET(larg_names ${arg_names})
    LIST(FIND larg_names "${arg}" is_arg_name)
    IF (is_arg_name GREATER -1)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE (is_arg_name GREATER -1)
      SET(loption_names ${option_names})
      LIST(FIND loption_names "${arg}" is_option)
      IF (is_option GREATER -1)
           SET(${prefix}_${arg} TRUE)
           LIST(APPEND ${prefix}_OPTIONS ${arg}) # [XX] Added 9/19/11
      ELSE (is_option GREATER -1)
           SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF (is_option GREATER -1)
    ENDIF (is_arg_name GREATER -1)
  ENDFOREACH(arg)
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO(PARSE_ARGUMENTS)

###########################################################################
# ASSERT(<test> <comment>)
#--------------------------------------------------------------------------
# If <test> is not true, then generate a fatal error whose message
# contains the given comment.
###########################################################################
MACRO (ASSERT TEST COMMENT)
  IF (NOT ${TEST})
     MESSAGE(FATAL_ERROR "Assertion Failed: ${COMMENT}")
  ENDIF (NOT ${TEST})
ENDMACRO (ASSERT)

###########################################################################
# GENERATE_PROJECT(<dir> <sources>)
#--------------------------------------------------------------------------
# Set the SOURCE_GROUP for each file in the given list of sources based
# on its directory component.  For example, if one of the sources is
# "foo/bar.cpp", then its SOURCE_GROUP will be set to "<dir>/foo".
#
# Based on: <http://www.cmake.org/Wiki/CMakeMacroGenerateProject>
###########################################################################
MACRO ( GENERATE_PROJECT ProjectDir ProjectSources )
   SET ( DirSources ${ProjectSources} )
   FOREACH ( Source ${DirSources} )
     # Strip ${ProjectDir} off the front
     STRING (REGEX REPLACE "^${ProjectDir}[\\\\/]" "" RelativePath "${Source}")
     # Strip the filename off the back.
     STRING (REGEX REPLACE "[\\\\/]?[^\\\\/]+$" "" RelativePath ${RelativePath})
     # For dynamic files, strip the dynamic dir off the front.
     STRING(REGEX REPLACE "^.*/source/static_feature_modules" "dynamic"
             RelativePath "${RelativePath}")
     # Replace / with \\.
     STRING ( REGEX REPLACE "/" "\\\\\\\\" RelativePath "${RelativePath}" )
     # Set the SOURCE_GROUP.
     SOURCE_GROUP ( "${RelativePath}" FILES ${Source} )
   ENDFOREACH ( Source )
ENDMACRO ( GENERATE_PROJECT )

###########################################################################
# ADD_CMAKE_AND_REL_PATH()
#--------------------------------------------------------------------------
# DEPRECATED: Use ADD_SERIF_LIBRARY_SUBDIR instead.
#
# This macro:
#   - Updates ${RELATIVE_PATH} to include ${CURRENT_DIR}
#   - Adds ${RELATIVE_PATH}/CMakeLists.txt to ${SOURCE_FILES}
#   - Adds ${RELATIVE_PATH}/<file> to ${SOURCE_FILES} for each <file>
#     in ${SOURCE_FILES}
#   - Includes <subdir>/CMakeLists.txt for each <subdir> in
#     ${LOCAL_SUBDIRS}
#   - Restores ${RELATIVE_PATH} to its original value.
#
# It uses several variables from the parent scoe:
#   - ${CURRENT_DIR}
#   - ${RELATIVE_PATH}
#   - ${OLD_PATHS}
#   - ${SOURCE_FILES}
#   - ${LOCAL_FILES}
#   - ${LOCAL_SUBDIRS}
###########################################################################
MACRO ( ADD_CMAKELIST_AND_REL_PATH )
      # keep the old path
      SET(OLD_PATHS ${OLD_PATHS} ${RELATIVE_PATH})
      IF (RELATIVE_PATH)
            SET(RELATIVE_PATH "${RELATIVE_PATH}/${CURRENT_DIR}")
      ELSE (RELATIVE_PATH)
            SET(RELATIVE_PATH "${CURRENT_DIR}")
      ENDIF(RELATIVE_PATH)

      # add CMakelists.txt to the project
      IF(INCLUDE_CMAKELISTS_FILES_IN_PROJECTS)
         SET(SOURCE_FILES ${SOURCE_FILES} "${RELATIVE_PATH}/CMakeLists.txt")
      ENDIF(INCLUDE_CMAKELISTS_FILES_IN_PROJECTS)

      FOREACH (FILE ${LOCAL_FILES})
        SET(SOURCE_FILES ${SOURCE_FILES} "${RELATIVE_PATH}/${FILE}")
      ENDFOREACH (FILE ${LOCAL_FILES})

      # recursively include subdirectories (defined in the ${LOCAL_SUBDIRS} var)
      IF(INCLUDE_CMAKELISTS_FILES_IN_PROJECTS)
        FOREACH (DIR ${LOCAL_SUBDIRS})
          INCLUDE("${RELATIVE_PATH}/${DIR}/CMakeLists.txt")
        ENDFOREACH (DIR ${LOCAL_FILES})
      ENDIF(INCLUDE_CMAKELISTS_FILES_IN_PROJECTS)

      # return to the old path
      SET (LAST_PATH)
      LIST(LENGTH OLD_PATHS OLD_LENGTH)
      IF (OLD_LENGTH GREATER 0)
           MATH(EXPR OLD_LENGTH ${OLD_LENGTH}-1)
           LIST(GET OLD_PATHS ${OLD_LENGTH} LAST_PATH)
           LIST(REMOVE_ITEM OLD_PATHS ${LAST_PATH})
      ENDIF (OLD_LENGTH GREATER 0)

      SET(RELATIVE_PATH ${LAST_PATH})
ENDMACRO ( ADD_CMAKELIST_AND_REL_PATH )

###########################################################################
# SET_INTERNAL_VAR(<var> <value>...)
#--------------------------------------------------------------------------
# Create an internal cache variable with the given name, and assign it
# the given value.  If multiple values are given, then they will be
# collected into a list.
#
# Internal cache variables are "global" variables, and are shared across
# all projects and subdirectories.  (This contrasts with normal variables,
# which are local to the subdirectory that defined them.)
###########################################################################
MACRO (SET_INTERNAL_VAR VAR)
   SET (${VAR} "${ARGN}" CACHE INTERNAL "Internal Variable")
ENDMACRO (SET_INTERNAL_VAR VAR)

###########################################################################
# INITIALIZE_INTERNAL_LIST(<list_var>)
#--------------------------------------------------------------------------
# Create a new internal cache variable with the given name, that is
# intended to contain a list.  Its value is initially set to the empty
# list.
###########################################################################
MACRO (INITIALIZE_INTERNAL_LIST LIST_NAME)
   SET_INTERNAL_VAR(${LIST_NAME})
ENDMACRO (INITIALIZE_INTERNAL_LIST)

###########################################################################
# APPEND_INTERNAL_LIST(<list_name> <value>...)
#--------------------------------------------------------------------------
# Append one or more values to a specified internal cache variable.
# See: INITIALIZE_INTERNAL_LIST().
###########################################################################
MACRO (APPEND_INTERNAL_LIST LIST_NAME)
   SET_INTERNAL_VAR(${LIST_NAME} ${${LIST_NAME}} ${ARGN})
ENDMACRO (APPEND_INTERNAL_LIST)


###########################################################################
# ADD_BBN_TEST(<TestFileName>)
#--------------------------------------------------------------------------
# This currently just expands to: INCLUDE(${TestFileName})
###########################################################################
# A simple macro to load a set of tests from a file
MACRO(ADD_BBN_TEST TestFileName)
  INCLUDE (${TestFileName})
  #MESSAGE("ADD_BBN_TEST ${TestFileName}")
  #ADD_CUSTOM_TARGET (${TestName})
  #ADD_DEPENDENCIES (${TestName} ${TestFileName})
ENDMACRO(ADD_BBN_TEST)

###########################################################################
# ADD_SERIF_EXECUTABLE(
#     name
#     [NO_FEATURE_MODULES]
#     [SOURCE_FILES <file> ...]
#     [INSTALL <install_subdir>]
#     [LINK_LIBRARIES <library>...]
#     [WIN_LINK_LIBRARIES <library>...]
#     [UNIX_LINK_LIBRARIES <library>...])
#--------------------------------------------------------------------------
# This macro creates a new executable target.
#
# The given source files should contain the executable's main()
# method.  The executable will be automatically linked to the Generic
# target and to any statically linked feature modules (including
# language modules). Additional library dependencies may be specified
# if necessary.
#
# If <install_subdir> is specified, then the executable will be
# installed to that subdirectory.
###########################################################################
MACRO(ADD_SERIF_EXECUTABLE name)
  # Parse the argument list.
  PARSE_ARGUMENTS(ARG "SOURCE_FILES;INSTALL;LINK_LIBRARIES;WIN_LINK_LIBRARIES;UNIX_LINK_LIBRARIES" "NO_FEATURE_MODULES" ${ARGN})
  LIST(FIND ARG_OPTIONS NO_FEATURE_MODULES no_feature_modules_flag)
  IF (ARG_DEFAULT_ARGS)
    MESSAGE(FATAL_ERROR "Unexpected arg to ADD_SERIF_EXECUTABLE:"
                        " \"${ARG_DEFAULT_ARGS}\"")
  ENDIF (ARG_DEFAULT_ARGS)
  LIST(GET "${ARG_INSTALL}" 2 ARG_INSTALL_2)
  IF(ARG_INSTALL_2)
    MESSAGE(FATAL_ERROR "only one INSTALL location allowed")
  ENDIF(ARG_INSTALL_2)

  # Language is selected at run-time; cmake targets should no longer
  # contain "<Language>".
  IF (${name} MATCHES "<Language>")
    MESSAGE(FATAL_ERROR "<Language> in target names is no longer allowed")
  ENDIF (${name} MATCHES "<Language>")

  # Expand any globs (eg *.h) in the source list.
  SAFE_FILE_GLOB(SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
       ${ARG_SOURCE_FILES})

  # Add a StaticFeatureModules.cpp source file (generated based on a
  # template).  This file is generated from a template separately for
  # each executable, since the set of statically linked libraries may
  # be different for different executables (eg., for Serif vs IDX).
  IF (no_feature_modules_flag LESS 0)
    SET(sfm_cpp "${DYNAMIC_STATIC_FEATURE_MODULES_DIR}/StaticFeatureModules_${name}.cpp")
    GENERATE_STATIC_FEATURE_MODULES_CPP("${sfm_cpp}" ${STATICALLY_LINKED_FEATURE_MODULES})
    LIST(APPEND SOURCE_FILES "${sfm_cpp}")
  ENDIF()

  # Add the executable target

  ADD_EXECUTABLE (${name} ${SOURCE_FILES})
  #MESSAGE(STATUS "Adding executable target: [${name}]")

  # Link in any requested libraries
  IF (UNIX)
    SET(LINK_LIBRARIES ${ARG_LINK_LIBRARIES} ${ARG_UNIX_LINK_LIBRARIES} -lrt)
  ELSE (UNIX)
    SET(LINK_LIBRARIES ${ARG_LINK_LIBRARIES} ${ARG_WIN_LINK_LIBRARIES})
  ENDIF (UNIX)

  # Link in SERIF's Generic library.  Let the linker know that it
  # should include unused symbols, because those symbols may be used
  # by dynamically linked feature modules.
  IF (UNIX AND NOT OSX)
    TARGET_LINK_LIBRARIES(${name} "-Wl,--whole-archive")
    TARGET_LINK_LIBRARIES(${name} Generic)
    TARGET_LINK_LIBRARIES(${name} "-Wl,--no-whole-archive")
  ELSE (UNIX AND NOT OSX)
    TARGET_LINK_LIBRARIES(${name} Generic)
  ENDIF (UNIX AND NOT OSX)

  # Link in any statically linked feature modules (unless the
  # NO_FEATURE_MODULES option was specified.)
  IF (no_feature_modules_flag LESS 0)
    TARGET_LINK_LIBRARIES(${name} ${STATICALLY_LINKED_FEATURE_MODULES})
  ENDIF()

  # Link in any additional libraries specified
  TARGET_LINK_LIBRARIES(${name} ${LINK_LIBRARIES})

  # Windows executables may need DLLs copied into place
  IF (WIN)
    # Generic always depends on Xerces, so check if we need to copy DLL
    IF (NOT "${XercesC_LIBRARIES}" MATCHES ".*-static")
      STRING(REGEX REPLACE "c_3([^\\.]+)\\.lib$" "c_3_1\\1.dll" XercesC_DLLS ${XercesC_LIBRARIES})
      ADD_CUSTOM_COMMAND(TARGET ${name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${XercesC_DLLS} $(OutDir))
    ENDIF (NOT "${XercesC_LIBRARIES}" MATCHES ".*-static")

    # If the License module is being used, it depends on OpenSSL
    IF (no_feature_modules_flag LESS 0 AND FEATUREMODULE_License_INCLUDED)
      ADD_CUSTOM_COMMAND(TARGET ${name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${OpenSSL_LIBEAY_DLL} $(OutDir))
      ADD_CUSTOM_COMMAND(TARGET ${name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${OpenSSL_SSLEAY_DLL} $(OutDir))
    ENDIF ()

    # If we're depending on PostgreSQL, check if we need to copy DLL
    IF (USE_POSTGRES)
      IF (NOT "${LIBPQXX_LIBRARY}" MATCHES ".*_static.*")
        # Need to copy both because they have different paths and CMake
        # can't filter build type for Visual Studio in a custom command
        ADD_CUSTOM_COMMAND(TARGET ${name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${LIBPQXX_DLL_DEBUG} $(OutDir))
        ADD_CUSTOM_COMMAND(TARGET ${name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${LIBPQXX_DLL_RELEASE} $(OutDir))
      ENDIF (NOT "${LIBPQXX_LIBRARY}" MATCHES ".*_static.*")

      # libpqxx always depends on libpq
      ADD_CUSTOM_COMMAND(TARGET ${name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${LIBPQXX_LIBPQ_DLL} $(OutDir))

      # libpq in turn depends on OpenSSL
      ADD_CUSTOM_COMMAND(TARGET ${name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${OpenSSL_LIBEAY_DLL} $(OutDir))
      ADD_CUSTOM_COMMAND(TARGET ${name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${OpenSSL_SSLEAY_DLL} $(OutDir))

      # ...and an i18n library
      ADD_CUSTOM_COMMAND(TARGET ${name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${LIBPQXX_INTL_DLL} $(OutDir))
    ENDIF (USE_POSTGRES)
  ENDIF (WIN)

  # Specify the installation location for this executable
  IF (ARG_INSTALL)
      INSTALL(TARGETS ${name} DESTINATION
        "bin/${ARG_INSTALL}")
  ELSE (ARG_INSTALL)
      INSTALL(TARGETS ${name} DESTINATION "bin")
  ENDIF (ARG_INSTALL)

  # Generate the local project directory structure
  GENERATE_PROJECT ("" "${ARG_SOURCE_FILES}")

ENDMACRO(ADD_SERIF_EXECUTABLE)

###########################################################################
# ADD_SERIF_FEATURE_MODULE(
#     <name>
#     [EXCLUDED_BY_DEFAULT]
#     [STATIC | MODULE]
#     <args...>)
#--------------------------------------------------------------------------
# Makes two calls to ADD_SERIF_LIBRARY, with <args>:
#   - Create a module library named <name>_MODULE
#   - Create a static library named <name>
#
# The EXCLUDED_BY_DEFAULT flag specifies that this feature module should
# be excluded from the generated project files, unless the cmake user
# overrides this by modifying the cmake configuration variable named
# "FEATUREMODULE_${name}_INCLUDED".
#
# The STATIC/MODULE flag determines whether this feature module should
# be statically linked when executables are built.  This can be
# overridden by the cmake user using the configuration variable
# "FEATUREMODULE_${name}_STATIC".  (If true, then the module is
# appended to the global variable STATICALLY_LINKED_FEATURE_MODULES.)
# Default: STATIC.
#
# On unix, set the default visibility of symbols in the module library
# to 'default'.
###########################################################################


MACRO(ADD_SERIF_FEATURE_MODULE name)
  PARSE_ARGUMENTS(SERIFFMARG "" "EXCLUDED_BY_DEFAULT;STATIC;MODULE" ${ARGN}) 
  LIST(FIND SERIFFMARG_OPTIONS MODULE module_flag)
  LIST(FIND SERIFFMARG_OPTIONS STATIC static_flag)
  LIST(FIND SERIFFMARG_OPTIONS EXCLUDED_BY_DEFAULT exclude_flag)
  SET(INCLUDED_default_value ON)
  IF (exclude_flag GREATER -1)
    SET(INCLUDED_default_value OFF)
  ENDIF()
  SET(STATIC_default_value ON)
  IF (UNIX AND (module_flag GREATER -1))
    SET(STATIC_default_value OFF)
  ENDIF()

  # If it's a feature module, then add configuration variables that
  # control whether it is included, and whether it is statically linked.
  SET(FEATUREMODULE_${name}_INCLUDED ${INCLUDED_default_value} CACHE BOOL
      "Should the ${name} feature module be included in the project?")
  SET(FEATUREMODULE_${name}_STATIC ${STATIC_default_value} CACHE BOOL
      "Should the ${name} feature module be static?")
  IF (NOT(UNIX OR FEATUREMODULE_${name}_STATIC))
    MESSAGE(FATAL_ERROR "Building non-static (dll) feature modules is "
        "not currently supported on Windows.  Please set "
        "FEATUREMODULE_${name}_STATIC=OFF.")
  ENDIF()

  IF (NOT FEATUREMODULE_${name}_INCLUDED)
    #MESSAGE(STATUS "Skipping feature module:  [${name}]")

  ELSE()
    # Create the library.
    ADD_SERIF_LIBRARY(${name} ${SERIFFMARG_DEFAULT_ARGS})

    # Add this library to the list of all feature modules.
    APPEND_INTERNAL_LIST(ALL_FEATURE_MODULES "${name}")

    # Add the 'module' (dll) target for this feature module.
    GET_TARGET_PROPERTY(SOURCE_FILES ${name} SOURCES)
    ADD_LIBRARY (${name}_MODULE MODULE ${SOURCE_FILES})
    SET_DEFAULT_SYMBOL_VISIBILITY("${name}_MODULE" "default")
    SET_TARGET_PROPERTIES("${name}_MODULE" PROPERTIES OUTPUT_NAME
                          "Serif${name}")

    # Set up static linking, if appropriate.
    IF (FEATUREMODULE_${name}_STATIC)
      APPEND_INTERNAL_LIST(STATICALLY_LINKED_FEATURE_MODULES "${name}")
    ENDIF (FEATUREMODULE_${name}_STATIC)
  ENDIF()
ENDMACRO()


MACRO(ADD_SERIF_TEST_MODULE name)
  PARSE_ARGUMENTS(SERIFTMARG "" "EXCLUDED_BY_DEFAULT;STATIC;MODULE" ${ARGN})
  LIST(FIND SERIFTMARG_OPTIONS MODULE module_flag)
  LIST(FIND SERIFTMARG_OPTIONS STATIC static_flag)
  LIST(FIND SERIFTMARG_OPTIONS EXCLUDED_BY_DEFAULT exclude_flag)
  SET(INCLUDED_default_value ON)
  IF (exclude_flag GREATER -1)
    SET(INCLUDED_default_value OFF)
  ENDIF()
  SET(STATIC_default_value ON)
  IF (UNIX AND (module_flag GREATER -1))
    SET(STATIC_default_value OFF)
  ENDIF()
  
  # Add configuration variables that control whether it is included, and 
  # whether it is statically linked.
  SET(TESTMODULE_${name}_INCLUDED ${INCLUDED_default_value} CACHE BOOL
      "Should the ${name} test module be included in the project?")
  SET(TESTMODULE_${name}_STATIC ${STATIC_default_value} CACHE BOOL
      "Should the ${name} test module be static?")
  IF (NOT(UNIX OR TESTMODULE_${name}_STATIC))
    MESSAGE(FATAL_ERROR "Building non-static (dll) test modules is "
        "not currently supported on Windows.  Please set "
        "TESTMODULE_${name}_STATIC=OFF.")
  ENDIF()
  
  IF (NOT TESTMODULE_${name}_INCLUDED)
    #MESSAGE(STATUS "Skipping test module:  [${name}]")
	
  ELSE()
    # Create the library.
	ADD_SERIF_LIBRARY(${name}Test ${SERIFTMARG_DEFAULT_ARGS})

    # Add this library to the list of all feature modules.
    APPEND_INTERNAL_LIST(ALL_TEST_MODULES "${name}Test")

    # Set up static linking, if appropriate.
    #IF (TESTMODULE_${name}_STATIC)
    #  APPEND_INTERNAL_LIST(STATICALLY_LINKED_TEST_MODULES "${name}Test")
    #ENDIF (TESTMODULE_${name}_STATIC)
  ENDIF()
	
ENDMACRO()

###########################################################################
# ADD_SERIF_LIBRARY(
#     <name>
#     [PUBLIC_DLL]
#     [SOURCE_FILES <file> ...]
#     [SUBDIRS <subdir> ...]
#     [LINK_LIBRARIES <library>...]
#     [WIN_LINK_LIBRARIES <library>...]
#     [UNIX_LINK_LIBRARIES <library>...])
#--------------------------------------------------------------------------
# This macro creates a new library target named <name>, containing all
# of the specified <file>s, as well as any files contained in the
# specified subdirectories.  Each <file> can be either a filename
# (e.g.  "en_Tokenizer.h") or a file glob (e.g. "*.cpp").  Each
# <subdir> should contain a CMakeLists.txt file that calls the
# ADD_SERIF_LIBRARY_SUBDIR command.
#
# The newly created library target is linked (w/ TARGET_LINK_LIBRARIES)
# to the libraries listed in LINK_LIBRARIES, WIN_LINK_LIBRARIES, and
# UNIX_LINK_LIBRARIES.  (The WIN and UNIX libraries are only linked
# when cmake is run on the specified platform).
#
# In addition, this macro sets the SOURCE_GROUP of each file to
# give Visual Studio hierarchical source groups that correspond with
# the directory structure.
#
# If the PUBLIC_DLL flag is specified, then the library is built as a
# shared library; but any Serif library dependencies specified by
# LINK_LIBRARIES are linked directly into the DLL.  This is used to
# build DLLs such as libSerifJNI.so and libSerifAPI.so that are used
# by external customers.
###########################################################################
MACRO(ADD_SERIF_LIBRARY name)
  # Parse the argument list.
  PARSE_ARGUMENTS(SERIFLIBARG "SOURCE_FILES;SUBDIRS;LINK_LIBRARIES;WIN_LINK_LIBRARIES;UNIX_LINK_LIBRARIES" "PUBLIC_DLL" ${ARGN})
  IF (SERIFLIBARG_DEFAULT_ARGS)
    MESSAGE(FATAL_ERROR "Unexpected arg to ADD_SERIF_LIBRARY:"
                        " \"${SERIFLIBARG_DEFAULT_ARGS}\"")
  ENDIF (SERIFLIBARG_DEFAULT_ARGS)
  LIST(FIND SERIFLIBARG_OPTIONS PUBLIC_DLL public_dll_flag)

  #MESSAGE(STATUS "Adding library target:    [${name}]")

  # Get an OS-specific list of dependencies.
  IF(UNIX)
    SET(LINK_LIBRARIES ${SERIFLIBARG_LINK_LIBRARIES}
                       ${SERIFLIBARG_UNIX_LINK_LIBRARIES})
  ELSE(UNIX)
    SET(LINK_LIBRARIES ${SERIFLIBARG_LINK_LIBRARIES}
                       ${SERIFLIBARG_WIN_LINK_LIBRARIES})
  ENDIF(UNIX)

  # Clear the "RELATIVE_PATH" variables.  This is used by the
  # ADD_SERIF_LIBRARY_SUBDIR macros in subdirectories.
  SET (RELATIVE_PATH "")

  # Set the initial ${SOURCE_FILES} list value.
  SET(SOURCE_FILES ${SERIFLIBARG_SOURCE_FILES})

  # Include subdirectories.  This will extend the ${SOURCE_FILES}
  # variable to include all of the source files in the subdirs.
  FOREACH (DIR ${SERIFLIBARG_SUBDIRS})
    INCLUDE("${DIR}/CMakeLists.txt")
  ENDFOREACH (DIR)

  # Expand any globs (eg *.h) in the source list.
  SAFE_FILE_GLOB(SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
       ${SOURCE_FILES})

  # Generate the local project directory structure
  GENERATE_PROJECT ("" "${SOURCE_FILES}")

  # For normal libraries, we create a static target, and link it to
  # the specified libraries.  This will later be statically linked
  # into an executable target.
  IF (public_dll_flag LESS 0)
    ADD_LIBRARY (${name} STATIC ${SOURCE_FILES})
    TARGET_LINK_LIBRARIES(${name} ${LINK_LIBRARIES})
  ENDIF (public_dll_flag LESS 0)

  # For PUBLIC_DLL libraries, we create a shared target.  We use
  # the TARGET_LINK_PIC_LIBRARIES macro (defined below) to ensure
  # that we link in the correct dependency versions.
  IF (public_dll_flag GREATER -1)
    ADD_LIBRARY (${name} SHARED ${SOURCE_FILES})
    TARGET_LINK_PIC_LIBRARIES(${name} ${LINK_LIBRARIES})
    SET_DEFAULT_SYMBOL_VISIBILITY("${name}" "default")
  ENDIF (public_dll_flag GREATER -1)

  # Create a convenience library target that turns on the 'PIC'
  # (position independant code) flag.  These PIC convenience libraries
  # are used when building PUBLIC_DLL Serif library tarets.
  IF (UNIX AND (public_dll_flag LESS 0))
    ADD_LIBRARY(${name}_PIC STATIC ${SOURCE_FILES})
    SET_TARGET_PROPERTIES(${name}_PIC PROPERTIES COMPILE_FLAGS -fPIC)
    SET_DEFAULT_SYMBOL_VISIBILITY("${name}" "default")
    TARGET_LINK_PIC_LIBRARIES(${name}_PIC ${LINK_LIBRARIES})
  ENDIF (UNIX AND (public_dll_flag LESS 0))

  # Record the source & binary dir for this library
  SET_TARGET_PROPERTIES("${name}" PROPERTIES
    TARGET_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    TARGET_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
ENDMACRO(ADD_SERIF_LIBRARY)

###########################################################################
# TARGET_LINK_PIC_LIBRARIES(<name> <targets...>)
#--------------------------------------------------------------------------
# Helper function used by ADD_SERIF_LIBRARY.
#
# Link the given target a specified list of libraries.  If any library
# has a version that was compiled with position-independent code
# (-fPIC), then link against that version instead.  In particular, if
# a link target's name is <depname>, and there exists a link target
# named <depname>_PIC, then link against <depname>_PIC instead of
# <depname>.  (On Windows, <depname>_PIC versions are not generated,
# so this is equivalent to just calling TARGET_LINK_LIBRARIES).
###########################################################################
MACRO(TARGET_LINK_PIC_LIBRARIES tllfs_name)
  FOREACH(lib ${ARGN})
    GET_TARGET_PROPERTY(lib_is_shared "${lib}_PIC" LOCATION)
    IF (lib_is_shared)
      #MESSAGE(STATUS "${tllfs_name}     depends on ${lib}_PIC")
      TARGET_LINK_LIBRARIES(${tllfs_name} "${lib}_PIC")
    ELSE (lib_is_shared)
      #MESSAGE(STATUS "${tllfs_name}     depends on ${lib}")
      TARGET_LINK_LIBRARIES(${tllfs_name} "${lib}")
    ENDIF (lib_is_shared)
  ENDFOREACH(lib ${LINK_LIBRARIES})
ENDMACRO()

###########################################################################
# ADD_SERIF_LIBRARY_SUBDIR(
#      <dirname>
#      [SOURCE_FILES <file> ...]
#      [SUBDIRS <subdir> ...])
#--------------------------------------------------------------------------
# This macro extends the ${SOURCE_FILES} variable to include the
# source files that are contained in a subdirectory.  In particular,
# this macro will:
#
#   - Update ${RELATIVE_PATH} to include <dirname>
#   - Append ${RELATIVE_PATH}/<file> to ${SOURCE_FILES} for each <file>
#   - Append ${RELATIVE_PATH}/CMakeLists.txt to ${SOURCE_FILES}
#   - Include ${RELATIVE_PATH}/<subdir>/CMakeLists.txt for each <subdir>
#   - Restore ${RELATIVE_PATH} to its original value
#
# Each <file> can be either a filename (e.g.  "en_Tokenizer.h") or a
# file glob (e.g. "*.cpp").
#
# Note: this macro uses two variables from the parent scope:
#   - ${RELATIVE_PATH}, which should contain the current relative path.
#     If ${RELATIVE_PATH} is non-empty, then it should should end in "/".
#   - ${SOURCE_FILES}, which is used for output.
###########################################################################
MACRO(ADD_SERIF_LIBRARY_SUBDIR dirname)
  # Parse the argument list.
  PARSE_ARGUMENTS(SERIFSUBDIRARG "SOURCE_FILES;SUBDIRS" "" ${ARGN})
  IF (SERIFSUBDIRARG_DEFAULT_ARGS)
    MESSAGE(FATAL_ERROR "Unexpected arg to ADD_SERIF_SUBDIR:"
                        " \"${SERIFSUBDIRARG_DEFAULT_ARGS}\"")
  ENDIF (SERIFSUBDIRARG_DEFAULT_ARGS)

  # Make sure RELATIVE_PATH ends with '/' if it's non-empty.
  IF(RELATIVE_PATH AND NOT RELATIVE_PATH MATCHES ".*/")
    SET(RELATIVE_PATH "${RELATIVE_PATH/")
  ENDIF(RELATIVE_PATH AND NOT RELATIVE_PATH MATCHES ".*/")

  # Set the RELATIVE_PATH directory.
  SET(RELATIVE_PATH "${RELATIVE_PATH}${dirname}/")

  # Add CMakelists.txt to the project.
  IF(INCLUDE_CMAKELISTS_FILES_IN_PROJECTS)
    LIST(APPEND SOURCE_FILES "${RELATIVE_PATH}CMakeLists.txt")
  ENDIF(INCLUDE_CMAKELISTS_FILES_IN_PROJECTS)

  # Add each source file to the project.
  FOREACH (FILE ${SERIFSUBDIRARG_SOURCE_FILES})
    LIST(APPEND SOURCE_FILES "${RELATIVE_PATH}${FILE}")
  ENDFOREACH (FILE)

  # Recursively include subdirectories.
  FOREACH (DIR ${SERIFSUBDIRARG_SUBDIRS})
    INCLUDE("${RELATIVE_PATH}${DIR}/CMakeLists.txt")
  ENDFOREACH (DIR)

  # Restore the RELATIVE_PATH to its original value
  STRING(REGEX REPLACE "[^/]+/$" "" RELATIVE_PATH "${RELATIVE_PATH}")

ENDMACRO(ADD_SERIF_LIBRARY_SUBDIR)

###########################################################################
# INCLUDE_CMAKELISTS_FILES_IN_PROJECTS (cache var)
#--------------------------------------------------------------------------
# This variable controls whether the ADD_SERIF_LIBRARY macro will
# include the CMakeLists.txt files in the list of source files for
# each library.
###########################################################################
SET (INCLUDE_CMAKELISTS_FILES_IN_PROJECTS ON CACHE BOOL
     "Include CMakeLists.txt files in the projects")
MARK_AS_ADVANCED (INCLUDE_CMAKELISTS_FILES_IN_PROJECTS)

###########################################################################
# SET_DEFAULT_SYMBOL_VISIBILITY(<target> <visibility>)
#--------------------------------------------------------------------------
# Set the default symbol visibility of the given target.
###########################################################################
MACRO(SET_DEFAULT_SYMBOL_VISIBILITY arg_target arg_visibility)
  IF (UNIX)
    # The "-Wno-attributes" disables a warning that is generated by
    # some versions of gcc when this is used.  The -rpath "$ORIGIN" is
    # used to say that it should search for dependency libraries (eg
    # boost) in the same directory that the shared library is in.  For
    # details on why we have both $ORIGIN and $$ORIGIN, see:
    # <http://www.cmake.org/pipermail/cmake/2008-January/019290.html>
    SET_PROPERTY(TARGET ${arg_target} APPEND PROPERTY
      COMPILE_FLAGS "-fvisibility=${arg_visibility} -Wno-attributes")
    SET_PROPERTY(TARGET ${arg_target} APPEND PROPERTY
      LINK_FLAGS "-fvisibility=${arg_visibility}"
                 "-Wno-attributes -Wl,-rpath,'$ORIGIN:$$ORIGIN'")
  ENDIF(UNIX)
ENDMACRO(SET_DEFAULT_SYMBOL_VISIBILITY)

###########################################################################
# GENERATE_STATIC_FEATURE_MODULES_CPP(<dst> <static_modules>...)
#--------------------------------------------------------------------------
# Fill in the StaticFeatureModules.cpp template based on the given
# list of static modules, and write the output to <dst>.  The
# StaticFeatureModules.cpp file define a single function that is
# called by FeatureModule::load(), that is responsible for registering
# the setup functions of statically linked modules. Some modules are
# registered as required, meaning that they will always be loaded.
#
# This macro is called by ADD_SERIF_EXECUTABLE, and probably should not
# be used in any other context.
###########################################################################
MACRO(GENERATE_STATIC_FEATURE_MODULES_CPP dst)
  # Fill in the template file
  SET(sfm_dir "${SERIF_PROJECT_SOURCE_DIR}/StaticFeatureModules")
  SET(src "${sfm_dir}/StaticFeatureModules.cpp")
  SET(REGISTER_STATIC_FEATURE_INCLUDES)
  SET(REGISTER_STATIC_FEATURE_COMMANDS)
  FOREACH(m ${ARGN})
    SET(REGISTER_STATIC_FEATURE_INCLUDES
        "${REGISTER_STATIC_FEATURE_INCLUDES}
         #include \"${m}/${m}Module.h\"")
    IF(m STREQUAL "License")
      SET(STATIC_FEATURE_REQUIRED ", true")
    ELSE()
      SET(STATIC_FEATURE_REQUIRED "")
    ENDIF()
    SET(REGISTER_STATIC_FEATURE_COMMANDS
        "${REGISTER_STATIC_FEATURE_COMMANDS}
         FeatureModule::registerModule(\"${m}\", setup_${m}${STATIC_FEATURE_REQUIRED});")
  ENDFOREACH(m)
  SET(AUTO_GENERATED_FILE
      "// Warning: this file was generated automatically!\n// DO NOT EDIT")
  CONFIGURE_FILE("${src}" "${dst}" @ONLY)
  #MESSAGE(STATUS "Generated: ${dst}")
  APPEND_INTERNAL_LIST (DYNAMIC_INCLUDES_FILES "${dst}")
ENDMACRO(GENERATE_STATIC_FEATURE_MODULES_CPP)

###########################################################################
# ADD_SERIF_SOLUTION(<name>
#     [NO_STATIC_FEATURE_MODULES]
#     [NO_GENERIC]
#     [PROJECTS <project>...]
#     [DEPENDENCIES <proj1>:<dep1> <proj2>:<dep2> ...])
#--------------------------------------------------------------------------
# Create a new Visual Studio solution with the given name.
#
# The projects are Visual Studio projects should be included in this
# solution. If any project's name contains the string "<Langauge>",
# then it will be expanded by replacing it with the name of each
# language that is included in this solution.
#
# Warning: CMake only allows one Visual Studio solution to be generated
# in any given cmake directory.
###########################################################################
MACRO(ADD_SERIF_SOLUTION name)
  # Parse the argument list.
  PARSE_ARGUMENTS(SERIFSLNARG "PROJECTS;DEPENDENCIES"
                  "NO_STATIC_FEATURE_MODULES;NO_GENERIC" ${ARGN})
  IF (SERIFSLNARG_DEFAULT_ARGS)
    MESSAGE(FATAL_ERROR "Unexpected arg to ADD_SERIF_SOLUTION:"
                        " \"${SERIFSLNARG_DEFAULT_ARGS}\"")
  ENDIF (SERIFSLNARG_DEFAULT_ARGS)

  # Parse the DEPENDENCIES argument.
  PARSE_DEPENDENCY_PAIRS(EXTRA_DEPENDENCIES_FOR ${SERIFSLNARG_DEPENDENCIES})

  # Create a project.  This will cause cmake to generate a solution file.
  PROJECT(${name})

  # Optionally include all cmakelists files in projects.
  SET(other_dependencies)
  IF(INCLUDE_CMAKELISTS_FILES_IN_PROJECTS)
    INCLUDE_EXTERNAL_MSPROJECT(Configurations
      ${SERIF_PROJECT_BINARY_DIR}/CMake/Configurations.vcproj)
    LIST(APPEND other_dependencies Configurations)
  ENDIF (INCLUDE_CMAKELISTS_FILES_IN_PROJECTS)

  # Check if this solution should include static feature modules.
  LIST(FIND SERIFSLNARG_OPTIONS NO_GENERIC no_generic)
  IF (no_generic LESS 0)
    # Include the Generic project
    INCLUDE_SERIF_MSPROJECT(Generic ${other_dependencies}
                            ${EXTRA_DEPENDENCIES_FOR_Generic})
    SET(GenericLib "Generic")
  ELSE()
    SET(GenericLib)
  ENDIF()

  # Check if this solution should include static feature modules.
  LIST(FIND SERIFSLNARG_OPTIONS NO_STATIC_FEATURE_MODULES no_static_fm)
  IF (no_static_fm LESS 0)

    # Include statically linked feature module projects (including
    # langauge modules).
    FOREACH(PROJ ${STATICALLY_LINKED_FEATURE_MODULES})
      INCLUDE_SERIF_MSPROJECT(${PROJ} ${GenericLib} ${other_dependencies}
                              ${EXTRA_DEPENDENCIES_FOR_${PROJ}})
    ENDFOREACH(PROJ)

    # # Include the StaticFeatureModules project
    # INCLUDE_SERIF_MSPROJECT(StaticFeatureModules
    # 			    ${other_dependencies}
    # 			    ${STATICALLY_LINKED_FEATURE_MODULES}
    #                         ${EXTRA_DEPENDENCIES_FOR_StaticFeatureModules})
  ENDIF()

  # Include the solution-specific projects.  Make them dependent on
  # generic and on any statically linked feature modules.
  FOREACH(name_template ${SERIFSLNARG_PROJECTS})
    INCLUDE_SERIF_MSPROJECT("${name_template}"
                            ${GenericLib}
			    ${STATICALLY_LINKED_FEATURE_MODULES}
			    #StaticFeatureModules
			    ${other_dependencies}
                            ${EXTRA_DEPENDENCIES_FOR_${name_template}})
  ENDFOREACH(name_template)

  # Add a windows shortcut in the build directory, so all of the
  # solutions can be found in one place.
  EXECUTE_PROCESS(COMMAND
      perl ${SERIF_PROJECT_SOURCE_DIR}/CMake/create_shortcut.pl
           ${PROJECT_BINARY_DIR}/${name}.sln
           ${PROJECT_BINARY_DIR}
           ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.sln.lnk)

ENDMACRO(ADD_SERIF_SOLUTION)

MACRO(PARSE_DEPENDENCY_PAIRS prefix)
  FOREACH(extra_dep ${ARGN})
    IF(NOT ${extra_dep} MATCHES "^[^:]+:[^:]+$")
      MESSAGE(FATAL_ERROR "Dependencies must have the form "
           "\"proj:dep\".  Got dependency: \"${extra_dep}\".")
    ENDIF(NOT ${extra_dep} MATCHES "^[^:]+:[^:]+$")
    IF (${extra_dep} MATCHES "<Language>")
      MESSAGE(FATAL_ERROR "<Language> in dependencies is no longer allowed")
    ENDIF (${extra_dep} MATCHES "<Language>")
    STRING(REGEX REPLACE "(^[^:]+):([^:]+$)" "\\1" proj "${extra_dep}")
    STRING(REGEX REPLACE "(^[^:]+):([^:]+$)" "\\2" dep "${extra_dep}")
    LIST(APPEND "${prefix}_${proj}" "${dep}")
    LIST(REMOVE_DUPLICATES "${prefix}_${proj}")
    #MESSAGE("${proj} depends on ${dep}")
  ENDFOREACH(extra_dep)
ENDMACRO(PARSE_DEPENDENCY_PAIRS)

###########################################################################
# INCLUDE_SERIF_MSPROJECT(<library> <dependencies...>)
#--------------------------------------------------------------------------
# Include the Visual Studio project for a given library in the given
# library in the solution that we're currently assembling.
###########################################################################
MACRO(INCLUDE_SERIF_MSPROJECT library)
  # Determine the location of the vcproj file for this library.
  GET_TARGET_PROPERTY(LOC ${library} LOCATION)
  IF(NOT LOC)
    MESSAGE(FATAL_ERROR "Unable to find location of \"${library}\"")
  ENDIF(NOT LOC)
  STRING(REGEX REPLACE "\\$\\(OutDir\\).*$"
         "${library}.vcproj" VCPROJ "${LOC}")
  #MESSAGE("[${library}] [${VCPROJ}]")
  SET(DEPENDENCIES ${ARGN})
  LIST(REMOVE_ITEM DEPENDENCIES Unspecified)

  SET(DEPENDENCIES)
  FOREACH(ARG ${ARGN})
    IF(ARG STREQUAL Unspecified)
      # Not a real library -- ignore in this context.
    ELSEIF(ARG STREQUAL Configurations)
      LIST(APPEND DEPENDENCIES Configurations)
    ELSE()
      LIST(APPEND DEPENDENCIES ${ARG}_msproj)
    ENDIF()
  ENDFOREACH(ARG ${ARGN})

  INCLUDE_EXTERNAL_MSPROJECT(${library}_msproj ${VCPROJ} ${DEPENDENCIES})
ENDMACRO(INCLUDE_SERIF_MSPROJECT)

###########################################################################
# SAFE_FILE_GLOB(<variable> [RELATIVE <path>]
#                           [<globbing expressions>...])
#--------------------------------------------------------------------------
# This macro is similar to FILE(GLOB...), except it complains if any
# of the globbing expressions do not expand to any files.
#
# This macro is mainly intended to help catch typos in filenames (rather
# than silently discarding those filenames).
###########################################################################
MACRO(SAFE_FILE_GLOB var)
  SET(${var})
  SET(RELATIVE_PATH)
  FOREACH(ARG ${ARGN})
    IF (ARG STREQUAL RELATIVE)
      SET(RELATIVE_PATH "**")
    ELSEIF (RELATIVE_PATH STREQUAL "**")
      SET(RELATIVE_PATH ${ARG})
    ELSE()
      IF(RELATIVE_PATH)
        FILE(GLOB EXPANDED RELATIVE "${RELATIVE_PATH}" ${ARG})
      ELSE(RELATIVE_PATH)
        FILE(GLOB EXPANDED ${ARG})
      ENDIF(RELATIVE_PATH)
      IF (NOT EXPANDED)
        MESSAGE(FATAL_ERROR "WARNING: in ${CMAKE_CURRENT_SOURCE_DIR}, "
                "\"/${ARG}\" did not match any files")
      ENDIF (NOT EXPANDED)
      SET(${var} ${${var}} ${EXPANDED})
    ENDIF ()
  ENDFOREACH(ARG)
ENDMACRO(SAFE_FILE_GLOB)

