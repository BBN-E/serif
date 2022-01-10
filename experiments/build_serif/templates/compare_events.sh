#!/bin/env bash

set -e # Exit immediately if any simple command fails.
set -u # Treat unset variables as an error.

function die() {
    echo "$*"
    exit 1
}

mkdir -p +compare_out_dir+

echo "compare_events.sh \n"
export LD_LIBRARY_PATH=+LD_LIBRARY_PATH+
echo $LD_LIBRARY_PATH

+PYTHON+ +compare_events+  +compare_out_file+ +parallel+ +testdata_dir+ --expt2 +serif_output_dir+

if [ -s "+compare_out_file+" ]; then
	die " Events check reported differences in  +compare_out_file+ "
else
	echo "  Events check wrote +compare_out_file+ with no diffs noted."
fi
