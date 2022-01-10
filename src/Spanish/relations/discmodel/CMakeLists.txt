###############################################################
# Copyright (c) 2011 by Raytheon BBN Technologies Corp.       #
# All Rights Reserved.                                        #
#                                                             #
# Spanish/relations/discmodel
###############################################################

ADD_SERIF_LIBRARY_SUBDIR(discmodel
  SOURCE_FILES
    es_P1RelationFeatureTypes.cpp
    es_P1RelationFeatureTypes.h
#    es_P1RelationFinder.cpp        # not included
#    es_P1RelationFinder.h      # not included
  SUBDIRS
    featuretypes
)
