####################################################################
# Copyright (c) 2007 by BBNT Solutions LLC                         #
# All Rights Reserved.                                             #
#                                                                  #
# CMake/standalones-includes.cmake                                 #
#                                                                  #
# Adding executables for Standalones and other solutions           #
####################################################################

SET(CREATE_STANDALONE_PROJECTS ON CACHE BOOL
    "Build the Standalone projects")
MARK_AS_ADVANCED (CREATE_STANDALONE_PROJECTS)

# Define a list of all standalone projects that we should build.
SET (STANDALONE_PROJECTS
    #ASRSentBreaker
    #AnnotatedParsePreprocessor
    #Cluster
    CombineTemporalTraining
    DTCorefTrainer
    DeriveTables
    #DescriptorClassifierTrainer
    #DescriptorLinkerTrainer
    #DiscourseRelTrainer
    DistillDocToLearnItDoc
    DocSim
    EquivalentNamesUploader
    EventFinder
    GraphicalModels
    Headify
    IdfTrainer
    IdfTrainerPreprocessor
    InstanceFinder
    K_Estimator
    L2Analyzer
    LBFGS-B
    LearnIt2Trainer
    MaxEntRelationTrainer
    #MaxEntSimulatedActiveLearning
    MentionMapper
    MorphAnalyzer
    MorphologyTrainer
    NameLinkerTrainer
    NegativeExampleProposer
    P1DescTrainer
    P1RelationTrainer
    PIdFQLSocketServer
    PIdFQuickLearn
    #PIdFSimulatedActiveLearning
    PIdFTrainer
    PNPChunkTrainer
    PosteriorRegularization
    PPartOfSpeechTrainer
    Preprocessor
    PronounLinkerTrainer
    UntypedCoref
    RelationTimexArgFinder
    RelationTrainer
    #RelationVectorExtractor
    SerifSocketServer
    StandaloneParser
    StandaloneSentenceBreaker
    StandaloneTokenizer
    StatSentBreakerTrainer
    StatsCollector
    TemporalDocToFV
    TemporalTrainer
    UnsupervisedEvents
    VocabCollector
    VocabPruner
    )

# Enable encryption binary build if we're using cipher stream
IF (FEATUREMODULE_BasicCipherStream_INCLUDED)
    SET(STANDALONE_PROJECTS ${STANDALONE_PROJECTS} BasicCipherStreamEncryptFile)
ENDIF(FEATUREMODULE_BasicCipherStream_INCLUDED)

# Enable license binary build if we're using licenses
IF (FEATUREMODULE_License_INCLUDED)
    SET(STANDALONE_PROJECTS ${STANDALONE_PROJECTS} create_license print_license verify_license)
ENDIF(FEATUREMODULE_License_INCLUDED)

IF (CREATE_STANDALONE_PROJECTS)

    # Include each standalone project's subdirectory
    FOREACH (SUBD ${STANDALONE_PROJECTS})
        IF (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SUBD}")
            ADD_SUBDIRECTORY(${SUBD})
        ENDIF (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SUBD}")
    ENDFOREACH (SUBD ${STANDALONE_PROJECTS})

    # These each get their own solution. (why?)
    ADD_SUBDIRECTORY(ParserTrainer)
    ADD_SUBDIRECTORY(Cluster)

ENDIF (CREATE_STANDALONE_PROJECTS)
