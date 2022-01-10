####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# GraphicalModels                                                  #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY(GraphicalModels
  SUBDIRS
    io
    learning
    pr
  SOURCE_FILES
    Alphabet.h
    Alphabet.cpp
    Factor.h
    Variable.h
    FactorGraphNode.h
    DataSet.h
    DumpVector.h
    Graph.h
    GraphicalModelTypes.h
    Message.h
    Message.cpp
    VectorUtils.h
    VariablePriorFactor.h
    SingleFeatureDistributionFactor.h
  LINK_LIBRARIES
    LBFGS-B
    LBFGS_LIBRARY
)
