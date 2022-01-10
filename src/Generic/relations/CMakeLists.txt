####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/relations                                                #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(relations
  SOURCE_FILES
    BackoffProbModel.h
    BackoffToPriorProbModel.h
    FamilyRelationFinder.cpp
    FamilyRelationFinder.h
    MaxEntRelationTrainer.cpp
    MaxEntRelationTrainer.h
    PatternRelationFinder.cpp
    PatternRelationFinder.h
    PotentialRelationCollector.cpp
    PotentialRelationCollector.h
    PotentialRelationInstance.cpp
    PotentialRelationInstance.h
    PotentialTrainingRelation.cpp
    PotentialTrainingRelation.h
    RelationFilter.h
    RelationFinder.cpp
    RelationFinder.h
    RelationModel.h
    RelationModel.cpp
    RelationStats.h
    RelationTypeSet.cpp
    RelationTypeSet.h
    RelationUtilities.cpp
    RelationUtilities.h
    VectorModel.cpp
    VectorModel.h
    specific_relation_vector_models.cpp
    specific_relation_vector_models.h
    xx_FamilyRelationFinder.h
    xx_FeatureVectorFilter.h
    xx_PotentialRelationCollector.h
    xx_PotentialRelationInstance.h
    xx_RelationFinder.h
    xx_RelationModel.h
    xx_RelationUtilities.h
  SUBDIRS
    HighYield
    discmodel
)
