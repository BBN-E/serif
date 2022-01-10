#!/bin/bash

# Copyright 2009 by BBN Technologies Corp.
# All Rights Reserved.

#required variables
PAR_TEMPLATE_FILE=$1
PAR_FILE=$2

#Assumed values for these variables.
# Should be overridden with values provided on command line. TODO
START_STAGE="START"
END_STAGE="END"
PARALLEL="000"
TEST_OUT_DIR=output
## OS specific settings
OS=`uname`
echo "OS = $OS"

if [ "$OS" = "Linux" ]; then
echo "Running on Linux"
# For linux
SERIF_DATA_DIR="/d4m/serif/data"
SERIF_REGRESSION_DIR="/nfs/traid04/u1/projects/Ace"
SERIF_SCORE_DIR="/nfs/traid04/u1/projects/Serif/score"
OS_SUFFIX="-Linux"
else # assume running on Windows
echo "Running on non Unix"
# For windows
SERIF_DATA_DIR="//traid01/speed"
SERIF_REGRESSION_DIR="//traid01/speed/Ace"
SERIF_SCORE_DIR="//traid01/speed/Serif/score"
OS_SUFFIX=""
fi


if [ -z $PAR_TEMPLATE_FILE ] ; then
    echo "Usage: $0 PAR_TEMPLATE_FILE PAR_FILE "
    exit 1
fi

if [ -z $PAR_FILE ] ; then
    echo "Usage: $0 PAR_TEMPLATE_FILE PAR_FILE "
    exit 1
fi

echo "Working directory `pwd`"
echo "Parameter Template file: $PAR_TEMPLATE_FILE"

echo "Generated parameter file $PAR_FILE"
# In -e s#+expt_dir+#$TEST_OUT_DIR# 
# Pound sign (#) acts as a separator and not back slash (/)
sed -e s/+start_stage+/$START_STAGE/ -e s/+end_stage+/$END_STAGE/  -e s/+parallel+/$PARALLEL/ \
    -e s#+expt_dir+#$TEST_OUT_DIR# -e s#+serif_data+#$SERIF_DATA_DIR# \
    -e s#+os_suffix+#$OS_SUFFIX# -e s#+serif_score+#$SERIF_SCORE_DIR# \
    -e s#+serif_regression+#$SERIF_REGRESSION_DIR# $PAR_TEMPLATE_FILE > $PAR_FILE

echo $PWD
exit 0