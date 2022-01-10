####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Spanish/edt/MentionGroups                                        #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(MentionGroups
  SOURCE_FILES
    es_MentionGroupConfiguration.cpp
    es_MentionGroupConfiguration.h
  SUBDIRS
    constraints
    extractors
    mergers
)
