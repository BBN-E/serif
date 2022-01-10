#!/opt/perl-5.8.6/bin/perl
eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}' && eval 'exec perl -w -S $0 $argv:q'
  if 0;
 
use strict;
use warnings;
use lib "/d4m/ears/releases/Cube2/R2010_02_08/install-optimize$ENV{ARCH_SUFFIX}/perl_lib";
use runjobs4;
use File::PathConvert qw(unix2win);
 
# Process command line arguments and get experiment directory and id
my ($exp_dir, $exp) = startjobs(
  newxg => "/nfs/titan/u126/ewu/newxg", 
  batch_queue    => "winsge600s",
  queue_priority => 5  
);
 
if (scalar(@ARGV) != 5) {
	die("Usage: $0 full_serif_data_path \"[date]-r[svn_rev]cache_folder_name\" " 
		. "file_of_machines win_share_name "
		. "serif_data_directories_history_log ");
}

my $serif_data = unix2win($ARGV[0]);
my $machines_file = unix2win($ARGV[1]);
my $history_file = unix2win($ARGV[2]);
my $share_name = $ARGV[3];
my $daterev_folder = $ARGV[4];
my $SERIF_PM_LOCATION = unix2win("$exp_dir/bin");
my $WIN_PERL = "c:\\Perl64\\bin\\perl.exe";
my $COPY_SCRIPT = "copy_serifdata_win.pl";

# escape all backslashes with an extra one
$serif_data =~ s/\\/\\\\/g;
$machines_file =~ s/\\/\\\\/g;
$history_file =~ s/\\/\\\\/g;
$SERIF_PM_LOCATION =~ s/\\/\\\\/g;
 
# Schedule a job
my $job = runjobs(
  [],             # No previous jobs
  "SERIFDataLocalization",     # Unique ID for this job
  # Below: specifying params with same-named vars
  # but doing it for explicitness
  {
    history_file => $history_file,
    share_name  => $share_name,
    daterev_folder    => $daterev_folder,
	serif_data		=> $serif_data,
	machines_file	=> $machines_file,
	SERIFDataFunctions_location => $SERIF_PM_LOCATION
  },
  [$WIN_PERL, $COPY_SCRIPT]
);
 
 
# Execute the job now that scheduling has finished
endjobs();
