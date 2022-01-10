#!/bin/env perl

use strict;
use warnings;

# Standard libraries:
use Getopt::Long;
use File::Basename;
use File::PathConvert;
use File::Glob;

# Runjobs libraries:
use lib ("/d4m/ears/releases/Cube2/R2010_11_21/".
         "install-optimize$ENV{ARCH_SUFFIX}/perl_lib");
use runjobs4;


# Package declaration:
package main;

#############################################################################
# Serif templates & data files
#############################################################################

# Trained models
my $LINUX_SERIF_DATA           = "/d4m/serif/data";
my $WINDOWS_SERIF_DATA         = unix2win($LINUX_SERIF_DATA);

my $TEMPLATE = "pidf-name-decode.par";

############################################################################
# Command-line argument Processing
############################################################################
# (must come before the call to startjobs().)

my $ARCHITECTURE     = "x86_64";
my $EXPERIMENT_NAME  = "PIDF_Decode_Experiment";

my $MODEL            = "%serif_data%/english/names/pidf/january2011.mc.all.weights";
my $TAGS             = "%serif_data%/english/names/pidf/ace2004.tags";
my $FEATURES         = "%serif_data%/english/names/pidf/ace2011.features";
my $CLUSTERS         = "%serif_data%/english/clusters/ace2005.hBits";

my $USE_CONSTRAINTS  = 0;
my $CONSTRAINT_TAGS  = "%serif_data%/english/names/pidf/ace2004.tags";

my $PIDF_BIN;
my @INPUT_FILES = ();
my $OUTPUT_DIR;

# Process our arguments; pass the rest through to runjobs.
Getopt::Long::Configure("pass_through");
GetOptions(
    "help"                  => sub { usage() },
    "architecture=s"        => \$ARCHITECTURE,
    "bin=s"                 => \$PIDF_BIN,
    "name=s"                => \$EXPERIMENT_NAME,
    "model=s"               => \$MODEL,
    "tags=s"                => \$TAGS,
    "constraint-tags=s"     => \$CONSTRAINT_TAGS,
    "features=s"            => \$FEATURES,
    "clusters=s"            => \$CLUSTERS,
    "input=s"               => \@INPUT_FILES,
    "output_dir=s"          => \$OUTPUT_DIR,
    "do-constrained-decode" => \$USE_CONSTRAINTS,
    );
Getopt::Long::Configure("no_pass_through");
die if $Getopt::Long::error;

@INPUT_FILES = map { glob($_) } @INPUT_FILES;

############################################################################
# Runjobs variables
############################################################################

our $QUEUE_PRIO = '5'; # Default queue priority
our ($exp_root, $exp) = startjobs();
my $runjobs_scheduler = &runjobs4::runjobs_object()->{SCHEDULER};

#############################################################################
# Experiment Configuration
#############################################################################

#;---------------------------------------------------------------------------
#; EXPERIMENT DIRECTORIES
#;---------------------------------------------------------------------------

my $exp_dir                     = "$exp_root/expts";
my $bin_dir                     = "$exp_root/bin";

#;---------------------------------------------------------------------------
#; Job Queues
#;---------------------------------------------------------------------------

# Queues used to run SERIF executables:
my %RUN_QUEUE = (
    i686     => 'ears',             # or 'rack1' or 'linuxbuild-32'
    x86_64   => 'nongale-x86_64',   # or 'dev-x86_64'
    Win32    => 'win64',
    Win64    => 'win64');

#;---------------------------------------------------------------------------
#; Architecture-Dependent Settings
#;---------------------------------------------------------------------------

my ($serif_data, $path_convert);
if ($ARCHITECTURE =~ /win.*/i) {
    # ------- Windows --------
    $serif_data = $WINDOWS_SERIF_DATA;
    $path_convert = \&File::PathConvert::unix2win;
} else {
    # ------- Linux --------
    $serif_data = $LINUX_SERIF_DATA;
    $path_convert = (sub {shift;});
}


############################################################################
#                          Job Sequencing
############################################################################

if (!defined($OUTPUT_DIR)) {
    $OUTPUT_DIR = "$exp_dir/output";
}

if (!defined($PIDF_BIN)) {
    $PIDF_BIN = "$bin_dir/" . pidf_binary(); 
}

# Parameter checking
die "$PIDF_BIN must exist!" unless (-f $PIDF_BIN);

mkdir "$OUTPUT_DIR", 0777 unless -d "$OUTPUT_DIR";
die "Could not create $OUTPUT_DIR" unless (-d "$OUTPUT_DIR");

mkdir "$OUTPUT_DIR/$EXPERIMENT_NAME", 0777 unless -d "$OUTPUT_DIR/$EXPERIMENT_NAME";
die "Could not create $OUTPUT_DIR/$EXPERIMENT_NAME" unless (-d "$OUTPUT_DIR/$EXPERIMENT_NAME");


print "$EXPERIMENT_NAME\n";
print "------------------------------------------------" . 
    "------------------------\n";

for my $input_file (@INPUT_FILES) {

    my ($file_name, $directory, $suffix) = fileparse(win2unix($input_file));

    my $output_file = "$OUTPUT_DIR/$EXPERIMENT_NAME/$file_name.decoded";
    my $job_name = "$EXPERIMENT_NAME/$file_name";

    print "$output_file\n";
    print "$job_name\n";

    my $jid = runjobs( [], $job_name,
		       {
			   serif_data              => $serif_data,
                           model_file              => &$path_convert($MODEL),
                           tags                    => &$path_convert($TAGS),
                           features                => &$path_convert($FEATURES),
                           clusters                => &$path_convert($CLUSTERS),
                           input_file              => &$path_convert($input_file),
                           output_file             => &$path_convert($output_file),
                           use_constraints         => &boolean2string($USE_CONSTRAINTS),
                           constraint_tags         => &$path_convert($CONSTRAINT_TAGS),
			   
			   BATCH_QUEUE             => $RUN_QUEUE{$ARCHITECTURE},
		       }, 
		       &$path_convert($PIDF_BIN), $TEMPLATE,
		       );

        

}

endjobs();

#----------------------------------------------------------------------
# pidf_binary() -> $filename
#
# Return the filename for the serif binary 
sub pidf_binary {
    my $bin = "PIdFTrainer";
    if ($ARCHITECTURE =~ /win.*/i) {
        $bin .= ".exe";
    }
    return $bin;
}

#----------------------------------------------------------------------
# boolean2string(boolean) -> string
#
# Return "true" or "false" depending on boolean value 
sub boolean2string($) {
    my $v = shift;
    if ($v) {
        return "true";
    }
    return "false";
}

sub usage {
    runjobs4::usage(); # <- this calls the runjobs usage script

    print qq{\nPIdF Decode Experiment Options:
       --help            Print this usage message and exit.
       --architecture    Architecture to use for running PIdF. Valid
                         settings are 'Win64' and 'x86_64'. Default=
                         $ARCHITECTURE.
       --bin PATH        Directory where the PIdFTrainer binary is
                         located.
       --name NAME       Experiment name.
       --model FILE      Model file to use for decoding. 
                         Default=$MODEL.
       --tags FILE       Tags file to use for decoding.
                         Default=$TAGS.
       --features FILE   Features file to use for decoding.
                         Default=$FEATURES.
       --clusters FILE   Word cluster bits file to use for decoding.
                         Default=$CLUSTERS 
       --do-constrained-decode
       --constaint-tags FILE
                         Tags file used for constraint input.
       --input FILE      Input file to decode. Should be sentence-
                         broken, tokenized text.  On sentence per
                         line, enclosed by parentheses, with space-
                         separated tokens. 
       --output_dir PATH \n};
     
    exit;  
}
