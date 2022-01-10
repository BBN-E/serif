####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# PosteriorRegularization                                          #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(PosteriorRegularization
  INSTALL LearnIt
  SOURCE_FILES
    PosteriorRegularization.cpp
    constraints/ClassRequiresFeatureConstraint.cpp
    constraints/AtLeastNOfClassIfFeatureConstraint.cpp
    constraints/AtMostNOfClassConstraint.cpp
  LINK_LIBRARIES
    ActiveLearning
    LearnIt
    Temporal
)
