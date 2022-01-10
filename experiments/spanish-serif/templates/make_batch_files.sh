#!/bin/bash
set -e # Exit immediately if any simple command fails.
set -u # Treat unset variables as an error.

BATCH_FILE="+BATCH_FILE+";
JOB_NAME="+JOB_NAME+";
NUM_JOBS="+NUM_JOBS+";
EXPT_DIR="+EXPT_DIR+";

num_lines=`cat $BATCH_FILE | wc -l`;
echo "Batch file contains $num_lines lines"

files_per_job=$[($num_lines+$NUM_JOBS-1)/$NUM_JOBS]
echo "Each job will process $files_per_job files"

BATCH_FILE_DIR=$EXPT_DIR/$JOB_NAME/batch_files
OUTPUT_DIR=$EXPT_DIR/$JOB_NAME/output

mkdir -p $BATCH_FILE_DIR
mkdir -p $OUTPUT_DIR

split -d -a 4 -l $files_per_job $BATCH_FILE $BATCH_FILE_DIR/batch_file_

