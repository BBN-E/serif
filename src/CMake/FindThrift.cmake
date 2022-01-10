# - Find Thrift (a cross platform RPC lib/tool)
# This module defines
# Thrift_VERSION, version string of ant if found
# Thrift_INCLUDE_DIR, where to find Thrift headers
# Thrift_LIBS, Thrift libraries
# Thrift_FOUND, If false, do not try to use ant

if(WIN)
  set(Thrift_HOME_PATHS
      ${CMAKE_SOURCE_DIR}/../../External/thrift-0.9.0
      CACHE String "List of directories where Thrift might be found")
  find_path(Thrift_HOME lib/cpp/src/thrift/Thrift.h
            PATHS ${Thrift_HOME_PATHS})
else()
  set(Thrift_HOME_PATHS
      /opt/thrift-0.9.0-x86_64
      CACHE String "List of directories where Thrift might be found")
  find_path(Thrift_HOME include/thrift/Thrift.h
            PATHS ${Thrift_HOME_PATHS})
endif(WIN)

if(Thrift_HOME)
  if(WIN)
    set(Thrift_VERSION "Thrift version")
    set(Thrift_LIB_PATHS "${Thrift_HOME}/lib/cpp/Release")  
    set(Thrift_INCLUDE_PATHS "${Thrift_HOME}/lib/cpp/src")
  else()
    exec_program(
        ${Thrift_HOME}/bin/thrift
        ARGS -version 
        OUTPUT_VARIABLE Thrift_VERSION
        RETURN_VALUE Thrift_RETURN)
    set(Thrift_LIB_PATHS 
        "${Thrift_HOME}/lib" "/usr/local/lib" "/opt/local/lib")  
    set(Thrift_INCLUDE_PATHS 
        "${Thrift_HOME}/include" "/usr/local/include" "/opt/local/include")
  endif(WIN)
  
  # Find the thrift include & lib.
  find_path(Thrift_INCLUDE_DIR thrift/Thrift.h 
      PATHS ${Thrift_INCLUDE_PATHS})
  
  find_library(Thrift_LIB NAMES thrift libthrift PATHS ${Thrift_LIB_PATHS})
  find_library(Thrift_NB_LIB NAMES thriftnb libthriftnb PATHS ${Thrift_LIB_PATHS})
  
  if (Thrift_VERSION MATCHES "^Thrift version" AND 
      Thrift_LIB AND Thrift_NB_LIB AND Thrift_INCLUDE_DIR)
    set(Thrift_FOUND TRUE)
    set(Thrift_LIBS ${Thrift_LIB} ${Thrift_NB_LIB})
  else ()
    set(Thrift_FOUND FALSE)
  endif ()
else()
    set(Thrift_FOUND FALSE)
endif()  
  
if (Thrift_FOUND)
  if (NOT Thrift_FIND_QUIETLY)
    message(STATUS "Found thrift: ${Thrift_LIBS}")
    message(STATUS " thrift compiler: ${Thrift_VERSION}")
  endif ()
else ()
  message(STATUS "Thrift compiler/libraries NOT found. "
  "Thrift support will be disabled (${Thrift_RETURN}, "
  "${Thrift_INCLUDE_DIR}, ${Thrift_LIB}, ${Thrift_NB_LIB})")
endif ()

mark_as_advanced(
  Thrift_LIB
  Thrift_NB_LIB
  Thrift_INCLUDE_DIR
)
