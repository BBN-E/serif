# Copyright BBN Technologies Corp. 2005
# All rights reserved

# Takes the data directory and experiment directory as its two args.
# (Assumes that the data directory contains source and key dirs and lists.)
# Calls the scorer and generates APF-views.

use strict;
use English; # Gives better names to variables like "$^X".
use FindBin qw($Bin);

die "Usage: do-score.pl data-dir exp-dir" unless @ARGV == 2;
my ($data_dir, $exp_dir) = @ARGV;
my $scorer_version = "ace05-eval-v14";

$exp_dir =~ s/\\/\//g;
$data_dir =~ s/\\/\//g;
open APFLIST, ">$exp_dir/apf-files.txt"
    or die "Couldn't open APF list file: $exp_dir/apf-files.txt\n";
for my $file (<$exp_dir/output/*.apf>){
    print APFLIST "$file\n";
}
close APFLIST;

### Do the scoring.


my $PERL=$EXECUTABLE_NAME;
system "$PERL $Bin/$scorer_version.pl -r $data_dir/key-files.txt -t $exp_dir/apf-files.txt > $exp_dir/ace2005-eval.txt";
die "Scoring script failed!" if $CHILD_ERROR;

system "$PERL $Bin/$scorer_version.pl -as -r $data_dir/key-files.txt -t $exp_dir/apf-files.txt > $exp_dir/ace2005-eval-full.txt";
die "Scoring script failed!" if $CHILD_ERROR;

