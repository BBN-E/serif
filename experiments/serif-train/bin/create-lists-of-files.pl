eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}' && eval 'exec perl -w -S $0 $argv:q'
  if 0;

# Copyright 2008 by BBN Technologies Corp.     All Rights Reserved.

die "Usage: $0 master-training-directory output-file num-batches" unless @ARGV == 3;

$input = shift;
$output = shift;
$num_batches = shift;

open OUT, ">$output" or die "Could not open file $output for writing";

for ($i = 0; $i < $num_batches; $i++) {
    $dir = $i;
    $dir = "000" . $dir if length($dir) == 1;
    $dir = "00" . $dir if length($dir) == 2;
    $dir = "0" . $dir if length($dir) == 3;

    $file = "$input/$dir/state-15-doc-relations-events";
    die "Could not find $file" unless -f $file;

    print OUT $file . "\n";
    
}    

close OUT;
