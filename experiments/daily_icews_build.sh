#!/bin/bash

set -e # Exit with an error immediately if any simple command fails.
set -u # Treat unset variables as an error.
#set -x # Print commands as we run them (useful for debugging)

####### make all files written easy to delete to avoid clashes between users
umask 000

###########################################################################
# Command-line argument processing
###########################################################################

export config="icews"
export EXPT_NAME="ICEWS SERIF Daily Build"
export BUILD_PREFIX="ICEWS_Daily"
export BUILD_OPTS="--build-only --build=Win32+Win64+x86_64/English/IcewsBest/Release"

###########################################################################
# Configuration
###########################################################################

# The number of days after which we should compress or delete an old
# daily-build run.
export COMPRESS_AFTER=2
export DELETE_AFTER=10000

# The email address that a report should be sent to.
export MAILTO="spectrum-regression@bbn.com"
export REPLYTO="pmartin@bbn.com"

# Output files
### fix back to shorter one if re-merging
export BUILD_ROOT="/d4m/serif/icews_daily_build"
export BASE_UTIL_BIN="/d4m/serif/bin"
export DELETE_LOG="$BUILD_ROOT/delete.log"
export BUILD_EXP=`/bin/date +"$BUILD_PREFIX%Y%m%d.%H%M%S"`
export BUILD_DIR="$BUILD_ROOT/$BUILD_EXP"
export BUILD_SERIF_DIR="$BUILD_DIR/SERIF_EXP"
export BUILD_ICEWS_PROJ_DIR="$BUILD_DIR/ICEWS_PROJ"
export SERIF_PAR_DIR="$BUILD_DIR/SERIF/par"
export SERIF_PYTHON_DIR="$BUILD_DIR/SERIF/python"
export BUILD_LOG="$BUILD_DIR/log.txt"
export BUILD_DIR_PATTERN="${BUILD_PREFIX}????????.??????"

# SVN URLs
export SVN_ROOT="svn+ssh://svn.d4m.bbn.com/export/svn-repository/text/trunk"
export BUILD_EXPT_SVN_URL="$SVN_ROOT/Active/Projects/SERIF/experiments/build_serif"
export SERIF_PAR_SVN_URL="$SVN_ROOT/Active/Projects/SERIF/par"
export SERIF_PYTHON_SVN_URL="$SVN_ROOT/Active/Projects/SERIF/python"
export BUILD_ICEWS_PROJ_SVN_URL="$SVN_ROOT/Active/Projects/W-ICEWS"

# Architecture
export ARCH=
if [ `uname -m` == "x86_64" ]; then
    export ARCH_SUFFIX="-x86_64"
else
    export ARCH_SUFFIX=""
fi

# Binaries & scripts
export PATH="/bin:/usr/bin"
export SVN="/opt/subversion-1.6.9$ARCH_SUFFIX/bin/svn"
export PERL="/opt/perl-5.8.6$ARCH_SUFFIX/bin/perl"
export BUILD_SERIF="$BUILD_SERIF_DIR/sequences/build_serif.pl"
export SEND_EMAIL="$PERL `dirname $0`/send_email.pl"

###########################################################################
# Error reporting
###########################################################################
# This function is used to report errors in the regression test script
# itself (as opposed to errors in SERIF).
function die () {
    echo "Error in ICEWS Daily Test_Compare Script: $*"
    echo "  Sending email to $MAILTO..."
    (
	echo "Error in ICEWS Test_Compare Regression Test: $*";
	echo " Build log: $BUILD_LOG";
	echo "Delete log: $DELETE_LOG";
        echo "-------------------- BUILD LOG --------------------";
	[ -e $BUILD_LOG ] && cat $BUILD_LOG || echo "Build Log File not found"
    ) | $SEND_EMAIL --to "$MAILTO" --from $REPLYTO \
                    --subject "ICEWS Test_Compare Regression Error"
    chmod a+w -R "$BUILD_DIR"
    exit 1
}

###########################################################################
# Success reporting
###########################################################################
# This function is used to report overall success in the regression test script
# itself (as opposed to errors in the SERIF experiment).
function succeed () {
    echo "Successful ICEWS Test_Compare Reg Test: $*"
    echo "  Sending email to $MAILTO..."
    (
	echo "Successful ICEWS Test_Compare Regression Test: $*";
	echo " Build log: $BUILD_LOG";
	echo "Delete log: $DELETE_LOG";
        echo "-------------------- BUILD LOG --------------------";
	[ -e $BUILD_LOG ] && cat $BUILD_LOG || echo "Build Log File not found"
    ) | $SEND_EMAIL --to "$MAILTO" --from $REPLYTO \
                    --subject "ICEWS Test_Compare Regression Success"
    chmod a+w -R "$BUILD_DIR"
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
echo "Compressing and deleting old $BUILD_PREFIX daily build files..."
echo "Log file: $DELETE_LOG"
date >>$DELETE_LOG 2>&1

# Compress build dirs that are older than $COMPRESS_AFTER days.  We
# currently  keep the ckpts, logfiles, etemplates, and regtest
# output files . We also keep the generated binary files --  They're in {}/install.
find -L $BUILD_ROOT -mindepth 1 -maxdepth 1 \
    -name "$BUILD_DIR_PATTERN" \
    -type d -mtime +$COMPRESS_AFTER \
    -exec echo Compressing: {} >> $DELETE_LOG 2>&1 \; \
    -execdir tar -czhf {}.tgz \
	    --ignore-failed-read \
        {}/log.txt {}/SERIF_EXP/expts/install {}/ICEWS_PROJ/experiments/detect_events \; \
    -execdir rm -rf {} \; \
    >> $DELETE_LOG 2>&1 \
    || die "Error while compressing old daily build runs"

# Delete any compressed builds that are older than $DELETE_AFTER old.
find -L $BUILD_ROOT -mindepth 1 -maxdepth 1 \
    -name "${BUILD_DIR_PATTERN}.tgz" \
    -type f -mtime +$DELETE_AFTER \
    -exec echo Deleting: {} >> $DELETE_LOG 2>&1 \; \
    -execdir rm {} \; \
    >> $DELETE_LOG 2>&1 \
    || die "Error while deleting old compressed daily build runs"

# Also delete any uncompressed build dirs that are still around.
find -L $BUILD_ROOT -mindepth 1 -maxdepth 1 \
    -name "$BUILD_DIR_PATTERN" \
    -type d -mtime +$DELETE_AFTER \
    -exec echo Deleting: {} >> $DELETE_LOG 2>&1 \; \
    -execdir rm -rf {} \; \
    >> $DELETE_LOG 2>&1 \
    || die "Error while deleting old daily build runs"


###########################################################################
# Run the new daily build experiment
###########################################################################
echo "Running $EXPT_NAME..."
echo "Log file: $BUILD_LOG"

# Ensure that runjobs does not create symlinks for its output directories
# by clearing the associated environment variables.
unset SEQUENCES TEMPLATES EXPTS CHECKPOINTS LOGFILES ETEMPLATES

# Create the output directories.
mkdir -p "$BUILD_DIR" || die "Error while creating $BUILD_DIR"
mkdir -p "$BUILD_SERIF_DIR" || die "Error while creating $BUILD_SERIF_DIR"

#verify that we have space to run the test
export MIN_SPACE="10";
$PERL $BASE_UTIL_BIN/space_check.pl $MIN_SPACE $BUILD_ROOT  >> "$BUILD_LOG" 2>&1 \
	||  die "Not enough disk space for icews regression test"


# Check the build-serif experiment out from svn
$SVN co "$BUILD_EXPT_SVN_URL" "$BUILD_SERIF_DIR" >> "$BUILD_LOG" 2>&1 \
    || die "Error while checking out $BUILD_EXPT_SVN_URL to $BUILD_SERIF_DIR"

# Check the ICEWS regtest experiment out from svn -- but get TWO levels higher in tree
    mkdir -p "$BUILD_ICEWS_PROJ_DIR" || die "Error while creating $BUILD_ICEWS_PROJ_DIR"
    $SVN co "$BUILD_ICEWS_PROJ_SVN_URL" "$BUILD_ICEWS_PROJ_DIR" >> "$BUILD_LOG" 2>&1 \
        || die "Error while checking out $BUILD_ICEWS_PROJ_SVN_URL to $BUILD_ICEWS_PROJ_DIR"

	# serif parameters are needed for the ICEWS reg test run
	mkdir -p "$SERIF_PAR_DIR" || die "Error while creating $SERIF_PAR_DIR"
    $SVN co "$SERIF_PAR_SVN_URL" "$SERIF_PAR_DIR" >> "$BUILD_LOG" 2>&1 \
        || die "Error while checking out $SERIF_PAR_SVN_URL to $SERIF_PAR_DIR"

	# serif python code is needed for the ICEWS reg test run
	mkdir -p "$SERIF_PYTHON_DIR" || die "Error while creating $SERIF_PYTHON_DIR"
    $SVN co "$SERIF_PYTHON_SVN_URL" "$SERIF_PYTHON_DIR" >> "$BUILD_LOG" 2>&1 \
        || die "Error while checking out $SERIF_PYTHON_SVN_URL to $SERIF_PYTHON_DIR"

export BUILD_ICEWS_DIR="$BUILD_ICEWS_PROJ_DIR/experiments/detect_events";


# Run the daily-build experiment, and send an email reporting the outcome.
$PERL "$BUILD_SERIF" --sge --name "$EXPT_NAME" \
    --return-success-after-reporting-failure \
    $BUILD_OPTS \
    --mailto "$MAILTO" --mailfrom "$REPLYTO" >> "$BUILD_LOG" 2>&1 \
    || die "Error while running the build serif regression test"

# insure that no symbolic links will be created when expt runs

echo "build dir for ICEWS reg test is $BUILD_ICEWS_DIR"
mkdir -p "$BUILD_ICEWS_DIR/ckpts"
mkdir -p "$BUILD_ICEWS_DIR/etemplates"
mkdir -p "$BUILD_ICEWS_DIR/expts"
mkdir -p "$BUILD_ICEWS_DIR/logfiles"
LINUX_RUN="ok"
echo "calling the ICEWS Unix regtest version of detect_events"
$PERL "$BUILD_ICEWS_DIR/sequences/detect_events.pl" -sge --reg_test --compare=true --linux >> "$BUILD_LOG" 2>&1 \
	||  LINUX_RUN="Error while running icews linux regression test version of detect_events"
echo "calling the ICEWS Windows regtest"
WIN_RUN="ok"
$PERL "$BUILD_ICEWS_DIR/sequences/detect_events.pl" -sge --reg_test --compare=true --nolinux >> "$BUILD_LOG" 2>&1 \
	||  WIN_RUN="Error while running icews windows regression test"


# Make sure that the new daily build is writable in case someone
# else needs to delete it or do something with it.
chmod a+w -R "$BUILD_DIR" >> "$BUILD_LOG" 2>&1 \
    || die "Error while setting permissions"

if [ "$LINUX_RUN" == "ok" ]; then
	echo "Linux run succeeded"
else
	echo "$LINUX_RUN"
	die "$LINUX_RUN"
fi

if [ "$WIN_RUN" == "ok" ]; then
	echo "Windows run succeeded"
else
	echo "$WIN_RUN"
	die "$WIN_RUN"
fi


rotate_log $BUILD_ROOT/cron.log

succeed "ICEWS runs and comparisons succeeded"

