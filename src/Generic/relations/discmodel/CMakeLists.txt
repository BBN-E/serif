####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/relations/discmodel                                      #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(discmodel
  SOURCE_FILES
    DTRelSentenceInfo.h
    DTRelationAltModel.cpp
    DTRelationAltModel.h
    DTRelationSet.h
#    P1RelationAltModel.cpp     # not included
#    P1RelationAltModel.h       # not included
    P1RelationFeatureType.cpp
    P1RelationFeatureType.h
    P1RelationFeatureTypes.cpp
    P1RelationFeatureTypes.h
    P1RelationTrainer.cpp
    P1RelationTrainer.h
    PropTreeLinks.cpp
    PropTreeLinks.h
    RelationObservation.cpp
    RelationObservation.h
    RelationPropLink.cpp
    RelationPropLink.h
    TreeNodeChain.cpp
    TreeNodeChain.h
    xx_P1RelationFeatureTypes.h
  SUBDIRS
    featuretypes
)
