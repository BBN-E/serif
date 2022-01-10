###############################################################
# Copyright (c) 2011 by Raytheon BBN Technologies Corp.       #
# All Rights Reserved.                                        #
#                                                             #
# Arabic/descriptors
# The name of our project is "Serif or Standalones".          #
#                                                             #
###############################################################

ADD_SERIF_LIBRARY_SUBDIR(descriptors
  SOURCE_FILES
    es_DescriptorClassifier.cpp
    es_DescriptorClassifier.h
    es_CompoundMentionFinder.cpp
    es_CompoundMentionFinder.h
    es_PronounClassifier.cpp
    es_PronounClassifier.h

    #es_DescClassModifiers.cpp
    #es_DescClassModifiers.h
    #es_NomPremodClassifier.cpp
    #es_NomPremodClassifier.h
    #es_PMDescriptorClassifier.cpp
    #es_PMDescriptorClassifier.h
    #es_TemporalIdentifier.cpp
    #es_TemporalIdentifier.h
  SUBDIRS
    DescriptorClassifierTrainer
    discmodel
)
