####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Arabic/descriptors
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(descriptors
  SOURCE_FILES
    ar_CompoundMentionFinder.cpp
    ar_CompoundMentionFinder.h
    ar_DescClassModifiers.cpp
    ar_DescClassModifiers.h
    ar_DescriptorClassifier.cpp
    ar_DescriptorClassifier.h
    ar_PMDescriptorClassifier.cpp
    ar_PMDescriptorClassifier.h
    ar_PronounClassifier.cpp
    ar_PronounClassifier.h
    DescriptorClassifierTrainer/ar_DescriptorClassifierTrainer.cpp
    DescriptorClassifierTrainer/ar_DescriptorClassifierTrainer.h
    discmodel/ar_P1DescFeatureTypes.cpp
    discmodel/ar_P1DescFeatureTypes.h
)
