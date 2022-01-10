#!/bin/sh
# Run the callgrind tool (from valgrind) on Serif.

set -e # Exit immediately if any simple command fails.
set -u # Treat unset variables as an error.

# To make it easier to understand Serif's behavior over time, we tell
# valgrind to generate a dump every 10 billion instructions.   A 
# typical profiling session will generate about 1 dump for each 10
# documents processed (in English, anyway).
DUMP_FREQUENCY=10000000000

mkdir -p `dirname "+OUT_FILE+"`

# If there are any old output files, then delete them.
rm -f "+OUT_FILE+"*

"+VALGRIND+" \
    --tool=callgrind \
    --log-file="+LOG_FILE+" -v \
    --callgrind-out-file="+OUT_FILE+"-part \
    --dump-every-bb=$DUMP_FREQUENCY \
    +OPTIONS+ \
    "+SERIF_BIN+" "+PAR_FILE+" -o "+OUT_DIR+"

# Once Serif is done running, we merge the individual output files
# into a single output file.  (Note: the callgrind tool seems to 
# use some formatting options that cg_merge doesn't understand; but
# kcachegrind can handle it if we just cat the files together.)
cat "+OUT_FILE+"-part* > "+OUT_FILE+.merged"
