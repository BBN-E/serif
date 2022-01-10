####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Arabic/morphSelection 
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(morphSelection
  SOURCE_FILES
#  MorphChartEntry.cpp  # not included
#  MorphChartEntry.h    # not included
    ar_MorphDecoder.cpp
    ar_MorphDecoder.h
    ar_MorphModel.cpp
    ar_MorphModel.h
)
