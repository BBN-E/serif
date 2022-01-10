###############################################################
# Copyright (c) 2011 by Raytheon BBN Technologies Corp.       #
# All Rights Reserved.                                        #
#                                                             #
# Spanish/relations
###############################################################

ADD_SERIF_LIBRARY_SUBDIR(relations
  SOURCE_FILES
    es_AttachmentFilter.h
    es_ComboRelationFinder.cpp
    es_ComboRelationFinder.h
    es_ConstructionFilter.h
    es_ExpFeatureVectorFilter.h
    es_FamilyRelationFinder.cpp
    es_FamilyRelationFinder.h
    es_FeatureVectorFilter.h
    es_NodeFilter.h
    es_PatternMatcherModel.cpp
    es_PatternMatcherModel.h
    es_PredicateFilter.h
    es_PriorFilter.h
    es_RelationFinder.cpp
    es_RelationFinder.h
    es_RelationModel.cpp
    es_RelationModel.h
    es_RelationUtilities.cpp
    es_RelationUtilities.h
    es_SpecialRelationCases.cpp
    es_SpecialRelationCases.h
    es_TreeModel.cpp
    es_TreeModel.h
    es_specific_relation_tree_models.h
  SUBDIRS
    discmodel
)
