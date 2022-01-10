####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Temporal                                                         #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY(Temporal
  SUBDIRS
    features
  SOURCE_FILES
    AllPossibilitiesTemporalInstanceGenerator.h
    AllPossibilitiesTemporalInstanceGenerator.cpp
    FeatureMap.h
    FeatureMap.cpp
    ManualTemporalInstanceGenerator.h
    ManualTemporalInstanceGenerator.cpp
    TemporalAttribute.h
    TemporalAttribute.cpp
    TemporalAttributeAdder.h
    TemporalAttributeAdder.cpp
    TemporalAttributeAdderAutomaticAttachment.h
    TemporalAttributeAdderAutomaticAttachment.cpp
    TemporalAttributeAdderManualAttachment.h
    TemporalAttributeAdderManualAttachment.cpp
    TemporalDB.h
    TemporalDB.cpp
    TemporalFeatureGenerator.h
    TemporalFeatureGenerator.cpp
    TemporalInstance.h
    TemporalInstance.cpp
    TemporalInstanceGenerator.h
    TemporalInstanceGenerator.cpp
    TemporalInstanceSerialization.h
    TemporalInstanceSerialization.cpp
    TemporalTrainingDataGenerator.h
    TemporalTrainingDataGenerator.cpp
    TemporalTypeTable.h
    TemporalTypeTable.cpp
    TemporalTypes.h
  LINK_LIBRARIES
    Generic
    ActiveLearning
    LearnIt
    ${Boost_LIBRARIES}
)
