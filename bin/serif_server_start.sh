#!/bin/sh

#########################################################################
# Copyright 2009 BBN Technologies, all rights reserved
#
# Script to start Serif Server given a pipeline.xml file
#########################################################################


if [ $# -ne 1 ] ; then
    echo "usage: serif_server_start.sh <pipeline-xml-file>"
    exit 1
fi

#########################################################################
# MUST EDIT this to point to your server_byblos_driver program
#########################################################################

SERVER_BYBLOS_DRIVER="$BYBLOS_DIST/install-optimize-static/bin/server_byblos_driver"

#########################################################################
# Optionally the number of Serif instances to start. We do not recommend
# more than 2 (3 tops).
#########################################################################

NUM_THREADS=1

#########################################################################
# Probably will not need to edit below here
#########################################################################

PIPELINE_XML="$1"

set -x

${SERVER_BYBLOS_DRIVER} -j${NUM_THREADS} --trigger-ext=go ${PIPELINE_XML} --unique-output-dirs
