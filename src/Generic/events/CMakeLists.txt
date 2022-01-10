####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/events                                                   #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(events
  SOURCE_FILES
    EventFinder.cpp
    EventFinder.h
    PatternEventFinder.cpp
    PatternEventFinder.h
    EventLinker.cpp
    EventLinker.h
    EventUtilities.cpp
    EventUtilities.h
    xx_EventFinder.h
    xx_EventLinker.h
    xx_EventUtilities.h
  SUBDIRS
    patterns
    stat
)
