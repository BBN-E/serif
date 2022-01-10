####################################################################
# Copyright (c) 2011 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# ParserTrainer                                                    #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(DeriveTables
  INSTALL Trainers/parser
  SOURCE_FILES DeriveTables/DeriveTablesMain.cpp)

ADD_SERIF_EXECUTABLE(Headify
  INSTALL Trainers/parser
  SOURCE_FILES Headify/HeadifyMain.cpp)

ADD_SERIF_EXECUTABLE(K_Estimator
  INSTALL Trainers/parser
  SOURCE_FILES K_Estimator/K_EstimatorMain.cpp)

ADD_SERIF_EXECUTABLE(StatsCollector
  INSTALL Trainers/parser
  SOURCE_FILES StatsCollector/StatsCollectorMain.cpp)

ADD_SERIF_EXECUTABLE(VocabCollector
  INSTALL Trainers/parser
  SOURCE_FILES VocabCollector/VocabCollectorMain.cpp)

ADD_SERIF_EXECUTABLE(VocabPruner
  INSTALL Trainers/parser
  SOURCE_FILES VocabPruner/VocabPrunerMain.cpp)

## Installation of perl scripts
INSTALL(PROGRAMS
                   serif-one-step-k-train.pl
                   serif-one-step-train-no-lambda.pl
                   serif-one-step-train.pl
                   serif-step1-collect-prune.pl
                   serif-step2-smooth.pl
                   serif-step3-derive-tables-no-lambda.pl
                   serif-step3-derive-tables.pl
        DESTINATION
                   ${BUILD_LANGUAGE}/Trainers/parser)
