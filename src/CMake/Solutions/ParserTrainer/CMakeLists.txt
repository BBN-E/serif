####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/Solutions/ParseTrainer/CMakeLists.txt                      #
#                                                                  #
# This file produces the ParseTrainer solution.                    #
#                                                                  #
####################################################################

ADD_SERIF_SOLUTION(ParserTrainer
  PROJECTS
    DeriveTables
    Headify
    K_Estimator
    StatsCollector
    VocabCollector
    VocabPruner
)
