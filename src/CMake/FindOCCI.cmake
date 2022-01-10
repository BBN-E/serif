# Locate OCCI include paths and libraries

SET(OCCI_PATHS
  /opt/oracle-instantclient_11_2-x86_64/
  "$ENV{ORACLE_HOME}"
)

STRING(REGEX REPLACE "([^;]+)" "\\1" OCCI_LIB_PATHS1 "${OCCI_PATHS}")
STRING(REGEX REPLACE "([^;]+)" "\\1/lib" OCCI_LIB_PATHS2 "${OCCI_PATHS}")
STRING(REGEX REPLACE "([^;]+)" "\\1/sdk/include" OCCI_INCLUDE_PATHS "${OCCI_PATHS}")


FIND_PATH(OCCI_INCLUDE occi.h ${OCCI_INCLUDE_PATHS})

LIST(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".so.11.1")      ## HACK!
FIND_LIBRARY(OCCI_LIBRARY
    NAMES occi
    PATHS ${OCCI_LIB_PATHS1} ${OCCI_LIB_PATHS2})
FIND_LIBRARY(OCCI_NNZ_LIBRARY
    NAMES nnz11
    PATHS ${OCCI_LIB_PATHS1} ${OCCI_LIB_PATHS2})
FIND_LIBRARY(OCCI_CLNTSH_LIBRARY
    NAMES clntsh
    PATHS ${OCCI_LIB_PATHS1} ${OCCI_LIB_PATHS2})
LIST(REMOVE_ITEM CMAKE_FIND_LIBRARY_SUFFIXES ".so.11.1") ## FIX HACK

MESSAGE(STATUS "Found Oracle (OCCI): ${OCCI_LIBRARY} " 
                        "${OCCI_NNZ_LIBRARY} "
                        "${OCCI_CLNTSH_LIBRARY}" )
IF ( (OCCI_INCLUDE) AND (OCCI_LIBRARY) AND (OCCI_NNZ_LIBRARY) AND (OCCI_CLNTSH_LIBRARY) )
    SET (OCCI_FOUND YES)
    SET (OCCI_LIBRARIES "${OCCI_LIBRARY}" 
                        "${OCCI_NNZ_LIBRARY}"
                        "${OCCI_CLNTSH_LIBRARY}" )
    MESSAGE(STATUS 
        "\n\n[!!] Warning: Be sure that we hold appropriate licenses "
        "before releasing any binaries that use Oracle [!!]\n")
ELSE()
    MESSAGE(STATUS "Oracle client (OCCI) not found")
ENDIF()

# Check for REQUIRED, QUIET
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OCCI
    "OCCI client was not found." 
    OCCI_LIBRARIES OCCI_INCLUDE)
