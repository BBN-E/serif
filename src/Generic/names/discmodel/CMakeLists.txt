####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/names/discmodel                                          #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(discmodel
  SOURCE_FILES
    FeatureTypesSymbols.cpp
    FeatureTypesSymbols.h
    PIdFActiveLearning.cpp
    PIdFActiveLearning.h
    PIdFActiveLearningSentence.cpp
    PIdFActiveLearningSentence.h
    #    PIdFActiveLearningStub.cpp    # not included
    #    PIdFActiveLearningStub.h  # not included
    #    PIdFDecoder.cpp       # not included
    #    PIdFDecoder.h     # not included
    PIdFFeatureType.cpp
    PIdFFeatureType.h
    PIdFFeatureTypes.cpp
    PIdFFeatureTypes.h
    PIdFModel.cpp
    PIdFModel.h
    #    PIdFModelStub.cpp     # not included
    #    PIdFModelStub.h       # not included
    PIdFSecondaryDecoders.cpp
    PIdFSecondaryDecoders.h
    PIdFSentence.cpp
    PIdFSentence.h
    #    PIdFSimActiveLearningTrainer.cpp  # not included
    #    PIdFSimActiveLearningTrainer.h    # not included
    #    PIdFTrainer.cpp           # not included
    #    PIdFTrainer.h         # not included
    TokenObservation.cpp
    TokenObservation.h
  SUBDIRS
    featuretypes
)
