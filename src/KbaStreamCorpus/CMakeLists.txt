###############################################################
# Copyright (c) 2011 by BBNT Solutions LLC                    #
# All Rights Reserved.                                        #
#                                                             #
# KbaStreamCorpus                                             #
#                                                             #
###############################################################
# This feature module is only included if thrift is found.

FIND_PACKAGE(Thrift)
IF(Thrift_FOUND)
  INCLUDE_DIRECTORIES("${Thrift_INCLUDE_DIR}")

  # todo: generate the thrift-cpp-gen files??
  ADD_SERIF_FEATURE_MODULE(KbaStreamCorpus
    EXCLUDED_BY_DEFAULT   # turned off unless the cmake user requests it.
    SOURCE_FILES
      KbaStreamCorpusChunk.h
      KbaStreamCorpusChunk.cpp
      KbaStreamCorpusDocumentDriver.cpp
      KbaStreamCorpusDocumentDriver.h
      KbaStreamCorpusItemReader.h
      KbaStreamCorpusItemReader.cpp
      KbaStreamCorpusItemWriter.h
      KbaStreamCorpusItemWriter.cpp
      KbaStreamCorpusModule.h
      KbaStreamCorpusModule.cpp
      KbaStreamCorpusQueueFeeder.h
  
      # Auto-generated files (generated from streamcorpus.thrift):
      thrift-cpp-gen/streamcorpus_constants.cpp
      thrift-cpp-gen/streamcorpus_constants.h 
      thrift-cpp-gen/streamcorpus_types.cpp 
      thrift-cpp-gen/streamcorpus_types.h
    LINK_LIBRARIES
      Generic ${Thrift_LIB}
  )
ENDIF()


