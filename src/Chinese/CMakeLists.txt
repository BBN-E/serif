###############################################################
# Copyright (c) 2011 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# Chinese                                                     #
#                                                             #
###############################################################

ADD_SERIF_FEATURE_MODULE(Chinese
  SOURCE_FILES
    ChineseModule.cpp
    ChineseModule.h
  SUBDIRS
    common
    descriptors
    docRelationsEvents
    edt
    events
    generics
    metonymy
    names
    parse
    propositions
    relations
    sentences
    tokens
    trainers
    values
    ### reader  # not included
  LINK_LIBRARIES
    Generic
)
