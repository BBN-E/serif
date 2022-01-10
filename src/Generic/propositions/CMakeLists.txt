###############################################################
# Copyright (c) 2011 by Raytheon BBN Technologies Corp.       #
# All Rights Reserved.                                        #
#                                                             #
# Generic/propositions
###############################################################

ADD_SERIF_LIBRARY_SUBDIR(propositions
  SOURCE_FILES
    LinearPropositionFinder.cpp
    LinearPropositionFinder.h
    PropositionFinder.cpp
    PropositionFinder.h
    PropositionStatusClassifier.cpp
    PropositionStatusClassifier.h
    SemTreeBuilder.cpp
    SemTreeBuilder.h
    xx_LinearPropositionFinder.h
    xx_PropositionStatusClassifier.h
    xx_SemTreeBuilder.h
  SUBDIRS
    sem_tree
)
