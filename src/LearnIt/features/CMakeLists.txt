####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# LearnIt/features                                                 #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(features
  SUBDIRS
    slot_identity
  SOURCE_FILES
    BrandyPatternFeature.h
    BrandyPatternFeature.cpp
    CanopyUtils.h
    Feature.h
    Feature.cpp
    FeatureCollection.h
    FeatureCollectionIterator.h
    FeatureCollectionIterator.cpp
    PropPatternFeatureCollection.h
    PropPatternFeatureCollection.cpp
    SentenceBiasFeature.h
    SentenceBiasFeature.cpp
    SlotBiasFeature.h
    SlotBiasFeature.cpp
    SlotContainsWordFeature.h
    SlotContainsWordFeature.cpp
    TextPatternFeatureCollection.h
    TextPatternFeatureCollection.cpp
    YYMMDDFeature.h
    YYMMDDFeature.cpp
    TopicFeature.h
    TopicFeature.cpp
)
