####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/parse                                                    #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(parse
  SOURCE_FILES
    BranchDirection.h
    BridgeExtension.cpp
    BridgeExtension.h
    BridgeKernel.cpp
    BridgeKernel.h
    BridgeType.h
    ChartDecoder.cpp
    ChartDecoder.h
    ChartEntry.cpp
    ChartEntry.h
    Constraint.h
    DefaultParser.cpp
    DefaultParser.h
    ExtensionKey.cpp
    ExtensionKey.h
    ExtensionTable.cpp
    ExtensionTable.h
    FlatParser.cpp
    FlatParser.h
    HeadProbDeriver.cpp
    HeadProbDeriver.h
    HeadProbs.cpp
    HeadProbs.h
    KernelKey.cpp
    KernelKey.h
    KernelTable.cpp
    KernelTable.h
    LanguageSpecificFunctions.cpp
    LanguageSpecificFunctions.h
    LexicalProbDeriver.cpp
    LexicalProbDeriver.h
    LexicalProbs.cpp
    LexicalProbs.h
    ModifierProbDeriver.cpp
    ModifierProbDeriver.h
    ModifierProbs.cpp
    ModifierProbs.h
    ParseNode.cpp
    ParseNode.h
    ParseResultCollector.cpp
    ParseResultCollector.h
    ParserBase.cpp
    ParserBase.h
    Parser.cpp
    Parser.h
    ParserTags.cpp
    ParserTags.h
    PartOfSpeechTable.cpp
    PartOfSpeechTable.h
    PriorProbTable.cpp
    PriorProbTable.h
    SequentialBigrams.cpp
    SequentialBigrams.h
    SignificantConstitNode.cpp
    SignificantConstitNode.h
    SignificantConstitOracle.cpp
    SignificantConstitOracle.h
    STags.cpp
    STags.h
    TokenTagTable.cpp
    TokenTagTable.h
    VocabularyTable.cpp
    VocabularyTable.h
    WordFeatures.cpp
    WordFeatures.h
    xx_LanguageSpecificFunctions.h
    xx_NodeInfo.h
    xx_Parser.h
    xx_SignificantConstitOracle.h
    xx_STags.h
    xx_STags.cpp
    xx_WordFeatures.h
    ParserConfig.h.in
#    ParserConfig.h        # dynamically created now
  SUBDIRS
    ParserTrainer
)

# Pick fast floating point comparison option 
# (slow will make Unix and windows more the same)
SET (PARSER_FAST_FLOATING_POINT_COMPARISON "ON" CACHE BOOL "Use Fast Parsing Floating Point Comparison (Slow makes Linux & Windows the same ON/OFF): ")
MARK_AS_ADVANCED(PARSER_FAST_FLOATING_POINT_COMPARISON)

SET (CURRENT_DIR "parse")
FILE (MAKE_DIRECTORY ${DYNAMIC_INCLUDES_DIR}/${CURRENT_DIR})
SET(CONFIG_FILE ${DYNAMIC_INCLUDES_DIR}/${CURRENT_DIR}/ParserConfig.h)
CONFIGURE_FILE(
      ${CMAKE_CURRENT_SOURCE_DIR}/${CURRENT_DIR}/ParserConfig.h.in
      ${CONFIG_FILE})
APPEND_INTERNAL_LIST (DYNAMIC_INCLUDES_FILES ${CONFIG_FILE})
APPEND_INTERNAL_LIST (DYNAMIC_INCLUDES_DIRS ${CURRENT_DIR})



