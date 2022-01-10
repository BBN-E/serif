###############################################################
# Copyright (c) 2011 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# Arabic                                                      #
#                                                             #
###############################################################

ADD_SERIF_FEATURE_MODULE(Arabic 
  SOURCE_FILES
    ArabicModule.cpp
    ArabicModule.h
  SUBDIRS
    BuckWalter
    metonymy
    morphSelection
    common
    clutter
    descriptors
    edt
    names
    normalizer
    parse
    partOfSpeech
    relations
    results
    sentences
    tokens
    trainers
    values
    ### BWMorph   # not included
    ### driver    # not included
    ### nltrain   # not included
    ### reader    # not included
    ### token     # not included
  LINK_LIBRARIES
    Generic
)
