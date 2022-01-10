###############################################################
# Copyright (c) 2015 by Raytheon BBN Technologies Corp.       #
# All Rights Reserved.                                        #
#                                                             #
# English/Test 
###############################################################

ADD_SERIF_FEATURE_MODULE(EnglishTest
  SOURCE_FILES
    EnglishTestModule.cpp
    EnglishTestModule.h	
  SUBDIRS
    test  
    tokens
  LINK_LIBRARIES
    Generic
    ${Boost_LIBRARIES}
)

  