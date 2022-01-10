####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/database                                                   #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(database
  SOURCE_FILES
    DatabaseConnection.cpp
    DatabaseConnection.h
    QueryProfiler.h
    QueryProfiler.cpp	
    SqliteDBConnection.cpp
    SqliteDBConnection.h
)
