#!/bin/env perl
#
# SERIF Profiling Experiment
# Copyright (C) 2012 BBN Technologies
# 

use strict;
use warnings;

# Standard libraries:
use Getopt::Long;

# Runjobs libraries:
use lib ("/d4m/ears/releases/Cube2/R2011_09_19/".
         "install-optimize$ENV{ARCH_SUFFIX}/perl_lib");
use runjobs4;
use File::Path qw(mkpath);

# Package declaration:
package main;

if (!defined($ENV{SGE_ROOT})) {
    print "WARNING: SGE_ROOT not defined; using a default value!\n";
    $ENV{SGE_ROOT} = "/opt/sge-6.2u5";
}

my %CACHE_CONFIGS = (
    "16k-16k-2mb" => # eg d213
    " --I1=16384,4,64 --D1=16384,8,64 --LL=2097152,8,64 ",
    "32k-32k-8mb" => # eg ?
    " --I1=32768,8,64 --D1=32768,8,64 --LL=8388608,16,64 ",
    "32k-32k-16mb" => # eg ?
    " --I1=32768,4,64 --D1=32768,8,64 --LL=16777216,16,64 ", 
    # Valgrind doesn't currently support these, since the cache
    # sizes are not powers of two:
    "32k-32k-6mb" => # eg h219 (used for throughput test)
    " --I1=32768,8,64 --D1=32768,8,64 --LL=6291456,24,64 ",
    "32k-32k-12mb" => # eg m303
    " --I1=32768,4,64 --D1=32768,8,64 --LL=12582912,16,64 ", 
    );
############################################################################
# Command-line argument Processing
############################################################################
# (must come before the call to startjobs().)

my $VALGRIND = "/opt/valgrind-3.6.1-x86_64/bin/valgrind";
my $CG_MERGE = "/opt/valgrind-3.6.1-x86_64/bin/cg_merge";
my $BASH = "/bin/bash";
my $SERIF_BIN;
my $PAR_DIR;
#my $BATCH_FILE = "/d4m/serif/Ace/data-2005-v4.0-Linux/English/all-regression/source-files.txt";
my $BATCH_FILE = "/d4m/serif/Ace/data-2005-v4.0-Linux/English/adj-1/source-files.txt";
#my $BATCH_FILE = "/home/eloper/code/text/Projects/SERIF/experiments/profile_serif/sequences/quick-source-files.txt";

my $PAR = "throughput.best-english.par";
my $QUEUE = "nongale-x86_64";
my $SERIF_DATA = "/d4m/serif/data";
my $OUTPUT_FORMAT = "serifxml";
my $NAME = "profile";
my $END_STAGE = "output";
my $SERIF_REGRESSION = "/d4m/serif/Ace";
my %TOOL_OPTIONS = ("callgrind" => "", "massif" => "");

# List of valgrind tools that should be used for profiling.
my @TOOLS = ();

# Process our arguments; pass the rest through to runjobs.
Getopt::Long::Configure("pass_through");
GetOptions(
    "serif-bin=s"        => \$SERIF_BIN,
    "batch-file=s"       => \$BATCH_FILE,
    "par=s"              => \$PAR,
    "queue=s"            => \$QUEUE,
    "data=s"             => \$SERIF_DATA,
    "output_format=s"    => \$OUTPUT_FORMAT,
    "par-dir=s"          => \$PAR_DIR,
    "name=s"             => \$NAME,
    "end=s"              => \$END_STAGE,
    "callgrind"          => sub {push @TOOLS, "callgrind"},
    "massif"             => sub {push @TOOLS, "massif"},
    "cache-sim"          => sub {$TOOL_OPTIONS{"callgrind"} .= 
				     " --cache-sim=yes "},
    "cache-config=s"     => sub {$TOOL_OPTIONS{"callgrind"} .= 
				     $CACHE_CONFIGS{$_[1]}},
    );
Getopt::Long::Configure("no_pass_through");
die if $Getopt::Long::error;
die "Expected --serif-bin" if !defined($SERIF_BIN);
die "Expected --batch-file" if !defined($BATCH_FILE);
die "name may not contain '/'." if ($NAME =~ /\//);

if (!@TOOLS) {
    die "Specify one or more tools (-callgrind or -massif)"
}

our $QUEUE_PRIO = '5'; # Default queue priority
our ($exp_root, $exp) = startjobs();

if (!defined($PAR_DIR)) {
    $PAR_DIR = "$exp_root/../../par";
    die "Expected --par-dir" if (! -e "$PAR_DIR/master.english.par");
}

my $PAR_FILE;
if ($PAR =~ m/^\/(.*)\/([^\/]+)/) {
    $PAR_FILE = $PAR;
    $PAR = $2;
} else {
    $PAR_FILE = "$PAR_DIR/$PAR";
}

#print "[$PAR_FILE]\n";
#print "[$PAR]\n";
#exit 1;

# First, we run a trivial job that's just used to expand the parameter
# file.  In particular, this will generate a new file whose name is
# "$etemplate_prefix.$PAR", containing the expanded version of the 
# parameter file that was specified on the command line.
my $etemplate_prefix="$exp_root/etemplates/" .
    "${NAME}/profile_serif-prepare-param-file";
my $prepare_jobid = runjobs(
    [], "${NAME}/prepare-param-file", {
	SCRIPT => 1,
	ARCH_SUFFIX => "-x86_64",
	parallel => "000",
	serif_data => $SERIF_DATA,
	serif_regression => $SERIF_REGRESSION,
        os_suffix => "-Linux",
	serif_score => "not_used",
	start_stage => "START",
	end_stage => $END_STAGE,
	output_format => $OUTPUT_FORMAT,
	par_dir => $PAR_DIR,
	state_saver_stages => "NONE",
	"batch_file" => "$BATCH_FILE",
	"expt_dir" => 
	    "$exp_root/expts/result"},
    "true", $PAR_FILE);

# Note: there's no real need to do multiple runs for each tool, since
# valgrind uses a virtual machine to isolate the profiled process from
# whatever else is running on the machine.
foreach my $tool (@TOOLS) {
    runjobs(
	[$prepare_jobid], "${NAME}/$tool", {
	    BATCH_QUEUE => $QUEUE,
	    SGE_VIRTUAL_FREE => "4G",
	    ARCH_SUFFIX => "-x86_64",
	    VALGRIND => $VALGRIND,
	    CG_MERGE => $CG_MERGE,
	    SERIF_BIN => $SERIF_BIN,
	    OPTIONS => ($TOOL_OPTIONS{$tool} || ""),
	    PAR_FILE => "$etemplate_prefix.$PAR",
	    OUT_DIR => "$exp_root/expts/${NAME}/$tool.serif-output",
	    LOG_FILE => "$exp_root/expts/${NAME}/$tool.log",
	    OUT_FILE => "$exp_root/expts/${NAME}/$tool.$NAME"},
	$BASH, "run_$tool.sh");
}

# Run jobs.
endjobs();


