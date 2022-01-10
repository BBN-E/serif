####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/Solutions/EnglishParseTrainer/CMakeLists.txt               #
#                                                                  #
# This file produces the EnglishParseTrainer solution.             #
#                                                                  #
####################################################################

ADD_SERIF_SOLUTION(EnglishParserTrainer
  LANGUAGES English
  PROJECTS
    DeriveTables
    Headify
    K_Estimator
    StatsCollector
    VocabCollector
    VocabPruner
  DEPENDENCIES
    Headify:English
    K_Estimator:English
    StatsCollector:English
    VocabCollector:English
    VocabPruner:English
)
