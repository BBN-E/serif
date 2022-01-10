#!/bin/env perl
#
# SERIF Build Experiment
#
# Build all versions of SERIF, and perform a test to make sure that
# they run correctly.  Run with --help for usage.
#
# Copyright (C) 2015 BBN Technologies
# [5/25/10] Rewritten by Edward Loper

use strict;
use warnings;

# Standard libraries:
use Getopt::Long;
use Cwd qw(abs_path);
use Net::SMTP;
use MIME::Lite;

use POSIX qw/strftime/;
use List::Util qw(sum min max shuffle);
use List::MoreUtils qw(zip);
use File::Glob ':glob';
use Statistics::Basic qw(stddev);
use File::Path 'rmtree';
use File::Path 'mkpath';

# Use a specific older release to avoid Windows compatibility issues in latest
use lib "/d4m/ears/releases/Cube2/R2015_01_15_1/install-optimize-static/perl_lib";
# this on works fine without Windows runs.
## actually it worked once, fails in cron job?
#use lib "/d4m/ears/releases/Cube2/R2016_07_21/install-optimize/perl_lib";

# Runjobs libraries:
use runjobs4;
use Scheduler::Scheduler;
use Scheduler::Job qw( JOB_FAILED );
use File::PathConvert qw(unix2win win2unix);
use Date::Parse;

# Package declaration:
package main;

if (!defined($ENV{SGE_ROOT})) {
    print "WARNING: SGE_ROOT not defined; using a default value!\n";
    $ENV{SGE_ROOT} = "/opt/sge-6.2u5";
}
## this var sets where the Unix 64-bit jobs  are to be run 
## -- can't be computed from where this perl script is running.
#my $unix_version = 'sl5';  ##Scientific Linux 5 == Centos5 (kernel 2.6.8) "Boron"
my $unix_version = 'sl6';  ##Scientific Linux 6 == Centos6 (kernel 2.6.32) "Carbon"

#############################################################################
# Build Configurations
#############################################################################
# A build configuration specifies an architecture and set of
# parameters that should be used to build a single set of SERIF
# binaries.  Each configuration consists of a hash mapping
# configuration fields (such as ARCHITECTURE or LANGUAGE) to values
# (such as Win64 or English).  The following data structure defines
# the list of configuration fields, and the acceptable values for each
# field.  (Note: the descriptions associated with each value aren't
# actually used anywhere.)

my @CONFIG_FIELDS = (

    # ==================== SVN Revision ======================
    SVN_REVISION => {
	'rev-\d+'        => 'A specific SVN revision',
	'rev-head'       => 'The most recent SVN revision',
	'rev-local'      => 'The local working copy (no svn checkout)',
	'tag-.*'         => 'A specified tagged SVN version',
	'branch-.*'      => 'A specified tagged SVN version',
	'date-\d\d\d\d-\d\d-\d\d'
	    => 'The SVN revision for the given date (YYYY-MM-DD)',
	'date-\d\d\d\d-\d\d-\d\d:\d\d\d\d-\d\d-\d\d(\[\d+\])?'
	    => 'All SVN revisions from the date range YYYY-MM-DD:YYYY-MM-DD',
    },

    # ============== CMake-Level Configuration ================
    ARCHITECTURE => {
	'Win32'          => '32-bit Windows (Visual Studio)',
	'Win64'          => '64-bit Windows (Visual Studio)',
	'i686'           => '32-bit Linux (gcc on SL5)',
	'x86_64'         => '64-bit Linux (gcc on SL6)',
	'x86_64_sl5'     => '64-bit Linux (gcc on SL5)'},
    LINK_TYPE => {
        'Dynamic'        => 'Build dynamically linked executables',
        'Static'         => 'Build statically linked executables'},
    SYMBOL => {
	'NoThread'         => 'Non-threadsafe reference counting symbols',
	'ThreadSafeSym'    => 'Thread safe reference counting symbols',
	'NoRefCountSym'    => 'No reference counting for symbols',
	'BigEntryBlockSym' => 'w/o thread-safety, large entry block',
	'BigStringBlockSym'=> 'w/o thread-safety, large string block'},

    # ============== Build-Level Configuration ================
    # (we could theoretically do all these in the same build directory;
    # but we would need to run cmake first.)
    BUILD_TARGET => {
	'Classic'        => 'Classic SERIF',
 	'Encrypt'        => 'Classic SERIF with BasicCipherStream module',
	'KbaStreamCorpus_Serif'=> 'Serif with KbaStreamCorpus modules',
	'KbaStreamCorpus_IDX'=> 'IDX with KbaStreamCorpus modules',
	'ProfileGenerator'=> 'SERIF + PG with Postgres module',
	'IDX'            => 'IDX (commercial)' },
    DEBUG_MODE => {
	'Release'        => 'Release Build',
	'DebugPerformance' => 'Debugging build with gperftools',
	'Debug'          => 'Debugging Build'},

    # =============== Run-Level Configuration =================
    # These all use the same binaries -- i.e., we build serif once,
    # and re-use it for each run-level configuration.
    LANGUAGE => {
	'English'        => 'English',
	'Chinese'        => 'Chinese',
	'Arabic'         => 'Arabic',
	'Spanish'        => 'Spanish',
	'Unspecified'    => 'Unspecified'},
    PARAMETERS => {
	'AceBest'        => 'ACE test on latest best parameters',
	'AceFast'        => 'ACE test on Fast Parser',
	'AwakeBest'      => 'Awake test on latest best parameters', 
	'IcewsBest'      => 'ICEWS test on latest best parameters',
	'TPutBest'       => 'Latest Best Parameters (Throughput Test)',
	'TPutFast'       => 'Fast Parser (Throughput Test)',
	'TPutFastAgg'    => 'Aggressive Fast Parser (Throughput Test)',
	'TPutNoParser'   => 'NP-Chunker (Throughput Test)',
	'Values'         => 'Best Parameters (through values only)'},
    );
my @CONFIG_FIELD_NAMES = even_items(@CONFIG_FIELDS); # preserve order
my %CONFIG_FIELD_VALUES = @CONFIG_FIELDS;

# List of the parameter files that use the 'fast parser'.  This is used
# (1) to determine what reference data to check for regression tests when
# comparing to reference sets that predate 08/01/2008; and (2) to split
# up parameters into groups when plotting graphs.
my %FAST_PARSER_PARAMETERS = map {$_ => 1} (
    "AceFast", "TPutFast", "TPutNoParser", "TPutFastAgg" );

# Special constants used for SVN revisions.
use constant SVN_LOCAL => 'local';
use constant SVN_HEAD  => 'head';

# Default values for configuration fields.  If no default is specified
# here for a field, then the default is assumed to be all possible values.
my %DEFAULT_CONFIG_VALUES = (
    SVN_REVISION    => [SVN_LOCAL],
    SYMBOL          => ["NoThread"],
    BUILD_TARGET    => ["Classic"],
    DEBUG_MODE      => ["Release"],
    PARAMETERS      => ["AceFast", "AceBest"],
    ARCHITECTURE    => ["Win32", "Win64", "i686", "x86_64"],
    LANGUAGE        => ["English", "Arabic", "Chinese"]
    );

#############################################################################
# Skipped Builds and tests
#############################################################################

my @SKIP_BUILD_CONDITIONS = (
    # The 'NoParser/FastAgg' par files are only defined for English:
    {LANGUAGE => "Chinese|Arabic|Unspecified",  PARAMETERS => "TPutNoParser|TPutFastAgg"},
	# icews and awake only run in English
	{PARAMETERS => "AceFast|IcewsBest|AwakeBest", LANGUAGE => "Chinese|Arabic|Spanish"},
    # gperftools is only available on 64-bit Linux
    { DEBUG_MODE => "DebugPerformance", ARCHITECTURE => "i686|x86_64_sl5|Win32|Win64"},
    );

my @SKIP_TEST_CONDITIONS = (
    {DEBUG_MODE => "Debug"},
    {LANGUAGE => "Unspecified"},
	{PARAMETERS => "IcewsFast|AwakeFast"},
    {PARAMETERS => "AceFast|IcewsBest|AwakeBest", LANGUAGE => "Chinese|Arabic|Spanish"},
	{PARAMETERS => "AwakeBest", ARCHITECTURE => "Win32|i686"},
    );

#############################################################################
# Serif templates & data files
#############################################################################

# External links
#my $mysql_win_lib = "/d4m/serif/External/MySQL Connector C 6.1/lib/";

# Trained models
my $LINUX_SERIF_DATA           = "/d4m/serif/data";
my $WINDOWS_SERIF_DATA         = unix2win($LINUX_SERIF_DATA);

# Regression tests
my $LINUX_SERIF_REGRESSION     = "/d4m/serif/Ace";
my $WINDOWS_SERIF_REGRESSION   = unix2win($LINUX_SERIF_REGRESSION);

my $TESTDATA_HOME              = "/d4m/serif/testdata";
my $LINUX_TESTDATA_DATE        = "200427"; # YMMDDD
my $WINDOWS_TESTDATA_DATE      = "200427"; # YYMMDD

my $IDX_TESTDATA_HOME              = "/d4m/serif/idx_testdata";
my $IDX_LINUX_TESTDATA_DATE        = "200427"; # YYMMDD
my $IDX_WINDOWS_TESTDATA_DATE      = "200427"; # YYMMDD

my %SERIF_TEMPLATES = (
  # Language_ParFile   => Parameter File
    English_AceBest      => "regtest.best-english.par",
    English_IcewsBest    => "regtest.icews.english.xmltext.par",
    English_AwakeBest    => "regtest.awake.english.sgm.for_nightly.par", 
    English_AceFast      => "regtest.speed.best-english.par", 
    Chinese_AceBest      => "regtest.ace2005-chinese.par",
    Chinese_AceFast      => "regtest.ace2005-chinese.par",
    Arabic_AceBest       => "regtest.ace2008-arabic.par",
    Arabic_AceFast       => "regtest.speed.ace2008-arabic.par",
    English_TPutBest     => "throughput.best-english.par",
    English_TPutFast     => "throughput.speed.best-english.par",
    English_TPutFastAgg  => "throughput.speed_agg.best-english.par",
    English_TPutNoParser => "throughput.speed_npchunk.best-english.par",
    Chinese_TPutBest     => "throughput.ace2005-chinese.par",
    Chinese_TPutFast     => "throughput.speed.ace2005-chinese.par",
    Arabic_TPutBest      => "throughput.ace2008-arabic.par",
    Arabic_TPutFast      => "throughput.speed.ace2008-arabic.par",
    English_Values       => "throughput.best-english.par",
    Chinese_Values       => "throughput.ace2005-chinese.par",
    Arabic_Values        => "throughput.ace2008-arabic.par",
    Spanish_AceBest         => "regtest.best-spanish.par",
);

my %SCORE_FILES = (
  # Language        => score-file
    English         => "ace2007-eval.txt",
    Arabic          => "ace2007-eval.txt",
    Chinese         => "ace2005-eval.txt",
    Spanish         => "ace2007-eval.txt",
);

############################################################################
# Throughput Tests
############################################################################

my $THROUGHPUT_HISTORY_FILE = "/d4m/serif/daily_build/tput_history.txt";
my $NUM_HISTORICAL_TPUT_DATES = 20;
my %HISTORICAL_TPUT_CONFIGURATIONS = (
    "Normal Parser" => ["x86_64/NoThread/Classic/Release/English/TPutBest",
			"x86_64/NoThread/Classic/Release/Chinese/TPutBest",
			"x86_64/NoThread/Classic/Release/Arabic/TPutBest"],
    "Fast Parser" => ["x86_64/NoThread/Classic/Release/English/TPutFast",
		      "x86_64/NoThread/Classic/Release/Chinese/TPutFast",
		      "x86_64/NoThread/Classic/Release/Arabic/TPutFast"],
    );
my %EXPECTED_TPUT = (
        "x86_64/NoThread/Classic/Release/Chinese/TPutBest"  =>  1.5,
	"x86_64_sl5/NoThread/Classic/Release/Chinese/TPutBest"  =>  1.5,
         "x86_64/NoThread/Classic/Release/Arabic/TPutBest"  =>  0.5,
	 "x86_64_sl5/NoThread/Classic/Release/Arabic/TPutBest"  =>  0.5,
         "x86_64/NoThread/Classic/Release/Chinese/TPutFast" =>  14.0,
     "x86_64_sl5/NoThread/Classic/Release/Chinese/TPutFast" =>  14.0,
         "x86_64/NoThread/Classic/Release/Arabic/TPutFast"  =>  5.0,
	 "x86_64_sl5/NoThread/Classic/Release/Arabic/TPutFast"  =>  5.0,
         "x86_64/NoThread/Classic/Release/English/TPutBest" =>  5.5,
	 "x86_64_sl5/NoThread/Classic/Release/English/TPutBest" =>  5.5,
         "x86_64/NoThread/Classic/Release/English/TPutFast" => 35.0,
	 "x86_64_sl5/NoThread/Classic/Release/English/TPutFast" => 35.0,
     "x86_64/NoThread/Classic/Release/English/TPutFastAgg"  => 40.0,
 "x86_64_sl5/NoThread/Classic/Release/English/TPutFastAgg"  => 40.0,
    "x86_64/NoThread/Classic/Release/English/TPutNoParser"  => 50.0,
"x86_64_sl5/NoThread/Classic/Release/English/TPutNoParser"  => 50.0,
);

my %THROUGHPUT_GRAPHS = (
    CURRENT => 1,
    PROFILE => 1,
    HISTORICAL => 1);

my $NUM_PROFILE_SECTIONS = 8;

############################################################################
# Subversion
############################################################################

#my $SVN_URL = "svn+ssh://d201.bbn.com/nfs/emc5/svn-repository/text";
my $SVN_URL = "svn+ssh://svn.d4m.bbn.com/export/svn-repository/text";

############################################################################
# Command-line argument Processing
############################################################################
# (must come before the call to startjobs().)

# control the size of the report
my $VERBOSE = 0;
my $REPORT_DONE_JOBS = 0;

# If $BUILD_ONLY is true, then don't run any regression tests.
my $BUILD_ONLY = 0;

# If true, then check out the data rather than using the shared copy.
# n.b.: this uses a lot of disk space!
my $CHECKOUT_DATA = 0;

#if set, this forces a specifc directory tree to be used as Serif_data
my $USE_SERIF_DATA_DIR = 0;

#if set, build specific deliverables
my $CUSTOM_DELIVERABLES = 0;

# If this is set, then send an email reporting success or failure to
# this address when we're done.
my $MAILTO;
my $MAILFROM;

# If this is set, then the build-serif script will return 0 (success)
# if an experiment fails, but it successfully reports the failure
# (e.g. by sending an email).  Otherwise, it will return 1.  This is
# used by scripts that call build_serif.pl to distinguish normal
# experiment failures (which will already be reported) from errors in
# this build_serif.pl script itself.
my $RETURN_SUCCESS_AFTER_REPORTING_FAILURE = 0;

# Name for the experiment; if not specified, one will be chosen
# automatically.  This is just used for logs/emails.
my $EXPERIMENT_NAME;

# If this is set, then generate throughput output...
my $CHECK_THROUGHPUT = 0;
my $SAVE_THROUGHPUT = 0;
my $RUN_ON_THE_SAME_MACHINE = 0;
my $THROUGHPUT_RUNS = 3;
my $THROUGHPUT_SO_FAR;

# If this is set, use a pre-built executable instead of building
#   on the fly. Configurations will be specified in @CONFIG_SPECS.
my $PREBUILT_EXECUTABLE = 0;

# What configurations should we build?
my @CONFIG_SPECS = ();

# Are we running the build experiment for a release?
my $ASSEMBLE_RELEASE = 0;

# Should we build shared libraries?  (IDXJNI_$language.so)
my $BUILD_SHARED_LIBS = 0;

# Should we build the java package (serif.jar)?
my $BUILD_JAVA = 0;

# Should we include the MTDecoder libary?
my $LINK_MT_DECODER = 0;

# If this is set, then generate state-saver output files for every
# stage.  These can be useful for testing specific stages of serif.
my $SAVE_STATE = 0;
my $ALL_STATE_SAVER_STAGES =
    "sent-break,tokens,part-of-speech,names,nested-names,values,parse," .
    "npchunk,dependency-parse,mentions,props,metonymy,sent-level-end,doc-entities," .
    "doc-relations-events,doc-values,clutter";

# If this is set, then read parameter files (aka templates) from
# this directory, rather than from svn.
my $PAR_DIRECTORY;

# Should we run doxygen?
my $RUN_DOXYGEN;

my $FORCE_SERIFXML = 0;

# Should we override the version string specified in version.cpp?
my $SERIF_VERSION_STRING = "version";

# Should we use any extra cmake options?
my @EXTRA_CMAKE_OPTIONS=();

# Process our arguments; pass the rest through to runjobs.
Getopt::Long::Configure("pass_through");
my $cmdline_description = "$0 " . join(" ", map {quote_str($_)} @ARGV);
GetOptions(
    "help"               => sub { build_experiment_usage() },
    "help-build"         => sub { help_build() },
    # Build Options
    "build=s"            => sub {push @CONFIG_SPECS, $_[1]},
    "build-only"         => \$BUILD_ONLY,
    "build-shared-libs"  => \$BUILD_SHARED_LIBS,
    "build-java"         => \$BUILD_JAVA,
    "link-mt-decoder"    => \$LINK_MT_DECODER,
    "assemble-release"   => \$ASSEMBLE_RELEASE,
    "serif-version=s"    => \$SERIF_VERSION_STRING,
    "cmake=s"            => sub {push @EXTRA_CMAKE_OPTIONS, $_[1]},
    # Using a pre-built executable
    "use-prebuilt=s"     => \$PREBUILT_EXECUTABLE,
    # Using a pre-built par file
    "pardir=s"           => \$PAR_DIRECTORY,
    # Regression Test Options
    "save-regtest=s"     => sub { print "The --save-regtest option is deprecated; use bin/save_regtest.py instead.\n"; exit 1; },
    "checkout-data"      => \$CHECKOUT_DATA,
	"use-data-dir=s"     => \$USE_SERIF_DATA_DIR,
    "save-state"         => \$SAVE_STATE,
    "force-serifxml"     => \$FORCE_SERIFXML,
    # Throughput Test Options
    "tput"               => \$CHECK_THROUGHPUT,
    "save-tput"          => \$SAVE_THROUGHPUT,
    "tput-runs=i"        => \$THROUGHPUT_RUNS,
    "one-machine"        => \$RUN_ON_THE_SAME_MACHINE,
    "tput-so-far"        => \$THROUGHPUT_SO_FAR,
    # Doxygen options
    "run-doxygen"        => \$RUN_DOXYGEN,
    # Reporting options
    "mailto=s"           => \$MAILTO,
    "mailfrom=s"         => \$MAILFROM,
    "name=s"             => \$EXPERIMENT_NAME,
	"report_done_jobs"   => \$REPORT_DONE_JOBS,
    "verbose"            => sub {++$VERBOSE},
    "return-success-after-reporting-failure"
                         => \$RETURN_SUCCESS_AFTER_REPORTING_FAILURE,
    );
Getopt::Long::Configure("no_pass_through");
die if $Getopt::Long::error;
print "Extra options: \n";
foreach my $option (@EXTRA_CMAKE_OPTIONS) {
	print "\t".$option."\n";
}
if ($USE_SERIF_DATA_DIR) {
	# reset these
	$LINUX_SERIF_DATA           = "$USE_SERIF_DATA_DIR";
	# this conversion doesn't work for some newwer serif/data paths
    # $WINDOWS_SERIF_DATA         = unix2win($LINUX_SERIF_DATA);
}

sub build_experiment_usage {
    runjobs4::usage(); # <- this calls the runjobs usage script

    print qq{SERIF Build Experiment Options:
    Build Options
      --build SPEC     Build all configurations that are indicated by the
                       given specification.  SPEC is a list of configuration
                       field values (such as English or Win32) separated by
                       slashes.  E.g.: i686/English/AceBest/Classic/Release
      --help-build     Print more information on what SPEC can contain.
      --build-only     Just build SERIF; don't run any regression tests.
      --build-java     Build java package (serif.jar)
      --build-shared-libs
                       Build shared libraries (eg IDXJNI_English.so).
      --run-doxygen    Build documentation for C++ code
      --use-prebuilt PATH_TO_EXECUTABLE
                       Test a pre-built version of SERIF. Configurations should
                       be specified using '--build'.
    Regression Test Options
      --checkout-data  Don't use the shared copy of the data; check out a
                       fresh copy.  This can be useful when testing old
                       revisions (where the data may have changed), but
                       checking out the data takes a lot of disk space.
    Throughput Test Options
      --tput           Run throughput tests
      --save-tput      Save throughput results to the throughput history file:
                       $THROUGHPUT_HISTORY_FILE
      --tput-runs N    Run the throughput test N times, to get more reliable
                       results.  Default=$THROUGHPUT_RUNS
      --one-machine    Run all throughput tests on a single machine, to get
                       more reliable results.
    Reporting options
      --mailto EMAIL   Email a report to the given address.
      --mailfrom EMAIL Reply-to address for the emailed report.
      --name NAME      Experiment name for the emailed report
      --verbose        Generate more verbose output to stdout.
      --return-success-after-reporting-failure
                       If the experiment fails but we successfully send an
                       email indicating the failure, then return true.\n};
    exit;
}
sub help_build {
    my $USE_LESS = 1;

    my $header = "SERIF BUILD SPECIFICATIONS";
    my $pipestr = "|cat";

    if ($USE_LESS) {
	$header = join("", map {"$_\b$_"} split //, $header); # bold.
	$pipestr = "|less";
    }

    open PIPE, $pipestr;
    print PIPE (
	"$header\n\n" .
	"The '--build SPEC' option is used to indicate a set of configurations\n" .
	"that should be built.  It may be repeated to indicate multiple groups\n" .
	"of configurations.  Any redundancy from overlap will be automatically\n" .
	"eliminated.  Each specification consists of a list of field values,\n" .
	"separated by slashes.  E.g.:\n\n" .
	"--build i686/English/AceBest/Classic/Release\n\n" .
	"Any field that is not specified will take the default set of values.\n" .
	"You may specify multiple values for a single field by using '+'.  E.g.:\n\n" .
	"--build Win32/English+Arabic/AceBest+AceFast\n\n" .
	"Fields may be specified in any order.  The following list describes\n" .
	"all of the field values, and gives the default value for each field\n" .
	"(in square brackets):\n\n");
    for my $field (@CONFIG_FIELD_NAMES) {
	my $defaults = ($DEFAULT_CONFIG_VALUES{$field} ||
		       [keys %{$CONFIG_FIELD_VALUES{$field}}]);
	print PIPE "$field [" . join("+", @$defaults) . "]\n";
	while (my ($value, $descr) = each(%{$CONFIG_FIELD_VALUES{$field}})) {
	    if (length($value) < 14) {
		print PIPE sprintf("    %-14s %s\n", $value, $descr);
	    } else {
		print PIPE "    $value\n        $descr\n";
	    }
	}
    }
    close PIPE;
    exit;
}

# Default value for configuration specifications:
@CONFIG_SPECS = ("ALL") unless @CONFIG_SPECS;

# Change the default speed settings for throughput.
if ($CHECK_THROUGHPUT) {
    $DEFAULT_CONFIG_VALUES{PARAMETERS} = [
	"TPutFast", "TPutBest",
	"TPutNoParser", "TPutFastAgg"];
    $DEFAULT_CONFIG_VALUES{ARCHITECTURE} = ["x86_64"];
    $DEFAULT_CONFIG_VALUES{BUILD_TARGET} = ["Classic"];
    $DEFAULT_CONFIG_VALUES{LANGUAGE} = ["English", "Arabic", "Chinese"];
    @SKIP_TEST_CONDITIONS = ();
}

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
#; Job Queues
#;---------------------------------------------------------------------------

# Queue used to check SERIF out from SVN:

my $PREPARE_QUEUE = 'nongale-sl6';

# Queues used to build SERIF executables:
my %BUILD_QUEUE = (
    i686     => 'nongale-x86_64',
    x86_64   => 'nongale-sl6',
    x86_64_sl5     => 'nongale-x86_64',
    Win32    => 'windev-x86_64',
    Win64    => 'windev-x86_64');

# Queues used to run SERIF executables:
my %RUN_QUEUE = (
    i686     => 'nongale-x86_64',
    x86_64   => 'nongale-sl6',
    x86_64_sl5     => 'nongale-x86_64',
    Win32    => 'win64',
    Win64    => 'win64');

# Maximum expected memory allocation for SERIF executables:
#my $RUN_SGE_VIRTUAL_FREE = "4G";
my %RUN_SGE_VIRTUAL_FREE = (
    i686     => '2G',  
    x86_64   => '4G',  
    x86_64_sl5   => '4G',  
    Win32    => '2G',
    Win64    => '4G');


# Queue used to run scoring scripts:
my $SCORE_QUEUE = 'nongale-sl6';
my $SCORE_ARCHITECTURE = 'x86_64';


# If we're checking throughput, then use a single machine to run all
# jobs (to make for a more fair comparison); and set SGE_VIRTUAL_FREE
# very high (to hog the machine).
if ($RUN_ON_THE_SAME_MACHINE) {
    %RUN_QUEUE = (
# swap out the usual one-machine to tst slowness cause 4/26/16
#		i686     => ($unix_version = 'sl6' ? 'm300' : 'meta100s'),
#		x86_64   => ($unix_version = 'sl6' ? 'm300' : 'meta100s'),
		i686     => ($unix_version = 'sl6' ? 'nongale-sl6' : 'meta100s'),
		x86_64   => ($unix_version = 'sl6' ? 'nongale-sl6' : 'meta100s'),
		x86_64_sl5   =>  'nongale',
		Win32    => 'windev-x86_64',
		Win64    => 'windev-x86_64');
    #$RUN_SGE_VIRTUAL_FREE = "16G";
	%RUN_SGE_VIRTUAL_FREE = (
# drop the hogging factor to fit on shared machines (test 4/26/16)
#		i686     => '16G',  
#		x86_64   => '16G',  
		i686     => '8G',  
		x86_64   => '8G',  
		x86_64_sl5   => '8G',  
		Win32    => '16G',
		Win64    => '16G');
}

#;---------------------------------------------------------------------------
#; CMake & Makefile targets
#;---------------------------------------------------------------------------

# The solution/project generator for cmake for windows builds:
my %WINDOWS_CMAKE_GENERATOR = (
    Win32 => "Visual Studio 9 2008",
    Win64 => "Visual Studio 9 2008 Win64",
    );

# The cmake option(s) for fast builds:
my $CMAKE_FAST_OPTION = "-DFAST_PARSING=ON";

# The cmake options for any build using encrypted models:
my @CMAKE_ENCRYPT_OPTIONS = (
    "-DFEATUREMODULE_BasicCipherStream_INCLUDED=ON"
    );

# The cmake options for any build of Serif and ProfileGenerator
my @CMAKE_PROFILE_GENERATOR_OPTIONS = (
    "-DUSE_POSTGRES=ON",
    "-DCREATE_PROFILE_GENERATOR_SOLUTION=ON"
    );

# The cmake options for any build of commerical IDX
my @CMAKE_IDX_OPTIONS = (
    "-DFEATUREMODULE_License_INCLUDED=ON"
    );

# The cmake options for doing performance testing
my @CMAKE_PERFTOOLS_OPTIONS = (
    "-DFEATUREMODULE_PerformanceTools_INCLUDED=ON"
    );

# The cmake option(s) for the KbaStreamCorpus system:
my @CMAKE_KbaStreamCorpus_OPTIONS = (
    "-DFEATUREMODULE_KbaStreamCorpus_INCLUDED=ON",
    "-DFEATUREMODULE_BasicCipherStream_INCLUDED=ON",
    "-DFEATUREMODULE_HTMLDocumentReader_INCLUDED=ON",
    # Exclude modules that we do not use:
    "-DFEATUREMODULE_Transliterator_INCLUDED=OFF",
    "-DFEATUREMODULE_CipherStream_INCLUDED=OFF",
    "-DFEATUREMODULE_Arabic_INCLUDED=OFF",
    "-DFEATUREMODULE_Chinese_INCLUDED=OFF",
    "-DFEATUREMODULE_SerifHTTPServer_INCLUDED=OFF",
    );

# The cmake option(s) for various symbol configurations
my %SYMBOL_CMAKE_OPTIONS = (
    "ThreadSafeSym"     => ["-DSYMBOL_REF_COUNT=ON",
			    "-DSYMBOL_THREADSAFE=ON"],
    "NoRefCountSym"     => ["-DSYMBOL_REF_COUNT=OFF",
			    "-DSYMBOL_THREADSAFE=ON"],
    "NoThread"          => ["-DSYMBOL_REF_COUNT=ON",
			    "-DSYMBOL_THREADSAFE=OFF"],
    "SmallEntryBlock"   => ["-DSYMBOL_REF_COUNT=ON",
			    "-DSYMBOL_THREADSAFE=OFF",
			    "-DSYMBOL_ENTRY_BLOCK_SIZE=1000"],
    "BigEntryBlockSym"  => ["-DSYMBOL_REF_COUNT=ON",
			    "-DSYMBOL_THREADSAFE=OFF",
			    "-DSYMBOL_ENTRY_BLOCK_SIZE=16350"],
    "BigStringBlockSym" => ["-DSYMBOL_REF_COUNT=ON",
			    "-DSYMBOL_THREADSAFE=OFF",
			    "-DSYMBOL_STRING_BLOCK_SIZE=128000"]);

# Extra targets for specific configurations:
my @EXTRA_CLASSIC_MAKE_TARGETS = (
    "DTCorefTrainer", "EventFinder", "IdfTrainer",
    "IdfTrainerPreprocessor", "MaxEntRelationTrainer",
    "NameLinkerTrainer", "P1DescTrainer",
    "P1RelationTrainer", "PIdFTrainer", "PNPChunkTrainer",
    "PPartOfSpeechTrainer", "PronounLinkerTrainer",
    "RelationTimexArgFinder", "RelationTrainer",
    "SerifSocketServer", "StandaloneParser",
    "StandaloneSentenceBreaker", "StandaloneTokenizer",
    "StatSentBreakerTrainer", "DeriveTables", "Headify",
    "K_Estimator", "StatsCollector", "VocabCollector",
    "VocabPruner",
    # These are intended for arabic only:
    "MorphologyTrainer", "MorphAnalyzer");
my @EXTRA_PROFILE_GENERATOR_MAKE_TARGETS = (
    "DocSim", "ProfileGenerator");
my @EXTRA_IDX_MAKE_TARGETS = (
    "create_license", "print_license",
    "verify_license");

# The drive where the build directory should be mounted for windows builds:
my $WINDOWS_BUILD_DRIVE = "Q:";

#;---------------------------------------------------------------------------
#; EXPERIMENT DIRECTORIES
#;---------------------------------------------------------------------------

my $expdir                     = "$exp_root/expts";
my $bindir                     = "$exp_root/bin";
my $logdir                     = "$exp_root/logfiles";
my $etemplatedir               = "$exp_root/etemplates";

#;---------------------------------------------------------------------------
#; BINARIES & LIBRARIES
#;---------------------------------------------------------------------------

my %ARCH_SUFFIX = (Win32 => 'Win32', x86_64 => '-x86_64',
		   x86_64_sl5 => '-x86_64',
		   Win64 => 'Win64', i686 => '');
my %ARCH_PREFIX = (Win32 => 'Windows', x86_64 => 'Linux',
		   x86_64_sl5 => 'Linux',
		   Win64 => 'Windows', i686 => 'Linux');
my %BITS        = (Win32 => '32', x86_64 => '64',
		   x86_64_sl5 => '64',
		   Win64 => '64', i686 => '32');

# Scripting & subversion:
my $CSH                     = "$bindir/run_csh_script";
my $BASH                    = "/bin/bash";
my %PERL                    = (
    i686   => "/opt/perl-5.8.8/bin/perl",
    x86_64 => "/opt/perl-5.14.1-x86_64/bin/perl",
    x86_64_sl5 => "/opt/perl-5.8.6-x86_64/bin/perl",
    Win32  => "c:\\Perl64\\bin\\perl.exe",
    Win64  => "c:\\Perl64\\bin\\perl.exe",
    );
my %PYTHON                  = (
    i686   => "/opt/Python-2.7.1/bin/python",
    x86_64 => "/opt/Python-2.7.8-x86_64/bin/python",
    x86_64_sl5 => "/opt/Python-2.7.1-x86_64/bin/python",
    Win32  => "c:\\Python27\\python.exe",
    Win64  => "c:\\Python27-x64\\python.exe",
    );
my %PYTHON_LIB                  = (
    i686   => "/opt/Python-2.7.1/lib",
    x86_64 => "/opt/Python-2.7.8-x86_64/lib",
    x86_64_sl5 => "/opt/Python-2.7.1-x86_64/lib",
    Win32  => "c:\\Python27\\lib",
    Win64  => "c:\\Python27-x64\\lib",
    );
my $SVN_HOME                = ($unix_version = 'sl6' ? "/opt/subversion-1.8.13-x86_64" : "/opt/subversion-1.6.9");
my $SVN                     = "${SVN_HOME}/bin/svn";
my $PLOT_HOME               = ($unix_version = 'sl6' ? "/usr/bin" : "/d4m/serif/bin/plot");
my $FIG2DEV                 = "${PLOT_HOME}/fig2dev";
my $CONVERT                 = "${PLOT_HOME}/convert";
my $GNUPLOT                 = "${PLOT_HOME}/gnuplot";
my $DOXYGEN                 = "/opt/doxygen-1.7.6.1-x86_64/bin/doxygen"; 

# Compiler & libraries:
my %GCC_HOME             = (
    i686   => "/usr",
    x86_64 => "/opt/gcc-4.4.7",
    x86_64_sl5 => "/usr",
    );
my %BOOST_HOME           = (
    i686   => "/opt/boost_1_40_0-gcc-4.1.2",
    x86_64 => "/opt/boost_1_55_0-gcc-4.4.6",
    x86_64_sl5 => "/opt/boost_1_40_0-gcc-4.1.2",
    );
my %BOOST_VERSION        = (
    Win32  => "1_40_0",
    Win64  => "1_40_0",
    );
my %CMAKE_HOME              = (
    i686   => "/opt/cmake-2.8.11",
    x86_64 => "/opt/cmake-2.8.12.2",
    x86_64_sl5 => "/opt/cmake-2.8.11",
    );
my %GPERFTOOLS_HOME      = (
    x86_64 => "/opt/gperftools-2.5",
    i686   => "",
    );
my %YAMCHA_HOME             = (
    i686   => "/opt/yamcha-0.33",
    x86_64 => "/opt/yamcha-0.33",
    x86_64_sl5 => "/opt/yamcha-0.33",
    Win32  => "c:/yamcha/yamcha-0.33-x86",
    Win64  => "c:/yamcha/yamcha-0.33-x86_64",
    );

#;---------------------------------------------------------------------------
#; REPORTING
#;---------------------------------------------------------------------------

# This email header explains how this version relates to previous versions;
# it can be shortened or eliminated later.
my $EMAIL_HEADER = join("\n", (
    "------------------------------------------------------------------------",
    "This is the new build-serif experiment, which is located in the svn",
    "repository at text/Active/Projects/SERIF/experiments/build-serif.  It ",
    "now includes option for testing ICEWS and Awake.",

    "------------------------------------------------------------------------",
    ));

############################################################################
#                          Job Sequencing
############################################################################
my $start_time=localtime;

# Pick an experiment name, if one wasn't given.
if (!defined($EXPERIMENT_NAME)) {
    $EXPERIMENT_NAME = ($BUILD_ONLY ? "SERIF Build Experiment" :
			($PREBUILT_EXECUTABLE ? "SERIF Test Experiment" :
			"SERIF Build and Test Experiment"));
}

# Construct the list of configurations.
#$DEFAULT_CONFIG_VALUES{SVN_REVISION} = [default_svn_revision()];
my $incl_config_description = "";  # Filled in by make_configuration_list.
my $skip_config_description = "";  # Filled in by make_configuration_list.
my $incl_regtest_description = ""; # Filled in by make_configuration_list.
my $skip_regtest_description = ""; # Filled in by make_configuration_list.
my @configurations = make_configuration_list();
my %CONFIG_VALUES = map {$_ => [config_values($_)]} @CONFIG_FIELD_NAMES;
my @SVN_REVISIONS = @{$CONFIG_VALUES{"SVN_REVISION"}};

# Let the user know what we're planning to do.
my $parameter_description =
    "     Config Specs: " .
    join("\n                   ", @CONFIG_SPECS) . "\n" .
    "                   (" . scalar(@configurations) .
    " build configurations)\n" .
    "    SVN Revisions: @SVN_REVISIONS\n" .
    "   Check out data: " . ($CHECKOUT_DATA ? "yes" : "no") . "\n" .
	"Use specific data: " . ($USE_SERIF_DATA_DIR ? "yes" : "no") . "\n" .
    "Build shared libs: " . ($BUILD_SHARED_LIBS ? "yes" : "no") . "\n" .
    "  Build serif.jar: " . ($BUILD_JAVA ? "yes" : "no") . "\n" .
    " Assemble release: " . ($ASSEMBLE_RELEASE ? "yes" : "no") . "\n";
$parameter_description .=
    "  Email report to: $MAILTO\n"
    if defined($MAILTO);
my $queue_description =
    "    SVN checkout queue: $PREPARE_QUEUE\n" .
    "    Linux 32-bit queue: $RUN_QUEUE{i686}\n" .
    "    Linux 64-bit queue: $RUN_QUEUE{x86_64}\n" .
    "  Windows 32-bit queue: $RUN_QUEUE{Win32}\n" .
    "  Windows 64-bit queue: $RUN_QUEUE{Win64}\n" .
    "   Windows build queue: $BUILD_QUEUE{Win64}\n" . # <- hmm
    "         Scoring queue: $SCORE_QUEUE\n";
print "$EXPERIMENT_NAME\n";
print "$parameter_description";
print "------------------------------------------------" .
    "------------------------\n";
print "Command-line:\n  $cmdline_description\n";
print "------------------------------------------------" .
    "------------------------\n";

# Decide if we should check out IDX.
my $BUILD_IDX = 0;
foreach my $config (@configurations) {
    $BUILD_IDX = 1 if ($config->{BUILD_TARGET} =~ /IDX/);
}

# Decide if we should check out KbaStreamCorpus.
my $BUILD_KBA_STREAMCORPUS = 0;
foreach my $config (@configurations) {
    $BUILD_KBA_STREAMCORPUS = 1 if ($config->{BUILD_TARGET} =~ /^KbaStreamCorpus/);
}

# Decide if we should require license
if ($ASSEMBLE_RELEASE) {
    push @EXTRA_CMAKE_OPTIONS, "-DFEATUREMODULE_License_INCLUDED=ON";
    push @EXTRA_CLASSIC_MAKE_TARGETS, "create_license";
}

# We automatically  include the Spanish feature module
push @EXTRA_CMAKE_OPTIONS, "-DFEATUREMODULE_Spanish_INCLUDED=ON";

# Use IDX queues if we're building IDX (or a variant)
if ($BUILD_IDX || $BUILD_KBA_STREAMCORPUS) {
    $PREPARE_QUEUE = 'c112';
    %BUILD_QUEUE = %RUN_QUEUE = (
	i686     => 'e-idx-100s',
	x86_64   => 'c112',
	x86_64_sl5   => 'e-idx-100s',
        Win32    => 'win-e-idx-100s',
        Win64    => 'win-e-idx-100s');
}

# Check out the requested revision(s) of SERIF.
my @prepare_jobs = ();
foreach my $revision (@SVN_REVISIONS) {

    my $job_name = "${revision}/Prepare";
    my $svn_dir = "$expdir/Serif/$revision";

    # Decide what subdirectories to check out.
    my @subdirs = qw(scoring bin);
    push @subdirs, "par" if (!$PAR_DIRECTORY);
    push @subdirs, "src" if (!$PREBUILT_EXECUTABLE);
    push @subdirs, "external" if (!$PREBUILT_EXECUTABLE);
    push @subdirs, "data" if ($CHECKOUT_DATA);
    push @subdirs, "doc" if ($ASSEMBLE_RELEASE);
    push @subdirs, "experiments" if ($ASSEMBLE_RELEASE);
    push @subdirs, "idx" if ($BUILD_IDX);
    if ($CUSTOM_DELIVERABLES) {
        ## not a module any more for Serif but pull it in for scripts and library
        push @subdirs, "icews";
        ## and we need the python for the includes of icews test code
        push @subdirs, "python";
        push @subdirs, "awake";
        push @subdirs, "spectrum";
        push @subdirs, "marathon";
    }
    push @subdirs, "kba-streamcorpus" if ($BUILD_KBA_STREAMCORPUS);
    push @subdirs, "accent" if ($ASSEMBLE_RELEASE);

    # Determine the appropriate base URL and svn revision number to
    # use for this revision.
    my ($svn_url, $svn_revision);
    if ($revision =~ /^rev-(.*)/) {
		$svn_revision = $1;
		$svn_url ="$SVN_URL/trunk/Active";
    } elsif ($revision =~ /^tag-(.*)/) {
		$svn_revision = SVN_HEAD;
		$svn_url = "$SVN_URL/tag/$1/Active";
    } elsif ($revision =~ /^branch-(.*)/) {
		$svn_revision = SVN_HEAD;
		$svn_url = "$SVN_URL/branch/$1/Active";
    } elsif ($revision =~ /^local/) {
		print "computing local url from exp_root=$exp_root\n";
		$svn_revision = SVN_LOCAL;
		$svn_url = abs_path("$exp_root/../..");
		die "Local serif code not found in $svn_url/src"
			unless (-d "$svn_url/src");
    } else {
		die "Bad revision $revision!\n";
    }

    die unless defined($svn_url);
    die unless defined($svn_revision);
    push(@prepare_jobs,
	 runjobs( [], $job_name,
		  {
		      BATCH_QUEUE     => $PREPARE_QUEUE,
		      SVN_URL         => $svn_url,
		      SVN_REVISION    => $svn_revision,
		      SERIF_VERSION_STRING => $SERIF_VERSION_STRING,
		      TARGET          => $svn_dir,
		      SUBDIRS         => "@subdirs",
		  },
		  $BASH, "svnget.sh"));
}

# Perform any jobs that we've queued so far.  This is important
# because the template files used by subsequent jobs are checked out
# from svn by the "Prepare" jobs.
$runjobs_scheduler->process_jobs() or report_failure();

my @build_configurations = group_configurations(
    \@configurations, ["SVN_REVISION", "ARCHITECTURE", "LINK_TYPE",
		       "SYMBOL", "BUILD_TARGET", "DEBUG_MODE"]);

# Build and test the selected jobs.  When we're checking throughput,
# we first run all the build jobs; and then run all the test jobs
# in random order (to prevent clustering of the jobs for a single
# configuration, which can skew the results).  If we're not checking
# throughput, then run the test jobs for each build once that
# build finishes.
my (@build_jobs, @regtest_jobs);
if ($PREBUILT_EXECUTABLE) {
    foreach my $config (@configurations) {
		push @regtest_jobs, test_serif([], {}, $config);
    }
} elsif ($CHECK_THROUGHPUT) {
    foreach my $build_config (@build_configurations) {
		push @build_jobs, build_serif(\@prepare_jobs, $build_config->{GROUP});
    }
    unless ($THROUGHPUT_SO_FAR) {
		$runjobs_scheduler->process_jobs() or report_failure();
    }
    foreach my $build_config (@build_configurations) {
		foreach my $config (@{$build_config->{CONFIGS}}) {
			push @regtest_jobs, test_serif(\@build_jobs,
					   $build_config->{GROUP}, $config);
		}
    }
    $runjobs_scheduler->{ready_jobs} =
		[shuffle(@{$runjobs_scheduler->{ready_jobs}})];
} else {
    foreach my $build_config (@build_configurations) {
		my @config_build_jobs = build_serif(\@prepare_jobs,
					    $build_config->{GROUP});
		push @build_jobs, @config_build_jobs;
		foreach my $config (@{$build_config->{CONFIGS}}) {
			push @regtest_jobs, test_serif(\@config_build_jobs,
					   $build_config->{GROUP},
					   $config);
		}
    }
}

if ($BUILD_JAVA) {
    foreach my $revision (@SVN_REVISIONS) {
		push @build_jobs, build_java(\@prepare_jobs, $revision);
    }
}

if ($RUN_DOXYGEN) {
    foreach my $revision (@SVN_REVISIONS) {
		push @build_jobs, run_doxygen(\@prepare_jobs, $revision);
    }
}

# This is a bit of a hack -- it lets us extract throughput results
# out of a running experiment without waiting for it to finish.
if ($THROUGHPUT_SO_FAR) {
    my ($tput_scores) = read_throughput_scores();
    if (%$tput_scores) {
		print scalar(keys %$tput_scores) .
			" configurations have results so far\n";
		report_throughput($tput_scores);
    } else {
		print "No throughput scores yet!\n";
    }
    exit;
}

# Run the queued jobs and report success or failure.
$runjobs_scheduler->process_jobs() or report_failure();
if ($CHECK_THROUGHPUT) {
    my ($tput_scores) = read_throughput_scores();
    report_throughput($tput_scores);
    update_throughput_history($tput_scores) if $SAVE_THROUGHPUT;
} else {
    report_success();
}

############################################################################
# Build & Test
############################################################################

#----------------------------------------------------------------------
# build_serif(\@prev_jobs, \%config) -> @jobids
#
# Run cmake and make to build the serif executables on the given
# configuration.  $config is a hash-ref describing the configuration
# to build.
sub build_serif {
    (my $prev_jobs, my $config) = @_;
    my @jobids = ();

    if (svn_revision_before($config, 30789)) {
	print "This version of build_serif.pl does not support " .
	    "revisions before 30789 (when langauge selection became " .
	    "a runtime option, rather than a compile-time option.";
	exit -1;
    }

    # Decide where to put the build files and output files.
    my $config_name = get_config_name($config);
    my $build_dir   = "$expdir/build/${config_name}";
    my $install_dir = "$expdir/install/${config_name}";
    my $svn_dir     = "$expdir/Serif/$config->{SVN_REVISION}";
    my $external_lib =  "";

    # Build the given architecture.
    my $build_jobid;
    if ($config->{ARCHITECTURE} =~ /win.*/i) {
	$build_jobid = build_windows(
	    $prev_jobs, $config, $svn_dir, $build_dir, $install_dir,
	    $config_name);
    } else {
	$build_jobid = build_linux(
	    $prev_jobs, $config, $svn_dir, $build_dir, $install_dir,
	    $config_name);
    }
    push @jobids, $build_jobid;

    # Copy the binary files to an install directory.
    push @jobids, runjobs([$build_jobid], "${config_name}/install", {
	    SCRIPT       => 1, # (don't spawn a job)
	    build_dir    => $build_dir,
	    install_dir  => $install_dir,
	    external_lib => $external_lib,
	    architecture => $config->{ARCHITECTURE}},
	$BASH, "install_serif.sh");

    return @jobids;
}

#----------------------------------------------------------------------
# test_serif(\@prev_jobs, \%build_config, \%config) -> @jobids
#
# Run a regression test to ensure that the executables work correctly.
# Ace is the default regression test, but also do Icews and Awake.
# $config is a hash-ref describing the configuration to build.
sub test_serif {
    my ($prev_jobs, $build_config, $config) = @_;
    my @jobids = ();

    # Decide where to put the build files and output files.
    my $build_config_name = get_config_name($build_config);
    my $config_name = get_config_name($config);
    my $build_dir   = "$expdir/build/${build_config_name}";
    my $test_dir    = "$expdir/test/${config_name}";
    my $svn_dir     = "$expdir/Serif/$config->{SVN_REVISION}";


    # Run tests
    if (check_conditions($config, \@SKIP_TEST_CONDITIONS) || $BUILD_ONLY) {
		return @jobids;
	}
	if ($CHECK_THROUGHPUT && $THROUGHPUT_RUNS) {
	    foreach my $run (1..$THROUGHPUT_RUNS) {
			push @jobids, regtest(
				$prev_jobs, $config, $svn_dir, $build_dir,
		        "$test_dir/iter$run",
				"$config_name/iter$run");
	    }
	} else { # includes AceXxx, IcewsXxx, AwakeXxx, NoParser, AggParser, FastParser, NormParser, Values
	    push @jobids, regtest(
			$prev_jobs, $config, $svn_dir, $build_dir,
			$test_dir,  $config_name);
	}

    return @jobids;
}

############################################################################
# Build
############################################################################

#----------------------------------------------------------------------
# build_linux(\@prev_jobs, \%config, $svn_dir, $build_dir, $install_dir,
#             $config_name) -> $jobid
#
# Run cmake and make to build the serif executables on the given linux
# architecture for the given language and build type.  This is a
# helper function used by build_serif().
#
sub build_linux {
    my ($prev_jobs, $config, $svn_dir, $build_dir, $install_dir,
	$config_name) = @_;

    # Choose the 'build type' for cmake.
    my $debug_mode = $config->{DEBUG_MODE};
    $debug_mode =~ s/Performance//;
    my $cmake_build_type = ($debug_mode);
    my @cmake_options = choose_cmake_options($config, $install_dir);

    # Choose which version of gcc & libraries to use.
    my $arch_suffix  = $ARCH_SUFFIX{$config->{ARCHITECTURE}};
    my $gcc_home = $GCC_HOME{$config->{ARCHITECTURE}};
    my $gcc_path;
    if ($gcc_home eq "/usr") {
	$gcc_path = "$gcc_home/bin";
    } else {
	$gcc_path = "$gcc_home$arch_suffix/bin";
    }
    my $boost_root   = "$BOOST_HOME{$config->{ARCHITECTURE}}${arch_suffix}";
    my $gperftools_root = "$GPERFTOOLS_HOME{$config->{ARCHITECTURE}}${arch_suffix}";
    my $yamcha_root  = "$YAMCHA_HOME{$config->{ARCHITECTURE}}${arch_suffix}";
    my $cmake_path   = "$CMAKE_HOME{$config->{ARCHITECTURE}}${arch_suffix}/bin";

    # Choose the right target architecture flags
    my $bits = $BITS{$config->{ARCHITECTURE}};
    push @cmake_options, "-DCMAKE_CXX_FLAGS=-m$bits";
    push @cmake_options, "-DCMAKE_C_FLAGS=-m$bits";
    my $linker_options = "-m$bits";
    if ($config->{LINK_TYPE} eq 'Static') {
        $linker_options .= " -static";
    }
    push @cmake_options, "-DCMAKE_EXE_LINKER_FLAGS=\"$linker_options\"";

    # Construct a list of targets for the makefile.
    my @make_targets = (($config->{BUILD_TARGET} =~ /Classic|KbaStreamCorpus_Serif|Encrypt|ProfileGenerator/) ? "Serif" :
			(($config->{BUILD_TARGET} =~ /IDX|KbaStreamCorpus_IDX/) ? "IDX" :
                         "UNKNOWN_BUILD_TARGET"));

    if ($config->{BUILD_TARGET} eq "Classic") {
		push @make_targets, @EXTRA_CLASSIC_MAKE_TARGETS;
    }
    
    if ($config->{BUILD_TARGET} eq "ProfileGenerator") {
	push @make_targets, @EXTRA_PROFILE_GENERATOR_MAKE_TARGETS;
	push @cmake_options, @CMAKE_PROFILE_GENERATOR_OPTIONS;
    }
    if ($config->{BUILD_TARGET} eq "IDX") {
		push @make_targets, @EXTRA_IDX_MAKE_TARGETS;
    }
    if ($BUILD_SHARED_LIBS) {
		push @make_targets, "IDXJNI";
    }
    if ($config->{BUILD_TARGET} =~ /^Classic|^IDX|^KbaStreamCorpus|^Encrypt/) {
		push @make_targets, "BasicCipherStreamEncryptFile";
		push @cmake_options, @CMAKE_ENCRYPT_OPTIONS;
    }

    if ($config->{DEBUG_MODE} eq "DebugPerformance") {
	push @cmake_options, @CMAKE_PERFTOOLS_OPTIONS;
    }

    my $src_dir="$svn_dir/src";
    if ($config->{BUILD_TARGET} =~ /IDX|KbaStreamCorpus_IDX/) {
		push @cmake_options, "-DUSE_PREBUILT_SERIF=OFF";
		push @cmake_options, "-DSERIF_SOURCE_DIR=$src_dir";
        push @cmake_options, @CMAKE_IDX_OPTIONS;
		$src_dir="$svn_dir/idx";
    }
    if ($config->{BUILD_TARGET} =~ /^KbaStreamCorpus/) {
		push @cmake_options, @CMAKE_KbaStreamCorpus_OPTIONS;
    }

    # Run cmake, followed by build.
    my $jobid = runjobs( $prev_jobs, "${config_name}/build",
		 {
		     BATCH_QUEUE      => $BUILD_QUEUE{$config->{ARCHITECTURE}},
		     SGE_VIRTUAL_FREE => "4G",
		     SRC_DIR          => $src_dir,
		     BUILD_DIR        => $build_dir,
		     BUILD_TYPE       => $cmake_build_type,
		     CMAKE_OPTIONS    => join(" ", @cmake_options),
		     ARCH_SUFFIX      => $arch_suffix,
		     BOOST_ROOT       => $boost_root,
		     GPERFTOOLS_ROOT  => $gperftools_root,
		     YAMCHA_ROOT      => $yamcha_root,
		     CMAKE_PATH       => $cmake_path,
		     GCC_PATH         => $gcc_path,
		     MAKE_TARGETS     => join(" ", @make_targets),
		 }, $BASH, "linux_build.sh");

    return $jobid;
}

#----------------------------------------------------------------------
# build_windows(\@prev_jobs, \%config, $svn_dir, $build_dir, $install_dir,
#               $config_name) -> $jobid
#
# Run cmake and make to build the serif executables on the given
# windows architecture for the given language and build type.  This is
# a helper function used by build_serif().
#
sub build_windows {
    my ($prev_jobs, $config, $svn_dir, $build_dir, $install_dir,
	$config_name) = @_;

    print "Config architecture: ".$config->{ARCHITECTURE};
    # Choose the cmake generator.
    my $cmake_generator = $WINDOWS_CMAKE_GENERATOR{$config->{ARCHITECTURE}};
    # believe the install_dir is not used by Win, so flag it empty here
    #my @cmake_options = choose_cmake_options($config, $install_dir);
    my @cmake_options = choose_cmake_options($config, '');

    # Choose which solution, project, and release to use.
    my ($release_name, $project_names) = windows_projects($config);

    # Choose which version of xerces to link against.
    my $external_dir = "$svn_dir/external";
    my $linux_xerces_home = "$external_dir/XercesLib";

    # Choose which version of OpenSSL to link against
    my $linux_openssl_home = "$external_dir/openssl-1.0.1g";

    # Choose which version of yamcha to link against.
    my $windows_yamcha_home = $YAMCHA_HOME{$config->{ARCHITECTURE}};

    # Classic now builds with optional ICEWS stages baked in, but we need the dependencies
	if ($config->{BUILD_TARGET} eq "Classic") {
	#push @cmake_options, @WINDOWS_CMAKE_ICEWS_OPTIONS;  ## zapped 5 oct 2015 
    }
    if ($config->{BUILD_TARGET} =~ /^Encrypt/) {
	push @cmake_options, @CMAKE_ENCRYPT_OPTIONS;
    }
    if ($config->{BUILD_TARGET} =~ /^KbaStreamCorpus/) {
	push @cmake_options, @CMAKE_KbaStreamCorpus_OPTIONS;
    }

    my $src_dir="$svn_dir/src";
    if ($config->{BUILD_TARGET} =~ /IDX/) {
	push @cmake_options, "-DUSE_PREBUILT_SERIF=OFF";
	push @cmake_options, "-DSERIF_SOURCE_DIR=" . unix2win($src_dir);
	push @cmake_options, "-DJAVA_AWT_INCLUDE_PATH=\"C:\\Progra~1\\Java\\jdk1.6.0_33\\include\"";
	push @cmake_options, "-DJAVA_AWT_LIBRARY=\"C:\\Progra~1\\Java\\jdk1.6.0_33\\lib\\jawt.lib\"";
	push @cmake_options, "-DJAVA_INCLUDE_PATH=\"C:\\Progra~1\\Java\\jdk1.6.0_33\\include\"";
	push @cmake_options, "-DJAVA_INCLUDE_PATH2=\"C:\\Progra~1\\Java\\jdk1.6.0_33\\include\"";
	push @cmake_options, "-DJAVA_JVM_LIBRARY=\"C:\\Progra~1\\Java\\jdk1.6.0_33\\lib\\jvm.lib\"";
	push @cmake_options, @CMAKE_IDX_OPTIONS;
	$src_dir="$svn_dir/idx";
	push @cmake_options, @CMAKE_ENCRYPT_OPTIONS;
    }

    # Format the project names as a Python dictionary
    my $project_names_dict = "{";
    foreach my $solution (keys(%{$project_names})) {
	$project_names_dict .= "r'$solution':[";
	foreach my $project (@{$project_names->{$solution}}) {
	    $project_names_dict .= "r'$project',";
	}
	$project_names_dict .= "],";
    }
    $project_names_dict .= "}";

    my $jobid = runjobs( $prev_jobs, "${config_name}/build",
	    {
		BATCH_QUEUE      => $BUILD_QUEUE{$config->{ARCHITECTURE}},
		SGE_VIRTUAL_FREE => "4G",
		build_dir        => unix2win($build_dir),
		src_dir          => unix2win($src_dir),
		cmake_generator  => $cmake_generator,
		cmake_options    => join(" ", @cmake_options),
		build_drive      => $WINDOWS_BUILD_DRIVE,
		boost_version    => $BOOST_VERSION{$config->{ARCHITECTURE}},
		xerces_root      => unix2win($linux_xerces_home),
		openssl_root     => unix2win($linux_openssl_home),
		yamcha_root      => $windows_yamcha_home,
		release_name     => $release_name,
		project_names    => $project_names_dict,
	    }, [$PYTHON{$config->{ARCHITECTURE}}, "win_build.py"]);

    return $jobid;
}

#----------------------------------------------------------------------
# build_java(\@prev_jobs, $revision) -> $jobid
#
# Build java stuff.
#
sub build_java {
    my ($prev_jobs, $revision) = @_;
    my $src_dir = "$expdir/Serif/$revision/java";
    my $doc_dir = "$expdir/Serif/$revision/doc";
    my $dst_dir = "$expdir/build/$revision/java";
    return runjobs( $prev_jobs, "${revision}/build_java",
	    {
		BATCH_QUEUE      => $BUILD_QUEUE{"x86_64"},
		SGE_VIRTUAL_FREE => "1G",
		src_dir          => $src_dir,
		doc_dir          => $doc_dir,
		dst_dir          => $dst_dir
	    }, $BASH, "java_build.sh");
}

#----------------------------------------------------------------------
# run_doxygen(\@prev_jobs, $revision) -> $jobid
#
# Run doxygen to generate HTML documentation for the code base
#
sub run_doxygen {
    my ($prev_jobs, $revision) = @_;
    my $src_dir = "$expdir/Serif/$revision/src";
    my $dst_dir = "$expdir/doc/$revision";
    my $mkdir_jobid = runjobs($prev_jobs, "${revision}/doxygen/mkdir",
                              {SCRIPT => 1, dir => $dst_dir},
                              $BASH, "mkdir.sh");
    return runjobs( [$mkdir_jobid], "${revision}/doxygen/run",
	    {
		BATCH_QUEUE      => $BUILD_QUEUE{"x86_64"},
		SGE_VIRTUAL_FREE => "1G", # probably overkill
		revision         => $revision,
		src_dir          => $src_dir,
		dst_dir          => $dst_dir
	    }, $DOXYGEN, "serif.doxygen");
}

############################################################################
# Regression Testing
############################################################################

#----------------------------------------------------------------------
# regtest(\@prev_jobs, \%config, $svn_dir, $build_dir,
#         $test_dir,  $config_name) -> $jobid
#
# Run a regression test (Ace, ICEWS, or Awake) on the SERIF binary with the given
# architecture, language, and build type.  This is a helper function
# used by build_serif().
#
sub regtest {
    my ($prev_jobs, $config, $svn_dir, $build_dir,
	    $test_dir,  $config_name) = @_;
	my $rt_test = "Ace";
	if ($config->{PARAMETERS} =~ /Icews/) { 
	 	$rt_test = "Icews";
	} elsif ($config->{PARAMETERS} =~ /Awake/) { 
		$rt_test = "Awake";
	}
    # Get the full path to the serif binary for this configuration.
    my $serif_bin = $PREBUILT_EXECUTABLE;
    if (!$serif_bin) {
		$serif_bin = "$build_dir/" . serif_binary($config);
    }

    # Set several variables that depend on whether we're testing a
    # windows binary or a linux binary:
    my ($serif_data, $serif_regression, $os_suffix, $path_convert,
		$testdata_date, $perl_bin);
    if ($config->{ARCHITECTURE} =~ /win.*/i) {
		# ------- Windows --------
		$serif_data = $WINDOWS_SERIF_DATA;
		$serif_regression = $WINDOWS_SERIF_REGRESSION;
		$perl_bin = $PERL{$config->{ARCHITECTURE}};
		$path_convert = \&File::PathConvert::unix2win;
		$os_suffix = "-Windows";
		$testdata_date = (($config->{BUILD_TARGET} =~ /IDX/)?
			  "${IDX_WINDOWS_TESTDATA_DATE}":
			  "${WINDOWS_TESTDATA_DATE}");
    } else {
		# ------- Linux --------
		$serif_data = $LINUX_SERIF_DATA;
		$serif_regression = $LINUX_SERIF_REGRESSION;
		$perl_bin = $PERL{$config->{ARCHITECTURE}};
		$path_convert = (sub {shift;});
		$os_suffix = "-Linux";
		$testdata_date = (($config->{BUILD_TARGET} =~ /IDX/)?
			  "${IDX_LINUX_TESTDATA_DATE}":
			  "${LINUX_TESTDATA_DATE}");
    }

    # Use the local data directory, if we checked it out.  (Otherwise,
    # we use the windows/linux shared location defined above.)
    if (-e "$svn_dir/data") {
		$serif_data = &$path_convert("$svn_dir/data");
    }

    # Choose a template and a score file
    my $template = choose_template($config);
    my @stages = choose_stages($config);
    my $score_file = choose_score_file($config);

    my $template_dir = ($PAR_DIRECTORY ? "$PAR_DIRECTORY" : "$svn_dir/par");
    my $template_path = "$template_dir/$template";
    my ($serif_batch_output_dirs, $batch_input_files) =
		extract_serif_output_dirs($template_dir, $template_path,
				  $serif_regression, $os_suffix);

    # Create the experiment directory (and delete its contents, if any)
    my $mkdir_jobid = runjobs($prev_jobs, "${config_name}/mkdir",
                              {SCRIPT => 1, dir => $test_dir},
                              $BASH, "mkdir.sh");

    # Start a job to process each batch directory.
    my $batch_num = 0;
    my @test_jobs = ();
    my @compare_icews_events_jobs = ();
    my @dump_sql_and_html_jobs = ();
    my $dump_file_name = "icews_sql_dump";
    my $testdata_dir = choose_testdata_dir($config, $testdata_date);
    foreach my $serif_output_dir (@$serif_batch_output_dirs) {
        my $parallel = sprintf("%03d", $batch_num);
	
		my $batch_file = $batch_input_files->{$serif_output_dir};

		# Run SERIF on the selected batch of files.
		my $serif_run_name = "${config_name}/run_serif-${parallel}";
		my ($run_serif_job, $logfiles) = run_serif(
			[$mkdir_jobid], $serif_run_name,
			{
				BATCH_QUEUE        => $RUN_QUEUE{$config->{ARCHITECTURE}},
				SGE_VIRTUAL_FREE   => $RUN_SGE_VIRTUAL_FREE{$config->{ARCHITECTURE}},
				ARCH_SUFFIX        => $ARCH_SUFFIX{$config->{ARCHITECTURE}},
				parallel           => $parallel,
				expt_dir           => &$path_convert($test_dir),
				serif_data         => $serif_data,
				serif_regression   => $serif_regression,
				os_suffix          => $os_suffix,
				serif_score        => &$path_convert("$svn_dir/scoring"),
				perl_binary        => $perl_bin,
				par_dir            => &$path_convert($template_dir),
				# used by non-Ace (Icews and maybe later Awake) runs 
				icews_lib_dir      => &$path_convert("$svn_dir/icews/lib"),
				awake_lib_dir      => &$path_convert("$svn_dir/awake/lib"),
				icews_sqlite_db    => &$path_convert("icews.current.sqlite"),
				# These are used by the throughput par files:
				CACHE_ENTRIES      => 1000,
				CACHE_TYPE         => "simple",
			},
			&$path_convert($serif_bin), $template_path,
			\@stages);
		push @test_jobs, $run_serif_job;

		# Depending on the command-line arguments we were called with,
		# we will now perform one of three actions:
		#   a) "-tput": Extract throughput scores from the log files
		#   b) Otherwise: Check the regression test results
		#   b-1) is Ace testing
		#   b-2) is Icews or Awake
		if ($CHECK_THROUGHPUT) {
			push @test_jobs, check_throughput(
				[$run_serif_job], $config, $config_name, $parallel, $test_dir,
				$serif_output_dir, $batch_input_files, $logfiles);

		} elsif ($rt_test eq "Ace") {
			my $summarize_score_serif_job = summarize_scores(
				[$run_serif_job], $config, $config_name, $parallel,
				$test_dir, $serif_output_dir, $score_file);
			push @test_jobs, $summarize_score_serif_job;

			push @test_jobs, check_regtest(
				[$summarize_score_serif_job], $config_name, $parallel,
				$test_dir, $testdata_dir, $serif_output_dir, $score_file);

		}  elsif ($rt_test eq "Icews")  {
			# Icews needs to run a comparison of events and output files

			my $compare_events_job = compare_events (
				[ $run_serif_job ], $config, $config_name, $parallel, $test_dir,
				$testdata_dir, $serif_output_dir, $svn_dir);
			push @test_jobs, $compare_events_job;
			push @compare_icews_events_jobs, $compare_events_job;

			my $sqlite_output_file = "$test_dir/$serif_output_dir/$dump_file_name";
			## my $icews_sql_db = &$path_convert("$test_dir/$serif_output_dir/icews-output.sqlite");
			my $icews_sql_db = "$test_dir/$serif_output_dir/icews-output.sqlite";
			my $dump_sql_job = dump_icews_sql (
				[ $run_serif_job ], $config_name, $parallel, $test_dir,
				$testdata_dir, $serif_output_dir, $icews_sql_db, 
				$svn_dir, $sqlite_output_file);
			push @test_jobs, $dump_sql_job;
			push @dump_sql_and_html_jobs, $dump_sql_job;

			## dump the events in a nice display format for later debugging
			my $dump_html_job = dump_icews_html (
				[ $run_serif_job ], $config, $config_name, $parallel, $test_dir,
				$testdata_dir, $serif_output_dir, $svn_dir);
			push @test_jobs, $dump_html_job;
			push @dump_sql_and_html_jobs, $dump_html_job;
			
			push @test_jobs, check_regtest(
				[$dump_sql_job], $config_name, $parallel,
				$test_dir, $testdata_dir, $serif_output_dir, $score_file);

		}  elsif ($rt_test eq "Awake")  {
			# Awake only comnpares serif_xml currently
			push @test_jobs, check_regtest(
				[$run_serif_job], $config_name, $parallel,
				$test_dir, $testdata_dir, $serif_output_dir, $score_file);
		} else {
			print "Don't know how to do test $rt_test.\n";
		}

        $batch_num++;
    }

    if ($rt_test eq "Icews") {  
        # ICEWS checks the merged and sorted SQL commands for changes
	# This one creates the compare directory, so we do it before calling concat_compare_events
	my $sql_compare_id = compare_icews_sql(
	    \@dump_sql_and_html_jobs, $config, $config_name, $test_dir,
	    $testdata_dir, $svn_dir, $dump_file_name);
	push @test_jobs, $sql_compare_id;
	push @compare_icews_events_jobs, $sql_compare_id;

	# Concat all the comparison files for easy viewing after the fact
	push @test_jobs, concat_compare_events(\@compare_icews_events_jobs, $config, $config_name, $test_dir);
    }

    return @test_jobs;
}

#----------------------------------------------------------------------
# run_serif(\@prev_jobs, $job_name_prefix, $params,
#           $serif_bin, $template, $stages) -> ($jobid, \@logfiles)
#
# Run SERIF, splitting it into multiple jobs if necessary (e.g., for
# Arabic serif on 32-bit windows).  The argument @$stages is a list of
# job specifications of the form "startstage-endstage"
#
# See also: choose_stages().
sub run_serif {
    my ($prev_jobs, $job_name_prefix, $params,
		$serif_bin, $template, $stages) = @_;
    my ($jobid, @logfiles);

    foreach my $stage (@$stages) {
		my ($start_stage, $end_stage) = split(/_/, $stage);

		my %params_copy = %$params;
		$params_copy{"start_stage"} = $start_stage;
		$params_copy{"end_stage"} = $end_stage;

		# If this job is not finishing with END, then be sure to save
		# our results so the next job can resume where we left off.
		# Otherwise, don't bother to save our results.
		if ($SAVE_STATE) {
			$params_copy{"state_saver_stages"} = $ALL_STATE_SAVER_STAGES;
		} elsif ($end_stage !~ /^(END|output)$/) {
			$params_copy{"state_saver_stages"} = $end_stage;
		} else {
			$params_copy{"state_saver_stages"} = "NONE";
		}

		$params_copy{"job_retries"} = 2;

		my $job_name = $job_name_prefix;
		$job_name .= "-$stage" if ($stage !~ /^START_(END|output)$/);
		#$job_name .= "-$stage";
		if ($FORCE_SERIFXML) {
			$jobid = runjobs($prev_jobs, $job_name, \%params_copy,
							 [$serif_bin, $template, "-p output_format=serifxml"]);
		} else {
			$jobid = runjobs($prev_jobs, $job_name, \%params_copy,
							 [$serif_bin, $template]);
		}
		$prev_jobs = [$jobid];
		my $logfile = "$logdir/$job_name.log";
		$logfile =~ s{(.*)/(.*)}{$1/$exp-$2};
		push @logfiles, $logfile;
    }
    return $jobid, \@logfiles;
}

sub summarize_scores {
    my ($prev_jobs, $config, $config_name, $parallel, $test_dir,
	$serif_output_dir, $score_file) = @_;
    return runjobs(
	$prev_jobs, "${config_name}/summarize-score-${parallel}",
	  {
	    BATCH_QUEUE        => $SCORE_QUEUE,
	    SGE_VIRTUAL_FREE   => "500M",
	    out_dir            => "$test_dir/$serif_output_dir",
	    SCORE_FILE         => $score_file,
	    SCORE_SUMMARY_FILE => "score-summary.txt",
	  },
	  $PERL{$SCORE_ARCHITECTURE}, "summarize_scores.pl");
}


sub check_regtest {
    my ($prev_jobs, $config_name, $parallel, $test_dir,
	$testdata_dir, $serif_output_dir, $score_file) = @_;
    return runjobs(
	  $prev_jobs, "${config_name}/check-${parallel}",
	  {
	    SCRIPT             => 1, # (don't spawn a job)
	    out_dir            => "$test_dir/$serif_output_dir",
	    testdata_dir       => "$testdata_dir/$serif_output_dir",
	    SCORE_SUMMARY_FILE => "score-summary.txt",
	  },
	  $BASH, "check_scores.sh");
}
sub dump_icews_html {
    my ($prev_jobs, $config, $config_name, $parallel, $test_dir,
	$testdata_dir, $serif_output_dir, $svn_dir) = @_;
	my $compare_dir = "$test_dir/$serif_output_dir/compare_events";
	my $display_dir = "$test_dir/display";
	## incoming serif_output_dir is a simple name like "icews3"
	my $compare_outfile = "$compare_dir/icews_ref-vs-regtest.html";
	my $display_outfile = "$display_dir/$serif_output_dir.html";
	my $compare_serif_events_script = "$svn_dir/icews/scripts/compare_serif_events.py";
	return runjobs(
		$prev_jobs, "${config_name}/dump_html-${parallel}",
		{ BATCH_QUEUE       => $SCORE_QUEUE,
		  parallel          => $parallel,
	      serif_output_dir  => "$test_dir/$serif_output_dir/output",
		  compare_events    => $compare_serif_events_script,
		  display_out_dir   => $display_dir,
		  display_out_file  => $display_outfile,
		  PYTHON            => $PYTHON{$SCORE_ARCHITECTURE},
		  LD_LIBRARY_PATH   => $PYTHON_LIB{$SCORE_ARCHITECTURE},
	    },
	    "$BASH", "dump_html.sh");
}
sub compare_events {
    my ($prev_jobs, $config, $config_name, $parallel, $test_dir,
		$testdata_dir, $serif_output_dir, $svn_dir) = @_;
    my $compare_dir = "$test_dir/$serif_output_dir/compare_events";
    ## incoming serif_output_dir is a simple name like "icews3"
    my $compare_outfile = "$compare_dir/icews_ref-vs-regtest.html";
    my $compare_serif_events_script = "$svn_dir/icews/scripts/compare_serif_events.py";
    return runjobs(
	$prev_jobs, "${config_name}/check_events-${parallel}",
	{ BATCH_QUEUE       => $SCORE_QUEUE,
	  parallel          => $parallel,
	  serif_output_dir  => "$test_dir/$serif_output_dir/output",
	  compare_events    => $compare_serif_events_script,
	  compare_out_dir   => $compare_dir,
	  compare_out_file  => $compare_outfile,
	  testdata_dir      => "$testdata_dir/$serif_output_dir/output",
	  PYTHON            => $PYTHON{$SCORE_ARCHITECTURE},
	  LD_LIBRARY_PATH   => $PYTHON_LIB{$SCORE_ARCHITECTURE},
	},
	"$BASH", "compare_events.sh");
}
sub concat_compare_events {
    my ($prev_jobs, $config, $config_name, $test_dir) = @_;
    my $outfile = "$test_dir/compare/icews_ref-vs-regtest.ALL.html";
    return runjobs(
	$prev_jobs, "${config_name}/concat_compare_events",
	{ BATCH_QUEUE       => $SCORE_QUEUE,
	  test_dir  => $test_dir,
	  out_file  => $outfile,
	  PYTHON            => $PYTHON{$SCORE_ARCHITECTURE},
	  LD_LIBRARY_PATH   => $PYTHON_LIB{$SCORE_ARCHITECTURE},
	},
	"$BASH", "concat_icews_compare_events.sh");    
}
sub dump_icews_sql {
	my ( $prev_jobs, $config_name, $parallel, $test_dir,
		 $testdata_dir, $serif_output_dir,  
		 $icews_sqlite_db, $svn_dir, $sqlite_out_file) = @_;
	return runjobs(
		$prev_jobs, "${config_name}/dump_sql-${parallel}",
		{  BATCH_QUEUE     => $SCORE_QUEUE,
		   DB_SOURCE       => "$icews_sqlite_db",
		   DST             => "$sqlite_out_file",
		},
		$BASH, "dump_sqlite_db.sh");
}

sub compare_icews_sql {
	my ( $prev_jobs, $config, $config_name, $test_dir,
		 $testdata_dir,
		 $svn_dir, $dump_file_name) = @_;
		my $compare_all_dir_name = "compare";
	return runjobs(
		$prev_jobs, "${config_name}/compare_icews_sql",
		{  BATCH_QUEUE       => $SCORE_QUEUE,
		   PERL              => $PERL{$SCORE_ARCHITECTURE},
		   TEST_DIR          => $test_dir,
		   TESTDATA_DIR      => $testdata_dir,
		   SQL_SOURCE_TAIL   => "$dump_file_name",
		   SORTED_SQL_NAME   => "sorted_icews.sql",
           COMPARE_ALL_DIR_NAME  => "$compare_all_dir_name",
           SQL_DIFFS_NAME    => "sorted_sql_vs_reference",
           HTML_DIFFS_NAME   => "all_html_vs_reference",
		},
		$BASH, "compare_icews_sql.sh");
}

sub check_throughput {
    my ($prev_jobs, $config, $config_name, $parallel, $test_dir,
	$serif_output_dir, $batch_input_files, $logfiles) = @_;
    my $tput_file = "$test_dir/$serif_output_dir/throughput.txt";
    return runjobs(
	$prev_jobs, "${config_name}/tput-${parallel}",
	{
	    BATCH_QUEUE      => $SCORE_QUEUE,
	    logfiles         => join("\n", @$logfiles),
	    batch_input_file => win2unix($batch_input_files->{$serif_output_dir}),
	    tput_file        => $tput_file,
	    LD_LIBRARY_PATH  => $PYTHON_LIB{$SCORE_ARCHITECTURE},
	},
	$PYTHON{$SCORE_ARCHITECTURE}, "check_throughput.py");
}

############################################################################
# Helper Functions
############################################################################

#----------------------------------------------------------------------
# svn_revision_before(\%config, $rnum)
#
# Return true if the given revision is a revision that comes before
# revision rnum.  This is used for backwards compatibility (eg for
# when make target names changed).
sub svn_revision_before {
    my ($config, $num) = @_;
    if ($config->{SVN_REVISION} =~ /^rev-(\d+)$/i) {
	if ($1 < $num) {
	    return 1;
	}
    }
    return 0;
}

sub even_items {
    my $n = 0;
    return grep{!($n++ % 2)} @_;
}

#----------------------------------------------------------------------
# group_configurations(\@configurations, \@group_by) -> @grouped
#
# Given a list of configurations and a list of config field names
# (such as SVN_REVISION or ARCHITECTURE), return a list that
# groups the configurations by the specified config field values.
# In particular, return a list of hashref elements:
#   {GROUP=>group, CONFIGS=>configs}
# Where `group` is a configuration hash containing only the
# specified fields, and `configs` is a list of configuration hashes
# that match those fields.  E.g.:
#
#   {GROUP   => {SVN_REVISION=>r1234},
#    CONFIGS => [ {SVN_REVISION=>r1234, ARCHITECTURE=>i686,  ...},
#                 {SVN_REVISION=>r1234, ARCHITECTURE=>Win64, ...}, ... ]}
sub group_configurations {
    my ($configs, $group_by) = @_;

    my %name_to_group;
    my %name_to_items;

    foreach my $config (@$configs) {
	my $group_name = join("/", map {$config->{$_}} @$group_by);
	my %group;
	foreach my $field (@$group_by) {
	    $group{$field} = $config->{$field};
	}
	$name_to_group{$group_name} = \%group;
	if (!$name_to_items{$group_name}) {
	    $name_to_items{$group_name} = []
	}
	push @{$name_to_items{$group_name}}, $config;
    }

    my @groups; # Return value.
    foreach my $name (keys %name_to_group) {
        push @groups, {GROUP => $name_to_group{$name},
                       CONFIGS => $name_to_items{$name}};
    }
    return @groups;
}

#----------------------------------------------------------------------
# make_configuration_list -> @configurations
#
# Create a list of configurations we should process, based on the
# command-line -build arguments.
sub make_configuration_list {
    my %configurations = (); # maps config name -> config hashref
    my $svn_head_revision;

    # Process each configuration specification from the command line.
    foreach my $config_spec (@CONFIG_SPECS) {
        my %config_values = (); # maps field name -> list of values.

		next if $config_spec eq "NONE";

        # Fill in the requested configuration values.
        my @pieces = split(/[\/\\\+]/, $config_spec);
        foreach my $piece (@pieces) {
			next if $piece eq "ALL";
			my ($field, @value) = lookup_config_field($piece);
			push @{$config_values{$field}}, @value;
        }

        # Any unspecified field gets the default values.
	foreach my $field (@CONFIG_FIELD_NAMES) {
	    if (!defined($config_values{$field})) {
		my $defaults = ($DEFAULT_CONFIG_VALUES{$field} ||
				[keys %{$CONFIG_FIELD_VALUES{$field}}]);
		$config_values{$field} = $defaults;
	    }
        }

        # Now build all combinations.
        my @new_configs = ({});
        foreach my $field (keys %config_values) {
	    my $field_values = $config_values{$field};
	    my @expanded_configs = ();
	    foreach my $field_value (@$field_values) {
		foreach my $config (@new_configs) {
		    my %expanded_config = %$config;
		    $expanded_config{$field} = $field_value;
		    push @expanded_configs, \%expanded_config;
		}
	    }
	    @new_configs = @expanded_configs;
        }

	# And add them to our list.
        foreach my $config (@new_configs) {
	    $configurations{get_config_name($config)} = $config;
	    if ($config->{SVN_REVISION} eq SVN_HEAD) {
		$svn_head_revision = svn_head_revision()
		    unless defined($svn_head_revision);
		$config->{SVN_REVISION} = $svn_head_revision;
	    }
        }
    }

    # Remove any configurations that we should skip; and initialize
    # the skip/incl description variables.
    foreach my $config_name (sort keys %configurations) {
	my $config = $configurations{$config_name};
	if (check_conditions($config, \@SKIP_BUILD_CONDITIONS)) {
	    print "  Skip: $config_name\n" if $VERBOSE>1;
	    $skip_config_description .= "  * $config_name\n";
	    delete $configurations{$config_name};
	} else {
	    print " Build: $config_name\n" if $VERBOSE;
	    $incl_config_description .= "  * $config_name\n";
	    unless ($BUILD_ONLY) {
			if (check_conditions($config,\@SKIP_TEST_CONDITIONS)){
				print "  Skip regtest: $config_name\n" if $VERBOSE>1;
				$skip_regtest_description .= "  * $config_name\n";
			} else {
				$incl_regtest_description .= "  * $config_name\n";
			}
	    }
	}
    }

    # Return the configurations as a sorted list.
    return (sort {get_config_name($a) cmp get_config_name($b)}
	    values(%configurations));
}

#----------------------------------------------------------------------
# lookup_config_field($requested_value) -> ($field_name, $normalized_value)
#
# Return the configuration field that defines the given value.
# E.g., lookup_config_field("English") returns "LANGUAGE".
sub lookup_config_field {
    my $requested_value = shift;
    foreach my $field (@CONFIG_FIELD_NAMES) {
	my $field_values = $CONFIG_FIELD_VALUES{$field};
	foreach my $field_value (keys %$field_values) {
	    my $field_value_regexp = "^$field_value\$";
	    if ($requested_value =~ /$field_value_regexp/i) {
		if ($field eq "SVN_REVISION") {
		    return ($field, normalize_revision($requested_value));
		} else {
		    return ($field, $field_value);
		}
	    }
	}
    }
    die "Bad configuration field value \"$requested_value\"!";
}

#----------------------------------------------------------------------
# config_values($field) -> @values
#
# Return the list of configuration values that are used for the given
# field.
sub config_values {
    (my $field) = @_;
    my %values;
    foreach my $config (@configurations) {
	$values{$config->{$field}} = 1;
    }
    return sort keys %values;
}

#----------------------------------------------------------------------
# choose_cmake_options(\%config, $install_dir) -> @cmake_options
sub choose_cmake_options {
    my ($config, $install_dir) = @_;

    my @cmake_options = ();

	if ($install_dir) {
		push @cmake_options,"-DCMAKE_INSTALL_PREFIX=$install_dir";
	}

    if ($LINK_MT_DECODER) {
        push @cmake_options, "-DUSE_MTDecoder=ON";
    }

    die "Unknown SYMBOL value $config->{SYMBOL}"
	if not defined($SYMBOL_CMAKE_OPTIONS{$config->{SYMBOL}});
    push @cmake_options, @{$SYMBOL_CMAKE_OPTIONS{$config->{SYMBOL}}};

    push @cmake_options, @EXTRA_CMAKE_OPTIONS;
	print "Cmake options:\n";
	foreach my $option (@cmake_options) {
		print "\t".$option."\n";
	}
    return @cmake_options;
}

#----------------------------------------------------------------------
# choose_template(\%config) -> $filename
sub choose_template {
    my $config = shift;
    my $language = $config->{LANGUAGE};
    my $parfile = $config->{PARAMETERS};
    my $key = "${language}_${parfile}";
    my $template = $SERIF_TEMPLATES{$key};
    if (!defined($template)) {die "No template for: $key";}
    return $template;
}

#----------------------------------------------------------------------
# choose_score_file(\%config) -> $filename
sub choose_score_file {
    my $language = shift->{LANGUAGE};
    my $score_file = $SCORE_FILES{$language};
    die "No score file for: ${language}" unless (defined($score_file));
    return $score_file;
}

#----------------------------------------------------------------------
# choose_stages(\%config) -> $@stages
#
# Each element in the returned array has the form "i-j", where "i"
# is a start stage and "j" is an end stage.  E.g.: "values-npchunk".
sub choose_stages {
    my $config = shift;
    my $language = $config->{LANGUAGE};
    my $arch = $config->{ARCHITECTURE};
    my $end_stage = "END";
    if ($config->{PARAMETERS} eq "Values") {
	$end_stage = "values";
    } elsif ($CHECK_THROUGHPUT) {
	$end_stage = "output";
    }

    # 32-bit arabic takes up a lot of memory, which can cause us to
    # die if we run out.  So split the serif run into 3 parts.
    if (($language eq "Arabic") && ($arch eq "Win32")) {
	return ("START_nested-names", "values_npchunk", "dependency-parse_$end_stage");
    } else {
	return ("START_$end_stage");
    }
}

#----------------------------------------------------------------------
# serif_binary(\%config) -> $filename
#
# Return the path to the serif binary with the given
# configuration.
sub serif_binary {
    my $config = shift;
    if ($config->{ARCHITECTURE} =~ /win.*/i) {
		if ($config->{BUILD_TARGET} =~ /Classic|KbaStreamCorpus_Serif|ProfileGenerator/) {
			return ("SerifMain/$config->{DEBUG_MODE}/Serif.exe");
		} elsif ($config->{BUILD_TARGET} =~ /IDX|KbaStreamCorpus_IDX/) {
			return ("IDX/$config->{DEBUG_MODE}/IDX.exe");
		} else { 
			die "Bad build modifier $config->{BUILD_TARGET}"; 
		}
    } else {
		if ($config->{BUILD_TARGET} =~ /Classic|KbaStreamCorpus_Serif|ProfileGenerator/) {
			return "SerifMain/Serif";
		} elsif ($config->{BUILD_TARGET} =~ /IDX|Serif_IDX/) {
			return "IDX/IDX";
		} else { 
			die "Bad build modifier $config->{BUILD_TARGET}"; 
		}
    }
}

#----------------------------------------------------------------------
# windows_projects(\%config) -> ($sln, $release, \@project_names)
#
# Return the Visual Studio solution file, the release name, and the
# project name(s) that should be used to build a given version of
# Serif on Windows.
sub windows_projects {
    my $config = shift;

    my %project_names = ();
    my $release_name = "$config->{DEBUG_MODE}";
    my $solutions_dir = "CMake/Solutions";
    if ($config->{BUILD_TARGET} =~ /Classic|Encrypt|KbaStreamCorpus_Serif/) {
		push(@{$project_names{"CMake/Solutions/Serif/Serif.sln"}}, "Serif");
		if ($config->{BUILD_TARGET} !~ /Classic/) {
			push(@{$project_names{"CMake/Solutions/Standalones/Standalones.sln"}}, "BasicCipherStreamEncryptFile");
		}
    } elsif ($config->{BUILD_TARGET} =~ /ProfileGenerator/) {
	push(@{$project_names{"CMake/Solutions/ProfileGenerator/ProfileGenerator.sln"}}, ("Serif", "DocSim", "ProfileGenerator"));
    } elsif ($config->{BUILD_TARGET} =~ /IDX|KbaStreamCorpus_IDX/) {
	push(@{$project_names{"Solutions/IDX/IDX.sln"}}, "IDX");
	push(@{$project_names{"SERIF_IMPORT/CMake/Solutions/Standalones/Standalones.sln"}}, ("create_license", "print_license", "verify_license", "BasicCipherStreamEncryptFile"));
    } else {
	die("Bad build target $config->{BUILD_TARGET}");
    }
    return ($release_name, \%project_names);
}

#----------------------------------------------------------------------
# testdata_dir(\%config, $date) -> $path
#
# Return the path to the saved regression test output for the given
# configuration for the given date.
sub choose_testdata_dir {
    my ($config, $date) = @_;
    my $testdata_home = $TESTDATA_HOME;
    if ($config->{BUILD_TARGET} =~ /IDX/) {
		$testdata_home = $IDX_TESTDATA_HOME;
    }
    if ($date lt "100725") {
		my $params = $config->{PARAMETERS};
		my $speed_suffix = ($FAST_PARSER_PARAMETERS{$params})?"Fast":"Normal";
		my $speed = $config->{SPEED_SETTING};
		my $arch = $config->{ARCHITECTURE};
		my $arch_prefix = $ARCH_PREFIX{$arch};
		my $arch_suffix = $ARCH_SUFFIX{$arch};
		return "${testdata_home}/${arch_prefix}$config->{LANGUAGE}/" .
			"regtest$date/output${speed_suffix}${arch_suffix}";
    } else {
		my $config_name = get_config_name($config);
		my ($rev, $build_name) = split(m(/), $config_name, 2);
		if ($date lt "100802") {
			my $params = $config->{PARAMETERS};
			my $speed = ($FAST_PARSER_PARAMETERS{$params})?"Fast":"Slow";
			$build_name =~ s{([^/]+/[^/]+)}{$1/$speed};
			$build_name =~ s{(Fast/.*)/Fast}{$1/Best};
		}
		## this name MAY need tweaking to reflect the Ace|Icews|Awake testing options... ???
                ## also, we added "Dynamic" or "Static" to the build name, but this is not part of the test data path, so we have to remove it
                $build_name =~ s{Dynamic/|Static/}{};
		return "${testdata_home}/regtest/$date/${build_name}";
    }
}

#----------------------------------------------------------------------
# get_param_lines($template_dir, $template_path) -> (\@param_lines)
#
# Read the given template file, and extract the parameter lines from
# it, following any include paths, recursively. We assume that all
# include paths are in the same directory and are specified exactly as
# INCLUDE +par_dir+/foo (or %par_dir%), since relative paths will not work
# in the context of runjobs. This technically allows nested param files
# but they are not recommended where they can be avoided, for clarity's sake.
sub get_param_lines {
    (my $template_dir, my $template_path) = @_;
    my @param_lines;
    my $IN;
    open ($IN, "$template_path") ||
	die "\n\nTemplate not found: $template_path\n\n";
    while (<$IN>) {
	chomp;
	if (/^\#/) {
	    next;
	} elsif (/^INCLUDE\s+(.*)/) {
	    my $include_path = $1;
	    $include_path =~ s/[\+\%]par_dir[\+\%]/$template_dir/;
	    push @param_lines,  get_param_lines($template_dir, $include_path);
	} else {
	    push @param_lines, $_;
	}
    }
    close $IN;
    return @param_lines;
}

#-----------------------------------------------------------------------
#get_param_value(\@param_lines, $tag) -> param_val_of_tag
#
# Search the collected parameter file lines, returning the value for the $tag
# and obeying any OVERRIDE or UNSET commands 
sub get_param_value {
	(my $param_lines_ref, my $tag) = @_;
	my $pval;
	my $safe_tag = quotemeta($tag);
	my @param_lines = @$param_lines_ref;
 	# implement OVERRIDE lines by searching tolerantly backwards,  else user gets old value!
	for (my $i=$#param_lines; $i > -1; $i--){
		if ($param_lines[$i] =~ /^(OVERRIDE\s+)?${safe_tag}:\s*(.*?)\s*$/) {
			$pval = $2;
			last;
		} elsif ($param_lines[$i] =~ /^UNSET\s+${safe_tag}:/) {
			last;
		}
	}
	return $pval;
}


#----------------------------------------------------------------------
# extract_serif_output_dirs($template_dir, $template_path,
#                           $serif_regression, $os_suffix)
#               -> (\@serif_output_dirs, \%batch_input_files);
#
# Read the given template file, and extract the list of output
# directories, and the associated input batch files. The base
# template dir is also provided so it can be passed as an
# argument to get_param_lines and used for the include files. Return
# a list of output directories, as well as a hash mapping from
# output directories to corresponding input batch files.
sub extract_serif_output_dirs {
    (my $template_dir, my $template_path,
     my $serif_regression, my $os_suffix) = @_;

    my (@serif_output_dirs, $batch_file_prefix, @batch_file_list);
    my @param_lines = get_param_lines($template_dir, $template_path);
	my $exp_dir_list = get_param_value(\@param_lines, "experiment_dir.list");
	if ($exp_dir_list) {
		@serif_output_dirs = split(' ', $exp_dir_list);
	}else {
		die "No experiment_dir.list found in template: $template_path";
	}
	my $batch_file_list_str = get_param_value(\@param_lines, "batch_file.list");
	if ($batch_file_list_str){
     	@batch_file_list = split(' ', $batch_file_list_str);
	} else {
		die "No batch_file.list found in template: $template_path";
	}
	$batch_file_prefix = get_param_value(\@param_lines, "batch_file.prefix");	
	if ($batch_file_prefix){
		$batch_file_prefix =~ s/[\%\+]serif_regression[\+\%]/$serif_regression/g;
		$batch_file_prefix =~ s/[\%\+]os_suffix[\%\+]/$os_suffix/g;
		die "unexpanded variable: $1"
			if ($batch_file_prefix =~ m/[\%\+].*[\%\+]/);
	}else {
		die "No batch_file.prefix found in template: $template_path";
	}

    @batch_file_list = map {"$batch_file_prefix/$_"} @batch_file_list;
    my %batch_input_files = zip @serif_output_dirs, @batch_file_list;
    return (\@serif_output_dirs, \%batch_input_files);
}

#----------------------------------------------------------------------
# default_svn_revision() -> $revision
#
# If we've already started with the experiment with a (single)
# revision, then reuse that revision.  Otherwise, use HEAD.
sub default_svn_revision {
    my @revisions = split(/\s+/, `ls $etemplatedir`);
    if ($#revisions == 0) {
	return $revisions[0];
    } else {
	return SVN_HEAD;
    }
}

#----------------------------------------------------------------------
# svn_head_revision() -> $revision
#
# Return the current head revision of the repository ${SVN_URL}.
sub svn_head_revision {
    print "Looking up current SVN head revision... ";
    my $svn_info = `${SVN} info ${SVN_URL}`;
    (my $svn_revision) = ($svn_info =~ m/Revision: *(.*)/);
    print "$svn_revision\n";
    return "rev-$svn_revision";
}

#----------------------------------------------------------------------
# svn_date_revision($date) -> $revision
#
# Return the svn revision of the repository ${SVN_URL} on the given date.
# The date must have the format yyyy-mm-dd.
sub svn_date_revision {
    my $date = shift;
    unless ($date =~ /^(\d\d\d\d-\d\d-\d\d)$/) {
	die "ERROR: '$date' is not a valid date!  Expected YYYY-MM-DD.\n";
    }
    print "Looking up SVN revision for date $date... ";
    my $svn_info = `${SVN} info ${SVN_URL} "-r\{$date\}"`;
    (my $svn_revision) = ($svn_info =~ m/Revision: *(.*)/);
    print "$svn_revision\n";
    return "rev-$svn_revision";
}

#----------------------------------------------------------------------
# svn_date_range($date) -> @revisions
sub svn_date_range {
    my $range = shift;
    my ($start, $end, $junk, $step) =
	($range =~ m/^(\d\d\d\d-\d\d-\d\d):(\d\d\d\d-\d\d-\d\d)(\[(\d+)\])?$/);
    if (!(defined($start) && defined($end))) {
	die "ERROR: '$range' is not a valid date range!\n" .
	    "       Expected YYYY-MM-DD:YYYY-MM-DD.\n";
    }

    my $svndir = "trunk/Active/Core/SERIF";
    print "Looking up SVN revisions in date range $range " .
	"where $svndir was modified...\n";
    my $svn_log = `${SVN} log ${SVN_URL}/$svndir -r '{$start}:{$end}' -q`;
    my @revisions = ();
    my $count = 0;
    foreach (split(/\n/,$svn_log)) {
	next if /^-----+/;
	my ($rev) = m/^r(\d+) .*/;
	$count++;
	if (defined($step) && (($count % $step) != 0)) {
	    print "    - r$rev -- Skipped (step=$step)\n";
	} else {
	    push @revisions, "rev-$rev";
	    print "    - r$rev\n";
	}
    }
    return @revisions;
}

#----------------------------------------------------------------------
# local_revision() -> $revision
sub local_revision {
    my $local_root = abs_path("$exp_root/../..");
    my @serif_dirs = ("$local_root/src");
    print "Computing checksum for local repository... ";
    my ($cksum) = split(/\s/,`find @serif_dirs -not \\( -path */expts* -or -path */logfiles* -or -path */etemplates* -or -path */ckpts* -or -path */\.git* \\) -ls | cksum`);
    print "$cksum\n";
    return "local-$cksum";
}

#----------------------------------------------------------------------
# normalize_revision($rev) -> $name
#
# Given a revision value, normalize it by looking up any revisions
# that are defined by date or by using the special symbol HEAD.
sub normalize_revision {
    my $rev = shift;
    if    ($rev =~ /^rev-head$/i)    { return SVN_HEAD; }
    elsif ($rev =~ /^rev-local$/i)   { return local_revision(); }
    elsif ($rev =~ /^rev-\d+$/i)     { return $rev; }
    elsif ($rev =~ /^tag-.*/i)       { return $rev; }
    elsif ($rev =~ /^branch-.*/i)    { return $rev; }
    elsif ($rev =~ /^date-(\d\d\d\d-\d\d-\d\d)$/)
                                     { return svn_date_revision($1); }
    elsif ($rev =~ /^date-(\d\d\d\d-\d\d-\d\d:\d\d\d\d-\d\d-\d\d(\[\d+\])?)$/)
                                     { return svn_date_range($1); }
    else                             { die "Bad revision $rev!"; }
}

#----------------------------------------------------------------------
# check_conditions(\%config, \@condition_list) -> boolean
#
# Return true if this configuration matches any of the given list of
# conditions.  Each condition is a hash from configuration variables
# (eg ARCHITECTURE) to regexps.
sub check_conditions {
    my ($config, $condition_list) = @_;
    foreach my $conditions (@$condition_list) {
	my $skip_it = 1;
	while ( my ($key, $regexp) = each(%$conditions) ) {
	    die "Bad condition key: $key" unless defined($config->{$key});
	    $skip_it = 0 unless ($config->{$key} =~ /$regexp/);
	}
	return 1 if ($skip_it);
    }
    return 0;
}

sub describe_conditions {
    my ($conditions, $or, $and, $all) = @_;
    $or = "; or " if not defined($or);
    $and = " and " if not defined($and);
    $all = "all" if not defined($all);
    my @condition_descrs;
    foreach my $condition (@$conditions) {
	my @values = map({defined($condition->{$_})?$condition->{$_}:()}
			 (@CONFIG_FIELD_NAMES, "BUILD_SPEED"));
	push @condition_descrs, join($and, @values);
    }
    return $all if !(@condition_descrs);
    return join($or, @condition_descrs);
}

#----------------------------------------------------------------------
# get_config_name(\%config) -> $name
#
# Return the name string for the given configuration.  This is used
# for the output directory and for job names.  Each configuration
# should have a unique name, to allow all configurations to be built
# in parallel.
sub get_config_name {
    my $config = shift;
    return join("/", map { $config->{$_} || () } @CONFIG_FIELD_NAMES);
}

#----------------------------------------------------------------------
# abbrev_config_name(\%config) -> $abbrev_name
# abbrev_config_name(\%config, $skip_field) -> $abbrev_name
#
# Return an abbreviated version of a given build name, formed by
# only keeping parameters that have multiple values for the current
# experiment -- e.g., only include the architecture if this experiment
# run is comparing multiple architectures.
sub abbrev_config_name {
    my ($config, $skip_field) = @_;
    my @pieces = ();
    foreach my $field (@CONFIG_FIELD_NAMES) {
	next if defined($skip_field) && ($skip_field eq $field);
	next if scalar(@{$CONFIG_VALUES{$field}}) <= 1;
	push @pieces, $config->{$field};
    }
    push @pieces, "SERIF" if (!@pieces);
    my $abbrev = join(" ", @pieces);
    return $abbrev;
}

sub shared_config_name {
    my @pieces = ();
    foreach my $field (@CONFIG_FIELD_NAMES) {
	if (scalar(@{$CONFIG_VALUES{$field}}) == 1) {
	    push @pieces, $CONFIG_VALUES{$field}[0];
	}
    }
    return "" if (!@pieces);
    return "(" . join(" ", @pieces) . ")";
}

############################################################################
# Outcome Reporting
############################################################################

#----------------------------------------------------------------------
# report_header($status) -> $header
#
# Return a header string for the report email.  $status is a string
# indicating whether the build experiment was successful.
sub report_header {
    my $status = shift;
    ## crashes if the root was a newer sever than the unix2win tables can handle
	#my $win_exp_root = File::PathConvert::unix2win($exp_root);
    return (#"${EMAIL_HEADER}\n" .
	    "$EXPERIMENT_NAME $status\n" .
	    #"  Unix: $exp_root\n" .
		#"  Win:  $win_exp_root\n");
		    "  Unix: $exp_root\n");
}

#----------------------------------------------------------------------
# report_footer($status) -> $footer
#
# Return a footer string for the report email.  $status is a string
# indicating whether the build experiment was successful.
sub report_footer {
    my $end_time=localtime;
    my $footer = (
	"\nStart Time: $start_time\n" .
	"  End Time: $end_time\n" .
	"\nParameters:\n" . $parameter_description .
	"\nBuild Configurations:\n" . $incl_config_description);
    if ($incl_regtest_description) {
	$footer .= "\nTest Configurations:\n" . $incl_regtest_description;
    }
    if ($skip_config_description) {
	$footer .= "\nIntentionally Skipped Building Configurations:\n" .
	    $skip_config_description;
    }
    if ($skip_regtest_description) {
	$footer .= "\nIntentionally Skipped Testing Configurations:\n" .
	    $skip_regtest_description;
    }
    $footer .= "\nQueues:\n" . $queue_description;
    $footer .= "\nCommand-line:\n  $cmdline_description\n";
    return $footer;
}

#----------------------------------------------------------------------
# quote_str($s) -> $quoted_s
#
# Helper used to do appropriate quoting when displaying the
# command-line arguments.  (Is there a built-in to do this?)
sub quote_str {
    my $s = shift;
    $s =~ s/\"/\\"/g;
    $s = "\"$s\"" if ($s =~ /[" ]/);
    return $s;
}

#----------------------------------------------------------------------
# report_success()
#
# Generate a message indicating that the build experiment completed
# successfully.  If $MAILTO is defined, then email it to that address.
# If $VERBOSE is true, then print it.
sub report_success {
    my $msg = report_header("OK!") . report_footer();
    if ($REPORT_DONE_JOBS) {
		$msg .= "\nJob List:\n" .
			join("\n", sort(keys %{$runjobs_scheduler->{done_jobs}}));
	}
    send_mail("$EXPERIMENT_NAME OK", $msg) if defined($MAILTO);
    print $msg if $VERBOSE;
    print "$EXPERIMENT_NAME finished successfully!\n";
}

#----------------------------------------------------------------------
# report_throughput(\%tput_scores)
#
# Generate a message describing the throughput after a successful
# throughtput experiment.  If $MAILTO is defined, then email it to
# that address.
sub report_throughput {
    my ($tput) = @_;

    # Check that the throughput scores are all within expected ranges;
    # and complain if any are too low.
    my $min_score = min(map {tput_score($_, $tput->{$_}{THROUGHPUT})}
			(keys %$tput)) || 0;
    my $status = ($min_score < -0.15 ? "ERROR: Low Throughput" :
		  $min_score < -0.05 ? "Warning: Low Throughput" : "OK");

    # Construct the message.  (We use HTML to make including the
    # graphs easier.)
    my $msg = "<body><pre>" . report_header($status) . "</pre>\n";
    my @graphs = ();

    # Add 'BUILD_SPEED' as to the config dictionary, so we can use
    # it to classify builds for the bargraphs.
    foreach my $tput_info (values %$tput) {
	my $params = $tput_info->{CONFIG}{PARAMETERS};
	my $speed = ($FAST_PARSER_PARAMETERS{$params})?"Fast":"Slow";
	$tput_info->{CONFIG}{BUILD_SPEED} = $speed;
    }

    # Draw a set of graphs to show the throughput scores.
    if ($THROUGHPUT_GRAPHS{HISTORICAL}) {
        my @historical_graphs =
	    (map {draw_historical_tput_bargraph($tput, $_)}
	     (sort keys %HISTORICAL_TPUT_CONFIGURATIONS));
	my $imgs = join(" ", map {"<img src=\"cid:$_->{Id}\">"}
			@historical_graphs);
	$msg .= "<p>$imgs</p>\n";
	push @graphs, @historical_graphs;
    }
    if ($THROUGHPUT_GRAPHS{CURRENT}) {
	my @tput_graphs = (
	    #draw_current_tput_bargraph(
	    #   $tput, "PARAMETERS", [{}])
	    draw_current_tput_bargraph(
		$tput, "PARAMETERS",
		[{LANGUAGE=>"English", BUILD_SPEED=>"Fast"}]),
	    draw_current_tput_bargraph(
		$tput, "PARAMETERS",
		[{LANGUAGE=>"English", BUILD_SPEED=>"Slow"}]),
	    draw_current_tput_bargraph(
		$tput, "PARAMETERS",
		[{LANGUAGE=>"Arabic|Chinese"}]),
	    );
	my $imgs = join(" ", map {"<img src=\"cid:$_->{Id}\">"}
			@tput_graphs);
	$msg .= "<p>$imgs</p>\n";
	push @graphs, @tput_graphs;
    }
    if ($THROUGHPUT_GRAPHS{PROFILE}) {
        my $profile_graph = draw_profile_section_bargraph($tput);
	$msg .= "<p><img src=\"cid:$profile_graph->{Id}\"></p>\n";
	push @graphs, $profile_graph;
    }

    my $num_tput_columns = max(map {scalar(@{$_->{THROUGHPUTS}})}
			       (values %$tput));

    $msg .= "<h3>Throughput By Configuration (MB/hr)</h3>\n";
    $msg .= "<table border=\"1\" cellpadding=\"1\" cellspacing=\"0\">\n";
    $msg .= "  <tr bgcolor=\"c0c0ff\"><th>Configuration</th><th></th>";
    $msg .= "<th>Expected</th><th></th>\n";
    $msg .= "<th>Average</th><th>StdDev</th><th></th>";
    $msg .= join("", map {"<th>Run $_</th>"} (1..$num_tput_columns));
    $msg .= "</tr>";
    foreach my $config_name (sort keys %$tput) {
	my $tput_info = $tput->{$config_name};
	my $errorbar = sprintf("%.2f", $tput_info->{ERRORBAR});
	my $expected = expected_tput($config_name) || "N/A";
	my @tputs = @{$tput_info->{THROUGHPUTS}};
	$#tputs = ($num_tput_columns-1);
	$msg .= "  <tr><td bgcolor=\"c0c0ff\"> $config_name </td><td></td>\n";
	$msg .= "    <td align=\"right\">$expected</td><td></td>\n";
	$msg .= tput_table_entry($config_name, $tput_info->{THROUGHPUT}, 1);
	$msg .= "    <td align=\"right\">$errorbar</td><td></td>\n";
	$msg .= join("", map {tput_table_entry($config_name, $_)} @tputs);
	$msg .= "  </tr>\n";
    }
    $msg .= "</table>\n";
    $msg .= "<h3>Experiment Information</h3>\n";
    $msg .= "<pre>" . report_footer() . "</pre>\n</body>\n";

    send_mail("$EXPERIMENT_NAME $status", $msg, \@graphs, "html")
	if defined($MAILTO);
    print $msg if $VERBOSE;
    print "$EXPERIMENT_NAME finished successfully!\n";

}

#----------------------------------------------------------------------
# tput_score($config_name, $tput) -> $score
#
# Return a normalized score, relative to the expected result: 0 is
# expected, -0.05 is 5% worse than expected, +0.10 is 10% better than
# expected.  If no expected result is available, return undefined.
sub tput_score {
    my ($config_name, $tput) = @_;
    my $expected = expected_tput($config_name);
    if (!defined($expected)) { return; }
    return ($tput-$expected)/$expected;
}

#----------------------------------------------------------------------
# expected_tput($config_name) -> $tput or undefined
#
sub expected_tput {
    my ($config_name) = @_;
    my ($rev, $build_name) = split(m(/), $config_name, 2);
    return $EXPECTED_TPUT{$build_name};
}

#----------------------------------------------------------------------
# tput_table_entry($config_name, $tput, $bold) -> $html
#
# Helper for report_throughput().
sub tput_table_entry {
    my ($config_name, $tput, $bold) = @_;
    return "    <td>&nbsp;</td>\n" if !defined($tput);
    $tput = sprintf("%.2f", $tput);
    my $score = tput_score($config_name, $tput);
    my $color = "#ffffff";
    if (defined($score)) {
	if    ($score > -0.05) { $color = "#80ff80"; }
	elsif ($score > -0.15) { $color = "#ffff80"; }
	else                   { $color = "#ff8080"; }
    }
    if ($bold) { $tput = "<b>$tput</b>"; }
    return "    <td align=\"right\" bgcolor=\"$color\">$tput</td>\n";
}

#----------------------------------------------------------------------
# draw_profile_section_bargraph(\%tput)
#
sub draw_profile_section_bargraph {
    my ($tput) = @_;
    print "Drawing profile bargraph...\n";

    # Find the set of all sections.
    my %section_total;
    foreach my $tput_info (values %$tput) {
	my $section_times = $tput_info->{SECTIME};
	foreach my $section (keys %$section_times) {
	    $section_total{$section} += $section_times->{$section};
	}
    }
    my @sections = keys %section_total;
    @sections = sort {$section_total{$b} <=> $section_total{$a}} @sections;
    my @display_sections = reverse(@sections[0..$NUM_PROFILE_SECTIONS-1]);
    my @other_sections = @sections[$NUM_PROFILE_SECTIONS..$#sections];

    # We use bargraph.pl to draw a pretty bargraph (using gnuplot).  We
    # then convert it to a PNG by rendering it at 6x magnification, and
    # then resizing it down, to get proper anti-aliasing.
    my $filename = "section_times";
    open GRAPH, ">$expdir/$filename.bargraph";
    print GRAPH "title=Running Time By Section\n";
    print GRAPH "extraops=set size 1,1.5\n";
    print GRAPH "ylabel=Percent\n";
    print GRAPH "rotateby=-45\n";
    print GRAPH "=stacked;All Other Stages;";
    print GRAPH join(";", @display_sections) . "\n";
    print GRAPH "=nocommas\n";
    print GRAPH "=table;\n";
    print GRAPH "legendx=right\nlegendy=center\n=nolegoutline\n";
    foreach my $config_name (sort keys %$tput) {
	my $tput_info = $tput->{$config_name};
	my $config = $tput_info->{CONFIG};
	my $times = $tput->{$config_name}{SECTIME};
	my $abbrev_name = abbrev_config_name($config);
	my @times = (sum(map {$times->{$_} || 0} @other_sections),
		     map {$times->{$_} || 0} @display_sections);
	my $total_time = sum(@times);
	print GRAPH "$abbrev_name;" .
	    join(";", map {100.0*$_/$total_time} @times) . "\n";

    }
    close GRAPH;
    draw_bargraph($filename);
    return {Type => 'image/png',
	    Path => "$expdir/$filename.png",
	    Id => "$filename.png"};
}

sub draw_bargraph {
    my ($filename) = @_;
    runjobs([], "bargraph-${filename}",
	    {
		BATCH_QUEUE      => $SCORE_QUEUE,
		SGE_VIRTUAL_FREE => "500M",
		PLOT_HOME        => $PLOT_HOME,
		src => "$expdir/$filename.bargraph",
		dst => "$expdir/$filename.png",
		bindir => $bindir,
	    }, $BASH, "draw_bargraph.sh");
    $runjobs_scheduler->process_jobs() or report_failure();
}

sub draw_current_tput_bargraph {
    my ($tput, $cluster_by, $conditions) = @_;
    my $condition_descr = describe_conditions($conditions);
    print "Drawing throughput bargraph ($condition_descr)...\n";

    my %clusters;
    my %cluster_item_set;
    if (!defined($cluster_by)) { $cluster_by = "NO_CLUSTER"; }
    while ( my ($config_name, $tput_info) = each(%$tput) ) {
	if (check_conditions($tput_info->{CONFIG}, $conditions)) {
	    my $config = $tput_info->{CONFIG};
	    my $abbrev_name = abbrev_config_name($config, $cluster_by);
	    my $cluster_item = $config->{$cluster_by} || "NO_CLUSTER";
	    $cluster_item_set{$cluster_item} = 1;
	    $clusters{$abbrev_name}{$cluster_item} = $tput_info;
	}
    }
    my @cluster_items = sort keys %cluster_item_set;

    if (!%clusters) {
	print "  No clusters found for this graph ($condition_descr)!\n";
	return ();
    }

    # We use bargraph.pl to draw a pretty bargraph (using gnuplot).  We
    # then convert it to a PNG by rendering it at 6x magnification, and
    # then resizing it down, to get proper anti-aliasing.
    my $suffix = lc(describe_conditions($conditions, "_OR_", "_"));
    $suffix =~ s/\W/_/g;
    my $filename = "current_throughput_$suffix";
    open GRAPH, ">$expdir/$filename.bargraph";
    print GRAPH "title=Throughput " . shared_config_name() . "\n";
    print GRAPH "extraops=set size 0.5,1.2\n";
    print GRAPH "ylabel=MB/hr\n";
    print GRAPH "rotateby=-45\n";
    print GRAPH "=nocommas\n";
    print GRAPH "=cluster;" . join(";", @cluster_items) . "\n";
    print GRAPH "=table;\n";
    print GRAPH "legendx=right\nlegendy=center\n=nolegoutline\n";
    foreach my $cluster_name (sort keys %clusters) {
	my $cluster = $clusters{$cluster_name};
	print GRAPH "$cluster_name;" .
	    join(";", map {$cluster->{$_}{THROUGHPUT} || 0} @cluster_items)
	    . "\n";
    }
    print GRAPH "=yerrorbars;\n";
    foreach my $cluster_name (sort keys %clusters) {
	my $cluster = $clusters{$cluster_name};
	print GRAPH "$cluster_name;" .
	    join(";", map {$cluster->{$_}{ERRORBAR} || 0} @cluster_items)
	    . "\n";
    }
    close GRAPH;
    draw_bargraph($filename);
    return {Type => 'image/png',
	    Path => "$expdir/$filename.png",
	    Id => "$filename.png"};
}

#----------------------------------------------------------------------
# draw_historical_tput_bargraph(\%tput, $tput_group)
#
# Helper for draw_historical_tput_bargraphs: draw one graph.
sub draw_historical_tput_bargraph {
    my ($tput, $tput_group) = @_;
    print "Drawing historical bargraph ($tput_group)...\n";
    my $build_names = $HISTORICAL_TPUT_CONFIGURATIONS{$tput_group};

    # Read the throughput from previous runs.
    my ($tput_history, $errorbar_history) = read_throughput_history();

    # Take the N most recent dates.  Dates are intentionally formatted
    # such that they will sort alphabetically.
    my @dates = sort { $a cmp $b } keys %$tput_history;
    @dates = ($NUM_HISTORICAL_TPUT_DATES >= @dates ?
	      @dates : @dates[-$NUM_HISTORICAL_TPUT_DATES..-1]);

    # Add the throughput from this run.
    my %current_dates;
    while ( my ($config_name, $tput_info) = each(%$tput) ) {
	my ($rev, $build_name) = split(m(/), $config_name, 2);
	my $today = strftime('%Y-%m-%d %H:%M',localtime) . " ($rev)";
	$tput_history->{$today}{$build_name} = $tput_info->{THROUGHPUT};
	$errorbar_history->{$today}{$build_name} = $tput_info->{ERRORBAR};
	$current_dates{$today} = 1;
    }
    push @dates, keys %current_dates;

    my $filename = "historic_throughput_\L$tput_group";
    $filename =~ s/[^\w_\.]/_/g;
    open GRAPH, ">$expdir/$filename.bargraph";
    print GRAPH "title=Throughput History ($tput_group)\n";
    print GRAPH "extraops=set size 0.8,0.7\n";
    print GRAPH "ylabel=MB/hr\n";
    print GRAPH "rotateby=0\n";
    print GRAPH "=nocommas\n";
    print GRAPH "=cluster;" . join(";", @dates) . "\n";
    print GRAPH "=table;\n";
    print GRAPH "legendx=right\nlegendy=center\n=nolegoutline\n";

    foreach my $build_name (@$build_names) {
	(my $label = $build_name) =~ s(/)(\\n)g;
	print GRAPH "$label;" .
	    join(";", map {$tput_history->{$_}{$build_name} || 0} @dates) .
	    "\n";
    }
    print GRAPH "=yerrorbars;\n";
    foreach my $build_name (@$build_names) {
	(my $label = $build_name) =~ s(/)(\\n)g;
	print GRAPH "$label;" .
	    join(";", map {$errorbar_history->{$_}{$build_name} || 0} @dates) .
	    "\n";
    }
    close GRAPH;
    draw_bargraph($filename);
    return {Type => 'image/png',
	    Path => "$expdir/$filename.png",
	    Id => "$filename.png"};
}

#----------------------------------------------------------------------
# read_throughput_history() -> \%tput_history, \%errorbar_history
#
# Read the throughput history information, and return it as a
# hash where $tput_history{DATE}{BUILD} => THROUGHPUT.
sub read_throughput_history {
    my %tput_history = ();
    my %errorbar_history = ();
    open(TPUT_HISTORY, "<$THROUGHPUT_HISTORY_FILE")
	or die "Error opening $THROUGHPUT_HISTORY_FILE";
    while(<TPUT_HISTORY>) {
	chomp;              # Strip newline
	s/#.*//;            # Strip comments.
	next if /^\s*$/;    # Skip blank lines
	my ($date, $config_name, $tput, $errorbar) = split(/\t/);
	my ($rev, $build_name) = split(m(/), $config_name, 2);
	$errorbar = 0 if !defined($errorbar);
	$tput_history{"$date ($rev)"}{$build_name} = $tput;
	$errorbar_history{"$date ($rev)"}{$build_name} = $errorbar;
	#print "TPUT [$date] [$build_name] [$tput] [$errorbar]\n";
    }
    close TPUT_HISTORY;
    return \%tput_history, \%errorbar_history;
}

#----------------------------------------------------------------------
# update_throughput_history($tput)
#
# Append the throughput scores that were obtained from this experiment
# to the global throughput history file.
sub update_throughput_history {
    my $tput = shift;
    my $date = strftime('%Y-%m-%d %H:%M',localtime);
    print "Writing throughput history to $THROUGHPUT_HISTORY_FILE...\n";
    open(TPUT_HISTORY, ">>$THROUGHPUT_HISTORY_FILE")
	or die "Error opening $THROUGHPUT_HISTORY_FILE";
    foreach my $config_name (sort keys %$tput) {
	print TPUT_HISTORY ("$date\t" .
			    "$config_name\t" .
			    "$tput->{$config_name}{THROUGHPUT}\t" .
			    "$tput->{$config_name}{ERRORBAR}\n");
    }
    print TPUT_HISTORY "\n";
    close TPUT_HISTORY;
}

#----------------------------------------------------------------------
# read_throughput_scores() -> \%tput
#
# Read the throughput scores from the individual throughput.txt files
# that were generated for each configuration.  Return them as a hash
# where %tput{CONFIG_NAME} => THROUGPUT
sub read_throughput_scores {
    my %tput = ();
    foreach my $config (@configurations) {
	my $config_name = get_config_name($config);
	my @tput_files = bsd_glob("$expdir/test/$config_name/*/*/throughput.txt");
	my @tput_infos = map {read_throughput_file($_)} @tput_files;
	if (!@tput_infos) {
	    print "Warning: no throughput info for $config_name!\n"
		unless $THROUGHPUT_SO_FAR;
	    next; # nothing to see here!
	}
	$tput{$config_name} = combine_throughput_info(\@tput_infos);
	$tput{$config_name}{CONFIG} = $config;
    }
    return \%tput;
}

#----------------------------------------------------------------------
# combine_throughput_info([\%tput_info, \%tput_info, ...]) -> \%combined_info
#
sub combine_throughput_info {
    my $tput_infos = shift;
    my %combined;

    # Compute totals & overall throughput.
    $combined{SIZE} = sum(map {$_->{SIZE}} @$tput_infos);
    $combined{TIME} = sum(map {$_->{TIME}} @$tput_infos);
    $combined{THROUGHPUTS} = [map {$_->{THROUGHPUT}} @$tput_infos];
    # my $len = @{$combined{THROUGHPUTS}};
    # if ($len >= 5) {
    # 	# Discard highest & lowest value
    # 	$combined{THROUGHPUTS} = [sort @{$combined{THROUGHPUTS}}];
    # 	$combined{THROUGHPUTS} = [@{$combined{THROUGHPUTS}}[1..$len-2]];
    # }
    $combined{MIN_THROUGHPUT} = min(map {$_->{THROUGHPUT}} @$tput_infos);
    $combined{MAX_THROUGHPUT} = max(map {$_->{THROUGHPUT}} @$tput_infos);
    $combined{STD_DEV} = stddev(map {$_->{THROUGHPUT}} @$tput_infos);
    foreach my $tput_info (@$tput_infos) {
	while ( my ($section, $time) = each(%{$tput_info->{SECTIME}}) ) {
	    $combined{SECTIME}{$section} += $time; # msec
	}
    }

    # Get throughput in MB/hour.  3.6=(3,600,000 msec/hr)/(1,000,000mb/byte)
    $combined{THROUGHPUT} = 3.6*$combined{SIZE}/$combined{TIME};
    $combined{ERRORBAR} = max($combined{THROUGHPUT}-$combined{MIN_THROUGHPUT},
			      $combined{MAX_THROUGHPUT}-$combined{THROUGHPUT});
    $combined{ERRORBAR} = $combined{STD_DEV};
    return \%combined;
}

#----------------------------------------------------------------------
# read_throughput_file() -> \%tput_info
#
sub read_throughput_file {
    my $tput_file = shift;
    my %tput_info;
    if (open(TPUT_FILE, "$tput_file")) {
	my $size;
	while(<TPUT_FILE>) {
	    if (m/Time \((.*)\):\s*(.*) msec/) {
		$tput_info{SECTIME}{$1} = $2;
	    }
	    if (m/Time:\s*(.*) msec/) {
		$tput_info{TIME} = $1;
	    }
	    if (m/Size:\s*(.*) bytes/) {
		$tput_info{SIZE} = $1;
	    }
	}
	close TPUT_FILE;
    } else {
	print "Warning: throughtput file missing!\n  $tput_file\n"
    }
    # Get throughput in MB/hour.  3.6=(3,600,000 msec/hr)/(1,000,000mb/byte)
    $tput_info{THROUGHPUT} = 3.6*$tput_info{SIZE}/$tput_info{TIME};
    return \%tput_info;
}

sub get_log_summary {
    my ($filename) = @_;
    my @head = ();
    my @tail = ();
    my $HEAD_SIZE = 20;
    my $TAIL_SIZE = 25;
    my $skipped_lines = 0;
    if (open(LOGFILE, $filename)) {
	while (<LOGFILE>) {
	    if (scalar(@head) < $HEAD_SIZE) {
		push @head, $_;
	    } else {
		push @tail, $_;
		if (scalar(@tail) > $TAIL_SIZE) {
		    shift @tail;
		    $skipped_lines += 1;
		}
	    }
	}
	if ($skipped_lines) {
	    push @head, ("[===== TRUNCATED $skipped_lines " .
			 "LINES FOR EMAIL =====]\n");
	}
	return join("", ("$filename\n", @head, @tail));
    } else {
	return "Log file $filename not found!";
    }
}

#----------------------------------------------------------------------
# report_failure()
#
# Generate a message indicating that the build experiment failed.  If
# $MAILTO is defined, then email it to that address.  If $VERBOSE is
# true, then print it.
sub report_failure {
    # Get a list of failed jobs and a list of unfinished jobs.
    my @failed_jobs = ();
    my @unfinished_jobs = ();
    my @attachments = ();
    foreach my $job (sort(values %{$runjobs_scheduler->{nodes}})) {
	if ($job->{status} == JOB_FAILED) {
	    push @failed_jobs, $job;
	    my $attachment_name = "$job->{id}.log";
	    $attachment_name =~ s(/)(-)g;
	    push @attachments, {
		Type => 'text/plain',
		Data => get_log_summary($job->log_file()),
		#Path => $job->log_file(),
		Filename => $attachment_name,
		Disposition => 'attachment'};
	} else {
	    push @unfinished_jobs, $job;
	}
    }

    # Construct a message that reports what we were doing, and lists
    # any failed/unfinished jobs.
    my $msg = report_header("Failed!");
    if (@failed_jobs) {
	$msg .= "\nFailed jobs:\n" .
	    join("", map {job_descr($_)} sort(@failed_jobs));
    }
#    if (@unfinished_jobs) {
#	$msg .= "\nUnfinished jobs:\n" .
#	    join("", map {job_descr($_)} sort(@unfinished_jobs));
#    }
    $msg .= report_footer();

    # If we wanted to, we could use sglist to find out what it thinks
    # happened to our jobs as well; but is it worth it?

    # Email our failure message
    send_mail("$EXPERIMENT_NAME ERROR", $msg, \@attachments)
	if defined($MAILTO);
    print "$msg\n" if $VERBOSE;
    if ($RETURN_SUCCESS_AFTER_REPORTING_FAILURE) {
	exit 0;
    } else {
	die "$EXPERIMENT_NAME Failed!"
    }
}

#----------------------------------------------------------------------
# job_descr($job) -> $descr
#
# Helper for report_failure: return a string describing a job.
sub job_descr {
    my $job = shift;
    return "  $job->{id} (" . $job->status_desc() . ")\n";
}

#----------------------------------------------------------------------
# send_mail($subject, $message, \@attachments, [$msg_type])
#
# Send an email with the given subject and message body to the
# email address $MAILTO.  Use $MAILFROM as the reply-to address
# (if it is defined).  @attachments is a list of hash-refs, each
# of which describes a single attachment file.  $msg_type may
# me either 'text' or 'html'.
sub send_mail {
    die "send_mail(): $MAILTO not defined" unless defined($MAILTO);
    my ($subject, $message, $attachments, $msg_type) = @_;
    my $replyto = defined($MAILFROM) ? $MAILFROM : $MAILTO;
    my ($mime_msg_type, $mime_body_type);
    if ((!defined($msg_type)) || ($msg_type eq "text")) {
		$mime_msg_type = "multipart/mixed";
		$mime_body_type = "TEXT";
    } elsif ($msg_type eq "html") {
		$mime_msg_type = "multipart/related";
		$mime_body_type = "text/html";
    }

    my $mime_msg = MIME::Lite->new(
	From     => $replyto,
	To       => $MAILTO,
	Subject  => $subject,
	Type     => $mime_msg_type)
		or die "Error creating MIME Body $!\n";

    $mime_msg->attach(
	Type => $mime_body_type,
	Data => $message);

    foreach my $attachment (@$attachments) {
	$mime_msg->attach(%$attachment)
	    or die "Error attaching file: $!\n";
    }

    $mime_msg->send('smtp', 'smtp.bbn.com')
		or print "Warning: failed while sending email to $MAILTO!\n";
    print "Sent email to $MAILTO:\n  $subject\n";
}

