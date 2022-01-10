#my $score_file_location = "$daily_dir/ace2007-eval.txt";
my $out_file = "+out_dir+/+SCORE_SUMMARY_FILE+";
my $scores = &GetScoreString;

open OUT, ">$out_file" or die;
print OUT "$scores\n";
close OUT;
print "$scores\n";


# from Alex's nightly-run.pl
sub GetScoreString
{
    my $file = "+out_dir+/+SCORE_FILE+";
    open IN, $file or die;
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

