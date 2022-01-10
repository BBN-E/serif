#!/bin/sh
# Run the massif tool (from valgrind) on Serif.

set -e # Exit immediately if any simple command fails.
set -u # Treat unset variables as an error.

mkdir -p `dirname "+OUT_FILE+"`

"+VALGRIND+" \
    --tool=massif \
    --log-file="+LOG_FILE+" \
    --depth=40 \
    --detailed-freq=1 \
    --max-snapshots=200 \
    --threshold=0.01 \
    --massif-out-file="+OUT_FILE+" \
    "+SERIF_BIN+" "+PAR_FILE+" -o "+OUT_DIR+"
# todo: add alloc-fn's?
