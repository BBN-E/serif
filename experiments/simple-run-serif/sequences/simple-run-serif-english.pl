#!/bin/env perl

# Simple experiment designed to simply run a precompiled Serif 
# on a lot of data. Used for a large regression test.

use strict;
use warnings;
use File::Path qw(mkpath);
use File::Basename;

$main::exp = 'simple-run-serif';           
$main::QUEUE_PRIO  = '5';        # Integral prioriority (0 lowest, 9 highest)
$main::SEPERATE_ERR_LOGGING = 0;
$main::SGE_VIRTUAL_FREE = '1.5G';
$main::win64_perl_interpreter   = 'C:\\Perl64\\bin\\perl.exe';

use lib "/d4m/ears/releases/Cube2/R2010_11_21/install-optimize$ENV{ARCH_SUFFIX}/perl_lib";
use runjobs4;
use File::PathConvert;

my $windows_queue = '@win64';

############################################################################
#                          Job Parameters
############################################################################

# Top level output directory name and top level experiment name
my $run_name = "English-r23868";

# A list of batch files to run on, including Serif parameter info
# On each line of the file should be a segment field name (or "sgm") 
# And a Serif batch file listing segment or sgm files to run on 
my $run_file = "/nfs/titan/u70/large-regression-test/run-files/English-rerun4.list";

# Output directory organized by information gleened from the run file and $run_name
# top level directory is $run_name, second level is field name (from the run file)
# third level is the Serif run directory which has the same name as the batch file
my $master_output_dir = "/nfs/titan/u70/large-regression-test/output/";

# Executable to run
my $serif_exe = "/nfs/titan/u70/large-regression-test/executables-win32/SerifEnglish-r23868.exe";

# Serif data
my $serif_data_path = "//titan2/u21/Serif-data/data";

# Parameter checking
unless (-f $run_file) {
    die "run_file must exist!";
}

unless (-f $serif_exe) {
    die "serif_exe must exist!";
}

mkdir "$master_output_dir", 0777 unless -d "$master_output_dir";
mkdir "$master_output_dir/$run_name", 0777 unless -d "$master_output_dir/$run_name";
unless (-d $master_output_dir) {
    die "Could not create master_output_dir/$run_name";
}


############################################################################
#                          Job Sequencing
############################################################################

my ($exp_dir, $exp) = startjobs();

# traid06 freaks out with too many parallel jobs
&runjobs4::runjobs_object()->{SCHEDULER}{max_jobs} = 60;

open IN, $run_file or die "Could not open $run_file for reading!";
my $dir1 = "$master_output_dir/$run_name";

while (<IN>) {
    next if /^\s*\#/;
    next if /^\s$/;

    s/\s+$//;
    
    my @pieces = split / /;
    die "bad line in run file: " . $_ unless scalar(@pieces) == 2;

    my $source_name = $pieces[0];
    my $batch_file = $pieces[1];

    my $batch_file_name = basename(win2unix($batch_file));
    $batch_file_name =~ s/\.txt$//;

    my $dir2 = "$dir1/$source_name";
    my $output_dir = "$dir2/$batch_file_name";
    
    mkdir $dir2, 0777 unless -d $dir2;
    mkdir $output_dir, 0777 unless -d $output_dir;

    die "Could not make output directory" unless -d $output_dir;

    my $job_name = "$run_name/$source_name/$batch_file_name";

    my $source_format = "sgm";
    my $do_sentence_breaking = "true";
    my $segment_input_field = "NONE";

    if ($source_name ne "sgm") {
	$source_format = "segments";
	$do_sentence_breaking = "false";
	$segment_input_field = $source_name;
    }
    
    my $jid = runjobs( [], $job_name,
		       {
			   serif_data              => $serif_data_path,
			   batch_file              => $batch_file,
			   experiment_dir          => unix2win($output_dir),

			   source_format           => $source_format,
			   do_sentence_breaking    => $do_sentence_breaking,
			   seg_field               => $segment_input_field,
			   
			   PATH_LAMBDA                     => $main::unix2win,
			   BATCH_QUEUE                     => $windows_queue,
			   SGE_VIRTUAL_FREE                => "4G",  
		       }, 
		       
		       unix2win($serif_exe), "serif-general-english.par",
		       );
}

endjobs();

