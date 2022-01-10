####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/discTagger                                               #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(discTagger
  SOURCE_FILES
    BlockFeatureTable.cpp
    BlockFeatureTable.h
    DT2IntFeature.cpp
    DT2IntFeature.h
    DT3IntFeature.cpp
    DT3IntFeature.h
    DT6gramFeature.cpp
    DT6gramFeature.h
    DTSexgramIntFeature.cpp
    DTSexgramIntFeature.h
    DTAltModelSet.cpp
    DTAltModelSet.h
    DTBigramFeature.cpp
    DTBigramFeature.h
    DTBigram2IntFeature.cpp
    DTBigram2IntFeature.h
    DTBigramIntFeature.cpp
    DTBigramIntFeature.h
    DTBigramStringFeature.cpp
    DTBigramStringFeature.h
    DTFeature.cpp
    DTFeature.h
    DTFeatureType.cpp
    DTFeatureType.h
    DTFeatureTypeSet.cpp
    DTFeatureTypeSet.h
    DTIntFeature.cpp
    DTIntFeature.h
    DTMonogramFeature.cpp
    DTMonogramFeature.h
    DTObservation.h
    DTQuadgram2IntFeature.cpp
    DTQuadgram2IntFeature.h
    DTQuadgramFeature.cpp
    DTQuadgramFeature.h
    DTQuadgramIntFeature.cpp
    DTQuadgramIntFeature.h
    DTQuintgramFeature.cpp
    DTQuintgramFeature.h
    DTQuintgramIntFeature.cpp
    DTQuintgramIntFeature.h
    DTQuintgramStringFeature.cpp
    DTQuintgramStringFeature.h
    DTState.h
    DTSeptgramFeature.cpp
    DTSeptgramFeature.h
    DTSeptgramIntFeature.cpp
    DTSeptgramIntFeature.h
    DTTagSet.cpp
    DTTagSet.h
    DTTrigram2IntFeature.cpp
    DTTrigram2IntFeature.h
    DTTrigramFeature.cpp
    DTTrigramFeature.h
    DTTrigramIntFeature.cpp
    DTTrigramIntFeature.h
    DTTrigramStringFeature.cpp
    DTTrigramStringFeature.h
    DTVariableSizeFeature.cpp
    DTVariableSizeFeature.h
    DTWordNameListFeatureType.h
    DTWordLCNameListFeatureType.h
    P1Decoder.cpp
    P1Decoder.h
    PDecoder.cpp
    PDecoder.h
#    PDecoderStub.cpp       # not included
#    PDecoderStub.h     # not included
    PWeight.h
)
