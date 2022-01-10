####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/events/stat                                              #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(stat
  SOURCE_FILES
    ETCluster1216VectorFilter.h
    ETCluster16VectorFilter.h
    ETCluster81216VectorFilter.h
    ETObjectArgVectorFilter.h
    ETProbModelSet.cpp
    ETProbModelSet.h
    ETSubjectArgVectorFilter.h
    ETWordNetVectorFilter.h
    ETWordPriorVectorFilter.h
    ETWordVectorFilter.h
    EventAAFeatureType.cpp
    EventAAFeatureType.h
    EventAAFeatureTypes.cpp
    EventAAFeatureTypes.h
    EventAAObservation.cpp
    EventAAObservation.h
    EventArgumentFinder.cpp
    EventArgumentFinder.h
    EventPMStats.h
    EventTriggerFeatureType.cpp
    EventTriggerFeatureType.h
    EventTriggerFeatureTypes.cpp
    EventTriggerFeatureTypes.h
    EventTriggerFilter.h
    EventTriggerFinder.cpp
    EventTriggerFinder.h
    EventTriggerObservation.cpp
    EventTriggerObservation.h
#    EventTriggerSentence.cpp   # not included
#    EventTriggerSentence.h # not included
    PotentialEventMention.h
    StatEventFinder.cpp
    StatEventFinder.h
    StatEventTrainer.cpp
    StatEventTrainer.h
    specific_pm_event_models.h
    xx_EventAAFeatureTypes.h
    xx_EventTriggerFeatureTypes.h
    EventModalityFeatureType.cpp
    EventModalityFeatureType.h
    EventModalityFeatureTypes.cpp
    EventModalityFeatureTypes.h
    EventModalityClassifier.cpp
    EventModalityClassifier.h
    EventModalityObservation.cpp
    EventModalityObservation.h
    xx_EventModalityFeatureTypes.h
  SUBDIRS
    featuretypes
)
