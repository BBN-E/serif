####################################################################
# Copyright (c) 2012 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# UnsupervisedEvents                                               #
#                                                                  #
####################################################################

ADD_SERIF_EXECUTABLE(UnsupervisedEvents
  INSTALL LearnIt
  SOURCE_FILES
    UnsupervisedEvents.cpp
    ACEEventGoldStandard.h
    ACEEventGoldStandard.cpp
    ACEPREMDecoder.h
    ACEPREMDecoder.cpp
    ACEScorer.h
    ACEScorer.cpp
    AnswerExtractor.h
    AnswerExtractor.cpp
    AnswerMatrix.h
    AnswerMatrix.cpp
    BaselineDecoder.h
    BaselineDecoder.cpp
    ConjunctionUtils.h
    ConjunctionUtils.cpp
    GuidedDecoder.h
    GuidedDecoder.cpp
    OracleDecoder.h
    OracleDecoder.cpp
    PassageDescription.h
    PassageDescription.cpp
    GraphDumper.h
    ACEEventDumper.h
    ACEEventDumper.cpp
    ACEPassageDescription.h
    ACEPassageDescription.cpp
    ProblemDefinition.cpp
    ProblemDefinition.h
    ACEEvent.h
    ACEEvent.cpp
    ACEEntityFactorGroup.h
    ACEEntityFactorGroup.cpp
    ACEEntityVariable.h
    ACEEntityVariable.cpp
    SerifDecoder.h
    SerifDecoder.cpp
    Sparkline.h
    Sparkline.cpp
  LINK_LIBRARIES
    LearnIt
    GraphicalModels
)
