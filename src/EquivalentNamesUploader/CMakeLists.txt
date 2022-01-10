####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# EquivalentNamesUploader                                          #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(EquivalentNamesUploader
  INSTALL LearnIt
  SOURCE_FILES
    EquivalentNamesUploader.cpp
    database/Database.cpp
    database/Database.h
    database/DBQuery.cpp
    database/DBQuery.h
    database/ResultSet.cpp
    database/ResultSet.h
    database/ValueVector.cpp
    database/ValueVector.h
    database/XDocRetriever.cpp
    database/XDocRetriever.h
  LINK_LIBRARIES
    ActiveLearning
    LearnIt
)
