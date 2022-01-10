####################################################################
# Copyright (c) 2010 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/UTCoref                                                  #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(UTCoref
  SOURCE_FILES
    LinkAllMentions.cpp
    LinkAllMentions.h
    xx_LinkAllMentions.h
    UTWordLists.cpp
    UTWordLists.h
    MaxentClassifier.h
    MaxentClassifier.cpp
    SVMClassifier.h
    SVMClassifier.cpp
    AbstractClassifier.h
    UTObservation.h
    UTObservation.cpp
  SUBDIRS
    featuretypes
)
