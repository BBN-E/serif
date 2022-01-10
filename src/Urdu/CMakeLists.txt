###############################################################
# Copyright (c) 2013 by Raytheon BBN Technologies             #
# All Rights Reserved.                                        #
#                                                             #
# Urdu                                                        #
#                                                             #
###############################################################

ADD_SERIF_FEATURE_MODULE(Urdu
  EXCLUDED_BY_DEFAULT   # turned off unless the cmake user requests it.
  SOURCE_FILES
    UrduModule.cpp
    UrduModule.h
  SUBDIRS
    common
    #descriptors
    #edt
    #docRelationsEvents
    #events
    #generics
    #metonymy
    #names
    parse
    partOfSpeech
    #propositions
    #relations
    sentences
    tokens
    trainers
    #values
    #reader
  LINK_LIBRARIES
    Generic
)
