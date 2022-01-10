# Copyright BBN Technologies Corp. 2008
# All rights reserved

# Takes the data directory and experiment directory as its two args.
# (Assumes that the data directory contains source and key dirs and lists.)
# Calls the scorer and generates APF-views.

use strict;
use English; # Gives better names to variables like "$^X".
$0=~/^(.+[\\\/])[^\\\/]+[\\\/]*$/;
my $Bin= $1 || "./";

die "Usage: do-score.pl data-dir exp-dir" unless @ARGV == 2;
my ($data_dir, $exp_dir) = @ARGV;
my $scorer_ver1 = "ace05-eval-v14";
my $scorer_ver2 = "ace07-eval-v01";
my $scorer_ver3 = "ace08-eval-v08";

my $out_prefix3 = "ace2008-eval";
my $out_prefix2 = "ace2007-eval";
my $out_prefix1 = "ace2005-eval";

$exp_dir =~ s/\\/\//g;
$data_dir =~ s/\\/\//g;
$Bin =~ s/\\/\//g;
open APFLIST, ">$exp_dir/apf-files.txt"
    or die "Couldn't open APF list file: $exp_dir/apf-files.txt\n";

opendir (DIR,"$exp_dir/output/") || die "Couldn't open dir: $exp_dir/output\n";
my @files = readdir(DIR);
for my $file (@files){
    if($file =~ m/\.apf/){ 
       print APFLIST "$exp_dir/output/$file\n";
    }
}
close APFLIST;

### Do the scoring.

# Run the subprocesses using the same version of perl that we were run with:
my $PERL=$EXECUTABLE_NAME;

system "$PERL $Bin/$scorer_ver3.pl -m -r $data_dir/key-files.txt -t $exp_dir/apf-files.txt > $exp_dir/${out_prefix3}.txt";
die "Scoring script ($scorer_ver3.pl) failed!" if $CHILD_ERROR;
system "$PERL $Bin/$scorer_ver3.pl -m -as -r $data_dir/key-files.txt -t $exp_dir/apf-files.txt > $exp_dir/${out_prefix3}-full.txt";
die "Scoring script ($scorer_ver3.pl) failed!" if $CHILD_ERROR;

system "$PERL $Bin/$scorer_ver2.pl -m -r $data_dir/key-files.txt -t $exp_dir/apf-files.txt > $exp_dir/${out_prefix2}.txt";
die "Scoring script ($scorer_ver2.pl) failed!" if $CHILD_ERROR;
system "$PERL $Bin/$scorer_ver2.pl -m -as -r $data_dir/key-files.txt -t $exp_dir/apf-files.txt > $exp_dir/${out_prefix2}-full.txt";
die "Scoring script ($scorer_ver2.pl) failed!" if $CHILD_ERROR;

system "$PERL $Bin/$scorer_ver1.pl -m -r $data_dir/key-files.txt -t $exp_dir/apf-files.txt > $exp_dir/${out_prefix1}.txt";
die "Scoring script ($scorer_ver1.pl) failed!" if $CHILD_ERROR;
system "$PERL $Bin/$scorer_ver1.pl -m -as -r $data_dir/key-files.txt -t $exp_dir/apf-files.txt > $exp_dir/${out_prefix1}-full.txt";
die "Scoring script ($scorer_ver1.pl) failed!" if $CHILD_ERROR;
