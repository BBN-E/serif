####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# LearnIt2Trainer                                                  #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(LearnIt2Trainer
  INSTALL LearnIt
  SOURCE_FILES
    LearnIt2Trainer.cpp
    LearnIt2TrainerUI.h
    LearnIt2TrainerUI.cpp
    OptimizationView.h
    OptimizationView.cpp
    ProbChart.h
    ProbChart.cpp
    trainer.h
    trainer.cpp
    objectives/UnlabeledDisagreementLoss.h
    objectives/UnlabeledDisagreementLoss.cpp
    objectives/OneSidedUnlabeledDisagreementLoss.h
    objectives/OneSidedUnlabeledDisagreementLoss.cpp
  LINK_LIBRARIES
    ActiveLearning
    LearnIt
  WIN_LINK_LIBRARIES
    ws2_32
)
