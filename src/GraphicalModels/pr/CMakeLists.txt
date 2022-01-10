####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# GraphicalModels/pr                                               #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(pr
  SOURCE_FILES
    Constraint.h
    Constraint.cpp
    ClassConstantLowerBoundsConstraint.h
    ClassConstantUpperBoundsConstraint.h
    ClassRequiresFeature.h
    FeatureImpliesClassConstraint.h
    FeatureImpliesClassConstraint.cpp
)
