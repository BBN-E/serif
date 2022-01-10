###############################################################
# Copyright (c) 2011 by Raytheon BBN Technologies Corp.       #
# All Rights Reserved.                                        #
#                                                             #
# English/descriptors
# The name of our project is "Serif or Standalones".          #
#                                                             #
###############################################################

ADD_SERIF_LIBRARY_SUBDIR(descriptors
  SOURCE_FILES
    en_CompoundMentionFinder.cpp
    en_CompoundMentionFinder.h
    en_DescClassModifiers.cpp
    en_DescClassModifiers.h
    en_DescriptorClassifier.cpp
    en_DescriptorClassifier.h
    en_NomPremodClassifier.cpp
    en_NomPremodClassifier.h
    en_PMDescriptorClassifier.cpp
    en_PMDescriptorClassifier.h
    en_PronounClassifier.cpp
    en_PronounClassifier.h
    en_TemporalIdentifier.cpp
    en_TemporalIdentifier.h
  SUBDIRS
    DescriptorClassifierTrainer
    discmodel
)
