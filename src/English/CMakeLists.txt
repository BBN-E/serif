###############################################################
# Copyright (c) 2011 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# English                                                     #
#                                                             #
###############################################################

ADD_SERIF_FEATURE_MODULE(English
  SOURCE_FILES
    EnglishModule.cpp
    EnglishModule.h
  SUBDIRS
    Cbrne
    clutter
    common
    confidences
    descriptors
    discourseRel
    docRelationsEvents
    edt
    eeml
    events
    generics
    metonymy
    names
    parse
    partOfSpeech
    propositions
    relations
    results
    sentences
    timex
    tokens
    trainers
    UTCoref
    values
    zoner
    ### not included ### adept
    ### not included ### npChunking
    ### not included ### reader
  LINK_LIBRARIES
    Generic
    ${Boost_LIBRARIES}
)