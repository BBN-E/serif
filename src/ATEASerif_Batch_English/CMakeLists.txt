####################################################################
# Copyright (c) 2011 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# ATEASerif_Batch_English                                          #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(ATEASerif_Batch_English
    SOURCE_FILES ATEAEnglishSerif.cpp
    LINK_LIBRARIES English ATEASerif_Generic ws2_32)
