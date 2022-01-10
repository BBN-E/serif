###############################################################
# Copyright (c) 2011 by Raytheon BBN Technologies Corp.       #
# All Rights Reserved.                                        #
#                                                             #
# English/relations
###############################################################

ADD_SERIF_LIBRARY_SUBDIR(relations
  SOURCE_FILES
    en_AttachmentFilter.h
    en_ComboRelationFinder.cpp
    en_ComboRelationFinder.h
    en_ConstructionFilter.h
    en_ExpFeatureVectorFilter.h
    en_FamilyRelationFinder.cpp
    en_FamilyRelationFinder.h
    en_FeatureVectorFilter.h
#    en_MaxEntRelationFinder.cpp    # not included
#    en_MaxEntRelationFinder.h      # not included
    en_NodeFilter.h
    en_OldRelationFinder.cpp
    en_OldRelationFinder.h
    en_PatternMatcherModel.cpp
    en_PatternMatcherModel.h
    en_PredicateFilter.h
    en_PriorFilter.h
    en_RelationFinder.cpp
    en_RelationFinder.h
    en_RelationModel.cpp
    en_RelationModel.h
    en_RelationUtilities.cpp
    en_RelationUtilities.h
    en_SpecialRelationCases.cpp
    en_SpecialRelationCases.h
    en_TreeModel.cpp
    en_TreeModel.h
    en_specific_relation_tree_models.h
  SUBDIRS
    discmodel
)
