###############################################################
# Copyright (c) 2005 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# Thai                                                        #
#                                                             #
###############################################################

ADD_SERIF_FEATURE_MODULE(Thai
  EXCLUDED_BY_DEFAULT   # turned off unless the cmake user requests it.
  SOURCE_FILES
    ThaiModule.cpp
    ThaiModule.h
  LINK_LIBRARIES
    Generic
)
