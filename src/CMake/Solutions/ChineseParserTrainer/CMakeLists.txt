####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/Solutions/ChineseParseTrainer/CMakeLists.txt               #
#                                                                  #
# This file produces the ChineseParseTrainer solution.             #
#                                                                  #
####################################################################

ADD_SERIF_SOLUTION(ChineseParserTrainer
  LANGUAGES Chinese
  PROJECTS
    DeriveTables
    Headify
    K_Estimator
    StatsCollector
    VocabCollector
    VocabPruner)
