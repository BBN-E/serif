###############################################################
# Copyright (c) 2011 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# Spanish                                                     #
#                                                             #
###############################################################

ADD_SERIF_FEATURE_MODULE(Spanish
  EXCLUDED_BY_DEFAULT   # turned off unless the cmake user requests it.
  SOURCE_FILES
    SpanishModule.cpp
    SpanishModule.h
  SUBDIRS
    common
    descriptors
    edt
    #docRelationsEvents
    #events
    #generics
    #metonymy
    #names
    parse
    propositions
    relations
    #sentences
    #tokens
    trainers
    #values
    #reader
  LINK_LIBRARIES
    Generic
)
