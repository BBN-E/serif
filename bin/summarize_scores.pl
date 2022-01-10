eval '(exit $?0)' && eval 'exec perl -w -S $0 ${1+"$@"}' && eval 'exec perl -w -S $0 $argv:q'
  if 0;

# Copyright 2015 by BBN Technologies Corp.     All Rights Reserved.

#my $score_file_location = "$daily_dir/ace2007-eval.txt";
#my $out_file = "+out_dir+/+SCORE_SUMMARY_FILE+";

if($#ARGV != 1)
  {
   print "Usage: summarize_scores.pl input_file( SCORE_FILE) output_file(SCORE_SUMMARY_FILE) \n";
   exit 1;
  }

my $input_file = $ARGV[0];
my $out_file = $ARGV[1];
my $scores = &GetScoreString;

open OUT, ">$out_file" or die "cannot open output file $out_file";
print OUT "$scores\n";
close OUT;


# from Alex's nightly-run.pl
sub GetScoreString
{
    my $file = $input_file;
    open IN, $file or die "cannot open input  file $file";
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

