# Locate The MTDecoder library

# This module defines
# MTDecoder_LIBRARIES, the libraries to link against.
# MTDecoder_FOUND, If false, don't try to use MTDecoder.

#MESSAGE(STATUS "Looking for MTDecoder library (for transliteration)")
SET(MTDecoder_USE_STATIC_LIBS ON CACHE BOOL "Use static MT decoder library?")

#IF(NOT $ENV{MT_DECODER_ROOT})
#  MESSAGE(STATUS "  Environment variable MT_DECODER_ROOT not set, using default search paths")
#ENDIF()

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set( MTDecoder_ARCH "x86_64" )
else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set( MTDecoder_ARCH "i686" )
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )

SET( _mtdecoder_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

# Find the static version of the library
IF(WIN32)
  SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ELSE(WIN32)
  SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ENDIF(WIN32)
FIND_LIBRARY(MTDecoder_static_LIBRARY 
  NAMES
    Decoder
  PATHS
    $ENV{MT_DECODER_ROOT}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../External/MTDecoder/${MTDecoder_ARCH}
    /d4m/serif/External/MTDecoder/${MTDecoder_ARCH})

# Find the shared (dynamic) version of the library
IF(WIN32)
  SET(CMAKE_FIND_LIBRARY_SUFFIXES .dll ${CMAKE_FIND_LIBRARY_SUFFIXES})
ELSE(WIN32)
  SET(CMAKE_FIND_LIBRARY_SUFFIXES .so ${CMAKE_FIND_LIBRARY_SUFFIXES})
ENDIF(WIN32)
FIND_LIBRARY(MTDecoder_shared_LIBRARY 
  NAMES
    Decoder
  PATHS
    $ENV{MT_DECODER_ROOT}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../External/MTDecoder/${MTDecoder_ARCH}
    /d4m/serif/External/MTDecoder/${MTDecoder_ARCH})

# Copy the desired library version to MTDecoder_LIBRARY
IF( MTDecoder_USE_STATIC_LIBS )
  SET(MTDecoder_LIBRARY ${MTDecoder_static_LIBRARY})
ELSE( MTDecoder_USE_STATIC_LIBS )
  SET(MTDecoder_LIBRARY ${MTDecoder_shared_LIBRARY})
ENDIF( MTDecoder_USE_STATIC_LIBS )

# Restore the original find-library suffix list.
SET(CMAKE_FIND_LIBRARY_SUFFIXES ${_mtdecoder_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})

IF(MTDecoder_LIBRARY)
  SET( MTDecoder_FOUND "YES" )
ENDIF(MTDecoder_LIBRARY)

set(MTDecoder_LIBRARIES ${MTDecoder_LIBRARY} )

# Check for REQUIRED, QUIET
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MTDecoder "MTDecoder was not found." MTDecoder_LIBRARY)
SET(MTDecoder_FOUND ${MTDecoder_FOUND})

MARK_AS_ADVANCED(
  MTDecoder_INCLUDE_DIR
  MTDecoder_LIBRARY
)
