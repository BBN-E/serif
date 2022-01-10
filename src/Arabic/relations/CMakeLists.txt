####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Arabic/relations 
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(relations
  SOURCE_FILES
    ar_ExpFeatureVectorFilter.h
    ar_PotentialRelationCollector.cpp
    ar_PotentialRelationCollector.h
    ar_PotentialRelationExtractor.cpp
    ar_PotentialRelationExtractor.h
    ar_PotentialRelationInstance.cpp
    ar_PotentialRelationInstance.h
    ar_RelationFinder.cpp
    ar_RelationFinder.h
    ar_RelationModel.cpp
    ar_RelationModel.h
    ar_RelationUtilities.cpp
    ar_RelationUtilities.h
    discmodel/ar_P1RelationFeatureTypes.cpp
    discmodel/ar_P1RelationFeatureTypes.h
    discmodel/ar_P1RelationFinder.cpp
    discmodel/ar_P1RelationFinder.h
    discmodel/featuretypes/ar_AdjModEntTypeClustFT.h
    discmodel/featuretypes/ar_AdjModEntTypeHWFT.h
    discmodel/featuretypes/ar_AdjPOSFT.h
    discmodel/featuretypes/ar_EntityAndMentionTypesFT.h
    discmodel/featuretypes/ar_HeadWordFT.h
    discmodel/featuretypes/ar_HeadWordWCFT.h
    discmodel/featuretypes/ar_LastPrepFT.h
    discmodel/featuretypes/ar_LastPrepNPrepsFT.h
    discmodel/featuretypes/ar_LeftCliticFT.h
    discmodel/featuretypes/ar_LeftEntityAndMentionTypesFT.h
    discmodel/featuretypes/ar_LeftHeadWordFT.h
    discmodel/featuretypes/ar_LeftHeadWordWCFT.h
    discmodel/featuretypes/ar_LeftIsDefiniteFT.h
    discmodel/featuretypes/ar_LeftIsModifierFT.h
    discmodel/featuretypes/ar_MentionChunkDistanceFT.h
#  discmodel/featuretypes/ar_MentionDistanceFT.h    # not included
    discmodel/featuretypes/ar_POSBetweenTypesFT.h
    discmodel/featuretypes/ar_ParenGPEFT.h
    discmodel/featuretypes/ar_PossModFT.h
    discmodel/featuretypes/ar_PossPronClustFT.h
    discmodel/featuretypes/ar_PossPronFT.h
    discmodel/featuretypes/ar_PrepChainFT.h
    discmodel/featuretypes/ar_PrepSepEntTypeClustFT.h
    discmodel/featuretypes/ar_PrepSepEntTypeHWFT.h
    discmodel/featuretypes/ar_RightCliticFT.h
    discmodel/featuretypes/ar_RightEntityAndMentionTypesFT.h
    discmodel/featuretypes/ar_RightHeadWordFT.h
    discmodel/featuretypes/ar_RightHeadWordWCFT.h
    discmodel/featuretypes/ar_RightIsDefiniteFT.h
    discmodel/featuretypes/ar_RightIsModifierFT.h
)
