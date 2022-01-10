###############################################################
# Copyright (c) 2011 by Raytheon BBN Technologies Corp.       #
# All Rights Reserved.                                        #
#                                                             #
# Generic/descriptors/discmodel                               #
#                                                             #
###############################################################

ADD_SERIF_LIBRARY_SUBDIR(discmodel
  SOURCE_FILES
    DescriptorObservation.cpp
    DescriptorObservation.h
    P1DescFeatureType.cpp
    P1DescFeatureType.h
    P1DescFeatureTypes.cpp
    P1DescFeatureTypes.h
    P1DescTrainer.cpp
    P1DescTrainer.h
    P1DescriptorClassifier.cpp
    P1DescriptorClassifier.h
    xx_P1DescFeatureTypes.h
  SUBDIRS
    ### not used ### descriptors
    featuretypes
)
