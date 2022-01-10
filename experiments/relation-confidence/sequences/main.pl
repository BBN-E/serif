#!/bin/env perl
#
# Relation Confidence
#

############################################################################
# Preliminaries
############################################################################
use strict;
use warnings;
use lib ("/d4m/ears/releases/Cube2/R2010_01_29/".
         "install-optimize$ENV{ARCH_SUFFIX}/perl_lib");
use runjobs4;
package main;

my $num_pipeline_jobs = 2; # this is per adj split -- i.e., multiply by 8.
my $language = "English";

############################################################################
# Runjobs Control Variables
############################################################################
# Note: we use 'our' rather than 'my' to export these variables so 
# runjobs can check them.

# Integral queue priority (0 lowest, 9 highest).
our $QUEUE_PRIO = '5' unless defined($QUEUE_PRIO);

# Use a single combined logfile & error file.
our $SEPERATE_ERR_LOGGING = 0;

# Approximate memory usage for jobs spawned by this experiment.  
# This will be used to limit the number of jobs that can be 
# started on a given machine.  It is not a hard limit on the
# memory usage of jobs (though 32 bit processes can only use up
# to 2gb memory max); but if total memory runs out, everyone 
# gets hurt.
our $SGE_VIRTUAL_FREE = '2G';

# Which queue should we use to run jobs?
our $BATCH_QUEUE = 'win64';

# Set up the path conversion function, which is used to convert unix
# paths to windows paths when we run jobs on windows machines.
our $PATH_CONVERT = \&File::PathConvert::unix2win;
*path_convert = \&File::PathConvert::unix2win;

# $exp_dir is the directory above sequences.
# $exp is the experiment number, which we don't use.
our ($exp_dir, $exp) = startjobs();

############################################################################
# Scripts and Binaries
############################################################################

my $bin_dir = "$exp_dir/bin";
my $run_pipeline_py = "$bin_dir/Pipeline/Pipeline.py";
my $serif_bin = "$bin_dir/Serif${language}.exe";
my $sevenzip_bin = "$bin_dir/7z.exe";
my $serif_template = "$exp_dir/templates/split.best-english.par";
my $win_python = 'C:\\Python25\\python.exe';

############################################################################
# Directories & Data Files
############################################################################

my $output_dir = "$exp_dir/expts";
my $input_dir = "\\\\traid01\\speed";
# my $sgm_subdirs;
# if ($split eq "all") {
#     $sgm_subdirs = "Ace\\data-2005-v4.0\\English ".
# 	"Training-Data/English/bbn-ace2005-relation-training/serif-ready-data ".
# 	"Training-Data/English/bbn-ace2005-event-training/serif-ready-data";
# }

############################################################################
# Main Script
############################################################################

for (my $split_num=1; $split_num<=8; $split_num++) {
    my $split = "adj-$split_num";
    my $sgm_subdirs = "Ace\\data-2005-v4.0\\English\\$split";
    my $document_list = "$output_dir/serif-$split/document_list.txt";

    # Make a list of documents to be processed.
    my $doclist_jobid = runjobs(
	[], "$exp-doclist-$split", 
	{input_dir => $input_dir, 
	 sgm_subdirs => $sgm_subdirs,
	 document_list => path_convert($document_list),
	 BATCH_QUEUE => $BATCH_QUEUE, PATH_CONVERT => \&path_convert},
	["$win_python", "make_doclist.py"]);

    # Run all of the selected documents through the SERIF pipeline.
    for (my $job_num = 0; $job_num < $num_pipeline_jobs; $job_num++) {
	my $job_name = sprintf("$exp-$split-%03d", $job_num);
	runjobs([$doclist_jobid], $job_name,
		{
                    # Job-related parameters
		    expt_name => $exp,
		    num_jobs => $num_pipeline_jobs,
		    job_num => $job_num,

		    # Document names
		    document_lists => path_convert($document_list),

		    # Metadata
		    language => "\L$language",

		    serif_data => "//traid01/speed/Serif-data/data",
		    serif_score => "//traid01/speed/Serif/score",

		    # Which model should we use?  E.g., 'all' or 'adj-1.'.
		    split => $split,

		    # Files & Directories
		    input_dir => path_convert($input_dir),
		    output_dir => path_convert("$output_dir/serif-$split"),
		    serif_template => path_convert($serif_template),
		    lockdir => path_convert("$exp_dir/expts/locks"),

		    # Which pipeline stages to run
		    start_stage => "START",
		    end_stage => "output",

		    # Binaries
		    serif_bin => path_convert($serif_bin),
		    sevenzip_bin => path_convert($sevenzip_bin),

		    # Runjobs parameters
		    BATCH_QUEUE => $BATCH_QUEUE,
		    PATH_CONVERT => \&path_convert,
		},
		["$win_python " . path_convert($run_pipeline_py), 
		 "pipeline.par"]
	    );
    }
}
endjobs();

############################################################################
# Successful import!
############################################################################
1;
