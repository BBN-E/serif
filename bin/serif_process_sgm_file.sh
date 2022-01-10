#!/bin/sh

#########################################################################
# Copyright 2009 BBN Technologies, all rights reserved
#
# Script to feed input files into the Serif Server
#########################################################################


if [ $# -ne 1 ] ; then
    echo "usage: serif_process_sgm_file.sh <FULL-path-sgm-filename>"
    exit 1
fi

#########################################################################
# MUST EDIT this to point to your Serif Server input directory
#########################################################################

SERIF_INPUT_DIR="$SERIF_PIPELINES/pipeline-1000/input_dir"

#########################################################################
# Edit here if you would like to process another file ext.
#########################################################################

SERIF_SGM_SUFFIX="sgm"

#########################################################################
# Probably will not need to edit below here
#########################################################################

INPUT_FILE="$1"

BASE_NAME=`basename ${INPUT_FILE} ${SERIF_SGM_SUFFIX}`

set -x

echo "${INPUT_FILE}" > "${SERIF_INPUT_DIR}/${BASE_NAME}source_files"
touch "${SERIF_INPUT_DIR}/${BASE_NAME}go"

