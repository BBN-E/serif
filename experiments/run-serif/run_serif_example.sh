#!/bin/bash

# 2018-09-29, John Greve <jgreve@bbn.com>
#
# An example bash script that runs serif against a folder of *.txt files.
# (Thank you Alex Z for filling in the details.)
#
# prereqs:
#   1) A serif executable (see SERIF_BIN below)
#   2) Some text files (see RAW_TEXT_DIR below)
#   3) Decide if you want 1 file for testing of all files (search on "-quit")
#   4) A copy of the serif experiments/ subdirectory, available from subversion as follows:
#   +------------------------------------------------------------------
#   |  $ export PATH=/opt/subversion-1.8.13-x86_64/bin:$PATH # need svn 1.7+ for --parents option.
#   |  $ TEXT_PROJ=svn+ssh://${USER}@svn.d4m.bbn.com/export/svn-repository/text/trunk
#   |  $ mkdir test_dir
#   |  $ cd test_dir
#   |  $ svn co --depth=empty $TEXT_PROJ TextGroup
#   |  $ svn up --parents --depth=immediates TextGroup/Active/Projects/SERIF/experiments/run-serif/sequences
#   |  $ svn up --parents --depth=immediates TextGroup/Active/Projects/SERIF/experiments/run-serif/templates
#   |  $ cd TextGroup/Active/Projects/SERIF/experiments/run-serif
#   |  $ export PS1='$ ' # optional, sets simple dollar sign prompt
#   |  $ ./run_serif_example.sh
#   +------------------------------------------------------------------

set -u
set -e
set -o pipefail

# set up sungrid env-vars
. sungrid.env # source sungrid environment vars
# TODO: is there a system-wide script that sets these up?
# Or... does everybody have one-offs hardwired into their ~/.bash_profile ?

TEST_NAME=serif_example
JOB_NAME=${TEST_NAME}_01 # edit this for successive runs.
JOB_MEMORY=8G
JOB_QUEUE=nongale-sl6-nocstars
echo "Hello from $0"
echo "$0: TEST_NAME=$JOB_NAME"
echo "$0: JOB_NAME=$JOB_NAME"
echo "$0: JOB_MEMORY=$JOB_MEMORY"
echo "$0: JOB_QUEUE=$JOB_QUEUE"

# RAW_TEXT_DIR is where we get our input *.txt files from that we will feed to Serif.
RAW_TEXT_DIR=/nfs/raid66/u15/aida/evals/m9_eval.preprocessed/txt.split_by_lang/eng
echo "$0: RAW_TEXT_DIR=$RAW_TEXT_DIR"
BATCH_NAME=$TEST_NAME
echo "$0: BATCH_NAME=$BATCH_NAME"
BATCH_FILE=$BATCH_NAME.batch # list of files for to Serif to process.

#find "$RAW_TEXT_DIR" -type f -print       > $BATCH_FILE # use all availble files
find "$RAW_TEXT_DIR" -type f -print -quit > $BATCH_FILE # For debugging (-quit stops after 1st file)
#------------------------------------^^^^
# For debugging, note the above -quit flag, which stops after first find result.
# The idea is to run a single file smoke test before launching a multi-thousand run.
# Note: when you do relaunch with all files, be sure to increment the JOB_NAME
# or clear the checkpoints. As per Alex Z (via email, 2018-09-28):
#   One other thing to be aware of -- the job name of the serif run (jgtest.batch/run-serif-0)
#   is based off of the --batch-file name. Say you get it to successfully run through.
#   And then you try running again with the same batch file. Runjobs will know that the job
#   jgtest.batch/run-serif-0 has already been completed, so it won't try to run it again,
#   it will just report that the job is done.
#   The information on what jobs have been successfully run through is stored in your
#   ckpts folder: CHECKPOINTS=$RAID_USER_DIR/runjobs/ckpts
#   So if you wanted to run on a new batch you would need to use a new batch file name, or alternatively,
#   you can set the job name from the parameters of run-serif.pl. E.g. "--job-name new-job-01"
#-----------------------------------------------------------------------------------------------------

echo "$0: BATCH_FILE=$BATCH_FILE has $(cat $BATCH_FILE | wc -l) files from RAW_TEXT_DIR."

# This example uses a version of serif built for Accent
ACCENT_HOME=/nfs/raid66/u12/users/rbock/aida/ACCENT/bbn-accent-release/installation
SERIF_BIN=$ACCENT_HOME/bin/BBN_ACCENT
echo "$0: SERIF_BIN=$SERIF_BIN"

SHARED_PAR_DIR=$ACCENT_HOME/par  # General default parameter directory
echo "$0: SHARED_PAR_DIR=$SHARED_PAR_DIR"
PAR_FILE=accent.text.par  # Set up an Accent-aware param file
mkdir -p templates # probably not necessary, should have come from subversion.
echo "$0: PAR_FILE=templates/$PAR_FILE"
#--------------------------------------------------------------
# PAR_FILE's location seems tricky, "run-serif.pl" wants PAR_FILE
# to be in the templates/ subdirectory but we can't include
# "templates/" in the actual name of PAR_FILE or "run-serif.pl"
# will complain about "templates/templates/accent.text.par not found."
# So we hardwire "templates/" into the CAT command on the following
# here-doc construction.
#--------------------------------------------------------------
#--- begin create PAR_FILE ---
# *** note we create PAR_FILE in templates/ ***
cat > templates/$PAR_FILE << __END_PAR_FILE__
# note: PAR_FILE copied 2018-09-28 from /nfs/raid66/u14/users/azamania/temp/accent.text.par
# (Thank you, Alex Z.)
INCLUDE +par_dir+/master.accent.english.par

# List of files to run on
batch_file: +batch_file+

# Output directory of CAMEO XML
experiment_dir: +expt_dir+

# Input data format
input_type: text

# Used for text input_type
use_filename_as_docid: true

OVERRIDE output_format: +output_format+
__END_PAR_FILE__
#--- end create PAR_FILE ---

# note: PERL env-var set in sungrid.env
$PERL sequences/run-serif.pl \
   -sge \
   --batch-file $BATCH_FILE \
   --serif-bin  $SERIF_BIN \
   --par        $PAR_FILE \
   --par-dir    $SHARED_PAR_DIR \
   --job-name   $JOB_NAME \
   --memory     $JOB_MEMORY \
   --queue      $JOB_QUEUE
echo "$0: Done."
