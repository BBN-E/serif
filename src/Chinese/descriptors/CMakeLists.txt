####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Chinese/descriptors                                                       #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(descriptors
  SOURCE_FILES
    ch_CompoundMentionFinder.cpp
    ch_CompoundMentionFinder.h
    ch_DescClassModifiers.cpp
    ch_DescClassModifiers.h
    ch_DescriptorClassifier.cpp
    ch_DescriptorClassifier.h
    ch_PMDescriptorClassifier.cpp
    ch_PMDescriptorClassifier.h
    ch_PronounClassifier.cpp
    ch_PronounClassifier.h
    DescriptorClassifierTrainer/ch_DescriptorClassifierTrainer.cpp
    DescriptorClassifierTrainer/ch_DescriptorClassifierTrainer.h
    discmodel/ch_P1DescFeatureTypes.cpp
    discmodel/ch_P1DescFeatureTypes.h
    discmodel/featuretypes/ContainsCharFT.h
    discmodel/featuretypes/LastCharFT.h
    discmodel/featuretypes/PrepFuncParentFT.h
    discmodel/featuretypes/PropCopulaFT.h
    discmodel/featuretypes/PropIObjFT.h
    discmodel/featuretypes/PropObjFT.h
    discmodel/featuretypes/PropSubjFT.h
)
