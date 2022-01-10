###############################################################
# Copyright (c) 2011 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# ICEWS                                                       #
#                                                             #
###############################################################

ADD_SERIF_FEATURE_MODULE(ICEWS
  EXCLUDED_BY_DEFAULT   # turned off unless the cmake user requests it.
  SOURCE_FILES
    ICEWSModule.cpp
    ICEWSModule.h
  SUBDIRS
  LINK_LIBRARIES
    Generic
    ${MySQLClient_LIBRARIES}
  )
