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

############################################################################
# Runjobs Control Variables
############################################################################
our $QUEUE_PRIO = '5' unless defined($QUEUE_PRIO);
our $SEPERATE_ERR_LOGGING = 0;
our $SGE_VIRTUAL_FREE = '2G';
#our $BATCH_QUEUE = 'gale64b8G+nongale';

# $exp_dir is the directory above sequences.
# $exp is the experiment number, which we don't use.
our ($exp_dir, $exp) = startjobs();

############################################################################
# Scripts and Binaries
############################################################################

my $bin_dir = "$exp_dir/bin";
my $megam = "$bin_dir/megam_i686.opt";
my $python = "/opt/Python-2.5.2-x86_64/bin/python";
my $serif_trainer = "$exp_dir/bin/MaxEntRelationTrainer.exe";

############################################################################
# Directories & Data Files
############################################################################

my $train_vector_file = "$exp_dir/expts/train.vector";
my $test_vector_file = "$exp_dir/expts/test.vector";
my $megam_output_file = "$exp_dir/expts/megam_weights.txt";
my $serif_data = "//traid04/u1/projects/Serif-data/data";
my $serif_output_dir = "$exp_dir/expts/serif-train-rel";

############################################################################
# Configuration Vars
############################################################################

my $held_out_percent = 20;

############################################################################
# Main Script
############################################################################
my @jobs = ();

*unix2win = \&File::PathConvert::unix2win;

push @jobs, runjobs(
    \@jobs, "train-rel-mkdir", {SCRIPT=>1},
    ["mkdir -p $serif_output_dir" ]);

push @jobs, runjobs(
    \@jobs, "train-rel-serif", 
    {output_dir => unix2win($serif_output_dir),
     train_vector_file => unix2win($train_vector_file),
     test_vector_file => unix2win($test_vector_file),
     serif_data => unix2win($serif_data),
     held_out_percent => $held_out_percent,
     PATH_CONVERT => \&File::PathConvert::unix2win,
     BATCH_QUEUE => "win64"},
    [unix2win($serif_trainer), "train_maxent_rel.par"]);

push @jobs, runjobs(
    \@jobs, "train-rel-megam", 
    {train_vector_file => $train_vector_file,
     test_vector_file => $test_vector_file,
     output_file => $megam_output_file,
     megam => $megam,
     BATCH_QUEUE => 'gale64b8G+nongale'},
    ["$python", "train_rel.py"]);

endjobs();

############################################################################
# Successful import!
############################################################################
1;
