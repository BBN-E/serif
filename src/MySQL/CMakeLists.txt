###############################################################
# Copyright (c) 2011 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# MySQL                                                       #
#                                                             #
###############################################################

SET(USE_MySQL No CACHE BOOL
  "Should allow code that uses MySQL?  Warning: be sure that we hold appropriate licenses before releasing any binaries built with this flag enabled!")
IF(USE_MySQL)
  ADD_DEFINITIONS( -DUSE_MYSQL )
  FIND_PACKAGE(MySQL REQUIRED)
  IF(MySQLClient_FOUND)
    INCLUDE_DIRECTORIES("${MySQLClient_INCLUDE}")
  ENDIF()
ENDIF()

IF (USE_MySQL)
  ADD_SERIF_FEATURE_MODULE(MySQL
    SOURCE_FILES
      MySQLModule.h
      MySQLModule.cpp
      MySQLDBConnection.h
      MySQLDBConnection.cpp
    LINK_LIBRARIES
      Generic
      ${MySQLClient_LIBRARIES}
  )
ENDIF()
