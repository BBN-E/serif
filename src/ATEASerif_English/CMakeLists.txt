####################################################################
# Copyright (c) 2011 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# ATEASerif_English                                                #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(ATEASerif_English
    SOURCE_FILES atea-server.cpp
    LINK_LIBRARIES English ATEASerif_Generic ws2_32)
