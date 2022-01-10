###############################################################
# Copyright (c) 2011 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# Postgres                                                    #
#                                                             #
###############################################################


SET(USE_POSTGRES No CACHE BOOL
  "Should allow code that uses PostgreSQL?")
IF(USE_POSTGRES)
  ADD_DEFINITIONS( -DUSE_POSTGRES )
  FIND_PACKAGE(Libpqxx REQUIRED)
  IF(LIBPQXX_FOUND)
    INCLUDE_DIRECTORIES("${LIBPQXX_INCLUDE}")
  ENDIF()
ENDIF()

IF (USE_POSTGRES)
  ADD_SERIF_FEATURE_MODULE(Postgres
    SOURCE_FILES
      PostgresModule.cpp
      PostgresModule.h
      PostgresDBConnection.cpp
      PostgresDBConnection.h
    LINK_LIBRARIES
      Generic 
      ${LIBPQXX_LIBRARY}
      ${LIBPQXX_LIBPQ_LIBRARY}
      ${OpenSSL_LIBRARY}
      ${OpenSSL_LINK_FLAG}
    UNIX_LINK_LIBRARIES
      -lz
      -lssl
  )
ENDIF()
