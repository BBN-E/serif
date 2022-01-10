####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic/common                                                   #
#                                                                  #
####################################################################

ADD_SERIF_LIBRARY_SUBDIR(common
  SOURCE_FILES
    AdeptWordConstants.cpp
    AdeptWordConstants.h
    ASCIIUtil.cpp
    ASCIIUtil.h
    Assert.cpp
    Assert.h
    Attribute.cpp
    Attribute.h
    AttributeValuePair.cpp
    AttributeValuePair.h
    AttributeValuePairExtractor.cpp
    AttributeValuePairExtractor.h
    AutoGrowVector.h
    BoundedPriorityQueue.h
    BoostLibrariesFound.h.in
    # BoostLibrariesFound.h
    BoostUtil.h
    Cache.h
    CacheStorage.h
    # CacheStorage.cpp
    CBRNEWordConstants.cpp
    CBRNEWordConstants.h
    cleanup_hooks.cpp
    cleanup_hooks.h
    ConsoleSessionLogger.cpp
    ConsoleSessionLogger.h
    DebugStream.cpp
    DebugStream.h
    # DescriptorInventory.cpp      # not included
    # DescriptorInventory.h        # not included
    dll_visibility.h
    Decompressor.h
    Decompressor.cpp
    DocumentTable.h
    DocumentTable.cpp
    GrowableArray.h
    GenericInternalTimer.h
    GenericTimer.cpp
    GenericTimer.h
    EvalSpecificRules.cpp
    EvalSpecificRules.h
    FeatureModule.cpp
    FeatureModule.h
    FileSessionLogger.cpp
    FileSessionLogger.h
    HeapChecker.cpp
    HeapChecker.h
    HeapStatus.cpp
    HeapStatus.h
    IDGenerator.h
    IStringStream.h
    InputUtil.cpp
    InputUtil.h
    InputStream.h
    InternalInconsistencyException.cpp
    InternalInconsistencyException.h
    LocatedString.cpp
    LocatedString.h
    LogMath.h
    LogMath.cpp
    lru_cache.h
    MemoryPool.h
    MemoryPool.cpp
    MinMaxHeap.h
    MinMaxHeap.cpp
    NameEquivalenceTable.cpp
    NameEquivalenceTable.h
    NationalityRecognizer.cpp
    NationalityRecognizer.h
    NgramScoreTable.cpp
    NgramScoreTable.h
    NullSessionLogger.h
    Offset.h
    OStringStream.h
    OutputStream.h
    OutputUtil.cpp
    OutputUtil.h
    ParamReader.cpp
    ParamReader.h
    ProbModel.cpp
    ProbModel.h
    ProfilingDefinition.h.in
    # ProfilingDefinition.h
    ProductName.h.in
    RegexMatch.cpp
    RegexMatch.h
    SGML.h
    Segment.cpp
    Segment.h
    SerifRestrictions.h.in
    SerifUtils.hpp
    SessionLogger.cpp
    SessionLogger.h
    Sexp.cpp
    Sexp.h
    SexpReader.cpp
    SexpReader.h
    StringTransliterator.cpp
    StringTransliterator.h
    StringView.h
    Symbol.cpp
    Symbol.h
    SymbolArray.h
    SymbolDefinitions.h.in
    SymbolConstants.cpp
    SymbolConstants.h
    SymbolHash.cpp
    SymbolHash.h
    SymbolListMap.cpp
    SymbolListMap.h
    SymbolSet.h
    SymbolUtilities.cpp
    SymbolUtilities.h
    TeeSessionLogger.cpp
    TeeSessionLogger.h
    TimedSection.cpp
    TimedSection.h
    TimexUtils.cpp
    TimexUtils.h
    TokenOffsets.cpp
    TokenOffsets.h
    UTF8InputStream.cpp
    UTF8InputStream.h
    UTF8OutputStream.cpp
    UTF8OutputStream.h
    UTF8Token.cpp
    UTF8Token.h
    UnicodeUtil.cpp
    UnicodeUtil.h
    UnexpectedInputException.cpp
    UnexpectedInputException.h
    UnsupportedOperationException.cpp
    UnsupportedOperationException.h
    UnrecoverableException.h
    WordConstants.h
    WordConstants.cpp
    bsp_declare.h
    hash_map.h
    hash_set.h
    leak_detection.h
    limits.h
    memory_ext.h
    std_hash.h
    version.h
    version.cpp
    XMLUtil.cpp
    XMLUtil.h
    # xx_AdeptWordConstants.h      # not included
    xx_CBRNEWordConstants.h
    xx_EvalSpecificRules.h
    xx_NationalityRecognizer.h
    xx_StringTransliterator.h
    xx_SymbolUtilities.cpp
    xx_SymbolUtilities.h
    xx_StringTransliterator.cpp
    xx_WordConstants.h
)

# Define Serif Profiling Options
#SET (DO_SERIF_PROFILING                      OFF   CACHE BOOL "Do Profiling (ON/OFF): ")
#MARK_AS_ADVANCED(DO_SERIF_PROFILING)

#SET (DO_SENTENCE_DRIVER_PROFILING            OFF  CACHE BOOL "Do Sentence Profiling (ON/OFF): ")
#MARK_AS_ADVANCED(DO_SENTENCE_DRIVER_PROFILING)

#SET (DO_DOCUMENT_DRIVER_PROFILING            OFF  CACHE BOOL "Do Document DriverProfiling (ON/OFF): ")
#MARK_AS_ADVANCED(DO_DOCUMENT_DRIVER_PROFILING)

SET (DO_DOC_RELATION_EVENT_PROFILING         OFF  CACHE BOOL "Do Relation Event Profiling (ON/OFF): ")
#MARK_AS_ADVANCED(DO_DOC_RELATION_EVENT_PROFILING)

SET (SERIF_SHOW_PROGRESS                        OFF  CACHE BOOL "Do Relation Event Profiling (ON/OFF): ")


SET (CURRENT_DIR "common")

## adding ProfileDefinition.h files
FILE (MAKE_DIRECTORY ${DYNAMIC_INCLUDES_DIR}/${CURRENT_DIR})
SET(PROFILING_CONFIG_FILE ${DYNAMIC_INCLUDES_DIR}/${CURRENT_DIR}/ProfilingDefinition.h)
CONFIGURE_FILE(
      ${CMAKE_CURRENT_SOURCE_DIR}/${CURRENT_DIR}/ProfilingDefinition.h.in
      ${PROFILING_CONFIG_FILE}
   )
APPEND_INTERNAL_LIST (DYNAMIC_INCLUDES_FILES ${PROFILING_CONFIG_FILE})
APPEND_INTERNAL_LIST (DYNAMIC_INCLUDES_DIRS ${CURRENT_DIR})


## adding BoostLibrariesFound.h.  This adds definitions which can be 
## used to check whether specific boost libraries (such as the regex
## library) were found for linking.
SET(BOOST_LIBRARIES_FOUND_CONFIG_FILE 
  ${DYNAMIC_INCLUDES_DIR}/${CURRENT_DIR}/BoostLibrariesFound.h)
CONFIGURE_FILE(
      ${CMAKE_CURRENT_SOURCE_DIR}/${CURRENT_DIR}/BoostLibrariesFound.h.in
      ${BOOST_LIBRARIES_FOUND_CONFIG_FILE}
   )
APPEND_INTERNAL_LIST (DYNAMIC_INCLUDES_FILES ${BOOST_LIBRARIES_FOUND_CONFIG_FILE})


## adding SymbolDefinitions.h files
SET (SYMBOL_TABLE_BUCKETS       70001    CACHE INTEGER "Set the Symbol table number of buckets")
SET (SYMBOL_STRING_BLOCK_SIZE   32000    CACHE INTEGER "Set the Symbol table string block size")
SET (SYMBOL_ENTRY_BLOCK_SIZE     4000    CACHE INTEGER "Set the Symbol table entry block size")
SET (SYMBOL_LEFT_BIT_SHIFT          2    CACHE INTEGER "The number of bits a character is shifted in Symbol hash_code()")
SET (SYMBOL_MAX_STRING_POOL_STRINGLEN 30 CACHE INTEGER "The maximum string length for which the string pool should be used for memory allocation")
SET (SYMBOL_REF_COUNT              ON    CACHE BOOL    "Use reference counting")
SET (SYMBOL_THREADSAFE             OFF   CACHE BOOL    "Make the symbol table thread-safe.")

## Other .h files
SET (BLOCK_FULL_SERIF_OUTPUT	   OFF	 CACHE BOOL    "Block some Serif functionality.")
SET (PRODUCT_NAME                  Serif CACHE STRING  "The product name to print out at startup in CommandLineInterface.cpp")

MATH (EXPR  SYMBOL_RIGHT_BIT_SHIFT     32-${SYMBOL_LEFT_BIT_SHIFT} )
MARK_AS_ADVANCED (SYMBOL_TABLE_BUCKETS SYMBOL_STRING_BLOCK_SIZE SYMBOL_ENTRY_BLOCK_SIZE SYMBOL_LEFT_BIT_SHIFT SYMBOL_MAX_STRING_POOL_STRINGLEN)

SET(SYMBOL_CONFIG_FILE ${DYNAMIC_INCLUDES_DIR}/${CURRENT_DIR}/SymbolDefinitions.h)
CONFIGURE_FILE(
      ${CMAKE_CURRENT_SOURCE_DIR}/${CURRENT_DIR}/SymbolDefinitions.h.in
      ${SYMBOL_CONFIG_FILE}
   )

SET(RESTRICTION_CONFIG_FILE ${DYNAMIC_INCLUDES_DIR}/${CURRENT_DIR}/SerifRestrictions.h)
CONFIGURE_FILE(
      ${CMAKE_CURRENT_SOURCE_DIR}/${CURRENT_DIR}/SerifRestrictions.h.in
      ${RESTRICTION_CONFIG_FILE}
   )
APPEND_INTERNAL_LIST (DYNAMIC_INCLUDES_FILES ${RESTRICTION_CONFIG_FILE})

SET(PRODUCT_NAME_CONFIG_FILE ${DYNAMIC_INCLUDES_DIR}/${CURRENT_DIR}/ProductName.h)
CONFIGURE_FILE(
      ${CMAKE_CURRENT_SOURCE_DIR}/${CURRENT_DIR}/ProductName.h.in
      ${PRODUCT_NAME_CONFIG_FILE}
)
APPEND_INTERNAL_LIST (DYNAMIC_INCLUDES_FILES ${PRODUCT_NAME_CONFIG_FILE})

APPEND_INTERNAL_LIST (DYNAMIC_INCLUDES_FILES ${SYMBOL_CONFIG_FILE})
APPEND_INTERNAL_LIST (DYNAMIC_INCLUDES_DIRS ${CURRENT_DIR})
