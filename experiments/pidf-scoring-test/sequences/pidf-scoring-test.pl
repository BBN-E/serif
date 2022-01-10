#!/bin/env/perl
#
# PIdF Scoring Test Experiment
#

use strict;
use warnings;

# Runjobs libraries:
use lib ("/d4m/ears/releases/Cube2/R2010_11_21/".
         "install-optimize$ENV{ARCH_SUFFIX}/perl_lib");
use runjobs4;

use Getopt::Long;
use File::PathConvert;
use File::Path;

# Package declaration:
package main;


############################################################################
# Command-line argument Processing
############################################################################

my $LANGUAGE = 'English';
my $EXPT_NAME = 'PIdF-Scoring';

# Process our arguments; pass the rest through to runjobs.
Getopt::Long::Configure("pass_through");
GetOptions(
    "help"               => sub { experiment_usage() },
    "language=s"         => \$LANGUAGE,
    "name=s"             => \$EXPT_NAME,
);
Getopt::Long::Configure("no_pass_through");
die if $Getopt::Long::error;

if ($LANGUAGE ne 'English' && $LANGUAGE ne 'Arabic' && $LANGUAGE ne 'Chinese') {
    die "--language: Expected 'English', 'Arabic', or 'Chinese'";
}

sub experiment_usage {
    runjobs4::usage(); # <- this calls the runjobs usage script
     
    print qq(SERIF PIdF Scoring Test Options:
      --help            Print this help message and exit.
      --language LANGUAGE
                        'English', 'Arabic' or 'Chinese'. Default=$LANGUAGE.
      --name NAME       Experiment name. Default=$EXPT_NAME.\n);

    exit;
}

############################################################################
# Runjobs variables
############################################################################

our $QUEUE_PRIO = '5'; # Default queue priority
our ($exp_root, $exp) = startjobs();

#############################################################################
# Experiment Configuration
#############################################################################

#;---------------------------------------------------------------------------
#; EXPERIMENT DIRECTORIES
#;---------------------------------------------------------------------------

my $data_dir                = "$exp_root/data";
my $bin_dir                 = "$exp_root/bin";
my $expt_dir                = "$exp_root/expts";
my $output_dir              = "$expt_dir/$EXPT_NAME";

#;---------------------------------------------------------------------------
#; BINARIES & LIBRARIES
#;---------------------------------------------------------------------------

my $PIDF                    = "$bin_dir/PIdFTrainer_$LANGUAGE";
my $MUC_SCORER              = "$bin_dir/ACE-Scorer.exe";
my $PERL                    = "/opt/perl-5.8.6/bin/perl";

#;---------------------------------------------------------------------------
#; Job Queues
#;---------------------------------------------------------------------------

my $LINUX_QUEUE             = 'nongale';
my $WINDOWS_QUEUE           = 'win64';


#############################################################################
# Serif data files
#############################################################################

my $SERIF_DATA = "/d4m/serif/data";

my %FEATURE_FILES = (
    Arabic        => "${SERIF_DATA}/arabic/names/pidf/ace2007-with-lists.features",
    Chinese       => "${SERIF_DATA}/chinese/names/pidf/bi_and_tri_wc.features",
    English       => "${SERIF_DATA}/english/names/pidf/ace2011.features",
);

my %TAG_FILES = (
    Arabic        => "${SERIF_DATA}/arabic/names/pidf/7Types-names.txt",
    Chinese       => "${SERIF_DATA}/chinese/names/pidf/ace2004.tags",
    English       => "${SERIF_DATA}/english/names/pidf/ace2004.tags",

);

my %CLUSTERS = (
    Arabic        => "${SERIF_DATA}/arabic/clusters/cl2_tdt4_wl.hBits",
    Chinese       => "${SERIF_DATA}/chinese/clusters/giga9804-tdt4bn.hBits",
    English       => "${SERIF_DATA}/english/clusters/ace2005.hBits",
);

############################################################################
#                          Job Sequencing
############################################################################

mkpath($output_dir) unless -d $output_dir;

my @training_sizes = qw(25k 75k 175k 375k);
if ($LANGUAGE eq 'English') {
    push(@training_sizes, "2425k");
}

my @scoring_jobs = ();
foreach my $size (@training_sizes) {
    my $config = "${LANGUAGE}-${size}";
 
    my $train_file      = "$data_dir/$LANGUAGE/train-$size.sexp";
    my $test_file       = "$data_dir/$LANGUAGE/message-25k";
    my $key_file        = "$data_dir/$LANGUAGE/message-25k.key";

    my $model_name      = "$output_dir/$EXPT_NAME-$config";
    my $test_output     = "$output_dir/message-25k.$EXPT_NAME-$config.sexp";
    my $enamex_output   = "$output_dir/message-25k.$EXPT_NAME-$config.enamex";
    my $scoring_prefix  = "$output_dir/$EXPT_NAME-$config";

    my $training_jobid = 
         runjobs( [], "$EXPT_NAME-$config-train-pidf",
                  {
                      BATCH_QUEUE          => $LINUX_QUEUE,
                      serif_data           => $SERIF_DATA,
                      model_name           => $model_name,
                      training_file        => $train_file,
                      features             => $FEATURE_FILES{$LANGUAGE},
                      tags                 => $TAG_FILES{$LANGUAGE},
                      clusters             => $CLUSTERS{$LANGUAGE},
                  },
                  $PIDF, "pidf-train.par");

    my $test_jobid = 
         runjobs( [$training_jobid], "$EXPT_NAME-$config-test-pidf",
                  {
                      BATCH_QUEUE          => $LINUX_QUEUE,
                      serif_data           => $SERIF_DATA,
                      model_name           => $model_name,
                      features             => $FEATURE_FILES{$LANGUAGE},
                      tags                 => $TAG_FILES{$LANGUAGE},
                      clusters             => $CLUSTERS{$LANGUAGE},
                      input_file           => $test_file,
                      output_file          => $test_output,
                  },
                  $PIDF, "pidf-test.par");

    my $convert_jobid = 
         runjobs( [$test_jobid], "$EXPT_NAME-$config-convert-to-enamex",
                  {
                      BATCH_QUEUE          => $LINUX_QUEUE,
                      sexp_file            => $test_output,
                      enamex_file          => $enamex_output,
                  },
                  $PERL, "convert_sexp_to_enamex.pl");

    my $scorer_jobid =
        runjobs( [$convert_jobid], "$EXPT_NAME-$config-score",
                 {
                     BATCH_QUEUE           => $WINDOWS_QUEUE,
                     key_file              => unix2win($key_file),
                     decoded_file          => unix2win($enamex_output),
                     report_prefix         => unix2win($scoring_prefix),
                 },
                 unix2win($MUC_SCORER), "scorer-config");

    push(@scoring_jobs, $scorer_jobid);
}


runjobs( \@scoring_jobs, "$EXPT_NAME-$LANGUAGE-summarize_scores",
         {
             SCRIPT              => 1, # (don't spawn a job)
             OUTPUT_DIR          => $output_dir,
             SCORE_SUMMARY_FILE  => "$output_dir/$EXPT_NAME-$LANGUAGE-score-summary.txt",
             EXPT_NAME           => $EXPT_NAME,
             LANGUAGE            => $LANGUAGE,
         },
         $PERL, "summarize_scores.pl");


endjobs();
