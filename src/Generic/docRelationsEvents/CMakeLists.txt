####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/docRelationsEvents/                                      #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(docRelationsEvents
  SOURCE_FILES
    DefaultZonedRelationFinder.cpp
    DefaultZonedRelationFinder.h
    DocEventHandler.cpp
    DocEventHandler.h
    DocRelationEventProcessor.cpp
    DocRelationEventProcessor.h
    DocumentRelationFinder.cpp
    DocumentRelationFinder.h
    EventLinkFeatureType.cpp
    EventLinkFeatureType.h
    EventLinkFeatureTypes.cpp
    EventLinkFeatureTypes.h
    EventLinkObservation.cpp
    EventLinkObservation.h
    RelationPromoter.cpp
    RelationPromoter.h
    RelationTimexArgFeatureType.cpp
    RelationTimexArgFeatureType.h
    RelationTimexArgFeatureTypes.cpp
    RelationTimexArgFeatureTypes.h
    RelationTimexArgFinder.cpp
    RelationTimexArgFinder.h
    RelationTimexArgObservation.cpp
    RelationTimexArgObservation.h
    StatEventLinker.cpp
    StatEventLinker.h
    StructuralRelationFinder.cpp
    StructuralRelationFinder.h
    ZonedRelationFinder.cpp
    ZonedRelationFinder.h
    xx_DocumentRelationFinder.h
    xx_EventLinkFeatureTypes.h
    xx_RelationTimexArgFeatureTypes.h
    xx_StructuralRelationFinder.h
  SUBDIRS
    featuretypes
)
