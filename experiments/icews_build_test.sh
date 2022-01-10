#!/bin/bash

set -e # Exit with an error immediately if any simple command fails.
set -u # Treat unset variables as an error.
#set -x # Print commands as we run them (useful for debugging)


###########################################################################
# Command-line argument processing
###########################################################################
echo "args are optional <build root> <pre-built Serif> <path to compare data>"

if [ $# -eq 0 ]; then 
	export BUILD_ROOT="."; 
else 
	export BUILD_ROOT="$1"; 
fi
echo "building at base $BUILD_ROOT"

if [ $# -ge 2 ]; then 
	export BUILT_SERIF_PATH="$2"; 
	echo "using pre-built Serif at $BUILT_SERIF_PATH"
else 
	export BUILT_SERIF_PATH=""; 
	echo "building Win32 and x86_64 Serif from SVN"
fi

if [ $# -ge 3 ]; then 
	export ICEWS_DATA="$3"; 
	echo "using comparison results at $ICEWS_DATA"
	export ICEWS_DATA_OPT="--icews-data $ICEWS_DATA"
else 
	export ICEWS_DATA=""; 
    export ICEWS_DATA_OPT=""
	echo "using stored comparison answers on /d4m/serif/icews_data"
fi
export EXPT_NAME="ICEWS Reg_Test"
export BUILD_PREFIX="ICEWS_Daily"
export BUILD_OPTS="--build-only --build=Win32+x86_64/English/Best/Release/ICEWS"


###########################################################################
# Configuration
###########################################################################

# The email address that a report should be sent to.
export MAILTO="${USER}@bbn.com"
export REPLYTO="${USER}@bbn.com"

# Output files

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
export SEND_EMAIL="$PERL /d4m/serif/bin/send_email.pl"

###########################################################################
# Error reporting
###########################################################################
# This function is used to report errors in the regression test script 
# itself (as opposed to errors in SERIF).
function die () {
    echo "Error in ICEWS Reg Test Script: $*"
    echo "  Sending email to $MAILTO..."
    (
	echo "Error in ICEWS Reg Test: $*";
	echo " Build log: $BUILD_LOG";
	echo "Delete log: $DELETE_LOG";
        echo "-------------------- BUILD LOG --------------------";
	[ -e $BUILD_LOG ] && cat $BUILD_LOG || echo "Build Log File not found"
    ) | $SEND_EMAIL --to "$MAILTO" --from $REPLYTO \
                    --subject "ICEWS Reg Test Error" 
    chmod a+w -R "$BUILD_DIR"
    exit 1
}

###########################################################################
# Success reporting
###########################################################################
# This function is used to report overall success in the regression test script 
# itself (as opposed to errors in the SERIF experiment).
function succeed () {
    echo "Successful ICEWS Reg Test: $*"
    echo "  Sending email to $MAILTO..."
    (
	echo "Successful ICEWS Reg Test: $*";
	echo " Build log: $BUILD_LOG";
	echo "Delete log: $DELETE_LOG";
        echo "-------------------- BUILD LOG --------------------";
	[ -e $BUILD_LOG ] && cat $BUILD_LOG || echo "Build Log File not found"
    ) | $SEND_EMAIL --to "$MAILTO" --from $REPLYTO \
                    --subject "ICEWS Reg Test Success" 
    chmod a+w -R "$BUILD_DIR"
}

date

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



# insure that no symbolic links will be created when expt runs

echo "build dir for ICEWS reg test is $BUILD_ICEWS_DIR"
mkdir -p "$BUILD_ICEWS_DIR/ckpts"
mkdir -p "$BUILD_ICEWS_DIR/etemplates"
mkdir -p "$BUILD_ICEWS_DIR/expts"
mkdir -p "$BUILD_ICEWS_DIR/logfiles"

if [ -z "$BUILT_SERIF_PATH" ]
then
    # Run the daily-build experiment, and send an email reporting the outcome.
	$PERL "$BUILD_SERIF" --sge --name "$EXPT_NAME" \
		--return-success-after-reporting-failure \
		$BUILD_OPTS \
		--mailto "$MAILTO" --mailfrom "$REPLYTO" >> "$BUILD_LOG" 2>&1 \
		|| die "Error while running the build serif regression test"
	echo "calling the ICEWS Unix regtest version of detect_events"
	$PERL "$BUILD_ICEWS_DIR/sequences/detect_events.pl" -sge --reg_test "$ICEWS_DATA_OPT" >> "$BUILD_LOG" 2>&1 \
		||  die "Error while running icews linux regression test"
	echo "calling the ICEWS Windows regtest version of detect_events"
	$PERL "$BUILD_ICEWS_DIR/sequences/detect_events.pl" -sge --reg_test --nolinux "$ICEWS_DATA_OPT" >> "$BUILD_LOG" 2>&1 \
		||  die "Error while running icews windows regression test"
else
	if [[ "$BUILT_SERIF_PATH" == *.exe ]]
	then
		export LINUX_FLAG="--nolinux"
	else
		export LINUX_FLAG=""
	echo "calling the ICEWS regtest version of detect_events with prebuilt Serif"
	$PERL "$BUILD_ICEWS_DIR/sequences/detect_events.pl" -sge "$LINUX_FLAG $ICEWS_DATA_OPT" --reg_test --serif-bin "$BUILT_SERIF_PATH">> "$BUILD_LOG" 2>&1 \
		||  die "Error while running icews prebuilt Serif regression test"
	fi
fi

succeed "ICEWS runs and comparisons succeeded"

