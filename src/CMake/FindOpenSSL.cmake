# Find OpenSSL include paths and libraries

IF(DEFINED ENV{OPENSSL_ROOT})
  SET(OPEN_SSL_HOME $ENV{OPENSSL_ROOT})
ELSE()
  SET(OPEN_SSL_HOME ${CMAKE_CURRENT_SOURCE_DIR}/../../External/openssl-1.0.1g)
ENDIF()

# Set platform-specific library build paths
IF(WIN32 OR WIN64)
  IF ((CMAKE_CL_64) OR (CMAKE_GENERATOR MATCHES "Win64"))
    SET(OpenSSL_build_subdir x64)
  ELSE()
    SET(OpenSSL_build_subdir Win32)
  ENDIF()
ELSE()
  IF ((CMAKE_CL_64) OR (CMAKE_GENERATOR MATCHES "x86_64"))
    SET(OpenSSL_build_subdir x86_64)
  ELSE()
    SET(OpenSSL_build_subdir i686)
  ENDIF()
ENDIF()

FIND_PATH(OpenSSL_INCLUDE_DIR openssl/aes.h
  "${OPEN_SSL_HOME}/${OpenSSL_build_subdir}/include/"
  /usr/local/include
  /usr/include
  )

# Get the actual library path
FIND_LIBRARY(OpenSSL_LIBRARY
  NAMES
      libeay32.lib      # windows
      libcrypto.a       # linux
      libcrypto.dylib   # os x
  PATHS
      "${OPEN_SSL_HOME}/${OpenSSL_build_subdir}/lib/"
      /usr/local/lib
      /usr/lib
  )

IF (WIN)
SET(OpenSSL_LIBEAY_DLL
  "${OPEN_SSL_HOME}/${OpenSSL_build_subdir}/bin/libeay32.dll"
  CACHE FILEPATH "OpenSSL dynamic library"
  )
SET(OpenSSL_SSLEAY_DLL
  "${OPEN_SSL_HOME}/${OpenSSL_build_subdir}/bin/ssleay32.dll"
  CACHE FILEPATH "OpenSSL dynamic library"
  )
ENDIF (WIN)

IF(OpenSSL_INCLUDE_DIR)
    IF(OpenSSL_LIBRARY)
        SET (OpenSSL_FOUND "YES")
	      MESSAGE(STATUS "Found OpenSSL: ${OpenSSL_INCLUDE_DIR} (${OpenSSL_LIBRARY})")
	      INCLUDE_DIRECTORIES(${OpenSSL_INCLUDE_DIR})
    ENDIF(OpenSSL_LIBRARY)
ENDIF(OpenSSL_INCLUDE_DIR)
