#!/bin/env perl
#
# SERIF Run ExperimentBuild Experiment
# Copyright (C) 2012 BBN Technologies
#

use strict;
use warnings;

# Standard libraries:
use Getopt::Long;
use File::Basename;

# Runjobs libraries:
use lib "/d4m/ears/releases/Cube2/R2016_07_21/install-optimize-x86_64/perl_lib";
use runjobs4;
use File::PathConvert;

# Package declaration:
package main;

if (!defined($ENV{SGE_ROOT})) {
    print "WARNING: SGE_ROOT not defined; using a default value!\n";
    $ENV{SGE_ROOT} = "/opt/sge-6.2u5";
}

############################################################################
# Command-line argument Processing
############################################################################
# (must come before the call to startjobs().)

my $BASH = "/bin/bash";
my $VERBOSE = 0;
my $SERIF_BIN;
my $JOB_NAME;
my $BATCH_FILE;
my $PAR_DIR;
my $PAR = "all.best-english.par";
my $BATCH_SIZE;
my $NUM_JOBS;
my $QUEUE = "nongale-sl6";
my $SERIF_DATA = "/d4m/serif/data";
my $OUTPUT_FORMAT = "serifxml";
my $SGE_VIRTUAL_FREE = "4G";

# Process our arguments; pass the rest through to runjobs.
Getopt::Long::Configure("pass_through");
GetOptions(
    "verbose"            => sub {++$VERBOSE},
    "serif-bin=s"        => \$SERIF_BIN,
    "batch-file=s"       => \$BATCH_FILE,
    "par=s"              => \$PAR,
    "num-jobs=i"         => \$NUM_JOBS,
    "queue=s"            => \$QUEUE,
    "data=s"             => \$SERIF_DATA,
    "output_format=s"    => \$OUTPUT_FORMAT,
    "par-dir=s"          => \$PAR_DIR,
    "job-name=s"         => \$JOB_NAME,
    "batch-size=i"       => \$BATCH_SIZE,
    "memory=s"           => \$SGE_VIRTUAL_FREE,
    );
Getopt::Long::Configure("no_pass_through");
die if $Getopt::Long::error;
die "Expected --serif-bin" if !defined($SERIF_BIN);
die "Expected --batch-file" if !defined($BATCH_FILE);

# We will either run a specified number of jobs OR
#   calculate the number of jobs based on our desired batch size
if (defined($NUM_JOBS) && defined($BATCH_SIZE)) {
    die "Can only specify one of --num-jobs or --batch-size";
}

# If no job name specified, use the basename of our batch file
if (!defined($JOB_NAME)) {
    $JOB_NAME = basename($BATCH_FILE);
}

if (!defined($NUM_JOBS)) {
    # calculate the number of jobs to run based on desired batch size (default = 100)
    if (!defined($BATCH_SIZE)) {
	$BATCH_SIZE = 100;
    }
    open my $fh, "<", $BATCH_FILE or die "could not open $BATCH_FILE: $!";
    my $num_files = 0;
    $num_files++ while <$fh>;
    $NUM_JOBS = int($num_files / $BATCH_SIZE) + 1;
    if ($NUM_JOBS > 10000) {
	die "\n  I'm sorry, this script assumes that you want to run fewer than 10000 jobs.\n  At this batch size ($BATCH_SIZE), there will be $NUM_JOBS.\n  Please adjust your batch size (--batch-size N) and try again.\n\n";
    }
    print "Batch file will be broken into $NUM_JOBS batches of approximate size $BATCH_SIZE\n";
} else {
    if ($NUM_JOBS > 10000) {
	die "\n  I'm sorry, this script assumes that you want to run fewer than 10000 jobs.\n  Please adjust your number of jobs (--num-jobs N) and try again.\n\n";
    }
    print "Batch file will be broken into $NUM_JOBS batches, as requested\n";
}

our $QUEUE_PRIO = '5'; # Default queue priority
our ($exp_root, $exp) = startjobs();

my $path_convert = (sub {shift;});
if (defined $QUEUE && $QUEUE =~ /(^|@)(win|wm)/) {
    $path_convert = \&File::PathConvert::unix2win;
}

if (!defined($PAR_DIR)) {
    $PAR_DIR = "$exp_root/../../par";
    die "Expected --par-dir" if (! -e "$PAR_DIR/master.english.par");
}

my @prepare_jobids = ();

push @prepare_jobids, runjobs(
    [], "$JOB_NAME/make_batch_files", {
	SCRIPT => 1, # (don't spawn a job)
	BATCH_FILE => $BATCH_FILE,
	JOB_NAME => $JOB_NAME,
	NUM_JOBS => $NUM_JOBS,
	EXPT_DIR => "$exp_root/expts"},
    $BASH, "make_batch_files.sh");


max_jobs($JOB_NAME => 400,); # don't overrun the queue
my @run_jobids = ();
for (my $n=0; $n<$NUM_JOBS; $n++) {
    my $job_batch_num = sprintf("%04d", $n);
    my $job_name = "$JOB_NAME/run-serif-$n";
    push @run_jobids, runjobs(
	\@prepare_jobids, $job_name, {
	    BATCH_QUEUE => $QUEUE,
	    SGE_VIRTUAL_FREE => $SGE_VIRTUAL_FREE,
	    ARCH_SUFFIX => "-x86_64", ## is this needed? it's not when we're on Windows...
	    parallel => "000",
	    serif_score => "not_used",
	    start_stage => "START",
	    end_stage => "output",
	    state_saver_stages => "NONE",
	    output_format => $OUTPUT_FORMAT,
	    par_dir => &$path_convert($PAR_DIR),
	    serif_data => &$path_convert($SERIF_DATA),
	    batch_file => &$path_convert("$exp_root/expts/$JOB_NAME/batch_files/batch_file_$job_batch_num"),
	    expt_dir => &$path_convert("$exp_root/expts/$JOB_NAME/output/batch_$job_batch_num")
	},
	&$path_convert($SERIF_BIN), "$PAR");
}
endjobs();
