#
# SERIF Build Makefile for UNIX
# 
# Copyright (c) 2011 by BBNT Technologies
# All Rights Reserved.
#

###########################################################################
# Configuration
###########################################################################

# Default build configuration
CONFIG = Release

# List of supported build configurations
CONFIGS = Release Debug RelWithDebInfo GProf

# These targets generate executable binaries:
EXECUTABLE_TARGETS = \
	Serif \
	Cluster \
	DTCorefTrainer \
	IdfTrainer \
	MaxEntRelationTrainer \
	MorphologyTrainer \
	NameLinkerTrainer \
	P1DescTrainer \
	P1RelationTrainer \
	PIdFTrainer \
	PNPChunkTrainer \
	PPartOfSpeechTrainer \
	DeriveTables \
	Headify \
	K_Estimator \
	StatsCollector \
	VocabCollector \
	VocabPruner \
	PronounLinkerTrainer \
	RelationTrainer \
	StatSentBreakerTrainer \
	StandaloneSentenceBreaker \
	EventFinder \
	IdfTrainerPreprocessor \
	RelationTimexArgFinder \
	SerifSocketServer \
	StandaloneParser \
	StandaloneTokenizer \
	BasicCipherStreamEncryptFile \
	KbaStreamCorpus

# These targets generate DLL feature modules:
MODULE_TARGETS = \
	libSerifEnglish.so \
	libSerifArabic.so \
	libSerifChinese.so 

# These targets are disabled by default:
DISABLED_TARGETS = \
	ASRSentBreaker \
	AnnotatedParsePreprocessor \
	DescriptorClassifierTrainer \
	DescriptorLinkerTrainer \
	DiscourseRelTrainer \
	ATEASerif_Batch_English \
	ATEASerif_English

# List of all targets (excluding disabled targets):
SERIF_TARGETS = ${EXECUTABLE_TARGETS} ${MODULE_TARGETS}

# Output directory for binaries
BIN = bin

###########################################################################
# General Make Targets
###########################################################################
.PHONY: usage targets clean all debug release relwithdebinfo

usage: 
	@echo
	@echo "Serif Build target:"
	@echo "    make Serif"
	@echo
	@echo "Other targets:"
	@echo "    make all     -- build all targets"
	@echo "    make targets -- for a full list of targets"\
	           "(eg to build trainers)"
	@echo "    make clean   -- remove all built files"
	@echo
	@echo "Make variables:"
	@for C in ${CONFIGS}; do \
	    if [ "$${C}" == "${CONFIG}" ]; then \
	        echo "    make CONFIG=$${C} ...            [default]"; \
	    else \
	        echo "    make CONFIG=$${C} ..."; \
	    fi; \
	 done
	@echo

targets:
	@echo "Serif Build targets:"
	@for T in $(foreach T,${SERIF_TARGETS},'${T}'); do \
		echo "    make $$T"; \
	done

all: $(SERIF_TARGETS)

clean:
	for C in ${CONFIGS}; do ; \
	    rm -rf $${C}; done
	rm -f ${SERIF_TARGETS}

debug: CONFIG=Debug
debug: Serif

release: CONFIG=Release
release: Serif

relwithdebinfo: CONFIG=RelWithDebInfo
relwithdebinfo: Serif

###########################################################################
# CMake Targets
###########################################################################

CMAKE_CACHE_FILES = $(foreach CONFIG,${CONFIGS},${CONFIG}/CMakeCache.txt)

CMAKE_OPTIONS=$(if ${ICEWS},-DUSE_MySQL=ON -DFEATUREMODULE_ICEWS_INCLUDED=ON -DUSE_ORACLE=ON -DFEATUREMODULE_BasicCipherStream_INCLUDED=ON,)

$(CMAKE_CACHE_FILES):%/CMakeCache.txt:
	mkdir -p $*
	(cd $*; cmake ../.. -DCMAKE_BUILD_TYPE=$* ${CMAKE_OPTIONS})

# Don't delete CMakeCache.txt intermediate files:
.PRECIOUS: ${CMAKE_CACHE_FILES}

###########################################################################
# Build Targets
###########################################################################

$(EXECUTABLE_TARGETS):%: ALWAYS_RUN ${CONFIG}/CMakeCache.txt
	@echo "Building: $* (${CONFIG})"
	$(MAKE) -C "${CONFIG}" depend
	$(MAKE) -C "${CONFIG}" $*
	mkdir -p ${BIN}
	cp ${CONFIG}/*/$* ${BIN}

$(MODULE_TARGETS):libSerif%.so: ALWAYS_RUN ${CONFIG}/CMakeCache.txt
	@echo "Building: $*_MODULE (${CONFIG})"
	$(MAKE) -C "${CONFIG}" depend
	$(MAKE) -C "${CONFIG}" $*_MODULE
	mkdir -p ${BIN}
	cp ${CONFIG}/*/libSerif$*.so ${BIN}

# This target (ALWAYS_RUN) makes sure that a rule runs even if the
# output file exists.
.PHONY: ALWAYS_RUN
