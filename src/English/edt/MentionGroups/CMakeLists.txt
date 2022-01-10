####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# English/edt/MentionGroups                                        #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(MentionGroups
  SOURCE_FILES
    en_MentionGroupConfiguration.cpp
    en_MentionGroupConfiguration.h
    en_DTRAMentionGroupConfiguration.cpp
    en_DTRAMentionGroupConfiguration.h
  SUBDIRS
    extractors
    mergers
)
