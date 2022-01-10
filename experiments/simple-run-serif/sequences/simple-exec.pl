#!/bin/env perl

use strict;
use warnings;
use File::Path qw(mkpath);
use File::Basename;

$main::exp = 'simple-serif';           
$main::QUEUE_PRIO  = '5';        # Integral prioriority (0 lowest, 9 highest)
$main::SEPERATE_ERR_LOGGING = 0;
$main::SGE_VIRTUAL_FREE = '1.5G';
$main::win64_perl_interpreter   = 'C:\\Perl64\\bin\\perl.exe';

use lib "/d4m/ears/releases/Cube2/R2010_11_21/install-optimize$ENV{ARCH_SUFFIX}/perl_lib";
use runjobs4;
use File::PathConvert;

my $windows_queue = '@win64';
my $linux_queue = 'nongale-x86_64';

############################################################################
#                          Job Parameters
############################################################################

my $job_name = "test";
my $exe = "some_serif_exec";
my $param = "some_serif_param";

my ($exp_dir, $exp) = startjobs();

my $jid = runjobs( [], $job_name,
		   {
		       BATCH_QUEUE                     => $windows_queue,
		       SGE_VIRTUAL_FREE                => "4G",  
		   }, 		       
		   $exe, $param,
    );

endjobs();

