###############################################################
# Copyright (c) 2005 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# Korean                                                      #
#                                                             #
###############################################################

ADD_SERIF_FEATURE_MODULE(Korean 
  EXCLUDED_BY_DEFAULT   # turned off unless the cmake user requests it.
  SOURCE_FILES
    KoreanModule.cpp
    KoreanModule.h
  SUBDIRS
    common
    descriptors
    edt
    morphology
    names
    parse
    partOfSpeech
    sentences
    tokens
  LINK_LIBRARIES
    Generic
)
