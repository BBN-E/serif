####################################################################
# Copyright (c) 2011 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/PropTree                                                 #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(PropTree
  SOURCE_FILES
    DocPropForest.cpp
    DocPropForest.h
    MentionToPropNodeMap.h
    MentionToPropNodeMap.cpp
    Predicate.cpp
    Predicate.h
    PropConstrainedMatch.cpp
    PropConstrainedMatch.h
    PropEdgeMatch.cpp
    PropEdgeMatch.h
    PropFactory.cpp
    PropFactory.h
    PropForestFactory.cpp
    PropForestFactory.h
    PropFullMatch.cpp
    PropFullMatch.h
    PropMatch.cpp
    PropMatch.h
    PropNode.cpp
    PropNode.h
    PropNodeMatch.cpp
    PropNodeMatch.h
  SUBDIRS
    expanders
)

