#!/bin/env bash

set -e # Exit immediately if any simple command fails.
set -u # Treat unset variables as an error.

function die() {
    echo "$*"
    exit 1
}


mkdir -p +display_out_dir+

#echo "dump_html.sh \n"
#echo "here's the LD_LIBRARY_PATH"
export LD_LIBRARY_PATH=+LD_LIBRARY_PATH+
echo $LD_LIBRARY_PATH
echo "dumping the icews to html"
#echo  " calling +PYTHON+ +compare_events+  +display_out_file+ +parallel+ \
#   +serif_output_dir+  "  
echo " "

+PYTHON+ +compare_events+  +display_out_file+ +parallel+ +serif_output_dir+ 

echo "dumped html to +display_out_file+"
