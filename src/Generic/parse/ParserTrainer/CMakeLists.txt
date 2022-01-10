####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# parse/ParserTrainer                                              #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(ParserTrainer
  SOURCE_FILES
    BridgeCollector.cpp
    BridgeCollector.h
    CategoryProbDeriver.cpp
    CategoryProbDeriver.h
    ChainFinder.cpp
    ChainFinder.h
    FeatureVectorTable.cpp
    FeatureVectorTable.h
    HeadlessParseNode.cpp
    HeadlessParseNode.h
    K_Estimator.cpp
    K_Estimator.h
    LinkedBridgeExtension.cpp
    LinkedBridgeExtension.h
    LinkedBridgeKernel.h
    LinkedTag.h
    ParserTrainerLanguageSpecificFunctions.h
    ParserTrainerLanguageSpecificFunctions.cpp
    PrunerPOS.cpp
    PrunerPOS.h
    StatsCollector.cpp
    StatsCollector.h
    TrainerExtensionTable.cpp
    TrainerExtensionTable.h
    TrainerKernelTable.cpp
    TrainerKernelTable.h
    TrainerPOS.cpp
    TrainerPOS.h
    TrainerVocab.cpp
    TrainerVocab.h
    VocabPruner.cpp
    VocabPruner.h
    WordCollector.cpp
    WordCollector.h
    xx_ParserTrainerLanguageSpecificFunctions.h
    xx_ParserTrainerLanguageSpecificFunctions.cpp
)
