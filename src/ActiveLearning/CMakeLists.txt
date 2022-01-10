####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# ActiveLearning                                                   #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY(ActiveLearning
  SUBDIRS
    alphabet
    exceptions
    objectives
    strategies
    ui
  SOURCE_FILES
    ActiveLearningData.h
    ActiveLearningData.cpp
    ActiveLearningUtilities.h
    ActiveLearningUtilities.cpp
    AnnotatedFeatureRecord.h
    AnnotatedFeatureRecord.cpp
    AnnotatedInstanceRecord.h
    AnnotatedInstanceRecord.cpp
    DataView.h
    DataView.cpp
    EventUtilities.h
    EventUtilities.cpp
    FeatureAnnotationDBView.h
    FeatureAnnotationDBView.cpp
    FeatureGrid.h
    InstanceAnnotationDB.h
    InstanceAnnotationDB.cpp
    InstanceHashes.h
    InstanceHashes.cpp
    MultilabelInferenceDataView.h
    MultilabelInferenceDataView.cpp
    SmoothedL1.h
    SmoothedL1.cpp
    StringStore.h
    StringStore.cpp
)
