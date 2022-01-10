eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}' && eval 'exec perl -w -S $0 $argv:q'
  if 0;

# Copyright 2009 by BBN Technologies Corp.     All Rights Reserved.

if (! $ARGV[0]) {
    print "Usage: perl summarize_scores.pl <output_batch_dir> [<output_file>]\n\n  where <output_batch_dir> contains ace2007-eval.txt (ex: output/adj-1)\n        <output_file> is an optional output file (ex: score-summary.txt)\n";
    exit;
}

my $output_batch_dir = $ARGV[0];
my $out_file = $ARGV[1]; #"score-summary.txt";
my $scores = &GetScoreString;

if ($out_file) {
    open OUT, ">$out_file" or die;
    print OUT "$scores\n";
    close OUT;
} else {
    print "$scores\n";
}


# from Alex's nightly-run.pl
sub GetScoreString
{
    #my $file = "output/adj-1/ace2007-eval.txt";
	my $file = "$output_batch_dir/ace2007-eval.txt";
    open IN, $file or die "cannot open Serif output dir $output_batch_dir";
    my $e_score = -1;
    my $r_score = "n/a";
    my $v_score = "n/a";
    while (<IN>) {
	last if /======== entity scoring ========/;
    }
    while (<IN>) {
        if( /^\s*total(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)\s+([-\d\.]+)(\s+[-\d\.]+)\s+([-\d\.]+)\s+([-\d\.]+)/ )
	{
	    $e_score = $14;
	    last;
	}
    }
    while (<IN>) {
	last if /======== relation scoring ========/;
    }
    while (<IN>) {
	if( /^\s*total(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)\s+([-\d\.]+)\s+([-\d\.]+)\s+([-\d\.]+)\s+([-\d\.]+)/ ) 
	{
	    $r_score = $14;
	    last;
	}
    }
    my $found = "";
    while (<IN>) {
	if (/======== event scoring ========/) {
            $found = 1;
            last;
        }
    }
    if ($found) {
        while (<IN>) {
            if( /^\s*total(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)(\s+[-\d\.]+)\s+([-\d\.]+)\s+([-\d\.]+)\s+([-\d\.]+)\s+/ )
            {
                $v_score = $14;
                last;
            }
        }
    }
     
    close IN;
    return "Entity score: $e_score    Relation score: $r_score    Event score: $v_score";
}

