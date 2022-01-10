###############################################################
# Copyright (c) 2011 by Raytheon BBN Technologies Corp.       #
# All Rights Reserved.                                        #
#                                                             #
# English/docRelationsEvents
###############################################################

ADD_SERIF_LIBRARY_SUBDIR(docRelationsEvents
  SOURCE_FILES
#    en_DocumentRelationFinder.cpp
#    en_DocumentRelationFinder.h
    en_EventLinkFeatureTypes.cpp
    en_EventLinkFeatureTypes.h
    en_RelationTimexArgFeatureTypes.cpp
    en_RelationTimexArgFeatureTypes.h
    en_StructuralRelationFinder.cpp
    en_StructuralRelationFinder.h
  SUBDIRS
    featuretypes
)
