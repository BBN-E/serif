# Locate libpqxx include paths and libraries
# libpqxx is the PostgreSQL driver for C++

FIND_PATH(LIBPQXX_INCLUDE pqxx
  ${COTS_DIR}/opt/libpqxx-4.0$ENV{ARCH_SUFFIX}/include
  /usr/local/include
  /usr/include
  "${SERIF_PROJECT_SOURCE_DIR}/../../External/libpqxx-4.0.1/include"
)

IF(WIN32 OR WIN64)
  IF ((CMAKE_CL_64) OR (CMAKE_GENERATOR MATCHES "Win64"))
    SET(LIBPQXX_BUILD_SUBDIR x64)
  ELSE()
    SET(LIBPQXX_BUILD_SUBDIR Win32)
  ENDIF()
ENDIF()

FIND_LIBRARY(LIBPQXX_LIBRARY_RELEASE
  NAMES
    libpqxx # windows
    pqxx    # linux
  PATHS
    ${COTS_DIR}/opt/libpqxx-4.0$ENV{ARCH_SUFFIX}/lib
    /usr/local/lib
    /usr/lib
    "${SERIF_PROJECT_SOURCE_DIR}/../../External/libpqxx-4.0.1/${LIBPQXX_BUILD_SUBDIR}/Release"
)

IF (UNIX)
  # Just use release library on Unix systems
  SET(LIBPQXX_LIBRARY
    ${LIBPQXX_LIBRARY_RELEASE}
    CACHE FILEPATH "libpqxx Unix static Release library"
    )
ELSE (UNIX)
  # We need Release/Debug split on Windows to avoid C++ runtime conflict
  FIND_LIBRARY(LIBPQXX_LIBRARY_DEBUG
    NAMES
      libpqxxD
    PATHS
      "${SERIF_PROJECT_SOURCE_DIR}/../../External/libpqxx-4.0.1/${LIBPQXX_BUILD_SUBDIR}/Debug"
  )

  SET(LIBPQXX_LIBRARY
    debug ${LIBPQXX_LIBRARY_DEBUG}
    optimized ${LIBPQXX_LIBRARY_RELEASE}
    CACHE FILEPATH "libpqxx Windows dynamic import Debug/Release library"
    )
ENDIF (UNIX)

IF (WIN)
  STRING(REGEX REPLACE "\\.lib$" ".dll" LIBPQXX_DLL_RELEASE ${LIBPQXX_LIBRARY_RELEASE})
  SET(LIBPQXX_DLL_RELEASE
    ${LIBPQXX_DLL_RELEASE}
    CACHE FILEPATH "libpqxx DLL"
    )
  STRING(REGEX REPLACE "\\.lib$" ".dll" LIBPQXX_DLL_DEBUG ${LIBPQXX_LIBRARY_DEBUG})
  SET(LIBPQXX_DLL_DEBUG
    ${LIBPQXX_DLL_DEBUG}
    CACHE FILEPATH "libpqxx DLL with debug information"
    )
ENDIF (WIN)

IF (UNIX)
  FIND_LIBRARY(LIBPQXX_LIBPQ_LIBRARY
    NAMES
      libpq.a
    PATHS
      ${COTS_DIR}/opt/postgresql-9.3.5$ENV{ARCH_SUFFIX}/lib
      ${COTS_DIR}/opt/postgresql-9.3.2$ENV{ARCH_SUFFIX}/lib
    )
ENDIF (UNIX)

IF ("${LIBPQXX_LIBRARY}" MATCHES ".*_static.*")
  FIND_LIBRARY(LIBPQXX_LIBPQ_LIBRARY
    NAMES
      libpq.lib
    PATHS
      "${SERIF_PROJECT_SOURCE_DIR}/../../External/postgresql-9.4/${LIBPQXX_BUILD_SUBDIR}"
  )
ENDIF ("${LIBPQXX_LIBRARY}" MATCHES ".*_static.*")

IF (WIN)
SET(LIBPQXX_LIBPQ_DLL
  "${SERIF_PROJECT_SOURCE_DIR}/../../External/postgresql-9.4/${LIBPQXX_BUILD_SUBDIR}/libpq.dll"
  CACHE FILEPATH "PostgreSQL dynamic library"
  )
IF ((CMAKE_CL_64) OR (CMAKE_GENERATOR MATCHES "Win64"))
  SET(LIBPQXX_INTL_DLL_NAME libintl-8)
ELSE()
  SET(LIBPQXX_INTL_DLL_NAME intl)
ENDIF()
SET(LIBPQXX_INTL_DLL
  "${SERIF_PROJECT_SOURCE_DIR}/../../External/postgresql-9.4/${LIBPQXX_BUILD_SUBDIR}/${LIBPQXX_INTL_DLL_NAME}.dll"
  CACHE FILEPATH "i18n dynamic library"
  )
ENDIF (WIN)

IF(LIBPQXX_INCLUDE)
  IF(LIBPQXX_LIBRARY)
	  SET( LIBPQXX_FOUND "YES" )
  ENDIF(LIBPQXX_LIBRARY)
ENDIF(LIBPQXX_INCLUDE)

# Check for REQUIRED, QUIET
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBPQXX
    "libpqxx was not found."
    LIBPQXX_INCLUDE LIBPQXX_LIBRARY)
SET(LIBPQXX_FOUND ${LIBPQXX_FOUND})

MARK_AS_ADVANCED(
  LIBPQXX_INCLUDE
  LIBPQXX_LIBRARY
  LIBPQXX_DLL_DEBUG
  LIBPQXX_DLL_RELEASE
  LIBPQXX_LIBPQ_LIBRARY
  LIBPQXX_LIBPQ_DLL
  LIBPQXX_INTL_DLL
)
