#!/bin/env bash

set -e # Exit immediately if any simple command fails.
set -u # Treat unset variables as an error.

function die() {
    echo "$*"
    exit 1
}

# Check if scores changed.  Note: we will immediately fail if the files
# differ because of the "set -e" above.
if [ -e  "+testdata_dir+/+SCORE_SUMMARY_FILE+" ]; then
	echo
	echo "Checking scores ..." 
	diff -u +testdata_dir+/+SCORE_SUMMARY_FILE+ +out_dir+/+SCORE_SUMMARY_FILE+ \
		|| die "Scores  changed."
	echo "  Ok."
else
	echo "No summary file in +testdata_dir+  so no summary test"
fi

# Check if output files changed (if they're available).  Note: we will
# immediately fail if the files differ because of the "set -e" above.
if [ -e "+testdata_dir+/output" ]; then
    echo
    if [ "`ls +testdata_dir+/output/*.xml 2>/dev/null`" ]; then
        echo "Checking output files (including serifxml)..."
        diff -ruq +testdata_dir+/output +out_dir+/output \
    	|| die "Output changed"
    else
        echo "Checking output files (excluding serifxml)..."
        diff --exclude='*.xml' -ruq +testdata_dir+/output +out_dir+/output \
    	|| die "Output changed"
    fi
    echo "  Ok."
else
	echo "No output files file in +testdata_dir+  so no test"
fi

echo "check_scores did not fail to match testdata scores"
