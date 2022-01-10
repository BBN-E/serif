####################################################################
# Copyright (c) 2013 by Raytheon BBN Technologies                  #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/Solutions/Brandy/CMakeLists.txt                            #
#                                                                  #
# This file produces the Brandy solution.                          #
#                                                                  #
####################################################################

ADD_SERIF_SOLUTION(
  Brandy

  PROJECTS
    AgileServer
    CombinedAFRGServer
    distill-generic
    DocumentRetrieverServer
    EQGenerator
    FactFinderServer
    PatternTester
    QueryInterpreterServer
    SCILFeatureExtractor
    StandaloneCombinedAFRGServer
    StandaloneDocumentRetrieverServer
    ThreadedDocumentLoaderTest

  DEPENDENCIES
    AgileServer:distill-generic
    CombinedAFRGServer:distill-generic
    DocumentRetrieverServer:distill-generic
    EQGenerator:distill-generic
    FactFinderServer:distill-generic
    QueryInterpreterServer:distill-generic
    SCILFeatureExtractor:distill-generic
    StandaloneCombinedAFRGServer:distill-generic
    StandaloneDocumentRetrieverServer:distill-generic
    ThreadedDocumentLoaderTest:distill-generic
)
