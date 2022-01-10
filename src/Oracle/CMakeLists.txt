###############################################################
# Copyright (c) 2011 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# Oracle                                                       #
#                                                             #
###############################################################

SET(USE_ORACLE No CACHE BOOL
  "Should allow code that uses Oracle?  Warning: be sure that we hold appropriate licenses before releasing any binaries built with this flag enabled!")
IF(USE_ORACLE)
  ADD_DEFINITIONS( -DUSE_ORACLE )
  FIND_PACKAGE(OCCI REQUIRED)
  IF(OCCI_FOUND)
    INCLUDE_DIRECTORIES("${OCCI_INCLUDE}")
  ENDIF()
ENDIF()

IF (USE_ORACLE)
  ADD_SERIF_FEATURE_MODULE(Oracle
    SOURCE_FILES
      OracleModule.h
      OracleModule.cpp
      OracleDBConnection.h
      OracleDBConnection.cpp
    LINK_LIBRARIES
      Generic
      ${OCCI_LIBRARIES}
  )
ENDIF()
