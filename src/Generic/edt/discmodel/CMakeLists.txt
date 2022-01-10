####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/edt/discmodel                                            #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(discmodel
  SOURCE_FILES
    CorefUtils.cpp
    CorefUtils.h
    DocumentMentionInformationMapper.cpp
    DocumentMentionInformationMapper.h
    DTCorefFeatureType.cpp
    DTCorefFeatureType.h
    DTCorefFeatureTypes.cpp
    DTCorefFeatureTypes.h
    DTCorefLinker.cpp
    DTCorefLinker.h
    DTCorefObservation.cpp
    DTCorefObservation.h
    DTCorefTrainer.cpp
    DTCorefTrainer.h
    DTNameLinker.h
    DTNameLinker.cpp
    DTNoneCorefObservation.cpp
    DTNoneCorefObservation.h
    DTPronounLinker.cpp
    DTPronounLinker.h
    xx_DTCorefFeatureTypes.h
  SUBDIRS
    featuretypes
)
