# Locate MySQL include paths and libraries

SET(MySQL_PATHS
  "C:\\Program Files (x86)\\MySQL\\MySQL Connector C 6.0.1"
  "C:\\Program Files (x86)\\MySQL\\MySQL Connector C 6.0.2"
  "C:\\Program Files (x86)\\MySQL\\MySQL Connector C 6.1"
  "\\\\raid68\\u17\\serif\\External\\MySQL Connector C 6.1"
  "${SERIF_PROJECT_SOURCE_DIR}/../../External/mysql-connector-c-6.0.2-linux-glibc2.3-x86-64bit"
  "/opt/mysql-connector-c-6.0.2-linux-glibc2.3-x86-64bit"
)

STRING(REGEX REPLACE "([^;]+)" "\\1/lib" MySQL_LIB_PATHS1 "${MySQL_PATHS}")
STRING(REGEX REPLACE "([^;]+)" "\\1/lib/opt" MySQL_LIB_PATHS2 "${MySQL_PATHS}")
STRING(REGEX REPLACE "([^;]+)" "\\1/include" MySQL_INCLUDE_PATHS "${MySQL_PATHS}")


FIND_PATH(MySQLClient_INCLUDE mysql.h ${MySQL_INCLUDE_PATHS} /usr/local/include/mysql)

FIND_LIBRARY(MySQLClient_libmysql_LIBRARY
    NAMES libmysql mysql libmysqld.a
    PATHS ${MySQL_LIB_PATHS1} ${MySQL_LIB_PATHS2} /usr/local/lib)
FIND_LIBRARY(MySQLClient_mysqlclient_LIBRARY
    NAMES libmysqlclient mysqlclient 
    PATHS ${MySQL_LIB_PATHS1} ${MySQL_LIB_PATHS2} /usr/local/lib)

IF ( (MySQLClient_INCLUDE) AND 
     (MySQLClient_libmysql_LIBRARY) AND
     (MySQLClient_mysqlclient_LIBRARY) )
    SET (MySQLClient_FOUND YES)
    IF (OSX)
      SET (MySQLClient_LIBRARIES "${MySQLClient_mysqlclient_LIBRARY}")
    ELSE (OSX)
      SET (MySQLClient_LIBRARIES "${MySQLClient_libmysql_LIBRARY}" 
                                 "${MySQLClient_mysqlclient_LIBRARY}")
    ENDIF (OSX)
    MESSAGE(STATUS 
        "\n\n[!!] Warning: Be sure that we hold appropriate licenses "
        "before releasing any binaries that use MySQL [!!]\n")
ELSE()
    MESSAGE(STATUS "MySQL client not found")
ENDIF()

# Check for REQUIRED, QUIET
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MySQLClient
    "MySQL client was not found." 
    MySQLClient_LIBRARIES MySQLClient_INCLUDE)
