####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Chinese/events
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(events
  SOURCE_FILES
    ch_EventLinker.cpp
    ch_EventLinker.h
    ch_EventUtilities.cpp
    ch_EventUtilities.h
    stat/ch_EventAAFeatureTypes.cpp
    stat/ch_EventAAFeatureTypes.h
    stat/ch_EventTriggerFeatureTypes.cpp
    stat/ch_EventTriggerFeatureTypes.h
    stat/featuretypes/ch_AACandLastCharFeatureType.h
    stat/featuretypes/ch_ETContainsCharFeatureType.h
    stat/featuretypes/ch_ETLastCharFeatureType.h
)
