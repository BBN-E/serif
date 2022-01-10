####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/edt/MentionGroups                                        #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(MentionGroups
  SOURCE_FILES
    EntitySetBuilder.cpp
    EntitySetBuilder.h
    LinkInfoCache.cpp
    LinkInfoCache.h
    MentionGroup.cpp
    MentionGroup.h
    MentionGroupConfiguration.cpp
    MentionGroupConfiguration.h
    MentionGroupConstraint.h
    MentionGroupMerger.cpp
    MentionGroupMerger.h
    MentionGroupUtils.cpp
    MentionGroupUtils.h
    MergeHistory.cpp
    MergeHistory.h
    xx_MentionGroupConfiguration.cpp
    xx_MentionGroupConfiguration.h
  SUBDIRS
    constraints
    extractors
    mergers
)
