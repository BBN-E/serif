####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/descriptors                                              #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(descriptors
  SOURCE_FILES
    AppositiveClassifier.cpp
    AppositiveClassifier.h
    ClassifierTreeSearch.cpp
    ClassifierTreeSearch.h
    CompoundMentionFinder.cpp
    CompoundMentionFinder.h
    DescClassModifiers.cpp
    DescClassModifiers.h
    DescriptorClassifier.cpp
    DescriptorClassifier.h
    DescriptorRecognizer.cpp
    DescriptorRecognizer.h
    ListClassifier.cpp
    ListClassifier.h
    MentionClassifier.h
    NestedClassifier.cpp
    NestedClassifier.h
    NomPremodClassifier.cpp
    NomPremodClassifier.h
    NoneClassifier.cpp
    NoneClassifier.h
    PartitiveClassifier.cpp
    PartitiveClassifier.h
    PronounClassifier.cpp
    PronounClassifier.h
    SubtypeClassifier.cpp
    SubtypeClassifier.h
    xx_CompoundMentionFinder.cpp
    xx_CompoundMentionFinder.h
    xx_DescClassModifiers.h
    xx_DescriptorClassifier.h
    xx_DescriptorRecognizer.h
    xx_NomPremodClassifier.h
    xx_PronounClassifier.h
  SUBDIRS
    DescriptorClassifierTrainer
    discmodel
)
