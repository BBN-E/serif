###############################################################
# Copyright (c) 2012 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# PredFinder                                                  #
#                                                             #
###############################################################

ADD_SERIF_FEATURE_MODULE(PredFinder
  EXCLUDED_BY_DEFAULT
  SUBDIRS
    common
    coref
    db
    elf
    inference
    macros
  SOURCE_FILES
    PredFinder.cpp
    PredFinder.h
    PredFinderCounts.cpp
    PredFinderCounts.h
    PredFinderModule.cpp
    PredFinderModule.h
  LINK_LIBRARIES
    Generic
    LearnIt
    Temporal
    ${Boost_LIBRARIES}
)
