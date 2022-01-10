###############################################################
# Copyright (c) 2011 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# 7Zip                                                        #
# Provides support for 7Zip decompression                     #
###############################################################
if (7Zip_FOUND)
ADD_SERIF_FEATURE_MODULE(SevenZip
    EXCLUDED_BY_DEFAULT
SOURCE_FILES
  SevenZipDecompressor.h
  SevenZipDecompressor.cpp
  SevenZipModule.h
  SevenZipModule.cpp
LINK_LIBRARIES
    ${7Zip_LIBRARY}
)
endif()
