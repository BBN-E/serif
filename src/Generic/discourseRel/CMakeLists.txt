####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/discourseRel                                             #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(discourseRel
  SOURCE_FILES
    ConnectiveCorpusStatistics.cpp
    ConnectiveCorpusStatistics.h
    CrossSentRelation.cpp
    CrossSentRelation.h
    DiscourseRelFeatureType.cpp
    DiscourseRelFeatureType.h
    DiscourseRelFeatureTypes.cpp
    DiscourseRelFeatureTypes.h
    DiscourseRelObservation.cpp
    DiscourseRelObservation.h
    DiscourseRelTrainer.cpp
    DiscourseRelTrainer.h
    PennDiscourseTreebank.cpp
    PennDiscourseTreebank.h
    StopWordFilter.cpp
    StopWordFilter.h
    TargetConnectives.cpp
    TargetConnectives.h
    xx_DiscourseRelFeatureTypes.h
    featuretypes/DRIsFirstWordFeatureType.h
    featuretypes/DRIsLastWordFeatureType.h
    featuretypes/DRLeftNeighborPOSFeatureType.h
    featuretypes/DRLeftSiblingPOSFeatureType.h
    featuretypes/DRP0LeftNeighborPOSCPPOSFeatureType.h
    featuretypes/DRP0RightNeighborPOSCPPOSFeatureType.h
    featuretypes/DRP0RightSiblingPOSFeatureType.h
    featuretypes/DRPOSFeatureType.h
    featuretypes/DRParentPOSFeatureType.h
    featuretypes/DRRightNeighborPOSFeatureType.h
    featuretypes/DRRightSiblingPOSFeatureType.h
)
