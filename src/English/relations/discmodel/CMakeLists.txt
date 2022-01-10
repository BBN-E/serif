###############################################################
# Copyright (c) 2011 by Raytheon BBN Technologies Corp.       #
# All Rights Reserved.                                        #
#                                                             #
# English/relations/discmodel
###############################################################

ADD_SERIF_LIBRARY_SUBDIR(discmodel
  SOURCE_FILES
    en_P1RelationFeatureTypes.cpp
    en_P1RelationFeatureTypes.h
#    en_P1RelationFinder.cpp        # not included
#    en_P1RelationFinder.h      # not included
  SUBDIRS
    featuretypes
)
