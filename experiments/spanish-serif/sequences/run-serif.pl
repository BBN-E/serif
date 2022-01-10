#!/bin/env perl
#
# SERIF Run ExperimentBuild Experiment
# Copyright (C) 2015 BBN Technologies
# 
# Derived from the runjobs script 
# in Projects/SERIF/experiments/run-serif/sequences/run-serif.pl
# 
# Includes changes to force Scott Miller's metro parser to run mid pipeline
#
# June 2015 - Jay DeYoung
#
#

use strict;
use warnings;

# Standard libraries:
use Getopt::Long;
use File::Basename;
use File::Path;

# Runjobs libraries:
use lib ("/d4m/ears/releases/Cube2/R2015_06_16/".
         "install-optimize$ENV{ARCH_SUFFIX}/perl_lib");
use runjobs4;
use File::PathConvert;
# from bue-text git repo; clone from /d4m/ears/git/buetext.git and add the perl-modules directory to your PERL5LIB env
use Parameters; 
use RunJobsUtils;

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
my $QUEUE = "nongale-x86_64";
my $SERIF_DATA = "/d4m/serif/data";
my $SGE_VIRTUAL_FREE = "6G";
my $PARSER_PARAMS_FILE;
my $OUTPUT_FORMAT="serifxml";
my $SOURCE_FORMAT="sgm";
my $INPUT_TYPE="sgm";
my $SEXP="FOR_CASERIF_ONLY";

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
    "parser-params=s"    => \$PARSER_PARAMS_FILE,
    "source-format=s"    => \$SOURCE_FORMAT,
    "sexp=s"             => \$SEXP,
    "input-type=s"       => \$INPUT_TYPE,
    );
Getopt::Long::Configure("no_pass_through");
die if $Getopt::Long::error;
die "Expected --serif-bin" if !defined($SERIF_BIN);
die "Expected --batch-file" if !defined($BATCH_FILE);
die "Expected --parser-params" if !defined($PARSER_PARAMS_FILE);



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
    $NUM_JOBS = int($num_files / $BATCH_SIZE) == ($num_files / $BATCH_SIZE) ? ($num_files / $BATCH_SIZE) : int($num_files / $BATCH_SIZE) + 1;
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
    die "Expected --par-dir" if (! -e "$PAR_DIR/master.spanish.par");
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
#my $params = Parameters::loadAndPrint($PAR);
#$params->set('parallel', '000');
#$params->set('par_dir',  &$path_convert($PAR_DIR));
my $metro_params = Parameters::loadAndPrint($PARSER_PARAMS_FILE);
my $metro_dir = $metro_params->get("metroDir");
for (my $n=0; $n<$NUM_JOBS; $n++) {
    my $job_batch_num = sprintf("%04d", $n);
    my $job_name = "$JOB_NAME/run-serif-$n";
    my $preparse_job_name = "$JOB_NAME/run-serif-$n-preparse";
    my $start_expt_dir =  &$path_convert("$exp_root/expts/$JOB_NAME/output/batch_$job_batch_num");
    my $base_batch_file = &$path_convert("$exp_root/expts/$JOB_NAME/batch_files/batch_file_$job_batch_num");
    File::Path::mkpath($start_expt_dir);
    
    my $job = runjobs(
	\@prepare_jobids, $preparse_job_name, 
        {
	    BATCH_QUEUE => $QUEUE,
	    SGE_VIRTUAL_FREE => $SGE_VIRTUAL_FREE,
	    #ARCH_SUFFIX => "-x86_64", ## is this needed? it's not when we're on Windows...
	    parallel => "000",
	    #serif_score => "not_used",
            start_stage => 'START',
            end_stage => 'values',
	    #state_saver_stages => "NONE",
	    output_format => "serifxml",
            source_format => "$SOURCE_FORMAT",
	    input_type => "$INPUT_TYPE",
	    par_dir => &$path_convert($PAR_DIR),
	    serif_data => &$path_convert($SERIF_DATA),
            batch_file => "$base_batch_file",
            key_dir =>  &$path_convert("$exp_root/expts/$JOB_NAME/batch_files/"),
            expt_dir =>  &$path_convert($start_expt_dir),
            sexp => $SEXP,
	},
	&$path_convert($SERIF_BIN), "$PAR");
    push @run_jobids, $job;

    # find and list all the files that were output by start -> values task
    my $pre_parse_output_list = "$base_batch_file.preparse.files.xml.list";
    File::Path::mkpath(File::Basename::dirname($pre_parse_output_list));
    my $find_pre_parsed_files_job = RunJobsUtils::listFilesInDirectory($job, $job_name . '/find_preparsed_files' , system("pwd"), $start_expt_dir . '/output/', "*.xml", $pre_parse_output_list);
    push @run_jobids, $find_pre_parsed_files_job;

    # run metro parser with the find job as dep.
    # TODO change this to use the templating system
    my $this_metro_params = $metro_params->copy();
    # inputFile is a fileList
    $this_metro_params->set('inputFile', $pre_parse_output_list);
    # outputFile is a directory
    $this_metro_params->set('outputFile',  &$path_convert("$exp_root/expts/$JOB_NAME/output-parse/batch_$job_batch_num/"));
    $this_metro_params->set('outputDocIDtoFileMap',  &$path_convert("$exp_root/expts/$JOB_NAME/output-parse/batch_$job_batch_num.docIDToFileMap"));
    File::Path::mkpath(File::Basename::dirname($this_metro_params->get("outputFile")));
    my $metro_job_name = $job_name . "-metro-parse";
    my $metro_job = runjobs($find_pre_parsed_files_job, $metro_job_name, { BATCH_QUEUE => $QUEUE, SGE_VIRTUAL_FREE => "16G", job_cores => $this_metro_params->get("num_threads"), GENERATED_PARAMS => $this_metro_params->dump() } ,  ["echo \$SHELL \$HOSTNAME \$JAVA_HOME; which java ; $metro_dir/metro-parser/target/appassembler/bin/parseSerif", "generated.par"]);
    push @run_jobids, $metro_job_name;

    # find and list all the files that were output by this task
    my $post_parse_output_list = "$base_batch_file.postparse.files.xml.list";
    my $find_post_parsed_files_job = RunJobsUtils::listFilesInDirectory($metro_job,  $job_name . "/find_post_parsed_files_" , system("pwd"), $this_metro_params->get("outputFile"), "*.xml", $post_parse_output_list);
    push @run_jobids, $find_post_parsed_files_job;

    # run the rest of serif with metro as dep
    my $end_expt_dir = &$path_convert("$exp_root/expts/$JOB_NAME/output-post-parse/batch_$job_batch_num");
    my $post_parse_name = $job_name . "-post_parse";
    
    File::Path::mkpath(&$path_convert($end_expt_dir . '/' . 'output'));
    my $post_parse_job = runjobs(
	$find_post_parsed_files_job, $post_parse_name,  
        {
	    BATCH_QUEUE => $QUEUE,
	    SGE_VIRTUAL_FREE => $SGE_VIRTUAL_FREE,
	    #ARCH_SUFFIX => "-x86_64", ## is this needed? it's not when we're on Windows...
	    parallel => "000",
            start_stage => 'mentions',
            end_stage => 'output',
	    #serif_score => "not_used",
	    #state_saver_stages => "NONE",
	    output_format => "$OUTPUT_FORMAT",
	    input_type => "$INPUT_TYPE",
            source_format => "serifxml",
	    par_dir => &$path_convert($PAR_DIR),
	    serif_data => &$path_convert($SERIF_DATA),
            batch_file => $post_parse_output_list,
            key_dir =>  &$path_convert("$exp_root/expts/$JOB_NAME/batch_files/"),
            expt_dir => $end_expt_dir,
            sexp => $SEXP,
	},
	&$path_convert($SERIF_BIN), "$PAR");
    push @run_jobids, $post_parse_job;

    # find and list all the files that were output by the final stage
    my $final_output_list = &$path_convert("$exp_root/expts/$JOB_NAME/batch_files/batch_file_$job_batch_num.final.xml.list");
    my $find_complete_output_job = RunJobsUtils::listFilesInDirectory($post_parse_job, $job_name . "/find_finished_files"  , system("pwd"), $end_expt_dir, "*.xml", $post_parse_output_list);
    push @run_jobids, $find_complete_output_job;
}
endjobs();
