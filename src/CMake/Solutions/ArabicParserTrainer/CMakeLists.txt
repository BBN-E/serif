####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/Solutions/ArabicParseTrainer/CMakeLists.txt                #
#                                                                  #
# This file produces the ArabicParseTrainer solution.              #
#                                                                  #
####################################################################

ADD_SERIF_SOLUTION(ArabicParserTrainer
  LANGUAGES Arabic
  PROJECTS
    DeriveTables
    Headify
    K_Estimator
    StatsCollector
    VocabCollector
    VocabPruner)
