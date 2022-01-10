####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/driver                                                   #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(driver
  SOURCE_FILES
    Batch.cpp
    Batch.h
    DocumentDriver.cpp
    DocumentDriver.h
    SentenceDriver.cpp
    SentenceDriver.h
    SessionProgram.cpp
    SessionProgram.h
    Stage.cpp
    Stage.h

    QueueDriver.h
    QueueDriver.cpp
    DiskQueueDriver.h
    DiskQueueDriver.cpp
)
