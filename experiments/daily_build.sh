#!/bin/bash

set -e # Exit with an error immediately if any simple command fails.
set -u # Treat unset variables as an error.
#set -x # Print commands as we run them (useful for debugging)

###########################################################################
# Command-line argument processing
###########################################################################

if [ $# -gt 1 ]; then
    echo "Usage: $0 [daily|tput]"
    exit -1
fi
if [ $# == 0 ]; then config="daily"; else config="$1"; fi
if [ "$config" == "daily" ]; then
    export EXPT_NAME="SERIF Daily Build"
    export BUILD_PREFIX="SerifDaily"
	# Note  AceBest is Ace test of old Classic with Best, IcewsBest is Icews test on Classic Best
    export BUILD_OPTS="--build AceFast+AceBest+AwakeBest+IcewsBest/Win32+Win64+i686+x86_64"
elif [ "$config" == "idx" ]; then
    export EXPT_NAME="IDX Daily Build"
    export BUILD_PREFIX="IDX_Daily"
    export BUILD_OPTS="--build IDX/Win32+i686+x86_64+x86_64_sl5"
elif [ "$config" == "tput" ]; then
    export EXPT_NAME="SERIF Throughput Experiment"
    export BUILD_PREFIX="SerifTput"
    export BUILD_OPTS="--tput --save-tput --one-machine --build Classic"
else
    echo "Usage: $0 [daily|idx|tput]"
    exit -1
fi

###########################################################################
# Configuration
###########################################################################

# The number of days after which we should compress or delete an old
# daily-build run.
export COMPRESS_AFTER=2
export DELETE_AFTER=30

# The email address that a report should be sent to.
export MAILTO="serif-regression@bbn.com"
export REPLYTO="serif-regression@bbn.com"

# Output files
export BUILD_ROOT="/d4m/serif/daily_build"
export DELETE_LOG="$BUILD_ROOT/delete.log"
export BUILD_EXP=`/bin/date +"$BUILD_PREFIX%Y%m%d.%H%M%S"`
export BUILD_DIR="$BUILD_ROOT/$BUILD_EXP"
export BUILD_LOG="$BUILD_DIR/log.txt"
export BUILD_DIR_PATTERN="${BUILD_PREFIX}????????.??????"

# SVN URLs
export SVN_ROOT="svn+ssh://svn.d4m.bbn.com/export/svn-repository/text/trunk"
export BUILD_EXPT_SVN_URL="$SVN_ROOT/Active/Projects/SERIF/experiments/build_serif"

# Architecture
export ARCH=
if [ `uname -m` == "x86_64" ]; then
    export ARCH_SUFFIX="-x86_64"
else
    export ARCH_SUFFIX=""
fi

# Binaries & scripts
export BUILD_SERIF="$BUILD_DIR/sequences/build_serif.pl"
export SEND_EMAIL="perl `dirname $0`/send_email.pl"

###########################################################################
# Error reporting
###########################################################################
# This function is used to report errors in the regression test script 
# itself (as opposed to errors in SERIF).
function die () {
    echo "Error in Daily Build Script: $*"
    echo "  Sending email to $MAILTO..."
    (
	echo "Error in Daily Build Script: $*";
	echo " Build log: $BUILD_LOG";
	echo "Delete log: $DELETE_LOG";
        echo "-------------------- BUILD LOG --------------------";
	[ -e $BUILD_LOG ] && cat $BUILD_LOG || echo "File not found"
    ) | $SEND_EMAIL --to "$MAILTO" --from $REPLYTO \
                    --subject "Daily Build Script Error" 
    chmod a+w -R "$BUILD_DIR"
    exit 1
}

date

function rotate_log() {
	for i in 4 3 2 1; do
		if [ -e "$1.$i" ]; then
			mv "$1.$i" "$1.$[$i+1]"
		fi
	done
	if [ -e "$1" ]; then 
		mv "$1" "$1.1"
	fi
}

###########################################################################
# Delete old daily build runs
###########################################################################
rotate_log $DELETE_LOG
echo "Compressing and deleting old SERIF daily build files..."
echo "Log file: $DELETE_LOG"
date >>$DELETE_LOG 2>&1

## we have been blocked from using these find-based deletes because "." was in the $PATH
export PATH_W_DOT=$PATH
export PATH=${PATH/.:/}


# Compress build dirs that are older than $COMPRESS_AFTER days.  We
# currently only keep the ckpts, logfiles, etemplates, and regtest
# output files (not including the *-eval.full.txt files).  Should we
# also keep the generated binary files?  They're in {}/install.
find $BUILD_ROOT -mindepth 1 -maxdepth 1 \
    -name "$BUILD_DIR_PATTERN" \
    -type d -mtime +$COMPRESS_AFTER \
    -exec echo Compressing: {} >> $DELETE_LOG 2>&1 \; \
    -execdir tar -czf {}.tgz \
	    --ignore-failed-read \
        {}/log.txt {}/logfiles {}/etemplates \
        {}/expts/test --exclude '*-eval-full.txt' \; \
    -execdir rm -rf {} \; \
    >> $DELETE_LOG 2>&1 \
    || die "Error while compressing and deleting old daily build runs"

# Delete any compressed builds that are older than $DELETE_AFTER old.
find $BUILD_ROOT -mindepth 1 -maxdepth 1 \
    -name "${BUILD_DIR_PATTERN}.tgz" \
    -type f -mtime +$DELETE_AFTER \
    -exec echo Deleting: {} >> $DELETE_LOG 2>&1 \; \
    -execdir rm {} \; \
    >> $DELETE_LOG 2>&1 \
    || die "Error while deleting old compressed daily build runs"

# Also delete any uncompressed build dirs that are still around.
find $BUILD_ROOT -mindepth 1 -maxdepth 1 \
    -name "$BUILD_DIR_PATTERN" \
    -type d -mtime +$DELETE_AFTER \
    -exec echo Deleting: {} >> $DELETE_LOG 2>&1 \; \
    -execdir rm -rf {} \; \
    >> $DELETE_LOG 2>&1 \
    || die "Error while deleting old daily build runs"

export PATH=$PATH_W_DOT
unset PATH_W_DOT

# Make sure anyone can overwrite a delete log
chmod 666 $DELETE_LOG || true

###########################################################################
# Run the new daily build experiment
###########################################################################
echo "Running $EXPT_NAME..."
echo "Log file: $BUILD_LOG"

# Ensure that runjobs does not create symlinks for its output directories
# by clearning the associated environment variables.
unset SEQUENCES TEMPLATES EXPTS CHECKPOINTS LOGFILES ETEMPLATES

# Create the output directory.
mkdir -p "$BUILD_DIR" || die "Error while creating $BUILD_DIR"

# Check the build-serif experiment out from svn
svn co "$BUILD_EXPT_SVN_URL" "$BUILD_DIR" >> "$BUILD_LOG" 2>&1 \
    || die "Error while checking out $BUILD_EXPT_SVN_URL to $BUILD_DIR"

# Run the daily-build experiment, and send an email reporting the outcome.
perl "$BUILD_SERIF" --sge --name "$EXPT_NAME" \
    --return-success-after-reporting-failure \
    $BUILD_OPTS \
    --mailto "$MAILTO" --mailfrom "$REPLYTO" >> "$BUILD_LOG" 2>&1 \
    || die "Error while running the regression test"

# Make sure that the new daily build is writable in case someone
# else needs to delete it or do something with it.
chmod a+w -R "$BUILD_DIR" >> "$BUILD_LOG" 2>&1 \
    || die "Error while setting permissions"

chmod 666 $BUILD_ROOT/cron.log
rotate_log $BUILD_ROOT/cron.log
