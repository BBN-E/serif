####################################################################
# Copyright (c) 2011 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# Generic                                                          #
#                                                                  #
####################################################################

  
ADD_SERIF_LIBRARY(Generic
  SUBDIRS
    actors
    ASR
    Cbrne
    PNPChunking
    SPropTrees
    apf
    causeEffect
    clutter
    commandLineInterface
    confidences
    database
    dependencyParse
    discTagger
    discourseRel
    distributionalKnowledge
    docRelationsEvents
    docentities
    driver
    eeml
    events
    factfinder
    generics
    icews
    linuxPort
    maxent
    metonymy
    mongoose
    morphAnalysis
    morphSelection
    nestedNames
    normalizer
    partOfSpeech
    patterns
    porterStemmer
    preprocessors
    propositions
    PropTree
    rawrelations
    reader
    results
    sentences
    sqlite
    state
    test
    tokens
    trainers
    transliterate
    UTCoref
    values
    wordClustering
    wordnet
    xdoc
    common
    descriptors
    edt
    names
    parse
    relations
    theories
    ### not included ### adept
    ### not included ### rawrelations
    ### not included ### wikipediaClustering
    CASerif
  LINK_LIBRARIES
    ${XercesC_LIBRARIES}
    ${Boost_LIBRARIES}
  UNIX_LINK_LIBRARIES
    dl pthread
)
