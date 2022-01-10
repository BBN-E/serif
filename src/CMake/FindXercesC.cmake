# Locate Xerces-C include paths and libraries
# Xerces-C can be found at http://xml.apache.org/xerces-c/
# Written by Frederic Heem, frederic.heem _at_ telsey.it

# This module defines
# XercesC_INCLUDE_DIR, where to find ptlib.h, etc.
# XercesC_LIBRARIES, the libraries to link against to use pwlib.
# XercesC_FOUND, If false, don't try to use pwlib.

SET(XercesC_DIR_SEARCH $ENV{XERCESCROOT})

FIND_PATH(XercesC_INCLUDE_DIR xercesc/dom/DOM.hpp
  $ENV{XERCESCROOT}/src
  $ENV{XERCESCROOT}/include
  ${COTS_DIR}/opt/xerces-c-3.1.1-nocurl$ENV{ARCH_SUFFIX}/include
  ${COTS_DIR}/opt/xerces-c-3.1.3-without-curl$ENV{ARCH_SUFFIX}/include
  /usr/local/include
  /usr/include
  "${CMAKE_CURRENT_SOURCE_DIR}/../../External/XercesLib/src"
)

IF(WIN32 OR WIN64)
  IF ((CMAKE_CL_64) OR (CMAKE_GENERATOR MATCHES "Win64"))
    SET(XercesC_build_subdir x64)
    SET(XercesC_arch_flag -x64)
  ELSE()
    SET(XercesC_build_subdir Win32)
    SET(XercesC_arch_flag -win32)
  ENDIF()
ENDIF()

FIND_LIBRARY(XercesC_LIBRARIES
  NAMES
    libxerces-c.a
    xerces-c
    xerces-c_3
    xerces-c_3${XercesC_arch_flag}
    xerces-c_3_1
    xerces-c_3_1${XercesC_arch_flag}
  PATHS
    $ENV{XERCESCROOT}/${XercesC_build_subdir}/Release
    $ENV{XERCESCROOT}/lib
    ${COTS_DIR}/opt/xerces-c-3.1.1-nocurl$ENV{ARCH_SUFFIX}/lib
    ${COTS_DIR}/opt/xerces-c-3.1.3-without-curl$ENV{ARCH_SUFFIX}/lib
    /usr/local/lib
    /usr/lib
    "${CMAKE_CURRENT_SOURCE_DIR}/../../External/XercesLib/${XercesC_build_subdir}/Release"
)

# if the include a the library are found then we have it
IF(XercesC_INCLUDE_DIR)
  IF(XercesC_LIBRARIES)
    SET( XercesC_FOUND "YES" )
    #MESSAGE(STATUS "Found Xerces-C++ xercesc/dom/DOM.hpp: ${XercesC_INCLUDE_DIR}")
  ENDIF(XercesC_LIBRARIES)
ENDIF(XercesC_INCLUDE_DIR)

# If we're linking to a static version on windows, then set the 
# XERCES_STATIC_LIBRARY preprocessor variable.  This will prevent 
# the xercesc library from generating "__declspec(dllimport)"-type
# declarations that are inappropriate when statically linking.
IF(WIN)
  IF ("${XercesC_LIBRARIES}" MATCHES ".*-static")
    ADD_DEFINITIONS( -DXERCES_STATIC_LIBRARY )
    MESSAGE(STATUS "Found static Xerces library; setting XERCES_STATIC_LIBRARY define")
  ENDIF("${XercesC_LIBRARIES}" MATCHES ".*-static")
ELSE(WIN)
  IF ("${XercesC_LIBRARIES}" MATCHES ".*[.]a")
    ADD_DEFINITIONS( -DXERCES_STATIC_LIBRARY )
    MESSAGE(STATUS "Found static Xerces library; setting XERCES_STATIC_LIBRARY define")
  ENDIF("${XercesC_LIBRARIES}" MATCHES ".*[.]a")
ENDIF(WIN)

# Check for REQUIRED, QUIET
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(XercesC 
    "Xerces-C was not found.  Use environment variable XERCESCROOT to specify its location" 
    XercesC_INCLUDE_DIR)
SET(XercesC_FOUND ${XERCESC_FOUND})

MARK_AS_ADVANCED(
  XercesC_INCLUDE_DIR
  XercesC_LIBRARIES
)
