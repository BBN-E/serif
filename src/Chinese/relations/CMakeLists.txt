####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Chinese/relations
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(relations
  SOURCE_FILES
    ch_ComboRelationFinder.cpp
    ch_ComboRelationFinder.h
    ch_ExpFeatureVectorFilter.h
    ch_MaxEntRelationFinder.cpp
    ch_MaxEntRelationFinder.h
#  ch_MaxEntRelationModel.cpp   # not used
#  ch_MaxEntRelationModel.h # not used
    ch_OldMaxEntRelationFinder.cpp
    ch_OldMaxEntRelationFinder.h
    ch_OldMaxEntRelationModel.cpp
    ch_OldMaxEntRelationModel.h
    ch_PotentialRelationCollector.cpp
    ch_PotentialRelationCollector.h
    ch_PotentialRelationInstance.cpp
    ch_PotentialRelationInstance.h
    ch_RelationFinder.cpp
    ch_RelationFinder.h
    ch_RelationModel.cpp
    ch_RelationModel.h
    ch_RelationUtilities.cpp
    ch_RelationUtilities.h
    discmodel/ch_P1RelationFeatureTypes.cpp
    discmodel/ch_P1RelationFeatureTypes.h
    discmodel/ch_P1RelationFinder.cpp
    discmodel/ch_P1RelationFinder.h
    discmodel/featuretypes/ch_NestedPropFT.h
    discmodel/featuretypes/ch_RefPropFT.h
    discmodel/featuretypes/ch_SimplePropFT.h
    discmodel/featuretypes/ch_SimplePropTypesFT.h
    discmodel/featuretypes/ch_SimplePropWCFT.h
    discmodel/featuretypes/ch_SimplePropWCNoTypesFT.h
)
