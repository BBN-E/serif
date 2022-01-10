####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/PNPChunking                                              #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(PNPChunking
  SOURCE_FILES
    NPChunkFinder.cpp
    NPChunkFinder.h
    DefaultNPChunkFinder.h
    DefaultNPChunkFinder.cpp
    xx_NPChunkFinder.h
    PNPChunkDecoder.cpp
    PNPChunkDecoder.h
    PNPChunkFeatureType.cpp
    PNPChunkFeatureType.h
    PNPChunkFeatureTypes.cpp
    PNPChunkFeatureTypes.h
    PNPChunkSentence.cpp
    PNPChunkSentence.h
    PNPChunkTrainer.cpp
    PNPChunkTrainer.h
    TokenPOSObservation.cpp
    TokenPOSObservation.h
  SUBDIRS
    featuretypes
)

