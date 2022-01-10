####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/names                                                    #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(names
  SOURCE_FILES
    DefaultIdFWordFeatures.cpp
    DefaultIdFWordFeatures.h
    DefaultNameRecognizer.cpp
    DefaultNameRecognizer.h
    IdFDecoder.cpp
    IdFDecoder.h
    IdFListSet.cpp
    IdFListSet.h
    IdFModel.cpp
    IdFModel.h
    IdFSentence.cpp
    IdFSentence.h
    IdFSentenceTheory.cpp
    IdFSentenceTheory.h
    IdFSentenceTokens.cpp
    IdFSentenceTokens.h
    IdFTrainer.cpp
    IdFTrainer.h
    IdFWordFeatures.cpp
    IdFWordFeatures.h
    NameClassTags.cpp
    NameClassTags.h
    NameRecognizer.cpp
    NameRecognizer.h
    PatternNameFinder.cpp
    PatternNameFinder.h
    xx_NameRecognizer.h
  SUBDIRS
    discmodel
)
